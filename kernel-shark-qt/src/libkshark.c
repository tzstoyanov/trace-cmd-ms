// SPDX-License-Identifier: LGPL-2.1

/*
 * Copyright (C) 2017 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
 */

 /**
 *  @file    libkshark.c
 *  @brief   API for processing of FTRACE (trace-cmd) data.
 */

/** Use GNU C Library. */
#define _GNU_SOURCE 1

// C
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

// KernelShark
#include "libkshark.h"

static __thread struct trace_seq seq;

static struct kshark_context *kshark_context_handler = NULL;

static bool kshark_default_context(struct kshark_context **context)
{
	struct kshark_context *kshark_ctx;

	kshark_ctx = calloc(1, sizeof(*kshark_ctx));
	if (!kshark_ctx)
		return false;

	kshark_ctx->stream = calloc(KS_MAX_NUM_STREAMS,
				    sizeof(*kshark_ctx->stream));

	kshark_ctx->event_handlers = NULL;
	kshark_ctx->plugins = NULL;

	/* Will free kshark_context_handler. */
	kshark_free(NULL);

	/* Will do nothing if *context is NULL. */
	kshark_free(*context);

	*context = kshark_context_handler = kshark_ctx;

	return true;
}

static bool init_thread_seq(void)
{
	if (!seq.buffer)
		trace_seq_init(&seq);

	return seq.buffer != NULL;
}

/**
 * @brief Initialize a kshark session. This function must be called before
 *	  calling any other kshark function. If the session has been
 *	  initialized, this function can be used to obtain the session's
 *	  context.
 *
 * @param kshark_ctx: Optional input/output location for context pointer.
 *		      If it points to a context, that context will become
 *		      the new session. If it points to NULL, it will obtain
 *		      the current (or new) session. The result is only
 *		      valid on return of true.
 *
 * @returns True on success, or false on failure.
 */
bool kshark_instance(struct kshark_context **kshark_ctx)
{
	if (*kshark_ctx != NULL) {
		/* Will free kshark_context_handler */
		kshark_free(NULL);

		/* Use the context provided by the user. */
		kshark_context_handler = *kshark_ctx;
	} else {
		if (kshark_context_handler) {
			/*
			 * No context is provided by the user, but the context
			 * handler is already set. Use the context handler.
			 */
			*kshark_ctx = kshark_context_handler;
		} else {
			/* No kshark_context exists. Create a default one. */
			if (!kshark_default_context(kshark_ctx))
				return false;
		}
	}

	if (!init_thread_seq())
		return false;

	/*
	 * Turn off function trace indent and turn on show parent
	 * if possible.
	 */
	trace_util_add_option("ftrace:parent", "1");
	trace_util_add_option("ftrace:indent", "0");

	return true;
}

static void kshark_free_task_list(struct kshark_data_stream *stream)
{
	struct kshark_task_list *task;
	int i;

	if (!stream->tasks)
		return;

	for (i = 0; i < KS_TASK_HASH_SIZE; ++i) {
		while (stream->tasks[i]) {
			task = stream->tasks[i];
			stream->tasks[i] = task->next;
			free(task);
		}
	}

	free(stream->tasks);
}

static void kshark_stream_free(struct kshark_data_stream *stream)
{
	if (!stream)
		return;

	tracecmd_filter_id_hash_free(stream->show_task_filter);
	tracecmd_filter_id_hash_free(stream->hide_task_filter);

	tracecmd_filter_id_hash_free(stream->show_event_filter);
	tracecmd_filter_id_hash_free(stream->hide_event_filter);

	kshark_free_task_list(stream);

	free(stream);
}

/**
 * @brief Add new Trace data stream.
 *
 * @param kshark_ctx: Input location for context pointer.
 *
 * @returns Zero on success or a negative error code in the case of an errno.
 */
int kshark_add_stream(struct kshark_context *kshark_ctx)
{
	struct kshark_data_stream *stream;
	int sd;

	for (sd = 0; sd < KS_MAX_NUM_STREAMS; ++sd)
		if (!kshark_ctx->stream[sd])
			break;

	if (sd == KS_MAX_NUM_STREAMS)
		return -EMFILE;

	stream = malloc(sizeof(*stream));
	if (!stream)
		return -ENOMEM;

	stream->filter_mask = 0x0;

	stream->show_task_filter = tracecmd_filter_id_hash_alloc();
	stream->hide_task_filter = tracecmd_filter_id_hash_alloc();

	stream->show_event_filter = tracecmd_filter_id_hash_alloc();
	stream->hide_event_filter = tracecmd_filter_id_hash_alloc();

	stream->tasks = calloc(KS_TASK_HASH_SIZE,
			       sizeof(*stream->tasks));

	if (!stream->show_task_filter ||
	    !stream->hide_task_filter ||
	    !stream->show_event_filter ||
	    !stream->hide_event_filter ||
	    !stream->tasks) {
		    kshark_stream_free(stream);
		    stream = NULL;
	}

	kshark_ctx->stream[sd] = stream;

	return sd;
}

/**
 * @brief Use an existing Trace data stream to open and prepare for reading
 *	  a trace data file specified by "file".
 *
 * @param stream: Input location for a Trace data stream pointer.
 * @param file: The file to load.
 *
 * @returns Zero on success or a negative error code in the case of an errno.
 */
int kshark_stream_open(struct kshark_data_stream *stream, const char *file)
{
	struct tracecmd_input *handle;

	if (!stream)
		return -EFAULT;

	handle = tracecmd_open(file);
	if (!handle)
		return -EEXIST;

	if (pthread_mutex_init(&stream->input_mutex, NULL) != 0) {
		tracecmd_close(handle);
		return -EAGAIN;
	}

	stream->handle = handle;
	stream->pevent = tracecmd_get_pevent(handle);

	stream->advanced_event_filter =
		tep_filter_alloc(stream->pevent);

	return 0;
}

/**
 * @brief Get an array containing the Ids of all opened Trace data streams.
 * 	  The User is responsible for freeing the array.
 *
 * @param kshark_ctx: Input location for context pointer.
 */
int *kshark_all_streams(struct kshark_context *kshark_ctx)
{
	int *ids, n, i, count = 0;

	n = kshark_ctx->n_streams;
	ids = malloc(n * (sizeof(*ids)));
	if (!ids) {
		fprintf(stderr,
			"Failed to allocate memory for stream array.\n");
		return NULL;
	}

	for (i = 0; i < KS_MAX_NUM_STREAMS; ++i)
		if (kshark_ctx->stream[i])
			ids[count++] = i;

	return ids;
}

/**
 * @brief Open and prepare for reading a trace data file specified by "file".
 *
 * @param kshark_ctx: Input location for context pointer.
 * @param file: The file to load.
 *
 * @returns The Id number of the data stream associated with this file on success.
 * 	    Otherwise a negative errno code.
 */
int kshark_open(struct kshark_context *kshark_ctx, const char *file)
{
	int sd, rt;

	sd = kshark_add_stream(kshark_ctx);
	if (sd < 0)
		return sd;

	rt = kshark_stream_open(kshark_ctx->stream[sd], file);
	if (rt < 0)
		return rt;

	kshark_ctx->n_streams++;

	return sd;
}

static void kshark_stream_close(struct kshark_data_stream *stream)
{
	if (!stream || !stream->handle)
		return;

	/*
	 * All filters are file specific. Make sure that the Pids and Event Ids
	 * from this file are not going to be used with another file.
	 */
	tracecmd_filter_id_clear(stream->show_task_filter);
	tracecmd_filter_id_clear(stream->hide_task_filter);
	tracecmd_filter_id_clear(stream->show_event_filter);
	tracecmd_filter_id_clear(stream->hide_event_filter);

	if (stream->advanced_event_filter) {
		tep_filter_reset(stream->advanced_event_filter);
		tep_filter_free(stream->advanced_event_filter);
		stream->advanced_event_filter = NULL;
	}

	tracecmd_close(stream->handle);
	stream->handle = NULL;
	stream->pevent = NULL;

	pthread_mutex_destroy(&stream->input_mutex);
}

/**
 * @brief Close the trace data file and free the trace data handle.
 *
 * @param kshark_ctx: Input location for the session context pointer.
 * @param sd: Data stream identifier.
 */
void kshark_close(struct kshark_context *kshark_ctx, int sd)
{
	struct kshark_data_stream *stream;

	stream = kshark_get_data_stream(kshark_ctx, sd);
	if (stream) {
		kshark_stream_close(kshark_ctx->stream[sd]);
		kshark_stream_free(kshark_ctx->stream[sd]);
		kshark_ctx->stream[sd] = NULL;
		kshark_ctx->n_streams--;
	}
}

/**
 * @brief Deinitialize kshark session. Should be called after closing all
 *	  open trace data files and before your application terminates.
 *
 * @param kshark_ctx: Optional input location for session context pointer.
 *		      If it points to a context of a sessuin, that sessuin
 *		      will be deinitialize. If it points to NULL, it will
 *		      deinitialize the current session.
 */
void kshark_free(struct kshark_context *kshark_ctx)
{
	int sd;

	if (kshark_ctx == NULL) {
		if (kshark_context_handler == NULL)
			return;

		kshark_ctx = kshark_context_handler;
		/* kshark_ctx_handler will be set to NULL below. */
	}

	for (sd = 0; sd < KS_MAX_NUM_STREAMS; ++sd)
		kshark_close(kshark_ctx, sd);

	free(kshark_ctx->stream);

	if(kshark_ctx->plugins) {
		kshark_handle_plugins(kshark_ctx, KSHARK_PLUGIN_UNLOAD);
		kshark_free_plugin_list(kshark_ctx->plugins);
		kshark_free_event_handler_list(kshark_ctx->event_handlers);
	}

	if (seq.buffer)
		trace_seq_destroy(&seq);

	if (kshark_ctx == kshark_context_handler)
		kshark_context_handler = NULL;

	free(kshark_ctx);
}

static inline uint8_t knuth_hash8(uint32_t val)
{
	/*
	 * Hashing functions, based on Donald E. Knuth's Multiplicative
	 * hashing. See The Art of Computer Programming (TAOCP).
	 * Multiplication by the Prime number, closest to the golden
	 * ratio of 2^8.
	 */
	return UINT8_C(val) * UINT8_C(157);
}

static struct kshark_task_list *
kshark_find_task(struct kshark_data_stream *stream, uint8_t key, int pid)
{
	struct kshark_task_list *list;

	for (list = stream->tasks[key]; list; list = list->next) {
		if (list->pid == pid)
			return list;
	}

	return NULL;
}

static struct kshark_task_list *
kshark_add_task(struct kshark_data_stream *stream, int pid)
{
	struct kshark_task_list *list;
	uint8_t key;

	key = knuth_hash8(pid);
	list = kshark_find_task(stream, key, pid);
	if (list)
		return list;

	list = malloc(sizeof(*list));
	if (!list)
		return NULL;

	list->pid = pid;
	list->next = stream->tasks[key];
	stream->tasks[key] = list;

	return list;
}

/**
 * @brief Get an array containing the Process Ids of all tasks presented in
 *	  the loaded trace data file.
 *
 * @param kshark_ctx: Input location for context pointer.
 * @param sd: Data stream identifier.
 * @param pids: Output location for the Pids of the tasks. The user is
 *		responsible for freeing the elements of the outputted array.
 *
 * @returns The size of the outputted array of Pids in the case of success,
 *	    or a negative error code on failure.
 */
ssize_t kshark_get_task_pids(struct kshark_context *kshark_ctx, int sd,
			     int **pids)
{
	size_t i, pid_count = 0, pid_size = KS_TASK_HASH_SIZE;
	struct kshark_data_stream *stream;
	struct kshark_task_list *list;
	int *temp_pids;

	stream = kshark_get_data_stream(kshark_ctx, sd);
	if (!stream)
		return -EBADF;

	*pids = calloc(pid_size, sizeof(int));
	if (!*pids)
		goto fail;

	for (i = 0; i < KS_TASK_HASH_SIZE; ++i) {
		list = kshark_ctx->stream[sd]->tasks[i];
		while (list) {
			(*pids)[pid_count] = list->pid;
			list = list->next;
			if (++pid_count >= pid_size) {
				pid_size *= 2;
				temp_pids = realloc(*pids, pid_size * sizeof(int));
				if (!temp_pids) {
					goto fail;
				}
				*pids = temp_pids;
			}
		}
	}

	if (pid_count) {
		temp_pids = realloc(*pids, pid_count * sizeof(int));
		if (!temp_pids)
			goto fail;

		/* Paranoid: In the unlikely case of shrinking *pids, realloc moves it */
		*pids = temp_pids;
	} else {
		free(*pids);
		*pids = NULL;
	}

	return pid_count;

fail:
	fprintf(stderr, "Failed to allocate memory for Task Pids.\n");
	free(*pids);
	*pids = NULL;
	return -ENOMEM;
}

static bool filter_find(struct tracecmd_filter_id *filter, int pid,
			bool test)
{
	return !filter || !filter->count ||
		!!(unsigned long)tracecmd_filter_id_find(filter, pid) == test;
}

static bool kshark_show_task(struct kshark_data_stream *stream, int pid)
{
	return filter_find(stream->show_task_filter, pid, true) &&
	       filter_find(stream->hide_task_filter, pid, false);
}

static bool kshark_show_event(struct kshark_data_stream *stream, int pid)
{
	return filter_find(stream->show_event_filter, pid, true) &&
	       filter_find(stream->hide_event_filter, pid, false);
}

/**
 * @brief Get an Id Filter.
 *
 * @param kshark_ctx: Input location for context pointer.
 * @param sd: Data stream identifier.
 * @param filter_id: Identifier of the filter.
 */
struct tracecmd_filter_id *
kshark_get_filter(struct kshark_context *kshark_ctx, int sd, int filter_id)
{
	struct kshark_data_stream *stream;

	stream = kshark_get_data_stream(kshark_ctx, sd);
	if (!stream)
		return NULL;

	switch (filter_id) {
	case KS_SHOW_EVENT_FILTER:
		return stream->show_event_filter;
	case KS_HIDE_EVENT_FILTER:
		return stream->hide_event_filter;
	case KS_SHOW_TASK_FILTER:
		return stream->show_task_filter;
	case KS_HIDE_TASK_FILTER:
		return stream->hide_task_filter;
	default:
		return NULL;
	}
}

/**
 * @brief Add an Id value to the filter specified by "filter_id".
 *
 * @param kshark_ctx: Input location for the session context pointer.
 * @param sd: Data stream identifier.
 * @param filter_id: Identifier of the filter.
 * @param id: Id value to be added to the filter.
 */
void kshark_filter_add_id(struct kshark_context *kshark_ctx, int sd,
			  int filter_id, int id)
{
	struct tracecmd_filter_id *filter;

	filter = kshark_get_filter(kshark_ctx, sd, filter_id);
	if (filter)
		tracecmd_filter_id_add(filter, id);
}

/**
 * @brief Get an array containing all Ids associated with a given Id Filter.
 *
 * @param kshark_ctx: Input location for context pointer.
 * @param sd: Data stream identifier.
 * @param filter_id: Identifier of the filter.
 * @param n: Output location for the size of the returned array.
 *
 * @return The user is responsible for freeing the array.
 */
int *kshark_get_filter_ids(struct kshark_context *kshark_ctx, int sd,
			   int filter_id, int *n)
{
	struct tracecmd_filter_id *filter;

	filter = kshark_get_filter(kshark_ctx, sd, filter_id);
	if (filter) {
		if (n)
			*n = filter->count;

		return tracecmd_filter_ids(filter);
	}

	if (n)
		*n = 0;

	return NULL;
}

/**
 * @brief Clear (reset) the filter specified by "filter_id".
 *
 * @param kshark_ctx: Input location for the session context pointer.
 * @param sd: Data stream identifier.
 * @param filter_id: Identifier of the filter.
 */
void kshark_filter_clear(struct kshark_context *kshark_ctx, int sd,
			 int filter_id)
{
	struct tracecmd_filter_id *filter;

	filter = kshark_get_filter(kshark_ctx, sd, filter_id);
	if (filter)
		tracecmd_filter_id_clear(filter);
}

static bool filter_is_set(struct tracecmd_filter_id *filter)
{
	return filter && filter->count;
}

static bool kshark_filter_is_set(struct kshark_data_stream *stream)
{
	return filter_is_set(stream->show_task_filter) ||
	       filter_is_set(stream->hide_task_filter) ||
	       filter_is_set(stream->show_event_filter) ||
	       filter_is_set(stream->hide_event_filter);
}

static void unset_event_filter_flag(struct kshark_data_stream *stream,
				    struct kshark_entry *e)
{
	/*
	 * All entries, filtered-out by the event filters, will be treated
	 * differently, when visualized. Because of this, ignore the value
	 * of the GRAPH_VIEW flag provided by the user via
	 * stream->filter_mask and unset the EVENT_VIEW flag.
	 */
	int event_mask = stream->filter_mask;

	event_mask &= ~KS_GRAPH_VIEW_FILTER_MASK;
	event_mask |= KS_EVENT_VIEW_FILTER_MASK;
	e->visible &= ~event_mask;
}

/**
 * @brief This function loops over the array of entries specified by "data"
 *	  and "n_entries" and sets the "visible" fields of each entry
 *	  according to the criteria provided by the filters of the session's
 *	  context. The field "filter_mask" of the session's context is used to
 *	  control the level of visibility/invisibility of the entries which
 *	  are filtered-out.
 *	  WARNING: Do not use this function if the advanced filter is set.
 *	  Applying the advanced filter requires access to prevent_record,
 *	  hence the data has to be reloaded using kshark_load_data_entries().
 *
 * @param kshark_ctx: Input location for the session context pointer.
 * @param sd: Data stream identifier.
 * @param data: Input location for the trace data to be filtered.
 * @param n_entries: The size of the inputted data.
 */
void kshark_filter_entries(struct kshark_context *kshark_ctx, int sd,
			   struct kshark_entry **data, size_t n_entries)
{
	struct kshark_data_stream *stream;
	int i;

	stream = kshark_get_data_stream(kshark_ctx, sd);
	if (!stream)
		return;

	if (stream->advanced_event_filter->filters) {
		/* The advanced filter is set. */
		fprintf(stderr,
			"Failed to filter (sd = %i)!\n", sd);
		fprintf(stderr,
			"Reset the Advanced filter or reload the data.\n");
		return;
	}

	if (!kshark_filter_is_set(stream))
		return;

	/* Apply only the Id filters. */
	for (i = 0; i < n_entries; ++i) {
		/* Chack is the entry belongs to this stream. */
		if (data[i]->stream_id != sd)
			continue;

		/* Start with and entry which is visible everywhere. */
		data[i]->visible = 0xFF;

		/* Apply event filtering. */
		if (!kshark_show_event(stream, data[i]->event_id))
			unset_event_filter_flag(stream, data[i]);

		/* Apply task filtering. */
		if (!kshark_show_task(stream, data[i]->pid))
			data[i]->visible &= ~stream->filter_mask;
	}
}

static void kshark_set_entry_values(struct kshark_data_stream *stream,
				    struct tep_record *record,
				    struct kshark_entry *entry)
{
	/* Offset of the record */
	entry->offset = record->offset;

	/* CPU Id of the record */
	entry->cpu = record->cpu;

	/* Time stamp of the record */
	entry->ts = record->ts;

	/* Event Id of the record */
	entry->event_id = tep_data_type(stream->pevent, record);

	/*
	 * Is visible mask. This default value means that the entry
	 * is visible everywhere.
	 */
	entry->visible = 0xFF;

	/* Process Id of the record */
	entry->pid = tep_data_pid(stream->pevent, record);
}

/**
 * rec_list is used to pass the data to the load functions.
 * The rec_list will contain the list of entries from the source,
 * and will be a link list of per CPU entries.
 */
struct rec_list {
	union {
		/* Used by kshark_load_data_records */
		struct {
			/** next pointer, matches entry->next */
			struct rec_list		*next;
			/** pointer to the raw record data */
			struct tep_record	*rec;
		};
		/** entry - Used for kshark_load_data_entries() */
		struct kshark_entry		entry;
	};
};

/**
 * rec_type defines what type of rec_list is being used.
 */
enum rec_type {
	REC_RECORD,
	REC_ENTRY,
};

static void free_rec_list(struct rec_list **rec_list, int n_cpus,
			  enum rec_type type)
{
	struct rec_list *temp_rec;
	int cpu;

	for (cpu = 0; cpu < n_cpus; ++cpu) {
		while (rec_list[cpu]) {
			temp_rec = rec_list[cpu];
			rec_list[cpu] = temp_rec->next;
			if (type == REC_RECORD)
				free_record(temp_rec->rec);
			free(temp_rec);
		}
	}
	free(rec_list);
}

static size_t get_records(struct kshark_context *kshark_ctx, int sd,
			  struct rec_list ***rec_list, enum rec_type type)
{
	struct kshark_event_handler *evt_handler;
	struct kshark_data_stream *stream;
	struct event_filter *adv_filter;
	struct kshark_task_list *task;
	struct tep_record *rec;
	struct rec_list **temp_next;
	struct rec_list **cpu_list;
	struct rec_list *temp_rec;
	size_t count, total = 0;
	int n_cpus;
	int pid;
	int cpu;

	stream = kshark_get_data_stream(kshark_ctx, sd);

	n_cpus = tracecmd_cpus(stream->handle);
	cpu_list = calloc(n_cpus, sizeof(*cpu_list));
	if (!cpu_list)
		return -ENOMEM;

	/* Just to shorten the name */
	if (type == REC_ENTRY)
		adv_filter = stream->advanced_event_filter;

	for (cpu = 0; cpu < n_cpus; ++cpu) {
		count = 0;
		cpu_list[cpu] = NULL;
		temp_next = &cpu_list[cpu];

		rec = tracecmd_read_cpu_first(stream->handle, cpu);
		while (rec) {
			*temp_next = temp_rec = calloc(1, sizeof(*temp_rec));
			if (!temp_rec)
				goto fail;

			temp_rec->next = NULL;

			switch (type) {
			case REC_RECORD:
				temp_rec->rec = rec;
				pid = tep_data_pid(stream->pevent, rec);
				break;
			case REC_ENTRY: {
				struct kshark_entry *entry;
				int ret;

				entry = &temp_rec->entry;
				kshark_set_entry_values(stream, rec, entry);

				/* Execute all plugin-provided actions (if any). */
				evt_handler = kshark_ctx->event_handlers;
				while ((evt_handler = find_event_handler(evt_handler,
									 entry->event_id))) {
					evt_handler->event_func(kshark_ctx, rec, entry);

					if ((evt_handler = evt_handler->next))
						entry->visible &= ~KS_PLUGIN_UNTOUCHED_MASK;
				}

				pid = entry->pid;
				/* Apply event filtering. */
				ret = FILTER_MATCH;
				if (adv_filter->filters)
					ret = tep_filter_match(adv_filter, rec);

				if (!kshark_show_event(stream, entry->event_id) ||
				    ret != FILTER_MATCH) {
					unset_event_filter_flag(stream, entry);
				}

				/* Apply task filtering. */
				if (!kshark_show_task(stream, entry->pid)) {
					entry->visible &= ~stream->filter_mask;
				}
				free_record(rec);
				break;
			} /* REC_ENTRY */
			}

			task = kshark_add_task(stream, pid);
			if (!task) {
				free_record(rec);
				goto fail;
			}

			temp_next = &temp_rec->next;

			++count;
			rec = tracecmd_read_data(stream->handle, cpu);
		}

		total += count;
	}

	*rec_list = cpu_list;
	return total;

 fail:
	free_rec_list(cpu_list, n_cpus, type);
	return -ENOMEM;
}

static int pick_next_cpu(struct rec_list **rec_list, int n_cpus,
			 enum rec_type type)
{
	uint64_t ts = 0;
	uint64_t rec_ts;
	int next_cpu = -1;
	int cpu;

	for (cpu = 0; cpu < n_cpus; ++cpu) {
		if (!rec_list[cpu])
			continue;

		switch (type) {
		case REC_RECORD:
			rec_ts = rec_list[cpu]->rec->ts;
			break;
		case REC_ENTRY:
			rec_ts = rec_list[cpu]->entry.ts;
			break;
		}
		if (!ts || rec_ts < ts) {
			ts = rec_ts;
			next_cpu = cpu;
		}
	}

	return next_cpu;
}

/**
 * @brief Load the content of the trace data file into an array of
 *	  kshark_entries. This function provides an abstraction of the
 *	  entries from the raw data that is read, however the "latency"
 *	  and the "info" fields can be accessed only via the offset
 *	  into the file. This makes the access to these two fields much
 *	  slower.
 *	  If one or more filters are set, the "visible" fields of each entry
 *	  is updated according to the criteria provided by the filters. The
 *	  field "filter_mask" of the session's context is used to control the
 *	  level of visibility/invisibility of the filtered entries.
 *
 * @param kshark_ctx: Input location for context pointer.
 * @param sd: Data stream identifier.
 * @param data_rows: Output location for the trace data. The user is
 *		     responsible for freeing the elements of the outputted
 *		     array.
 *
 * @returns The size of the outputted data in the case of success, or a
 *	    negative error code on failure.
 */
ssize_t kshark_load_data_entries(struct kshark_context *kshark_ctx, int sd,
				struct kshark_entry ***data_rows)
{
	struct kshark_data_stream *stream;
	struct kshark_entry **rows;
	struct rec_list **rec_list;
	enum rec_type type = REC_ENTRY;
	size_t count, total = 0;
	int n_cpus;

	if (*data_rows)
		free(*data_rows);

	stream = kshark_get_data_stream(kshark_ctx, sd);
	if (!stream)
		return -EBADF;;

	total = get_records(kshark_ctx, sd, &rec_list, type);
	if (total < 0)
		goto fail;

	n_cpus = tracecmd_cpus(stream->handle);

	rows = calloc(total, sizeof(struct kshark_entry *));
	if (!rows)
		goto fail_free;

	for (count = 0; count < total; count++) {
		int next_cpu;

		next_cpu = pick_next_cpu(rec_list, n_cpus, type);

		if (next_cpu >= 0) {
			rows[count] = &rec_list[next_cpu]->entry;
			rec_list[next_cpu] = rec_list[next_cpu]->next;
		}
	}

	free_rec_list(rec_list, n_cpus, type);
	*data_rows = rows;
	return total;

 fail_free:
	free_rec_list(rec_list, n_cpus, type);
	for (count = 0; count < total; count++) {
		if (!rows[count])
			break;
		free(rows[count]);
	}
	free(rows);
 fail:
	fprintf(stderr, "Failed to allocate memory during data loading.\n");
	return -ENOMEM;
}

/**
 * @brief Load the content of the trace data file into an array of
 *	  tep_records. Use this function only if you need fast access
 *	  to all fields of the record.
 *
 * @param kshark_ctx: Input location for the session context pointer.
 * @param sd: Data stream identifier.
 * @param data_rows: Output location for the trace data. Use free_record()
 *	 	     to free the elements of the outputted array.
 *
 * @returns The size of the outputted data in the case of success, or a
 *	    negative error code on failure.
 */
ssize_t kshark_load_data_records(struct kshark_context *kshark_ctx, int sd,
				struct tep_record ***data_rows)
{
	struct kshark_data_stream *stream;
	struct tep_record **rows;
	struct tep_record *rec;
	struct rec_list **rec_list;
	struct rec_list *temp_rec;
	enum rec_type type = REC_RECORD;
	size_t count, total = 0;
	int n_cpus;

	if (*data_rows)
		free(*data_rows);

	stream = kshark_get_data_stream(kshark_ctx, sd);
	if (!stream)
		return -EBADF;

	total = get_records(kshark_ctx, sd, &rec_list, REC_RECORD);
	if (total < 0)
		goto fail;

	rows = calloc(total, sizeof(struct tep_record *));
	if (!rows)
		goto fail;

	n_cpus = tracecmd_cpus(stream->handle);

	for (count = 0; count < total; count++) {
		int next_cpu;

		next_cpu = pick_next_cpu(rec_list, n_cpus, type);

		if (next_cpu >= 0) {
			rec = rec_list[next_cpu]->rec;
			rows[count] = rec;

			temp_rec = rec_list[next_cpu];
			rec_list[next_cpu] = rec_list[next_cpu]->next;
			free(temp_rec);
			/* The record is still referenced in rows */
		}
	}

	/* There should be no records left in rec_list */
	free_rec_list(rec_list, n_cpus, type);
	*data_rows = rows;
	return total;

 fail:
	fprintf(stderr, "Failed to allocate memory during data loading.\n");
	return -ENOMEM;
}

/**
 * @brief A thread-safe read of a record from a specific offset.
 *
 * @param kshark_ctx: Input location for the session context pointer.
 * @param offset: the offset into the file to find the record.
 *
 * @returns The returned pevent_record must be freed.
 */
struct tep_record *kshark_read_at(struct kshark_context *kshark_ctx, int sd,
				  uint64_t offset)
{
	/*
	 * It turns that tracecmd_read_at() is not thread-safe.
	 * TODO: Understand why and see if this can be fixed.
	 * For the time being use a mutex to protect the access.
	 */

	struct kshark_data_stream *stream;
	struct tep_record *data;

	stream = kshark_get_data_stream(kshark_ctx, sd);
	if (!stream)
		return NULL;

	pthread_mutex_lock(&stream->input_mutex);

	data = tracecmd_read_at(stream->handle, offset, NULL);

	pthread_mutex_unlock(&stream->input_mutex);

	return data;
}

static const char *kshark_get_latency(struct tep_handle *pe,
				      struct tep_record *record)
{
	if (!record)
		return NULL;

	trace_seq_reset(&seq);
	tep_data_lat_fmt(pe, &seq, record);
	return seq.buffer;
}

static const char *kshark_get_info(struct tep_handle *pe,
				   struct tep_record *record,
				   struct event_format *event)
{
	char *pos;

	if (!record || !event)
		return NULL;

	trace_seq_reset(&seq);
	tep_event_info(&seq, event, record);

	/*
	 * The event info string contains a trailing newline.
	 * Remove this newline.
	 */
	if ((pos = strchr(seq.buffer, '\n')) != NULL)
		*pos = '\0';

	return seq.buffer;
}

/**
 * @brief This function allows for an easy access to the original value of the
 *	  Process Id as recorded in the tep_record object. The record is read
 *	  from the file only in the case of an entry being touched by a plugin.
 *	  Be aware that using the kshark_get_X_easy functions can be
 *	  inefficient if you need an access to more than one of the data fields
 *	  of the record.
 *
 * @param entry: Input location for the KernelShark entry.
 *
 * @returns The original value of the Process Id as recorded in the
 *	    tep_record object.
 */
int kshark_get_pid_easy(struct kshark_entry *entry)
{
	struct kshark_data_stream *stream;
	int pid = KS_EMPTY_BIN;

	if (entry->visible & KS_PLUGIN_UNTOUCHED_MASK) {
		pid = entry->pid;
	} else {
		struct kshark_context *kshark_ctx = NULL;
		if (!kshark_instance(&kshark_ctx))
			return pid;

		struct tep_record *data = kshark_read_at(kshark_ctx,
							 entry->stream_id,
							 entry->offset);

		stream = kshark_get_data_stream(kshark_ctx, entry->stream_id);
		if (stream)
			pid = tep_data_pid(stream->pevent, data);

		free_record(data);
	}

	return pid;
}

/**
 * @brief This function allows for an easy access to the original value of the
 *	  Task name as recorded in the tep_record object. The record is read
 *	  from the file only in the case of an entry being touched by a plugin.
 *	  Be aware that using the kshark_get_X_easy functions can be
 *	  inefficient if you need an access to more than one of the data fields
 *	  of the record.
 *
 * @param entry: Input location for the KernelShark entry.
 *
 * @returns The original name of the task, retrieved from the Process Id
 *	    recorded in the tep_record object.
 */
const char *kshark_get_task_easy(struct kshark_entry *entry)
{
	struct kshark_data_stream *stream;
	int pid = kshark_get_pid_easy(entry);
	struct kshark_context *kshark_ctx = NULL;

	if (!kshark_instance(&kshark_ctx))
		return NULL;

	stream = kshark_get_data_stream(kshark_ctx, entry->stream_id);
	if (!stream)
		return NULL;

	return tep_data_comm_from_pid(stream->pevent, pid);
}

/**
 * @brief This function allows for an easy access to the latency information
 *	  recorded in the tep_record object. The record is read from the file
 *	  using the offset field of kshark_entry. Be aware that using the
 *	  kshark_get_X_easy functions can be inefficient if you need an access
 *	  to more than one of the data fields of the record.
 *
 * @param entry: Input location for the KernelShark entry.
 *
 * @returns A string showing the latency information, coded into 5 fields:
 *	    interrupts disabled, need rescheduling, hard/soft interrupt,
 *	    preempt count and lock depth.
 */
const char *kshark_get_latency_easy(struct kshark_entry *entry)
{
	struct kshark_context *kshark_ctx = NULL;
	struct kshark_data_stream *stream;
	const char *lat = NULL;

	if (!kshark_instance(&kshark_ctx))
		return lat;

	stream = kshark_get_data_stream(kshark_ctx, entry->stream_id);
	if (stream)
		return NULL;

	struct tep_record *data = kshark_read_at(kshark_ctx,
						 entry->stream_id,
						 entry->offset);

	lat = kshark_get_latency(stream->pevent, data);

	free_record(data);
	return lat;
}

/**
 * @brief This function allows for an easy access to the original name of the
 *	  trace event as recorded in the tep_record object. The record is read
 *	  from the file only in the case of an entry being touched by a plugin.
 *	  Be aware that using the kshark_get_X_easy functions can be
 *	  inefficient if you need an access to more than one of the data fields
 *	  of the record.
 *
 * @param entry: Input location for the KernelShark entry.
 *
 * @returns The mane of the trace event recorded in the tep_record object on
 *	    success, otherwise "[UNKNOWN EVENT]".
 */
const char *kshark_get_event_name_easy(struct kshark_entry *entry)
{
	struct kshark_context *kshark_ctx = NULL;
	struct kshark_data_stream *stream;
	struct event_format *event;
	int event_id;

	if (!kshark_instance(&kshark_ctx))
		return NULL;

	stream = kshark_get_data_stream(kshark_ctx, entry->stream_id);
	if (stream)
		goto unknown;

	if (entry->visible & KS_PLUGIN_UNTOUCHED_MASK) {
		event_id = entry->event_id;
	} else {
		struct tep_record *data = kshark_read_at(kshark_ctx,
							 entry->stream_id,
							 entry->offset);

		event_id = tep_data_type(stream->pevent, data);
	}

	event = tep_data_event_from_type(stream->pevent, event_id);
	if (event)
		return event->name;

 unknown:
	return "[UNKNOWN EVENT]";
}

/**
 * @brief This function allows for an easy access to the tep_record's info
 *	  streang. The record is read from the file using the offset field of
 *	  kshark_entry. Be aware that using the kshark_get_X_easy functions can
 *	  be inefficient if you need an access to more than one of the data
 *	  fields of the record.
 *
 * @param entry: Input location for the KernelShark entry.
 *
 * @returns A string showing the data output of the trace event.
 */
const char *kshark_get_info_easy(struct kshark_entry *entry)
{
	struct kshark_context *kshark_ctx = NULL;
	struct kshark_data_stream *stream;
	struct event_format *event;
	const char *info;
	int event_id;

	if (!kshark_instance(&kshark_ctx))
		return NULL;

	stream = kshark_get_data_stream(kshark_ctx, entry->stream_id);
	if (stream)
		return NULL;

	struct tep_record *data = kshark_read_at(kshark_ctx,
						    entry->stream_id,
						    entry->offset);
	event_id = tep_data_type(stream->pevent, data);
	event = tep_data_event_from_type(stream->pevent, event_id);

	info = kshark_get_info(stream->pevent, data, event);
	free_record(data);
	return info;
}

/**
 * @brief Convert the timestamp of the trace record (nanosecond precision) into
 *	  seconds and microseconds.
 *
 * @param time: Input location for the timestamp.
 * @param sec: Output location for value of the seconds.
 * @param usec: Output location for value of the microseconds.
 */
void kshark_convert_nano(uint64_t time, uint64_t *sec, uint64_t *usec)
{
	*sec = time / 1000000000ULL;
	*usec = (time / 1000) % 1000000;
}

/**
 * @brief Dump into a string the content of one entry. The function allocates
 *	  a null terminated string and returns a pointer to this string. The
 *	  user has to free the returned string.
 *
 * @param entry: A Kernel Shark entry to be printed.
 *
 * @returns The returned string contains a semicolon-separated list of data
 *	    fields.
 */
char* kshark_dump_entry(const struct kshark_entry *entry)
{
	const char *event_name, *task, *lat, *info;
	struct kshark_context *kshark_ctx;
	struct kshark_data_stream *stream;
	struct tep_record *data;
	struct event_format *event;
	char *temp_str, *entry_str;
	int event_id, size = 0;

	kshark_ctx = NULL;
	if (!kshark_instance(&kshark_ctx) || !init_thread_seq())
		return NULL;

	stream = kshark_get_data_stream(kshark_ctx, entry->stream_id);
	if (!stream)
		return NULL;

	data = kshark_read_at(kshark_ctx, entry->stream_id, entry->offset);

	event_id = tep_data_type(stream->pevent, data);
	event = tep_data_event_from_type(stream->pevent, event_id);

	event_name = event? event->name : "[UNKNOWN EVENT]";
	task = tep_data_comm_from_pid(stream->pevent, entry->pid);
	lat = kshark_get_latency(stream->pevent, data);

	size = asprintf(&temp_str, "%li %s-%i; CPU %i; %s;",
			entry->ts,
			task,
			entry->pid,
			entry->cpu,
			lat);

	info = kshark_get_info(stream->pevent, data, event);
	if (size > 0) {
		size = asprintf(&entry_str, "%s %s; %s; 0x%x",
				temp_str,
				event_name,
				info,
				entry->visible);

		free(temp_str);
	}

	free_record(data);

	if (size > 0)
		return entry_str;

	return NULL;
}

/**
 * @brief Binary search inside a time-sorted array of kshark_entries.
 *
 * @param time: The value of time to search for.
 * @param data: Input location for the trace data.
 * @param l: Array index specifying the lower edge of the range to search in.
 * @param h: Array index specifying the upper edge of the range to search in.
 *
 * @returns On success, the first kshark_entry inside the range, having a
	    timestamp equal or bigger than "time".
	    If all entries inside the range have timestamps greater than "time"
	    the function returns BSEARCH_ALL_GREATER (negative value).
	    If all entries inside the range have timestamps smaller than "time"
	    the function returns BSEARCH_ALL_SMALLER (negative value).
 */
ssize_t kshark_find_entry_by_time(uint64_t time,
				 struct kshark_entry **data,
				 size_t l, size_t h)
{
	size_t mid;

	if (data[l]->ts > time)
		return BSEARCH_ALL_GREATER;

	if (data[h]->ts < time)
		return BSEARCH_ALL_SMALLER;

	/*
	 * After executing the BSEARCH macro, "l" will be the index of the last
	 * entry having timestamp < time and "h" will be the index of the first
	 * entry having timestamp >= time.
	 */
	BSEARCH(h, l, data[mid]->ts < time);
	return h;
}

/**
 * @brief Binary search inside a time-sorted array of tep_records.
 *
 * @param time: The value of time to search for.
 * @param data: Input location for the trace data.
 * @param l: Array index specifying the lower edge of the range to search in.
 * @param h: Array index specifying the upper edge of the range to search in.
 *
 * @returns On success, the first tep_record inside the range, having a
	    timestamp equal or bigger than "time".
	    If all entries inside the range have timestamps greater than "time"
	    the function returns BSEARCH_ALL_GREATER (negative value).
	    If all entries inside the range have timestamps smaller than "time"
	    the function returns BSEARCH_ALL_SMALLER (negative value).
 */
ssize_t kshark_find_record_by_time(uint64_t time,
				   struct tep_record **data,
				   size_t l, size_t h)
{
	size_t mid;

	if (data[l]->ts > time)
		return BSEARCH_ALL_GREATER;

	if (data[h]->ts < time)
		return BSEARCH_ALL_SMALLER;

	/*
	 * After executing the BSEARCH macro, "l" will be the index of the last
	 * record having timestamp < time and "h" will be the index of the
	 * first record having timestamp >= time.
	 */
	BSEARCH(h, l, data[mid]->ts < time);
	return h;
}

/**
 * @brief Simple Pid matching function to be user for data requests.
 *
 * @param kshark_ctx: Input location for the session context pointer.
 * @param e: kshark_entry to be checked.
 * @param sd: Data stream identifier.
 * @param pid: Matching condition value.
 *
 * @returns True if the Pid of the entry matches the value of "pid".
 *	    Else false.
 */
bool kshark_match_pid(struct kshark_context *kshark_ctx,
		      struct kshark_entry *e, int sd, int pid)
{
	if (e->stream_id == sd && e->pid == pid)
		return true;

	return false;
}

/**
 * @brief Simple Cpu matching function to be user for data requests.
 *
 * @param kshark_ctx: Input location for the session context pointer.
 * @param e: kshark_entry to be checked.
 * @param sd: Data stream identifier.
 * @param cpu: Matching condition value.
 *
 * @returns True if the Cpu of the entry matches the value of "cpu".
 *	    Else false.
 */
bool kshark_match_cpu(struct kshark_context *kshark_ctx,
		      struct kshark_entry *e, int sd, int cpu)
{
	if (e->stream_id == sd && e->cpu == cpu)
		return true;

	return false;
}

/**
 * @brief Create Data request. The request defines the properties of the
 *	  requested kshark_entry.
 *
 * @param first: Array index specifying the position inside the array from
 *		 where the search starts.
 * @param n: Number of array elements to search in.
 * @param cond: Matching condition function.
 * @param sd: Data stream identifier.
 * @param val: Matching condition value, used by the Matching condition
 *	       function.
 * @param vis_only: If true, a visible entry is requested.
 * @param vis_mask: If "vis_only" is true, use this mask to specify the level
 *		    of visibility of the requested entry.
 *
 * @returns Pointer to kshark_entry_request on success, or NULL on failure.
 *	    The user is responsible for freeing the returned
 *	    kshark_entry_request.
 */
struct kshark_entry_request *
kshark_entry_request_alloc(size_t first, size_t n,
			   matching_condition_func cond, int sd, int val,
			   bool vis_only, int vis_mask)
{
	struct kshark_entry_request *req = malloc(sizeof(*req));

	if (!req) {
		fprintf(stderr,
			"Failed to allocate memory for entry request.\n");
		return NULL;
	}

	req->next = NULL;
	req->first = first;
	req->n = n;
	req->cond = cond;
	req->sd = sd;
	req->val = val;
	req->vis_only = vis_only;
	req->vis_mask = vis_mask;

	return req;
}

/**
 * @brief Free all Data requests in a given list.
 * @param req: Intput location for the Data request list.
 */
void kshark_free_entry_request(struct kshark_entry_request *req)
{
	struct kshark_entry_request *last;

	while (req) {
		last = req;
		req = req->next;
		free(last);
	}
}

/** Dummy entry, used to indicate the existence of filtered entries. */
const struct kshark_entry dummy_entry = {
	.next		= NULL,
	.visible	= 0x00,
	.cpu		= KS_FILTERED_BIN,
	.pid		= KS_FILTERED_BIN,
	.event_id	= -1,
	.offset		= 0,
	.ts		= 0
};

static const struct kshark_entry *
get_entry(const struct kshark_entry_request *req,
          struct kshark_entry **data,
          ssize_t *index, size_t start, ssize_t end, int inc)
{
	struct kshark_context *kshark_ctx = NULL;
	const struct kshark_entry *e = NULL;
	ssize_t i;

	if (index)
		*index = KS_EMPTY_BIN;

	if (!kshark_instance(&kshark_ctx))
		return e;

	for (i = start; i != end; i += inc) {
		if (req->cond(kshark_ctx, data[i], req->sd, req->val)) {
			/*
			 * Data satisfying the condition has been found.
			 */
			if (req->vis_only &&
			    !(data[i]->visible & req->vis_mask)) {
				/* This data entry has been filtered. */
				e = &dummy_entry;
			} else {
				e = data[i];
				break;
			}
		}
	}

	if (index) {
		if (e)
			*index = (e->event_id >= 0)? i : KS_FILTERED_BIN;
		else
			*index = KS_EMPTY_BIN;
	}

	return e;
}

/**
 * @brief Search for an entry satisfying the requirements of a given Data
 *	  request. Start from the position provided by the request and go
 *	  searching in the direction of the increasing timestamps (front).
 *
 * @param req: Input location for Data request.
 * @param data: Input location for the trace data.
 * @param index: Optional output location for the index of the returned
 *		 entry inside the array.
 *
 * @returns Pointer to the first entry satisfying the matching conditionon
 *	    success, or NULL on failure.
 *	    In the special case when some entries, satisfying the Matching
 *	    condition function have been found, but all these entries have
 *	    been discarded because of the visibility criteria (filtered
 *	    entries), the function returns a pointer to a special
 *	    "Dummy entry".
 */
const struct kshark_entry *
kshark_get_entry_front(const struct kshark_entry_request *req,
                       struct kshark_entry **data,
                       ssize_t *index)
{
	ssize_t end = req->first + req->n;

	return get_entry(req, data, index, req->first, end, +1);
}

/**
 * @brief Search for an entry satisfying the requirements of a given Data
 *	  request. Start from the position provided by the request and go
 *	  searching in the direction of the decreasing timestamps (back).
 *
 * @param req: Input location for Data request.
 * @param data: Input location for the trace data.
 * @param index: Optional output location for the index of the returned
 *		 entry inside the array.
 *
 * @returns Pointer to the first entry satisfying the matching conditionon
 *	    success, or NULL on failure.
 *	    In the special case when some entries, satisfying the Matching
 *	    condition function have been found, but all these entries have
 *	    been discarded because of the visibility criteria (filtered
 *	    entries), the function returns a pointer to a special
 *	    "Dummy entry".
 */
const struct kshark_entry *
kshark_get_entry_back(const struct kshark_entry_request *req,
                      struct kshark_entry **data,
                      ssize_t *index)
{
	ssize_t end = req->first - req->n;

	if (end < 0)
		end = -1;

	return get_entry(req, data, index, req->first, end, -1);
}
