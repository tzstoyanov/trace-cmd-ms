// SPDX-License-Identifier: LGPL-2.1
/*
 * trace-msg.c : define message protocol for communication between clients and
 *               a server
 *
 * Copyright (C) 2013 Hitachi, Ltd.
 * Created by Yoshihiro YUNOMAE <yoshihiro.yunomae.ez@hitachi.com>
 *
 */

#include <errno.h>
#include <poll.h>
#include <fcntl.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <linux/types.h>
#include <linux/vm_sockets.h>

#include "trace-cmd-local.h"
#include "trace-local.h"
#include "trace-msg.h"

typedef __u16 u16;
typedef __s16 s16;
typedef __u32 u32;
typedef __be32 be32;
typedef __u64 u64;
typedef __s64 s64;

static inline void dprint(const char *fmt, ...)
{
	va_list ap;

	if (!debug)
		return;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

/* Two (4k) pages is the max transfer for now */
#define MSG_MAX_LEN			8192

#define MSG_HDR_LEN			sizeof(struct tracecmd_msg_header)

#define MSG_MAX_DATA_LEN		(MSG_MAX_LEN - MSG_HDR_LEN)

unsigned int page_size;

/* Try a few times to get an accurate time sync */
#define TSYNC_TRIES 300

struct clock_synch_event {
	int			id;
	int			cpu;
	int			pid;
	unsigned long long	ts;
};

struct tracecmd_msg_opt {
	be32 size;
	be32 opt_cmd;
} __attribute__((packed));

struct tracecmd_msg_tinit {
	be32 cpus;
	be32 page_size;
	be32 opt_num;
} __attribute__((packed));

struct tracecmd_msg_rinit {
	be32 cpus;
} __attribute__((packed));

struct tracecmd_msg_trace_req {
	be32 flags;
	be32 argc;
} __attribute__((packed));

struct tracecmd_msg_trace_resp {
	be32 flags;
	be32 cpus;
	be32 page_size;
} __attribute__((packed));

struct tracecmd_msg_time_sync_init {
	char clock[32];
} __attribute__((packed));

struct tracecmd_msg_time_sync {
	u64 tlocal_ts;
	u16 tlocal_cpu;
} __attribute__((packed));

struct tracecmd_msg_header {
	be32	size;
	be32	cmd;
	be32	cmd_size;
} __attribute__((packed));

#define MSG_MAP								\
	C(CLOSE,	0,	0),					\
	C(TINIT,	1,	sizeof(struct tracecmd_msg_tinit)),	\
	C(RINIT,	2,	sizeof(struct tracecmd_msg_rinit)),	\
	C(SEND_DATA,	3,	0),					\
	C(FIN_DATA,	4,	0),					\
	C(NOT_SUPP,	5,	0),					\
	C(TRACE_REQ,	6,	sizeof(struct tracecmd_msg_trace_req)),	\
	C(TRACE_RESP,	7,	sizeof(struct tracecmd_msg_trace_resp)),\
	C(TIME_SYNC_INIT,8, sizeof(struct tracecmd_msg_time_sync_init)),\
	C(TIME_SYNC,	9, sizeof(struct tracecmd_msg_time_sync)),

#undef C
#define C(a,b,c)	MSG_##a = b

enum tracecmd_msg_cmd {
	MSG_MAP
	MSG_NR_COMMANDS
};

#undef C
#define C(a,b,c)	c

static be32 msg_cmd_sizes[] = { MSG_MAP };

#undef C
#define C(a,b,c)	#a

static const char *msg_names[] = { MSG_MAP };

static const char *cmd_to_name(int cmd)
{
	if (cmd < 0 || cmd >= MSG_NR_COMMANDS)
		return "Unknown";
	return msg_names[cmd];
}

struct tracecmd_msg {
	struct tracecmd_msg_header		hdr;
	union {
		struct tracecmd_msg_tinit		tinit;
		struct tracecmd_msg_rinit		rinit;
		struct tracecmd_msg_trace_req		trace_req;
		struct tracecmd_msg_trace_resp		trace_resp;
		struct tracecmd_msg_time_sync_init	time_sync_init;
		struct tracecmd_msg_time_sync		time_sync;
	};
	union {
		struct tracecmd_msg_opt		*opt;
		be32				*port_array;
		void				*buf;
	};
} __attribute__((packed));

static int msg_write(int fd, struct tracecmd_msg *msg)
{
	int cmd = ntohl(msg->hdr.cmd);
	int msg_size, data_size;
	int ret;

	if (cmd < 0 || cmd >= MSG_NR_COMMANDS)
		return -EINVAL;

	dprint("msg send: %d (%s) [%d]\n",
	       cmd, cmd_to_name(cmd), ntohl(msg->hdr.size));

	msg_size = MSG_HDR_LEN + ntohl(msg->hdr.cmd_size);
	data_size = ntohl(msg->hdr.size) - msg_size;
	if (data_size < 0)
		return -EINVAL;

	ret = __do_write_check(fd, msg, msg_size);
	if (ret < 0)
		return ret;

	if (!data_size)
		return 0;

	return __do_write_check(fd, msg->buf, data_size);
}

enum msg_trace_flags {
	MSG_TRACE_USE_FIFOS = 1 << 0,
};

enum msg_opt_command {
	MSGOPT_USETCP = 1,
};

static int make_tinit(struct tracecmd_msg_handle *msg_handle,
		      struct tracecmd_msg *msg)
{
	struct tracecmd_msg_opt *opt;
	int cpu_count = msg_handle->cpu_count;
	int opt_num = 0;
	int data_size = 0;

	if (msg_handle->flags & TRACECMD_MSG_FL_USE_TCP) {
		opt_num++;
		opt = malloc(sizeof(*opt));
		if (!opt)
			return -ENOMEM;
		opt->size = htonl(sizeof(*opt));
		opt->opt_cmd = htonl(MSGOPT_USETCP);
		msg->opt = opt;
		data_size += sizeof(*opt);
	}

	msg->tinit.cpus = htonl(cpu_count);
	msg->tinit.page_size = htonl(page_size);
	msg->tinit.opt_num = htonl(opt_num);

	msg->hdr.size = htonl(ntohl(msg->hdr.size) + data_size);

	return 0;
}

static int make_rinit(struct tracecmd_msg *msg, int cpus, int *ports)
{
	int i;

	msg->rinit.cpus = htonl(cpus);
	msg->port_array = malloc(sizeof(*ports) * cpus);
	if (!msg->port_array)
		return -ENOMEM;

	for (i = 0; i < cpus; i++)
		msg->port_array[i] = htonl(ports[i]);

	msg->hdr.size = htonl(ntohl(msg->hdr.size) + sizeof(*ports) * cpus);

	return 0;
}

static void tracecmd_msg_init(u32 cmd, struct tracecmd_msg *msg)
{
	memset(msg, 0, sizeof(*msg));
	msg->hdr.size = htonl(MSG_HDR_LEN + msg_cmd_sizes[cmd]);
	msg->hdr.cmd = htonl(cmd);
	msg->hdr.cmd_size = htonl(msg_cmd_sizes[cmd]);
}

static void msg_free(struct tracecmd_msg *msg)
{
	free(msg->buf);
	memset(msg, 0, sizeof(*msg));
}

static int tracecmd_msg_send(int fd, struct tracecmd_msg *msg)
{
	int ret = 0;

	ret = msg_write(fd, msg);
	if (ret < 0)
		ret = -ECOMM;

	msg_free(msg);

	return ret;
}

static int msg_read(int fd, void *buf, u32 size, int *n)
{
	ssize_t r;

	while (size) {
		r = read(fd, buf + *n, size);
		if (r < 0) {
			if (errno == EINTR)
				continue;
			return -errno;
		} else if (!r)
			return -ENOTCONN;
		size -= r;
		*n += r;
	}

	return 0;
}

static char scratch_buf[MSG_MAX_LEN];

static int msg_read_extra(int fd, struct tracecmd_msg *msg,
			  int *n, int size)
{
	int cmd, cmd_size, rsize;
	int ret;

	cmd = ntohl(msg->hdr.cmd);
	if (cmd < 0 || cmd >= MSG_NR_COMMANDS)
		return -EINVAL;

	cmd_size = ntohl(msg->hdr.cmd_size);
	if (cmd_size < 0)
		return -EINVAL;

	if (cmd_size > 0) {
		rsize = cmd_size;
		if (rsize > msg_cmd_sizes[cmd])
			rsize = msg_cmd_sizes[cmd];

		ret = msg_read(fd, msg, rsize, n);
		if (ret < 0)
			return ret;

		ret = msg_read(fd, scratch_buf, cmd_size - rsize, n);
		if (ret < 0)
			return ret;
	}

	if (size > *n) {
		size -= *n;
		msg->buf = malloc(size);
		if (!msg->buf)
			return -ENOMEM;

		*n = 0;
		return msg_read(fd, msg->buf, size, n);
	}

	return 0;
}

/*
 * Read header information of msg first, then read all data
 */
static int tracecmd_msg_recv(int fd, struct tracecmd_msg *msg)
{
	u32 size = 0;
	int n = 0;
	int ret;

	ret = msg_read(fd, msg, MSG_HDR_LEN, &n);
	if (ret < 0)
		return ret;

	dprint("msg received: %d (%s) [%d]\n",
	       ntohl(msg->hdr.cmd), cmd_to_name(ntohl(msg->hdr.cmd)),
	       ntohl(msg->hdr.size));

	size = ntohl(msg->hdr.size);
	if (size > MSG_MAX_LEN)
		/* too big */
		goto error;
	else if (size < MSG_HDR_LEN)
		/* too small */
		goto error;
	else if (size > MSG_HDR_LEN)
		return msg_read_extra(fd, msg, &n, size);

	return 0;
error:
	plog("Receive an invalid message(size=%d)\n", size);
	return -ENOMSG;
}

#define MSG_WAIT_MSEC	5000
static int msg_wait_to = MSG_WAIT_MSEC;

bool tracecmd_msg_done(struct tracecmd_msg_handle *msg_handle)
{
	return (volatile int)msg_handle->done;
}

void tracecmd_msg_set_done(struct tracecmd_msg_handle *msg_handle)
{
	msg_handle->done = true;
}

static void error_operation(struct tracecmd_msg *msg)
{
	warning("Message: cmd=%d size=%d\n",
		ntohl(msg->hdr.cmd), ntohl(msg->hdr.size));
}

/*
 * A return value of 0 indicates time-out
 */
static int tracecmd_msg_recv_wait(int fd, struct tracecmd_msg *msg)
{
	struct pollfd pfd;
	int ret;

	pfd.fd = fd;
	pfd.events = POLLIN;
	ret = poll(&pfd, 1, debug ? -1 : msg_wait_to);
	if (ret < 0)
		return -errno;
	else if (ret == 0)
		return -ETIMEDOUT;

	return tracecmd_msg_recv(fd, msg);
}

static int tracecmd_msg_wait_for_msg(int fd, struct tracecmd_msg *msg)
{
	u32 cmd;
	int ret;

	ret = tracecmd_msg_recv_wait(fd, msg);
	if (ret < 0) {
		if (ret == -ETIMEDOUT)
			warning("Connection timed out\n");
		return ret;
	}

	cmd = ntohl(msg->hdr.cmd);
	if (cmd == MSG_CLOSE)
		return -ECONNABORTED;

	return 0;
}

static int tracecmd_msg_send_notsupp(struct tracecmd_msg_handle *msg_handle)
{
	struct tracecmd_msg msg;

	tracecmd_msg_init(MSG_NOT_SUPP, &msg);
	return tracecmd_msg_send(msg_handle->fd, &msg);
}

static int handle_unexpected_msg(struct tracecmd_msg_handle *msg_handle,
				 struct tracecmd_msg *msg)
{
	/* Don't send MSG_NOT_SUPP back if we just received one */
	if (ntohl(msg->hdr.cmd) == MSG_NOT_SUPP)
		return 0;

	return tracecmd_msg_send_notsupp(msg_handle);

}

int tracecmd_msg_send_init_data(struct tracecmd_msg_handle *msg_handle,
				unsigned int **client_ports)
{
	struct tracecmd_msg msg;
	int fd = msg_handle->fd;
	unsigned int *ports;
	int i, cpus;
	int ret;

	*client_ports = NULL;

	tracecmd_msg_init(MSG_TINIT, &msg);
	ret = make_tinit(msg_handle, &msg);
	if (ret < 0)
		goto out;

	ret = tracecmd_msg_send(fd, &msg);
	if (ret < 0)
		goto out;

	msg_free(&msg);

	ret = tracecmd_msg_wait_for_msg(fd, &msg);
	if (ret < 0)
		goto out;

	if (ntohl(msg.hdr.cmd) != MSG_RINIT) {
		ret = -EOPNOTSUPP;
		goto error;
	}

	cpus = ntohl(msg.rinit.cpus);
	ports = malloc_or_die(sizeof(*ports) * cpus);
	for (i = 0; i < cpus; i++)
		ports[i] = ntohl(msg.port_array[i]);

	*client_ports = ports;

	msg_free(&msg);
	return 0;

error:
	error_operation(&msg);
	if (ret == -EOPNOTSUPP)
		handle_unexpected_msg(msg_handle, &msg);
out:
	msg_free(&msg);
	return ret;
}

static bool process_option(struct tracecmd_msg_handle *msg_handle,
			   struct tracecmd_msg_opt *opt)
{
	/* currently the only option we have is to us TCP */
	if (ntohl(opt->opt_cmd) == MSGOPT_USETCP) {
		msg_handle->flags |= TRACECMD_MSG_FL_USE_TCP;
		return true;
	}
	return false;
}

struct tracecmd_msg_handle *
tracecmd_msg_handle_alloc(int fd, unsigned long flags)
{
	struct tracecmd_msg_handle *handle;

	handle = calloc(1, sizeof(struct tracecmd_msg_handle));
	if (!handle)
		return NULL;

	handle->fd = fd;
	handle->flags = flags;
	return handle;
}

void tracecmd_msg_handle_close(struct tracecmd_msg_handle *msg_handle)
{
	close(msg_handle->fd);
	free(msg_handle);
}

#define MAX_OPTION_SIZE 4096

int tracecmd_msg_initial_setting(struct tracecmd_msg_handle *msg_handle)
{
	struct tracecmd_msg_opt *opt;
	struct tracecmd_msg msg;
	int pagesize;
	int options, i, s;
	int cpus;
	int ret;
	int offset = 0;
	u32 size;

	ret = tracecmd_msg_recv_wait(msg_handle->fd, &msg);
	if (ret < 0) {
		if (ret == -ETIMEDOUT)
			warning("Connection timed out\n");
		return ret;
	}

	if (ntohl(msg.hdr.cmd) != MSG_TINIT) {
		ret = -EOPNOTSUPP;
		goto error;
	}

	cpus = ntohl(msg.tinit.cpus);
	plog("cpus=%d\n", cpus);
	if (cpus < 0) {
		ret = -EINVAL;
		goto error;
	}

	msg_handle->cpu_count = cpus;

	pagesize = ntohl(msg.tinit.page_size);
	plog("pagesize=%d\n", pagesize);
	if (pagesize <= 0) {
		ret = -EINVAL;
		goto error;
	}

	size = MSG_HDR_LEN + ntohl(msg.hdr.cmd_size);
	options = ntohl(msg.tinit.opt_num);
	for (i = 0; i < options; i++) {
		if (size + sizeof(*opt) > ntohl(msg.hdr.size)) {
			plog("Not enough message for options\n");
			ret = -EINVAL;
			goto error;
		}
		opt = (void *)msg.opt + offset;
		offset += ntohl(opt->size);
		size += ntohl(opt->size);
		if (ntohl(msg.hdr.size) < size) {
			plog("Not enough message for options\n");
			ret = -EINVAL;
			goto error;
		}
		/* prevent a client from killing us */
		if (ntohl(opt->size) > MAX_OPTION_SIZE) {
			plog("Exceed MAX_OPTION_SIZE\n");
			ret = -EINVAL;
			goto error;
		}
		s = process_option(msg_handle, opt);
		/* do we understand this option? */
		if (!s) {
			plog("Cannot understand(%d:%d:%d)\n",
			     i, ntohl(opt->size), ntohl(opt->opt_cmd));
			ret = -EINVAL;
			goto error;
		}
	}

	msg_free(&msg);
	return pagesize;

error:
	error_operation(&msg);
	if (ret == -EOPNOTSUPP)
		handle_unexpected_msg(msg_handle, &msg);
	msg_free(&msg);
	return ret;
}

int tracecmd_msg_send_port_array(struct tracecmd_msg_handle *msg_handle,
				 int *ports)
{
	struct tracecmd_msg msg;
	int ret;

	tracecmd_msg_init(MSG_RINIT, &msg);
	ret = make_rinit(&msg, msg_handle->cpu_count, ports);
	if (ret < 0)
		return ret;

	ret = tracecmd_msg_send(msg_handle->fd, &msg);
	if (ret < 0)
		return ret;

	return 0;
}

int tracecmd_msg_send_close_msg(struct tracecmd_msg_handle *msg_handle)
{
	struct tracecmd_msg msg;

	tracecmd_msg_init(MSG_CLOSE, &msg);
	return tracecmd_msg_send(msg_handle->fd, &msg);
}

int tracecmd_msg_data_send(struct tracecmd_msg_handle *msg_handle,
			   const char *buf, int size)
{
	struct tracecmd_msg msg;
	int fd = msg_handle->fd;
	int n;
	int ret;
	int count = 0;

	tracecmd_msg_init(MSG_SEND_DATA, &msg);

	msg.buf = malloc(MSG_MAX_DATA_LEN);
	if (!msg.buf)
		return -ENOMEM;

	msg.hdr.size = htonl(MSG_MAX_LEN);

	n = size;
	while (n) {
		if (n > MSG_MAX_DATA_LEN) {
			memcpy(msg.buf, buf + count, MSG_MAX_DATA_LEN);
			n -= MSG_MAX_DATA_LEN;
			count += MSG_MAX_DATA_LEN;
		} else {
			msg.hdr.size = htonl(MSG_HDR_LEN + n);
			memcpy(msg.buf, buf + count, n);
			n = 0;
		}
		ret = msg_write(fd, &msg);
		if (ret < 0)
			break;
	}

	msg_free(&msg);
	return ret;
}

int tracecmd_msg_finish_sending_data(struct tracecmd_msg_handle *msg_handle)
{
	struct tracecmd_msg msg;
	int ret;

	tracecmd_msg_init(MSG_FIN_DATA, &msg);
	ret = tracecmd_msg_send(msg_handle->fd, &msg);
	if (ret < 0)
		return ret;
	return 0;
}

int tracecmd_msg_read_data(struct tracecmd_msg_handle *msg_handle, int ofd)
{
	struct tracecmd_msg msg;
	int t, n, cmd;
	ssize_t s;
	int ret;

	while (!tracecmd_msg_done(msg_handle)) {
		ret = tracecmd_msg_recv_wait(msg_handle->fd, &msg);
		if (ret < 0) {
			if (ret == -ETIMEDOUT)
				warning("Connection timed out\n");
			else
				warning("reading client");
			return ret;
		}

		cmd = ntohl(msg.hdr.cmd);
		if (cmd == MSG_FIN_DATA) {
			/* Finish receiving data */
			break;
		} else if (cmd != MSG_SEND_DATA) {
			ret = handle_unexpected_msg(msg_handle, &msg);
			if (ret < 0)
				goto error;
			goto next;
		}

		n = ntohl(msg.hdr.size) - MSG_HDR_LEN - ntohl(msg.hdr.cmd_size);
		t = n;
		s = 0;
		while (t > 0) {
			s = write(ofd, msg.buf+s, t);
			if (s < 0) {
				if (errno == EINTR)
					continue;
				warning("writing to file");
				ret = -errno;
				goto error;
			}
			t -= s;
			s = n - t;
		}

next:
		msg_free(&msg);
	}

	return 0;

error:
	error_operation(&msg);
	msg_free(&msg);
	return ret;
}

int tracecmd_msg_collect_data(struct tracecmd_msg_handle *msg_handle, int ofd)
{
	int ret;

	ret = tracecmd_msg_read_data(msg_handle, ofd);
	if (ret)
		return ret;

	return tracecmd_msg_wait_close(msg_handle);
}

int tracecmd_msg_wait_close(struct tracecmd_msg_handle *msg_handle)
{
	struct tracecmd_msg msg;
	int ret = -1;

	memset(&msg, 0, sizeof(msg));
	while (!tracecmd_msg_done(msg_handle)) {
		ret = tracecmd_msg_recv(msg_handle->fd, &msg);
		if (ret < 0)
			goto error;

		if (ntohl(msg.hdr.cmd) == MSG_CLOSE)
			return 0;

		error_operation(&msg);
		ret = handle_unexpected_msg(msg_handle, &msg);
		if (ret < 0)
			goto error;

		msg_free(&msg);
	}

error:
	msg_free(&msg);
	return ret;
}

static int make_trace_req(struct tracecmd_msg *msg, int argc, char **argv, bool use_fifos)
{
	size_t args_size = 0;
	char *p;
	int i;

	for (i = 0; i < argc; i++)
		args_size += strlen(argv[i]) + 1;

	msg->hdr.size = htonl(ntohl(msg->hdr.size) + args_size);
	msg->trace_req.flags = use_fifos ? htonl(MSG_TRACE_USE_FIFOS) : htonl(0);
	msg->trace_req.argc = htonl(argc);
	msg->buf = calloc(args_size, 1);
	if (!msg->buf)
		return -ENOMEM;

	p = msg->buf;
	for (i = 0; i < argc; i++)
		p = stpcpy(p, argv[i]) + 1;

	return 0;
}

int tracecmd_msg_send_trace_req(struct tracecmd_msg_handle *msg_handle,
				int argc, char **argv, bool use_fifos)
{
	struct tracecmd_msg msg;
	int ret;

	tracecmd_msg_init(MSG_TRACE_REQ, &msg);
	ret = make_trace_req(&msg, argc, argv, use_fifos);
	if (ret < 0)
		return ret;

	return tracecmd_msg_send(msg_handle->fd, &msg);
}

 /*
  * NOTE: On success, the returned `argv` should be freed with:
  *     free(argv[0]);
  *     free(argv);
  */
int tracecmd_msg_recv_trace_req(struct tracecmd_msg_handle *msg_handle,
				int *argc, char ***argv, bool *use_fifos)
{
	struct tracecmd_msg msg;
	char *p, *buf_end, **args;
	int i, ret, nr_args;
	ssize_t buf_len;

	ret = tracecmd_msg_recv(msg_handle->fd, &msg);
	if (ret < 0)
		return ret;

	if (ntohl(msg.hdr.cmd) != MSG_TRACE_REQ) {
		ret = -ENOTSUP;
		goto out;
	}

	nr_args = ntohl(msg.trace_req.argc);
	if (nr_args <= 0) {
		ret = -EINVAL;
		goto out;
	}

	buf_len = ntohl(msg.hdr.size) - MSG_HDR_LEN - ntohl(msg.hdr.cmd_size);
	buf_end = (char *)msg.buf + buf_len;
	if (buf_len <= 0 && ((char *)msg.buf)[buf_len-1] != '\0') {
		ret = -EINVAL;
		goto out;
	}

	args = calloc(nr_args, sizeof(*args));
	if (!args) {
		ret = -ENOMEM;
		goto out;
	}

	p = msg.buf;
	for (i = 0; i < nr_args; i++) {
		if (p >= buf_end) {
			ret = -EINVAL;
			goto out_args;
		}
		args[i] = p;
		p = strchr(p, '\0');
		if (!p) {
			ret = -EINVAL;
			goto out_args;
		}
		p++;
	}

	*argc = nr_args;
	*argv = args;
	*use_fifos = ntohl(msg.trace_req.flags) & MSG_TRACE_USE_FIFOS;

	/*
	 * On success we're passing msg.buf to the caller through argv[0] so we
	 * reset it here before calling msg_free().
	 */
	msg.buf = NULL;
	msg_free(&msg);
	return 0;

out_args:
	free(args);
out:
	error_operation(&msg);
	if (ret == -EOPNOTSUPP)
		handle_unexpected_msg(msg_handle, &msg);
	msg_free(&msg);
	return ret;
}

#define EVENTS_CHUNK	10
static int
get_events_in_page(struct tep_handle *tep, void *page,
		    int size, int cpu, struct clock_synch_event **events,
		    int *events_count, int *events_size)
{
	struct clock_synch_event *events_array = NULL;
	struct tep_record *last_record = NULL;
	struct tep_event *event = NULL;
	struct tep_record *record;
	int id, cnt = 0;

	if (size <= 0)
		return 0;

	if (*events == NULL) {
		*events = malloc(EVENTS_CHUNK*sizeof(struct clock_synch_event));
		*events_size = EVENTS_CHUNK;
		*events_count = 0;
	}

	while (true) {
		event = NULL;
		record = tracecmd_read_page_record(tep, page, size,
						   last_record);
		if (!record)
			break;
		free_record(last_record);
		id = tep_data_type(tep, record);
		event = tep_data_event_from_type(tep, id);
		if (event) {
			if (*events_count >= *events_size) {
				events_array = realloc(*events,
					(*events_size + EVENTS_CHUNK)*sizeof(struct clock_synch_event));
				if (events_array) {
					*events = events_array;
					(*events_size) += EVENTS_CHUNK;
				}
			}

			if (*events_count < *events_size) {
				(*events)[*events_count].ts = record->ts;
				(*events)[*events_count].cpu = cpu;
				(*events)[*events_count].id = id;
				(*events)[*events_count].pid = tep_data_pid(tep, record);
				(*events_count)++;
			}
		}
		last_record = record;
	}
	free_record(last_record);

	return cnt;
}

static int
find_sync_events(struct tep_handle *pevent, struct clock_synch_event *recorded,
		 int rsize, struct clock_synch_event *events)
{
	int i = 0, j = 0;

	while (i < rsize) {
		if (!events[j].ts && events[j].id == recorded[i].id &&
			(!events[j].pid || events[j].pid == recorded[i].pid)) {
			events[j].cpu = recorded[i].cpu;
			events[j].ts = recorded[i].ts;
			j++;
		} else if (j > 0 && events[j-1].id == recorded[i].id &&
			  (!events[j-1].pid || events[j-1].pid == recorded[i].pid)) {
			events[j-1].cpu = recorded[i].cpu;
			events[j-1].ts = recorded[i].ts;
		}
		i++;
	}
	return j;
}

static int sync_events_cmp(const void *a, const void *b)
{
	const struct clock_synch_event *ea = (const struct clock_synch_event *)a;
	const struct clock_synch_event *eb = (const struct clock_synch_event *)b;

	if (ea->ts > eb->ts)
		return 1;
	if (ea->ts < eb->ts)
		return -1;
	return 0;
}

static int clock_synch_find_events(struct tep_handle *tep,
				   struct buffer_instance *instance,
				   struct clock_synch_event *events)
{
	struct clock_synch_event *events_array = NULL;
	int events_count = 0;
	int events_size = 0;
	struct dirent *dent;
	int ts = 0;
	void *page;
	char *path;
	char *file;
	DIR *dir;
	int cpu;
	int len;
	int fd;
	int r;

	page_size = getpagesize();
#ifdef TSYNC_RBUFFER_DEBUG
	file = get_instance_file(instance, "trace");
	if (!file)
		return ts;
	{
		char *buf = NULL;
		FILE *fp;
		size_t n;
		int r;

		printf("Events:\n\r");
		fp = fopen(file, "r");
		while ((r = getline(&buf, &n, fp)) >= 0) {

			if (buf[0] != '#')
				printf("%s", buf);

			free(buf);
			buf = NULL;
		}
		fclose(fp);
	}
	tracecmd_put_tracing_file(file);
#endif /* TSYNC_RBUFFER_DEBUG */
	path = get_instance_file(instance, "per_cpu");
	if (!path)
		return ts;

	dir = opendir(path);
	if (!dir)
		goto out;

	len = strlen(path);
	file = malloc(len + strlen("trace_pipe_raw") + 32);
	page = malloc(page_size);
	if (!file || !page)
		die("Failed to allocate time_stamp info");

	while ((dent = readdir(dir))) {

		const char *name = dent->d_name;

		if (strncmp(name, "cpu", 3) != 0)
			continue;
		cpu = atoi(&name[3]);
		sprintf(file, "%s/%s/trace_pipe_raw", path, name);
		fd = open(file, O_RDONLY | O_NONBLOCK);
		if (fd < 0)
			continue;
		do {
			r = read(fd, page, page_size);
			if (r > 0) {
				get_events_in_page(tep, page, r, cpu,
						   &events_array, &events_count,
						   &events_size);
			}
		} while (r > 0);
		close(fd);
	}
	qsort(events_array, events_count, sizeof(*events_array), sync_events_cmp);
	r = find_sync_events(tep, events_array, events_count, events);
#ifdef TSYNC_RBUFFER_DEBUG
	len = 0;
	while (events[len].id) {
		printf("Found %d @ cpu %d: %lld\n\r",
			events[len].id,  events[len].cpu, events[len].ts);
		len++;
	}
#endif
	free(events_array);
	free(file);
	free(page);
	closedir(dir);

 out:
	tracecmd_put_tracing_file(path);
	return r;
}

int tracecmd_msg_rcv_time_sync(struct tracecmd_msg_handle *msg_handle)
{
	struct clock_synch_event events[3];
	struct buffer_instance *vinst;
	unsigned int lcid, lport, rcid, rport;
	struct tep_event *event;
	struct tracecmd_msg msg;
	struct tep_handle *tep;
	int *cpu_pid = NULL;
	int ret, cpu;
	char *clock;
	char vsock_rx_filter[255];
	char *systems[] = {"vsock", "kvm", NULL};
	struct clock_synch_event_descr events_descr[] = {
		{"events/kvm/kvm_exit/enable", "1", "0"},
		{"events/vsock/virtio_transport_recv_pkt/enable", "1", "0"},
		{"events/vsock/virtio_transport_recv_pkt/filter", vsock_rx_filter, "\0"},
		{NULL, NULL, NULL}
	};

	ret = tracecmd_msg_recv(msg_handle->fd, &msg);
	if (ret < 0 || ntohl(msg.hdr.cmd) != MSG_TIME_SYNC_INIT)
		return 0;
	if (!msg.time_sync_init.clock[0])
		return 0;
	clock = strdup(msg.time_sync_init.clock);
	memset(events, 0, sizeof(struct clock_synch_event)*3);

	rcid = 0;
	get_vsocket_params(msg_handle->fd, &lcid, &lport, &rcid, &rport);
	if (rcid)
		cpu_pid = get_guest_vcpu_pids(rcid);
	snprintf(vsock_rx_filter, 255,
		"src_cid==%u && src_port==%u && dst_cid==%u && dst_port==%u && len!=0",
		rcid, rport, lcid, lport);

	vinst = clock_synch_enable(clock, events_descr);
	tep = clock_synch_get_tep(vinst, systems);
	event = tep_find_event_by_name(tep, "kvm", "kvm_exit");
	if (event)
		events[0].id = event->id;
	event = tep_find_event_by_name(tep, "vsock", "virtio_transport_recv_pkt");
	if (event)
		events[1].id = event->id;
	tracecmd_msg_init(MSG_TIME_SYNC_INIT, &msg);
	tracecmd_msg_send(msg_handle->fd, &msg);

	do {
		events[0].ts = 0;	/* kvm exit ts */
		events[0].pid = 0;
		events[1].ts = 0;	/* vsock receive ts */
		ret = tracecmd_msg_recv(msg_handle->fd, &msg);
		if (ret < 0 || ntohl(msg.hdr.cmd) != MSG_TIME_SYNC)
			break;
		ret = tracecmd_msg_recv(msg_handle->fd, &msg);
		if (ret < 0 || ntohl(msg.hdr.cmd) != MSG_TIME_SYNC)
			break;
		/* Get kvm_exit events related to the corresponding VCPU */
		cpu = ntohs(msg.time_sync.tlocal_cpu);
		if (cpu_pid && cpu < VCPUS_MAX)
			events[0].pid = cpu_pid[cpu];
		ret = clock_synch_find_events(tep, vinst, events);
		tracecmd_msg_init(MSG_TIME_SYNC, &msg);
		msg.time_sync.tlocal_ts = htonll(events[0].ts);
		tracecmd_msg_send(msg_handle->fd, &msg);
	} while (true);
	clock_synch_disable(vinst, events_descr);
	msg_free(&msg);
	tep_free(tep);
	free(clock);
	return 0;
}

#define TSYNC_DEBUG

int tracecmd_msg_snd_time_sync(struct tracecmd_msg_handle *msg_handle,
			       char *clock_str, long long *toffset)
{
	static struct buffer_instance *vinst;
	struct clock_synch_event events_s[3];
	struct tracecmd_msg msg_resp;
	struct tracecmd_msg msg_req;
	int sync_loop = TSYNC_TRIES;
	long long min = 0, max = 0;
	long long  offset_av = 0;
	struct tep_event *event;
	struct tep_handle *tep;
	int k = 0, n, ret = 0;
	long long tresch = 0;
	long long offset = 0;
	long long m_t1 = 0;
	long long s_t2 = 0;
	long long *offsets;
	int probes = 0;
	char *clock;
	char vsock_tx_filter[255];
	unsigned int lcid, lport, rcid, rport;
	char *systems[] = {"vsock", "ftrace", NULL};
	struct clock_synch_event_descr events_descr[] = {
		{"set_ftrace_filter", "vp_notify", "\0"},
		{"current_tracer", "function", "nop"},
		{"events/vsock/virtio_transport_alloc_pkt/enable", "1", "0"},
		{"events/vsock/virtio_transport_alloc_pkt/filter", vsock_tx_filter, "\0"},
		{NULL, NULL, NULL}
	};
#ifdef TSYNC_DEBUG
/* Write all ts in a file, used to analyze the raw data */
	struct timespec tsStart, tsEnd;
	int zm = 0, zs = 0;
	long long duration;
	char buff[256];
	int iFd;
#endif
	clock = clock_str;
	if (!clock)
		clock = "local";

	tracecmd_msg_init(MSG_TIME_SYNC_INIT, &msg_req);
	if (toffset == NULL) {
		msg_req.time_sync_init.clock[0] = 0;
		tracecmd_msg_send(msg_handle->fd, &msg_req);
		return 0;
	}
	offsets = calloc(sizeof(long long), TSYNC_TRIES);
	if (!offsets)
		return 0;

	strncpy(msg_req.time_sync_init.clock, clock, 16);
	tracecmd_msg_send(msg_handle->fd, &msg_req);
	ret = tracecmd_msg_recv(msg_handle->fd, &msg_resp);
	if (ret < 0 || ntohl(msg_resp.hdr.cmd) != MSG_TIME_SYNC_INIT) {
		free(offsets);
		return 0;
	}
	get_vsocket_params(msg_handle->fd, &lcid, &lport, &rcid, &rport);
	snprintf(vsock_tx_filter, 255,
		"src_cid==%u && src_port==%u && dst_cid==%u && dst_port==%u && len!=0",
		lcid, lport, rcid, rport);

	memset(events_s, 0, sizeof(struct clock_synch_event)*3);
	vinst = clock_synch_enable(clock_str, events_descr);
	tep = clock_synch_get_tep(vinst, systems);
	event = tep_find_event_by_name(tep, "vsock", "virtio_transport_alloc_pkt");
	if (event)
		events_s[0].id = event->id;
	event = tep_find_event_by_name(tep, "ftrace", "function");
	if (event)
		events_s[1].id = event->id;

	*toffset = 0;
#ifdef TSYNC_DEBUG
	sprintf(buff, "s-%s.txt", clock);
	iFd = open(buff, O_CREAT|O_WRONLY|O_TRUNC, 0644);
	clock_gettime(CLOCK_MONOTONIC, &tsStart);
#endif
	do {
		memset(&msg_resp, 0, sizeof(msg_resp));
		events_s[0].ts = 0;	/* vsock send ts */
		events_s[0].cpu = 0;
		events_s[1].ts = 0;	/* vp_notify ts */
		events_s[1].cpu = 0;
		tracecmd_msg_init(MSG_TIME_SYNC, &msg_req);
		tracecmd_msg_send(msg_handle->fd, &msg_req);
		/* Get the ts and CPU of the sent event */
		clock_synch_find_events(tep, vinst, events_s);
		tracecmd_msg_init(MSG_TIME_SYNC, &msg_req);
		msg_req.time_sync.tlocal_ts = htonll(events_s[0].ts);
		msg_req.time_sync.tlocal_cpu = htons(events_s[0].cpu);
		tracecmd_msg_send(msg_handle->fd, &msg_req);
		ret = tracecmd_msg_recv(msg_handle->fd, &msg_resp);
		if (ret < 0 || ntohl(msg_resp.hdr.cmd) != MSG_TIME_SYNC)
			break;
		m_t1 = events_s[1].ts;
		s_t2 = htonll(msg_resp.time_sync.tlocal_ts); /* Client kvm exit ts */
#ifdef TSYNC_DEBUG
		if (!s_t2)
			zs++;
		if (!m_t1)
			zm++;
#endif
		if (!s_t2 || !m_t1)
			continue;
		offsets[probes] = s_t2 - m_t1;
		offset_av += offsets[probes];
		if (!min || min > llabs(offsets[probes]))
			min = llabs(offsets[probes]);
		if (!max || max < llabs(offsets[probes]))
			max = llabs(offsets[probes]);
		probes++;
#ifdef TSYNC_DEBUG
		sprintf(buff, "%lld %lld\n", m_t1, s_t2);
		write(iFd, buff, strlen(buff));
#endif
	} while (--sync_loop);

#ifdef TSYNC_DEBUG
	clock_gettime(CLOCK_MONOTONIC, &tsEnd);
	close(iFd);
#endif
	clock_synch_disable(vinst, events_descr);
	if (probes)
		offset_av /= (long long)probes;
	tresch = (long long)((max - min)/10);
	for (n = 0; n < TSYNC_TRIES; n++) {
		/* filter the offsets with deviation up to 10% */
		if (offsets[n] &&
		    llabs(offsets[n] - offset_av) < tresch) {
			offset += offsets[n];
			k++;
		}
	}
	if (k)
		offset /= (long long)k;

	tracecmd_msg_init(MSG_TIME_SYNC_INIT, &msg_req);
	msg_req.time_sync_init.clock[0] = 0;
	tracecmd_msg_send(msg_handle->fd, &msg_req);

	msg_free(&msg_req);
	msg_free(&msg_resp);
	free(offsets);

	*toffset = offset;

#ifdef TSYNC_DEBUG
	duration = tsEnd.tv_sec * 1000000000LL;
	duration += tsEnd.tv_nsec;
	duration -= (tsStart.tv_sec * 1000000000LL);
	duration -= tsStart.tv_nsec;

	printf("\n selected: %lld (in %lld ns), used %s clock, %d probes\n\r",
		*toffset, duration, clock, probes);
	printf("\t good probes: %d / %d, threshold %lld, Zm %d, Zs %d\n\r",
			k, TSYNC_TRIES, tresch, zm, zs);
#endif
	return 0;
}

static int make_trace_resp(struct tracecmd_msg *msg, int page_size, int nr_cpus,
			   unsigned int *ports, bool use_fifos)
{
	int ports_size = nr_cpus * sizeof(*msg->port_array);
	int i;

	msg->hdr.size = htonl(ntohl(msg->hdr.size) + ports_size);
	msg->trace_resp.flags = use_fifos ? htonl(MSG_TRACE_USE_FIFOS) : htonl(0);
	msg->trace_resp.cpus = htonl(nr_cpus);
	msg->trace_resp.page_size = htonl(page_size);

	msg->port_array = malloc(ports_size);
	if (!msg->port_array)
		return -ENOMEM;

	for (i = 0; i < nr_cpus; i++)
		msg->port_array[i] = htonl(ports[i]);

	return 0;
}

int tracecmd_msg_send_trace_resp(struct tracecmd_msg_handle *msg_handle,
				 int nr_cpus, int page_size,
				 unsigned int *ports, bool use_fifos)
{
	struct tracecmd_msg msg;
	int ret;

	tracecmd_msg_init(MSG_TRACE_RESP, &msg);
	ret = make_trace_resp(&msg, page_size, nr_cpus, ports, use_fifos);
	if (ret < 0)
		return ret;

	return tracecmd_msg_send(msg_handle->fd, &msg);
}

int tracecmd_msg_recv_trace_resp(struct tracecmd_msg_handle *msg_handle,
				 int *nr_cpus, int *page_size,
				 unsigned int **ports, bool *use_fifos)
{
	struct tracecmd_msg msg;
	ssize_t buf_len;
	int i, ret;

	ret = tracecmd_msg_recv(msg_handle->fd, &msg);
	if (ret < 0)
		return ret;

	if (ntohl(msg.hdr.cmd) != MSG_TRACE_RESP) {
		ret = -ENOTSUP;
		goto out;
	}

	buf_len = ntohl(msg.hdr.size) - MSG_HDR_LEN - ntohl(msg.hdr.cmd_size);
	if (buf_len <= 0 ||
	    buf_len != sizeof(*msg.port_array) * ntohl(msg.trace_resp.cpus)) {
		ret = -EINVAL;
		goto out;
	}

	*use_fifos = ntohl(msg.trace_resp.flags) & MSG_TRACE_USE_FIFOS;
	*nr_cpus = ntohl(msg.trace_resp.cpus);
	*page_size = ntohl(msg.trace_resp.page_size);
	*ports = calloc(*nr_cpus, sizeof(**ports));
	if (!*ports) {
		ret = -ENOMEM;
		goto out;
	}
	for (i = 0; i < *nr_cpus; i++)
		(*ports)[i] = ntohl(msg.port_array[i]);

	msg_free(&msg);
	return 0;

out:
	error_operation(&msg);
	if (ret == -EOPNOTSUPP)
		handle_unexpected_msg(msg_handle, &msg);
	msg_free(&msg);
	return ret;
}
