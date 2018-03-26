// C
#define _GNU_SOURCE 1
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/time.h>

// trace-cmd
#include "trace-cmd.h"
#include "trace-hash.h"

// Kernel shark 2
#include "libkshark.h"
#include "KsDeff.h"

static __thread struct trace_seq seq;
static struct kshark_context *kshark_context_handler = NULL;

struct kshark_entry dummy_entry;

static int kshark_default_context(struct kshark_context **context)
{
	dummy_entry.pid = KS_FILTERED_BIN;
	dummy_entry.cpu = KS_FILTERED_BIN;
	dummy_entry.visible = 0;

	struct kshark_context *kshark_ctx;

	kshark_ctx = calloc(1, sizeof(*kshark_ctx));
	if (!kshark_ctx)
		return -1;

	kshark_ctx->show_task_filter = filter_id_hash_alloc();
	kshark_ctx->hide_task_filter = filter_id_hash_alloc();

	kshark_ctx->show_event_filter = filter_id_hash_alloc();
	kshark_ctx->hide_event_filter = filter_id_hash_alloc();

	kshark_ctx->filter_mask = 0x0;

	if (*context && *context != kshark_context_handler) {
		free(*context);
	}

	if (kshark_context_handler) {
		free(kshark_context_handler);
	}

	kshark_context_handler = kshark_ctx;
	*context = kshark_ctx;

	return 0;
}

int kshark_instance(struct kshark_context **context)
{
	if (!seq.buffer)
		trace_seq_init(&seq);

	if (*context == NULL && kshark_context_handler == NULL) {
		// No kshark_context exists. Create a default one.
		int status = kshark_default_context(context);
		if (status != 0)
			return status;
	} else if (*context != NULL) {
		// The context handler is already set. Use it.
		if (kshark_context_handler)
			free(kshark_context_handler);

		kshark_context_handler = *context;
	} else {
		// The context is already set. Use it.
		*context = kshark_context_handler;
	}

	return true;
}

static void free_task_hash(struct kshark_context *kshark_ctx)
{
	struct task_list *list;
	int i;

	for (i = 0; i < TASK_HASH_SIZE; i++) {
		while (kshark_ctx->tasks[i]) {
			list = kshark_ctx->tasks[i];
			kshark_ctx->tasks[i] = list->next;
			free(list);
		}
	}
}

static void kshark_set_file_context(struct tracecmd_input *handle,
				    struct kshark_context *kshark_ctx)
{
	kshark_ctx->handle = handle;
	kshark_ctx->pevt = tracecmd_get_pevent(handle);
	free_task_hash(kshark_ctx);
}

bool kshark_open(struct kshark_context *kshark_ctx, const char *file)
{
	free_task_hash(kshark_ctx);
	struct tracecmd_input *handle = tracecmd_open(file);
	if (!handle)
		return false;
	
	kshark_ctx->handle = handle;
	kshark_ctx->pevt = tracecmd_get_pevent(handle);

	/* Turn off function trace indent and turn on show parent
	 * if possible. */
	trace_util_add_option("ftrace:parent", "1");
	trace_util_add_option("ftrace:indent", "0");

	/* Also, show the function name in the tail for function graph */
// 	trace_util_add_option("fgraph:tailprint", "1");

	return true;
}

void kshark_close(struct kshark_context *kshark_ctx)
{
	if (!kshark_ctx || !kshark_ctx->handle)
		return;

	tracecmd_close(kshark_ctx->handle);
}

void kshark_free(struct kshark_context *kshark_ctx)
{
	if (kshark_ctx == NULL && kshark_context_handler == NULL)
		return;

	if (kshark_ctx == NULL) {
		kshark_ctx = kshark_context_handler;
		kshark_context_handler = NULL;
	}

	filter_id_hash_free(kshark_ctx->show_task_filter);
	filter_id_hash_free(kshark_ctx->hide_task_filter);

	filter_id_hash_free(kshark_ctx->show_event_filter);
	filter_id_hash_free(kshark_ctx->hide_event_filter);

	if(kshark_ctx->plugins) {
		kshark_handle_plugins(kshark_ctx, KSHARK_PLUGIN_UNLOAD);
		kshark_free_plugin_list(kshark_ctx->plugins);
		kshark_free_event_handler_list(kshark_ctx->event_handlers);
	}

	free_task_hash(kshark_ctx);

	if (seq.buffer)
		trace_seq_destroy(&seq);
	
	free(kshark_ctx);
}

const char *kshark_get_comm_from_pid(struct pevent *pevt, int pid)
{
	return pevent_data_comm_from_pid(pevt, pid);
}

int kshark_get_pid_lazy(struct kshark_entry *entry)
{
	int pid;
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	struct pevent_record *data =
		tracecmd_read_at(kshark_ctx->handle,
				 entry->offset,
				 NULL);

	pid = pevent_data_pid(kshark_ctx->pevt, data);
	free_record(data);
	return pid;
}

const char *kshark_get_task(struct pevent *pe, struct kshark_entry *entry)
{
	return pevent_data_comm_from_pid(pe, entry->pid);
}

const char *kshark_get_latency(struct pevent *pe,
			       struct pevent_record *record)
{
	if (!seq.buffer)
		trace_seq_init(&seq);
	
	trace_seq_reset(&seq);
	pevent_data_lat_fmt(pe, &seq, record);
	return seq.buffer;
}

const char *kshark_get_event_name(struct pevent *pe, struct kshark_entry *entry)
{
	trace_seq_reset(&seq);
	struct event_format *event
		= pevent_data_event_from_type(pe, entry->event_id);

	if (event)
		return event->name;

	return "[UNKNOWN EVENT]";
}

const char *kshark_get_info(struct pevent *pe,
			    struct pevent_record *record,
			    struct kshark_entry *entry)
{
	if (!seq.buffer)
		trace_seq_init(&seq);

	trace_seq_reset(&seq);
	struct event_format *event
		= pevent_data_event_from_type(pe, entry->event_id);

	if (!event)
		return NULL;

	pevent_event_info(&seq, event, record);

	char *pos;
	if ((pos = strchr(seq.buffer, '\n')) != NULL)
		*pos = '\0';

	return seq.buffer;
}

const char *kshark_get_task_lazy(struct kshark_entry *entry)
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	struct pevent_record *data =
		tracecmd_read_at(kshark_ctx->handle,
				 entry->offset,
				 NULL);

	int pid = pevent_data_pid(kshark_ctx->pevt, data);
	free_record(data);
	return pevent_data_comm_from_pid(kshark_ctx->pevt, pid);
}

const char *kshark_get_latency_lazy(struct kshark_entry *entry)
{
	const char *lat;
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	struct pevent_record *data =
		tracecmd_read_at(kshark_ctx->handle,
				 entry->offset,
				 NULL);

	lat = kshark_get_latency(kshark_ctx->pevt, data);
	free_record(data);
	return lat;
}

const char *kshark_get_event_name_lazy(struct kshark_entry *entry)
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	return kshark_get_event_name(kshark_ctx->pevt, entry);
}

const char *kshark_get_info_lazy(struct kshark_entry *entry)
{
	const char *info;
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	struct pevent_record *data =
		tracecmd_read_at(kshark_ctx->handle,
				 entry->offset,
				 NULL);

	info = kshark_get_info(kshark_ctx->pevt, data, entry);
	free_record(data);
	return info;
}

void kshark_set_entry_values(struct kshark_context *kshark_ctx,
			     struct pevent_record *record,
			     struct kshark_entry *entry)
{
	// Offset
	entry->offset = record->offset;

	// CPU Id
	entry->cpu = record->cpu;

	// Time stamp
	entry->ts = record->ts;

	// Event
	entry->event_id = pevent_data_type(kshark_ctx->pevt, record);

	// Is vizible
	entry->visible = 0xFF;

	// PID
	entry->pid = pevent_data_pid(kshark_ctx->pevt, record);
}

char* kshark_dump_entry(struct kshark_entry *entry, int *size)
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	char *tmp_str, *entry_str;
	int size_tmp;
	*size = 0;

	int cpu;
	struct pevent_record *data = tracecmd_read_at(kshark_ctx->handle,
					       entry->offset,
					       &cpu);

	size_tmp = asprintf(&tmp_str, "%s-%i; CPU %i; %s;",
			      kshark_get_task(kshark_ctx->pevt, entry),
			      entry->pid,
			      entry->cpu,
			      kshark_get_latency(kshark_ctx->pevt, data));

	if (size_tmp) {
		*size = asprintf(&entry_str, "%s %s; %s;",
				 tmp_str,
				 kshark_get_event_name(kshark_ctx->pevt, entry),
				 kshark_get_info(kshark_ctx->pevt, data, entry));

		free(tmp_str);
	}

	free_record(data);

	if (size > 0)
		return entry_str;

	return NULL;
}

static unsigned int get_task_hash_key(int pid)
{
	return knuth_hash(pid) % TASK_HASH_SIZE;
}

static struct task_list *find_task_hash(struct kshark_context *kshark_ctx,
					int key, int pid)
{
	struct task_list *list;

	for (list = kshark_ctx->tasks[key]; list; list = list->next) {
		if (list->pid == pid)
			return list;
	}

	return NULL;
}

static struct task_list *add_task_hash(struct kshark_context *kshark_ctx,
				       int pid)
{
	struct task_list *list;
	uint key = get_task_hash_key(pid);

	list = find_task_hash(kshark_ctx, key, pid);
	if (list)
		return list;

	list = malloc(sizeof(*list));
	list->pid = pid;
	list->next = kshark_ctx->tasks[key];
	kshark_ctx->tasks[key] = list;

	return list;
}

static bool kshark_view_task(struct kshark_context *kshark_ctx, int pid)
{
	return (!kshark_ctx->show_task_filter ||
		!filter_id_count(kshark_ctx->show_task_filter) ||
		filter_id_find(kshark_ctx->show_task_filter, pid)) &&
		(!kshark_ctx->hide_task_filter ||
		 !filter_id_count(kshark_ctx->hide_task_filter) ||
		 !filter_id_find(kshark_ctx->hide_task_filter, pid));
}

static bool kshark_view_event(struct kshark_context *kshark_ctx, int event_id)
{
	return (!kshark_ctx->show_event_filter ||
		!filter_id_count(kshark_ctx->show_event_filter) ||
		filter_id_find(kshark_ctx->show_event_filter, event_id)) &&
		(!kshark_ctx->hide_event_filter ||
		 !filter_id_count(kshark_ctx->hide_event_filter) ||
		 !filter_id_find(kshark_ctx->hide_event_filter, event_id));
}

static bool kshark_show_entry(struct kshark_context *kshark_ctx, int pid, int event_id)
{
	if (kshark_view_task(kshark_ctx, pid) && kshark_view_event(kshark_ctx, event_id))
		return true;

	return false;
}

size_t kshark_load_data_records(struct kshark_context *kshark_ctx,
				struct pevent_record ***data_rows)
{
	int n_cpus = tracecmd_cpus(kshark_ctx->handle);
	int cpu;
	size_t count, total = 0;
	struct pevent_record *data;
	kshark_set_file_context(kshark_ctx->handle, kshark_ctx);

	struct temp {
		struct pevent_record	*rec;
		struct temp		*next;
	} **cpu_list, **temp_next, *temp_rec;

	cpu_list = calloc(n_cpus, sizeof(struct temp *));

	for (cpu = 0; cpu < n_cpus; ++cpu) {
		count = 0;
		cpu_list[cpu] = NULL;
		temp_next = &cpu_list[cpu];

		data = tracecmd_read_cpu_first(kshark_ctx->handle, cpu);
		while (data) {
			*temp_next = temp_rec = malloc(sizeof(*temp_rec));
			assert(temp_rec != NULL);

			add_task_hash(kshark_ctx, pevent_data_pid(kshark_ctx->pevt, data));
			temp_rec->rec = data;
			temp_rec->next = NULL;
			temp_next = &(temp_rec->next);

			++count;
			data = tracecmd_read_data(kshark_ctx->handle, cpu);
		}

		total += count;
	}

	struct pevent_record **rows;
	rows = calloc(total, sizeof(struct pevent_record *));
	if(!rows) {
		fprintf(stderr, "Failed to allocate memory during data loading.\n");
		return 0;
	}

	count = 0;
	int next_cpu;
	uint64_t ts;
	while (count < total) {
		ts = 0;
		next_cpu = -1;
		for (cpu = 0; cpu < n_cpus; ++cpu) {
			if (!cpu_list[cpu])
				continue;

			if (!ts || cpu_list[cpu]->rec->ts < ts) {
				ts = cpu_list[cpu]->rec->ts;
				next_cpu = cpu;
			}
		}

		if (next_cpu >= 0) {
			rows[count] = cpu_list[next_cpu]->rec;
			temp_rec = cpu_list[next_cpu];
			cpu_list[next_cpu] = cpu_list[next_cpu]->next;
			free (temp_rec);
		}
		++count;
	}

	free(cpu_list);
	*data_rows = rows;
	return total;
}

size_t kshark_load_data_entries(struct kshark_context *kshark_ctx,
				struct kshark_entry ***data_rows)
{
	
	int n_cpus = tracecmd_cpus(kshark_ctx->handle);
	int cpu;
	size_t count, total = 0;
	struct gui_event_handler *evt_handler;
	struct pevent_record *rec;

	if (*data_rows)
		free(*data_rows);

	struct kshark_entry *entry, **next;
	struct kshark_entry **cpu_list = calloc(n_cpus, sizeof(struct kshark_entry *));

	for (cpu = 0; cpu < n_cpus; ++cpu) {
		count = 0;
		cpu_list[cpu] = NULL;
		next = &cpu_list[cpu];

		rec = tracecmd_read_cpu_first(kshark_ctx->handle, cpu);
		while (rec) {
			*next = entry = malloc( sizeof(struct kshark_entry) );
			assert(entry != NULL);

			kshark_set_entry_values(kshark_ctx, rec, entry);
			add_task_hash(kshark_ctx, entry->pid);

			evt_handler = find_gui_event_handler(kshark_ctx->event_handlers, entry->event_id);
			if (evt_handler)
				evt_handler->event_func(kshark_ctx, rec, entry);

			entry->next = NULL;
			next = &(entry->next);
			free_record(rec);

			++count;
			rec = tracecmd_read_data(kshark_ctx->handle, cpu);
		}
		total += count;
	}

	struct kshark_entry **rows;
	rows = calloc(total, sizeof(struct kshark_entry *));
	if(!rows) {
		fprintf(stderr, "Failed to allocate memory during data loading.\n");
		return 0;
	}

	count = 0;
	int next_cpu;
	uint64_t ts;
	while (count < total) {
		ts = 0;
		next_cpu = -1;
		for (cpu = 0; cpu < n_cpus; ++cpu) {
			if (!cpu_list[cpu])
				continue;

			if (!ts || cpu_list[cpu]->ts < ts) {
				ts = cpu_list[cpu]->ts;
				next_cpu = cpu;
			}
		}

		if (next_cpu >= 0) {
			rows[count] = cpu_list[next_cpu];
			cpu_list[next_cpu] = cpu_list[next_cpu]->next;
		}
		++count;
	}

	free(cpu_list);
	*data_rows = rows;
	return total;
}

size_t kshark_load_data_matrix(struct kshark_context *kshark_ctx,
			       uint64_t	**offset_array,
			       uint8_t	**cpu_array,
			       uint64_t	**ts_array,
			       uint16_t	**pid_array,
			       int	**event_array,
			       uint8_t	**vis_array)
{
	int n_cpus = tracecmd_cpus(kshark_ctx->handle);
	int cpu;
	size_t count, total = 0;
	struct pevent_record *data;
	kshark_set_file_context(kshark_ctx->handle, kshark_ctx);

	struct kshark_entry *rec, **next;
	struct kshark_entry **cpu_list = calloc(n_cpus, sizeof(struct kshark_entry *));

	for (cpu = 0; cpu < n_cpus; ++cpu) {
		count = 0;
		cpu_list[cpu] = NULL;
		next = &cpu_list[cpu];

		data = tracecmd_read_cpu_first(kshark_ctx->handle, cpu);
		while (data) {
			*next = rec = malloc( sizeof(struct kshark_entry) );
			assert(rec != NULL);

			kshark_set_entry_values(kshark_ctx, data, rec);
			add_task_hash(kshark_ctx, rec->pid);
			rec->next = NULL;
			next = &(rec->next);
			free_record(data);

			++count;
			data = tracecmd_read_data(kshark_ctx->handle, cpu);
		}
		total += count;
	}

	if (offset_array) {
		*offset_array = calloc(total, sizeof(uint64_t));
		if (!offset_array)
			goto free_all;
	}

	if (cpu_array) {
		*cpu_array = calloc(total, sizeof(uint8_t));
		if (!cpu_array)
			goto free_all;
	}

	if (ts_array) {
		*ts_array = calloc(total, sizeof(uint64_t));
		if (!ts_array)
			goto free_all;
	}

	if (pid_array) {
		*pid_array = calloc(total, sizeof(uint16_t));
		if (!pid_array)
			goto free_all;
	}

	if (event_array) {
		*event_array = calloc(total, sizeof(int));
		if (!event_array)
			goto free_all;
	}

	if (vis_array) {
		*vis_array = calloc(total, sizeof(uint8_t));
		if (!vis_array)
			goto free_all;
	}

	count = 0;
	int next_cpu;
	uint64_t ts;
	while (count < total) {
		ts = 0;
		next_cpu = -1;
		for (cpu = 0; cpu < n_cpus; ++cpu) {
			if (!cpu_list[cpu])
				continue;

			if (!ts || cpu_list[cpu]->ts < ts) {
				ts = cpu_list[cpu]->ts;
				next_cpu = cpu;
			}
		}

		if (next_cpu >= 0) {
			if (offset_array)
				(*offset_array)[count] = cpu_list[next_cpu]->offset;

			if (cpu_array)
				(*cpu_array)[count] = cpu_list[next_cpu]->cpu;

			if (ts_array)
				(*ts_array)[count] = cpu_list[next_cpu]->ts;

			if (pid_array)
				(*pid_array)[count] = cpu_list[next_cpu]->pid;

			if (event_array)
				(*event_array)[count] = cpu_list[next_cpu]->event_id;

			if (vis_array)
				(*vis_array)[count] = cpu_list[next_cpu]->visible;

			rec = cpu_list[next_cpu];
			cpu_list[next_cpu] = cpu_list[next_cpu]->next;
			free(rec);
		}

		++count;
	}

	free(cpu_list);
	return total;


free_all:
	fprintf(stderr, "Failed to allocate memory during data loading.\n");

	if (offset_array)
		free(offset_array);

	if (cpu_array)
		free(cpu_array);

	if (ts_array)
		free(ts_array);

	if (pid_array)
		free(pid_array);

	if (event_array)
		free(event_array);

	if (vis_array)
		free(vis_array);
	
	return 0;
}

size_t kshark_trace2matrix(const char *fname,
			   uint64_t **offset_array,
			   uint8_t **cpu_array,
			   uint64_t **ts_array,
			   uint16_t **pid_array,
			   int **event_array,
			   uint8_t **vis_array)
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	kshark_open(kshark_ctx, fname);

	return kshark_load_data_matrix(kshark_ctx, offset_array,
					    cpu_array,
					    ts_array,
					    pid_array,
					    event_array,
					    vis_array);
}

uint32_t kshark_find_entry_row(uint64_t time,
			       struct kshark_entry **data_rows,
			       uint32_t l, uint32_t h)
{
	if (data_rows[l]->ts >= time)
		return l;

	if (data_rows[h]->ts < time)
		return h;

	uint32_t mid;
	while (h - l > 1) {
		mid = (l + h) / 2;
		if (time > data_rows[mid]->ts)
			l = mid;
		else
			h = mid;
	}

	return h;
}

uint32_t kshark_find_record_row(uint64_t time,
				struct pevent_record **data_rows,
				uint32_t l, uint32_t h)
{
	if (data_rows[l]->ts >= time)
		return l;

	if (data_rows[h]->ts < time)
		return h;

	uint32_t mid;
	while (h - l > 1) {
		mid = (l + h) / 2;
		if (time > data_rows[mid]->ts)
			l = mid;
		else
			h = mid;
	}

	return h;
}

void kshark_filter_add_id(struct kshark_context *kshark_ctx, int filter_id, int id)
{
	switch (filter_id) {
		case SHOW_EVENT_FILTER:
			filter_id_add(kshark_ctx->show_event_filter, id);
			break;

		case HIDE_EVENT_FILTER:
			filter_id_add(kshark_ctx->hide_event_filter, id);
			break;

		case SHOW_TASK_FILTER:
			filter_id_add(kshark_ctx->show_task_filter, id);
			break;

		case HIDE_TASK_FILTER:
			filter_id_add(kshark_ctx->hide_task_filter, id);
			break;

		default:
			break;
	}
}

void kshark_filter_clear(struct kshark_context *kshark_ctx, int filter_id)
{
	switch (filter_id) {
		case SHOW_EVENT_FILTER:
			filter_id_clear(kshark_ctx->show_event_filter);
			break;

		case HIDE_EVENT_FILTER:
			filter_id_clear(kshark_ctx->hide_event_filter);
			break;

		case SHOW_TASK_FILTER:
			filter_id_clear(kshark_ctx->show_task_filter);
			break;

		case HIDE_TASK_FILTER:
			filter_id_clear(kshark_ctx->hide_task_filter);
			break;

		default:
			break;
	}
}

size_t kshark_filter_entries(struct kshark_context *kshark_ctx,
			     struct kshark_entry **data_rows,
			     size_t n_entries)
{
	int i, count = 0;
	for (i = 0; i < n_entries; ++i) {
		data_rows[i]->visible = 0xFF;

		if (!kshark_show_entry(kshark_ctx, data_rows[i]->pid, data_rows[i]->event_id)) {
			data_rows[i]->visible = kshark_ctx->filter_mask;
			++count;
		}
	}

	return count;
}

void kshark_convert_nano(uint64_t time, uint64_t *sec, uint64_t *usec)
{
	*sec = time / 1000000000ULL;
	*usec = (time / 1000) % 1000000;
}

bool kshark_check_pid(struct kshark_context *kshark_ctx, struct kshark_entry *e, int pid)
{
	if (e->pid == pid)
		return true;

	return false;
}

bool kshark_check_cpu(struct kshark_context *kshark_ctx, struct kshark_entry *e, int cpu)
{
	if (e->cpu == cpu)
		return true;

	return false;
}

struct kshark_entry *kshark_get_entry_front(size_t first,
					    size_t n,
					    matching_condition_func cond,
					    int val,
					    bool vis_only,
					    int vis_mask,
					    struct kshark_entry **data)
{
	struct kshark_entry *e = NULL;
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	size_t end = first + n;
	for (size_t i = first; i < end; ++i) {
		if (cond(kshark_ctx, data[i], val)) {
			/* Data from this Task has been found. */
			if (vis_only &&
			    !(data[i]->visible & vis_mask)) {
				/* This data entry has been filtered. */
				e = &dummy_entry;
			} else {
				return data[i];
			}
		}
	}

	return e;
}

int kshark_get_pid_front(size_t first, size_t n, int cpu, bool vis_only, int vis_mask,
			 struct kshark_entry **data)
{
	if (cpu < 0)
		return KS_EMPTY_BIN;

	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	struct kshark_entry *entry = kshark_get_entry_front(first, n, kshark_check_cpu,
							    cpu, vis_only, vis_mask, data);

	if (!entry) // No data has been found.
		return KS_EMPTY_BIN;

	if (!entry->visible) // Some data has been found, but it is filtered.
		return KS_FILTERED_BIN;

	return entry->pid;
}

int kshark_get_cpu_front(size_t first, size_t n, int pid, bool vis_only, int vis_mask,
			 struct kshark_entry **data)
{
	struct kshark_entry *entry = kshark_get_entry_front(first, n, kshark_check_pid,
							    pid, vis_only, vis_mask, data);

	if (!entry) // No data has been found.
		return KS_EMPTY_BIN;

	if (!entry->visible) // Some data has been found, but it is filtered.
		return KS_FILTERED_BIN;

	return entry->cpu;
}

struct kshark_entry *kshark_get_entry_back(size_t first,
					   size_t n,
					   matching_condition_func cond,
					   int val,
					   bool vis_only,
					   int vis_mask,
					   struct kshark_entry **data)
{
	struct kshark_entry *e = NULL;
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	size_t last = 0;
	if (first - n + 1 > 0)
		last = first - n + 1;

	/* "first" is unsigned and "last" can be 0. We don't want to do "first >= 0,
	 * because this is always true. */
	for (size_t i = first + 1; i-- != last;) {
		if (cond(kshark_ctx, data[i], val)) {
			/* Data from this Task has been found. */
			if (vis_only &&
			    !(data[i]->visible & vis_mask)) {
				/* This data entry has been filtered. */
				e = &dummy_entry;
			} else {
				return data[i];
			}
		}
	}

	return e;
}

int kshark_get_pid_back(size_t first, size_t n, int cpu, bool vis_only, int vis_mask,
			struct kshark_entry **data)
{
	if (cpu < 0)
		return KS_EMPTY_BIN;

	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	struct kshark_entry *entry = kshark_get_entry_back(first, n, kshark_check_cpu,
							   cpu, vis_only, vis_mask, data);

	if (!entry)
		return KS_EMPTY_BIN;

	if (!entry->visible)  // Some data has been found, but it is filtered.
		return KS_FILTERED_BIN;

	return entry->pid;
}

int kshark_get_cpu_back(size_t first, size_t n, int pid, bool vis_only, int vis_mask,
			struct kshark_entry **data)
{
	struct kshark_entry *entry = kshark_get_entry_back(first, n, kshark_check_pid,
							   pid, vis_only, vis_mask, data);

	if (!entry)
		return KS_EMPTY_BIN;

	if (!entry->visible)  // Some data has been found, but it is filtered.
		return KS_FILTERED_BIN;

	return entry->cpu;
}

struct kshark_entry *kshark_get_entry_by_pid_back(size_t first,
						  size_t n,
						  int pid,
						  bool vis_only,
						  int vis_mask,
						  struct kshark_entry **data)
{
	return kshark_get_entry_back(first, n, kshark_check_pid,
				     pid, vis_only, vis_mask, data);
}

struct kshark_entry *kshark_get_entry_by_pid_front(size_t first,
						   size_t n,
						   int pid,
						   bool vis_only,
						   int vis_mask,
						   struct kshark_entry **data)
{
	return kshark_get_entry_front(first, n, kshark_check_pid,
				      pid, vis_only, vis_mask, data);
}

int kshark_get_plugins(char ***plugins)
{
	const char *tracing;
	char **all_plugins;
	int i = 0;

	tracing = tracecmd_get_tracing_dir();
	all_plugins = tracecmd_local_plugins(tracing);

	while (all_plugins[i]) {
// 		printf("plugin %i %s\n", i, all_plugins[i]);
		 ++i;
	}

	*plugins = all_plugins;
	return i;
}

struct pevent *kshark_local_events()
{
	const char *tracing = tracecmd_get_tracing_dir();
	return tracecmd_local_events(tracing);
}

struct event_format *kshark_find_event(struct pevent *pevt, int event_id)
{
	return pevent_find_event(pevt, event_id);
}
