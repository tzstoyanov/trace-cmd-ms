// C
#define _GNU_SOURCE 1
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/time.h>

// trace-cmd
#include "trace-hash.h"

// Kernel shark
#include "libkshark.h"
#include "KsDeff.h"

static __thread struct trace_seq seq;
static struct kshark_context *kshark_context_handler = NULL;

static int kshark_default_context(struct kshark_context **context)
{
	struct kshark_context *ctx;

	ctx = calloc(1, sizeof(*ctx));
	if (!ctx)
		return -1;

	ctx->show_task_filter = filter_task_hash_alloc();
	ctx->hide_task_filter = filter_task_hash_alloc();

	ctx->show_event_filter = filter_task_hash_alloc();
	ctx->hide_event_filter = filter_task_hash_alloc();

	if (*context && *context != kshark_context_handler) {
		free(*context);
	}

	if (kshark_context_handler) {
		free(kshark_context_handler);
	}

	kshark_context_handler = ctx;
	*context = ctx;

	return 0;
}

void kshark_close(struct kshark_context *ctx)
{
	if (!ctx)
		return;

	filter_task_hash_free(ctx->show_task_filter);
	filter_task_hash_free(ctx->hide_task_filter);

	filter_task_hash_free(ctx->show_event_filter);
	filter_task_hash_free(ctx->hide_event_filter);

	if (ctx == kshark_context_handler)
		kshark_context_handler = NULL;

	free(ctx);
}

int kshark_instance(struct kshark_context **context)
{
	if (*context == NULL && kshark_context_handler == NULL) {
		// No kshark_context exists. Create a default one.
		int status = kshark_default_context(context);
		if (status != 0)
			return status;
	} else if (*context != NULL) {
		// The context handler is already set. Use it.
		kshark_context_handler = *context;
	} else {
		// The context is already set. Use it.
		*context = kshark_context_handler;
	}

	return true;
}

const char *kshark_get_task(struct pevent *pe, struct kshark_entry *entry)
{
	return pevent_data_comm_from_pid(pe, entry->pid);
}

char *kshark_get_latency(struct pevent *pe,
			 struct pevent_record *record)
{
	trace_seq_reset(&seq);
	pevent_data_lat_fmt(pe, &seq, record);
	return seq.buffer;
}

char *kshark_get_event_name(struct pevent *pe, struct kshark_entry *entry)
{
	trace_seq_reset(&seq);
	struct event_format *event
		= pevent_data_event_from_type(pe, entry->event_id);

	if (event)
		return event->name;

	return NULL;
}

char *kshark_get_info(struct pevent *pe,
		      struct pevent_record *record,
		      struct kshark_entry *entry)
{
	trace_seq_reset(&seq);
	struct event_format *event
		= pevent_data_event_from_type(pe, entry->event_id);

	if (!event)
		return NULL;

	pevent_event_info(&seq, event, record);
	return seq.buffer;
}

const char *kshark_get_task_lazy(int16_t pid)
{
	struct kshark_context *ctx = NULL;
	kshark_instance(&ctx);
	return pevent_data_comm_from_pid(ctx->pevt, pid);
}

char *kshark_get_latency_lazy(struct kshark_entry *entry)
{
	char *lat;
	struct kshark_context *ctx = NULL;
	kshark_instance(&ctx);

	struct pevent_record *data =
		tracecmd_read_at(ctx->handle,
				 entry->offset,
				 NULL);

	lat = kshark_get_latency(ctx->pevt, data);
	free_record(data);
	return lat;
}

char *kshark_get_event_name_lazy(struct kshark_entry *entry)
{
	struct kshark_context *ctx = NULL;
	kshark_instance(&ctx);

	return kshark_get_event_name(ctx->pevt, entry);
}
char *kshark_get_info_lazy(struct kshark_entry *entry)
{
	char *info;
	struct kshark_context *ctx = NULL;
	kshark_instance(&ctx);

	struct pevent_record *data =
		tracecmd_read_at(ctx->handle,
				 entry->offset,
				 NULL);

	info = kshark_get_info(ctx->pevt, data, entry);
	free_record(data);
	return info;
}

void kshark_set_entry_values(struct pevent *pe,
			     struct pevent_record *record,
			     struct kshark_entry *entry)
{
	// Offset
	entry->offset = record->offset;

	// CPU Id
	entry->cpu = record->cpu;

	// Time stamp
	entry->ts = record->ts;

	// PID
	entry->pid = pevent_data_pid(pe, record);

	// Event
	entry->event_id = pevent_data_type(pe, record);

	// Is vizible
	entry->visible = 1;
}

char* kshark_dump_entry(struct kshark_entry *entry, int *size)
{
	struct kshark_context *ctx = NULL;
	kshark_instance(&ctx);

	char *tmp_str, *entry_str;
	int size_tmp;
	*size = 0;

	int cpu;
	struct pevent_record *data = tracecmd_read_at(ctx->handle,
					       entry->offset,
					       &cpu);

	size_tmp = asprintf(&tmp_str, "%s-%i; CPU %i; %s;",
			      kshark_get_task(ctx->pevt, entry),
			      entry->pid,
			      entry->cpu,
			      kshark_get_latency(ctx->pevt, data));

	if (size_tmp) {
		*size = asprintf(&entry_str, "%s %s; %s;",
				 tmp_str,
				 kshark_get_event_name(ctx->pevt, entry),
				 kshark_get_info(ctx->pevt, data, entry));

		free(tmp_str);
	}

	free_record(data);

	if (size > 0)
		return entry_str;

	return NULL;
}

struct kshark_entry* kshark_get_entry(struct pevent *pe,
				      struct pevent_record *record)
{
	struct kshark_entry *e = malloc(sizeof(struct kshark_entry));
	kshark_set_entry_values(pe, record, e);
	return e;
}

static unsigned int get_task_hash_key(int pid)
{
	return trace_hash(pid) % TASK_HASH_SIZE;
}

static struct task_list *find_task_hash(struct kshark_context *ctx,
					int key, int pid)
{
	struct task_list *list;

	for (list = ctx->tasks[key]; list; list = list->next) {
		if (list->pid == pid)
			return list;
	}

	return NULL;
}

static struct task_list *add_task_hash(struct kshark_context *ctx,
				       int pid)
{
	struct task_list *list;
	uint key = get_task_hash_key(pid);

	list = find_task_hash(ctx, key, pid);
	if (list)
		return list;

	list = malloc(sizeof(*list));
	list->pid = pid;
	list->next = ctx->tasks[key];
	ctx->tasks[key] = list;

	return list;
}

static void free_task_hash(struct kshark_context *ctx)
{
	struct task_list *list;
	int i;

	for (i = 0; i < TASK_HASH_SIZE; i++) {
		while (ctx->tasks[i]) {
			list = ctx->tasks[i];
			ctx->tasks[i] = list->next;
			free(list);
		}
	}
}

static void kshark_reset_file_context(struct tracecmd_input *handle,
				      struct kshark_context *ctx)
{
	ctx->handle = handle;
	ctx->pevt = tracecmd_get_pevent(handle);
	free_task_hash(ctx);
}

static bool kshark_view_task(struct kshark_context *ctx, int pid)
{
	return (!ctx->show_task_filter ||
		!filter_task_count(ctx->show_task_filter) ||
		filter_task_find_pid(ctx->show_task_filter, pid)) &&
		(!ctx->hide_task_filter ||
		 !filter_task_count(ctx->hide_task_filter) ||
		 !filter_task_find_pid(ctx->hide_task_filter, pid));
}

static bool kshark_view_event(struct kshark_context *ctx, int event_id)
{
	return (!ctx->show_event_filter ||
		!filter_task_count(ctx->show_event_filter) ||
		filter_task_find_pid(ctx->show_event_filter, event_id)) &&
		(!ctx->hide_event_filter ||
		 !filter_task_count(ctx->hide_event_filter) ||
		 !filter_task_find_pid(ctx->hide_event_filter, event_id));
}

static bool kshark_show_entry(struct kshark_context *ctx, int pid, int event_id)
{
	if (kshark_view_task(ctx, pid) && kshark_view_event(ctx, event_id))
		return true;

	return false;
}

size_t kshark_load_data_records(struct tracecmd_input *handle,
				struct pevent_record ***data_rows)
{
	int cpus = tracecmd_cpus(handle);
	int cpu;
	size_t count, total = 0;
	struct pevent_record *data;
	struct kshark_context *ctx = NULL;
	kshark_instance(&ctx);
	kshark_reset_file_context(handle, ctx);

	if (!seq.buffer)
		trace_seq_init(&seq);

	struct temp {
		struct pevent_record	*rec;
		struct temp		*next;
	} **cpu_list, **temp_next, *temp_rec;

	cpu_list = calloc(cpus, sizeof(struct temp *));

	for (cpu = 0; cpu < cpus; ++cpu) {
		count = 0;
		cpu_list[cpu] = NULL;
		temp_next = &cpu_list[cpu];

		data = tracecmd_read_cpu_first(handle, cpu);
		while (data) {
			*temp_next = temp_rec = malloc(sizeof(*temp_rec));
			assert(temp_rec != NULL);

			add_task_hash(ctx, pevent_data_pid(ctx->pevt, data));
			temp_rec->rec = data;
			temp_rec->next = NULL;
			temp_next = &(temp_rec->next);

			++count;
			data = tracecmd_read_data(handle, cpu);
		}

		total += count;
	}

	struct pevent_record **rows;
	rows = calloc(total, sizeof(struct pevent_record *));

	count = 0;
	int next_cpu;
	uint64_t ts;
	while (count < total) {
		ts = 0;
		next_cpu = -1;
		for (cpu = 0; cpu < cpus; ++cpu) {
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

size_t kshark_load_data_entries(struct tracecmd_input *handle,
				struct kshark_entry ***data_rows)
{
	int cpus = tracecmd_cpus(handle);
	int cpu;
	size_t count, total = 0;
	struct pevent_record *data;
	struct kshark_context *ctx = NULL;
	kshark_instance(&ctx);
	kshark_reset_file_context(handle, ctx);

	if (!seq.buffer)
		trace_seq_init(&seq);

	struct kshark_entry *rec, **next;
	struct kshark_entry **cpu_list = calloc(cpus, sizeof(struct kshark_entry *));

	for (cpu = 0; cpu < cpus; ++cpu) {
		count = 0;
		cpu_list[cpu] = NULL;
		next = &cpu_list[cpu];

		data = tracecmd_read_cpu_first(handle, cpu);
		while (data) {
			*next = rec = malloc( sizeof(struct kshark_entry) );
			assert(rec != NULL);

			kshark_set_entry_values(ctx->pevt, data, rec);
			add_task_hash(ctx, rec->pid);

			rec->next = NULL;
			next = &(rec->next);
			free_record(data);

			++count;
			data = tracecmd_read_data(handle, cpu);
		}
		total += count;
	}

	struct kshark_entry **rows;
	rows = calloc(total, sizeof(struct kshark_entry *));

	count = 0;
	int next_cpu;
	uint64_t ts;
	while (count < total) {
		ts = 0;
		next_cpu = -1;
		for (cpu = 0; cpu < cpus; ++cpu) {
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

size_t kshark_load_data_matrix(struct tracecmd_input *handle,
			       uint64_t **offset_array,
			       uint8_t **cpu_array,
			       uint64_t **ts_array,
			       uint16_t **pid_array,
			       int **event_array,
			       uint8_t **vis_array)
{
	int cpus = tracecmd_cpus(handle);
	int cpu;
	size_t count, total = 0;
	struct pevent_record *data;
	struct kshark_context *ctx = NULL;
	kshark_instance(&ctx);
	kshark_reset_file_context(handle, ctx);

	if (!seq.buffer)
		trace_seq_init(&seq);

	struct kshark_entry *rec, **next;
	struct kshark_entry **cpu_list = calloc(cpus, sizeof(struct kshark_entry *));

	for (cpu = 0; cpu < cpus; ++cpu) {
		count = 0;
		cpu_list[cpu] = NULL;
		next = &cpu_list[cpu];

		data = tracecmd_read_cpu_first(handle, cpu);
		while (data) {
			*next = rec = malloc( sizeof(struct kshark_entry) );
			assert(rec != NULL);

			kshark_set_entry_values(ctx->pevt, data, rec);
			add_task_hash(ctx, rec->pid);
			rec->next = NULL;
			next = &(rec->next);
			free_record(data);

			++count;
			data = tracecmd_read_data(handle, cpu);
		}
		total += count;
	}

	*offset_array	= calloc(total, sizeof(uint64_t));
	*cpu_array	= calloc(total, sizeof(uint8_t));
	*ts_array	= calloc(total, sizeof(uint64_t));
	*pid_array	= calloc(total, sizeof(uint16_t));
	*event_array	= calloc(total, sizeof(int));
	*vis_array	= calloc(total, sizeof(uint8_t));

	count = 0;
	int next_cpu;
	uint64_t ts;
	while (count < total) {
		ts = 0;
		next_cpu = -1;
		for (cpu = 0; cpu < cpus; ++cpu) {
			if (!cpu_list[cpu])
				continue;

			if (!ts || cpu_list[cpu]->ts < ts) {
				ts = cpu_list[cpu]->ts;
				next_cpu = cpu;
			}
		}

		if (next_cpu >= 0) {
			(*offset_array)[count] = cpu_list[next_cpu]->offset;
			(*cpu_array)[count] = cpu_list[next_cpu]->cpu;
			(*ts_array)[count] = cpu_list[next_cpu]->ts;
			(*pid_array)[count] = cpu_list[next_cpu]->pid;
			(*event_array)[count] = cpu_list[next_cpu]->event_id;
			(*event_array)[count] = cpu_list[next_cpu]->event_id;
			(*event_array)[count] = cpu_list[next_cpu]->event_id;

			rec = cpu_list[next_cpu];
			cpu_list[next_cpu] = cpu_list[next_cpu]->next;
			free(rec);
		}

		++count;
	}

	free(cpu_list);
	return total;
}

size_t kshark_trace2matrix(const char *fname,
			   uint64_t **offset_array,
			   uint8_t **cpu_array,
			   uint64_t **ts_array,
			   uint16_t **pid_array,
			   int **event_array,
			   uint8_t **vis_array)
{
	struct tracecmd_input *handle = tracecmd_open(fname);
	return kshark_load_data_matrix(handle, offset_array,
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

void kshark_filter_add_id(struct kshark_context *ctx, int filter_id, int id)
{
	switch (filter_id) {
		case SHOW_EVENT_FILTER:
			filter_task_add_pid(ctx->show_event_filter, id);
			break;

		case HIDE_EVENT_FILTER:
			filter_task_add_pid(ctx->hide_event_filter, id);
			break;

		case SHOW_TASK_FILTER:
			filter_task_add_pid(ctx->show_task_filter, id);
			break;

		case HIDE_TASK_FILTER:
			filter_task_add_pid(ctx->hide_task_filter, id);
			break;

		default:
			break;
	}
}

void kshark_filter_clear(struct kshark_context *ctx, int filter_id)
{
	switch (filter_id) {
		case SHOW_EVENT_FILTER:
			filter_task_clear(ctx->show_event_filter);
			break;

		case HIDE_EVENT_FILTER:
			filter_task_clear(ctx->hide_event_filter);
			break;

		case SHOW_TASK_FILTER:
			filter_task_clear(ctx->show_task_filter);
			break;

		case HIDE_TASK_FILTER:
			filter_task_clear(ctx->hide_task_filter);
			break;

		default:
			break;
	}
}

size_t kshark_filter_entries(struct kshark_context *ctx,
			     struct kshark_entry **data_rows,
			     size_t n_entries)
{
	int i, count = 0;
	for (i = 0; i < n_entries; ++i) {
		data_rows[i]->visible = false;

		if (kshark_show_entry(ctx, data_rows[i]->pid, data_rows[i]->event_id)) {
			data_rows[i]->visible = true;
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