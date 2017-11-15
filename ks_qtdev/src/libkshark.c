
#define _GNU_SOURCE  
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/time.h>    

#include "libkshark.h"


static __thread struct trace_seq seq;
static struct kshark_context *kshark_context_handler = NULL;

static int kshark_default_context(struct kshark_context **context)
{
	struct kshark_context *ctx;

	ctx = calloc(1, sizeof(*ctx));
	if (!ctx)
	return -1;

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

int kshark_init(struct kshark_context **context)
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
	struct kshark_context *ctx = kshark_context_handler;
	if (!ctx)
		return NULL;

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

size_t kshark_load_data_entries(struct tracecmd_input *handle,
				struct kshark_entry ***data_rows)
{

	struct kshark_context *ctx = NULL;
	kshark_init(&ctx);

	ctx->handle = handle;
	ctx->pevt = tracecmd_get_pevent(handle);

	if (!seq.buffer)
		trace_seq_init(&seq);

	struct timeval t1, t2, t3;
	double elapsedTime;
	gettimeofday(&t1, NULL);

	int cpus = tracecmd_cpus(handle);
	int cpu;
	size_t count, total = 0;
	struct kshark_entry **cpu_list = calloc(cpus, sizeof(struct kshark_entry *));
	struct kshark_entry *rec, **next;

	struct pevent_record *data;

	for (cpu = 0; cpu < cpus; ++cpu) {
		count = 0;
		cpu_list[cpu] = NULL;
		next = &cpu_list[cpu];

		data = tracecmd_read_cpu_first(handle, cpu);
		while (data) {
			*next = rec = malloc( sizeof(struct kshark_entry) );
			assert(rec != NULL);

			kshark_set_entry_values(ctx->pevt, data, rec);

			rec->next = NULL;
			next = &(rec->next);
			free_record(data);

			++count;
			data = tracecmd_read_data(handle, cpu);
		}
		total += count;
	}

	gettimeofday(&t2, NULL);
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
	elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;
	fprintf(stderr, "tot: %zu  load time1: %f ms\n", total, elapsedTime);

	struct kshark_entry **rows;
	rows = calloc (total, sizeof(struct kshark_entry *));

	count = 0;
	int next_cpu;
	uint64_t ts;
	while (count < total) {
		ts=0;
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

	gettimeofday(&t3, NULL);
	elapsedTime = (t3.tv_sec - t2.tv_sec) * 1000.0;      // sec to ms
	elapsedTime += (t3.tv_usec - t2.tv_usec) / 1000.0;
	fprintf(stderr, "load time2: %f ms\n", elapsedTime);

	return total;
}

size_t kshark_load_data_records(struct tracecmd_input *handle,
				struct pevent_record ***data_rows)
{
	struct timeval t1, t2, t3;
	double elapsedTime;
	gettimeofday(&t1, NULL);

	if (!seq.buffer)
		trace_seq_init(&seq);

	int cpus = tracecmd_cpus(handle), cpu;
	size_t count, total=0;

	struct temp {
		struct pevent_record	*rec;
		struct temp		*next;
	} **cpu_list, **temp_next, *temp_rec;

	cpu_list = calloc(cpus, sizeof(struct temp *));

	struct pevent_record	*data;

	for (cpu = 0; cpu < cpus; ++cpu) {
		count = 0;
		cpu_list[cpu] = NULL;
		temp_next = &cpu_list[cpu];

		data = tracecmd_read_cpu_first(handle, cpu);
		while (data) {
			*temp_next = temp_rec = malloc(sizeof(*temp_rec));

			temp_rec->rec = data;
			temp_rec->next = NULL;
			temp_next = &(temp_rec->next);

			++count;
			data = tracecmd_read_data(handle, cpu);
		}

		total += count;
	}

	gettimeofday(&t2, NULL);
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
	elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;
	fprintf(stderr, "load time1: %f ms\n", elapsedTime);

	struct pevent_record **rows;
	rows = calloc (total, sizeof(struct pevent_record *));

	count = 0;
	int next_cpu;
	uint64_t ts;
	while (count < total) {
		ts=0;
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

	gettimeofday(&t3, NULL);
	elapsedTime = (t3.tv_sec - t2.tv_sec) * 1000.0;      // sec to ms
	elapsedTime += (t3.tv_usec - t2.tv_usec) / 1000.0;
	fprintf(stderr, "load time2: %f ms\n", elapsedTime);

	return total;
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
