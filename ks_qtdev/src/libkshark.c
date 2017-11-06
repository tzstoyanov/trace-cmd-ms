
#define _GNU_SOURCE  
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/time.h>    

#include "libkshark.h"

void kshark_set_entry_values(struct pevent *pevent,
			     struct pevent_record *record,
			     struct kshark_entry *entry)
{
	// Position
	entry->pos = -1;

	// CPU Id
	entry->cpu = record->cpu;

	// Time stamp
	entry->timestamp = record->ts;

	// PID
	entry->pid = pevent_data_pid(pevent, record);

	// Task name
	const char *comm = pevent_data_comm_from_pid(pevent, entry->pid);
	entry->task = malloc(strlen(comm) + 1);
	strcpy(entry->task, comm);

	// Latency
	struct trace_seq s;
	trace_seq_init(&s);
	pevent_data_lat_fmt(pevent, &s, record);
	entry->latency = malloc(strlen(s.buffer) + 1);
	strcpy(entry->latency, s.buffer);
	trace_seq_destroy(&s);

	// Event
	int etype = pevent_data_type(pevent, record);
	struct event_format *event = pevent_data_event_from_type(pevent, etype);
	if (!event) {
		entry->event = malloc(16);
		strcpy(entry->event, "[UNKNOWN EVENT]");
	} else {
		entry->event = malloc(strlen(event->name) + 1);
		strcpy(entry->event, event->name);
	}
	
	// Info
	trace_seq_init(&s);
	pevent_event_info(&s, event, record);
	entry->info = malloc(strlen(s.buffer) + 1);
	strcpy(entry->info, s.buffer);
	trace_seq_destroy(&s);
}

char* kshark_dump_entry(struct kshark_entry *entry, int *size)
{
	char* entry_str;
	*size = asprintf(&entry_str, "%s-%i;  CPU %i;  %s;  %s;  %s;", 
			 entry->task,
			 entry->pid,
			 entry->cpu,
			 entry->latency,
			 entry->event,
			 entry->info);
	if (size > 0)
		return entry_str;

	return NULL;
}

struct kshark_entry* kshark_get_entry(struct pevent *pevent,
				      struct pevent_record *record)
{
	struct kshark_entry *e = malloc(sizeof(struct kshark_entry));
	kshark_set_entry_values(pevent, record, e);
	return e;
}

void kshark_clear_entry(struct kshark_entry *entry)
{
	free(entry->task);
	free(entry->latency);
	free(entry->event);
	free(entry->info);
}

void kshark_free_entry(struct kshark_entry *entry)
{
	kshark_clear_entry(entry);
	free(entry);
	entry = NULL;
}

size_t kshark_load_data_old(struct tracecmd_input *handle, struct kshark_entry ***data_rows)
{
	int cpus = tracecmd_cpus(handle), cpu;
	size_t count, total=0;
	struct kshark_entry **cpu_list = calloc(cpus, sizeof(struct kshark_entry *));
	struct kshark_entry *rec, **next;

	struct pevent_record	*data;
	struct pevent			*pevt;

	for (cpu = 0; cpu < cpus; ++cpu) {
		count = 0;
		cpu_list[cpu] = NULL;
		next = &cpu_list[cpu];

		data = tracecmd_read_cpu_first(handle, cpu);
		while (data) {
			*next = rec = malloc( sizeof(struct kshark_entry) );
			assert(rec != NULL);

			pevt = tracecmd_get_pevent(handle);
			kshark_set_entry_values(pevt, data, rec);
// 			rec.cpu_pos = count;
		
			rec->next = NULL;
			next = &(rec->next);
			free_record(data);
			
			++count;
			data = tracecmd_read_data(handle, cpu);
		}
		total += count;
	}

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

			if (!ts || cpu_list[cpu]->timestamp < ts) {
				ts = cpu_list[cpu]->timestamp;
				next_cpu = cpu;
			}
		}

		if (next_cpu >= 0) {
			rows[count] = cpu_list[next_cpu];
			rows[count]->pos = count;
			cpu_list[next_cpu] = cpu_list[next_cpu]->next;
		}
		++count;
	}

	free(cpu_list);
	*data_rows = rows;
	return total;
}


size_t kshark_load_data(struct tracecmd_input *handle, struct pevent_record ***data_rows)
{
	struct timeval t1, t2, t3;
	double elapsedTime;
	gettimeofday(&t1, NULL);
	
	int cpus = tracecmd_cpus(handle), cpu;
	size_t count, total=0;

	struct temp {
		struct pevent_record	*rec;
		struct temp		*next;
	} **cpu_list, **temp_next, *temp_rec;

	cpu_list = calloc(cpus, sizeof(struct temp *));

	struct pevent_record	*data;
// 	struct pevent			*pevt;

	for (cpu = 0; cpu < cpus; ++cpu) {
		count = 0;
		cpu_list[cpu] = NULL;
		temp_next = &cpu_list[cpu];

		data = tracecmd_read_cpu_first(handle, cpu);
// 		pevt = tracecmd_get_pevent(handle);
		while (data) {
			*temp_next = temp_rec = malloc( sizeof(*temp_rec) );
			assert(temp_rec != NULL);

// 			kshark_set_entry_values(pevt, data, rec);
// 			rec.cpu_pos = count;

			temp_rec->rec = data;
			temp_rec->next = NULL;
			temp_next = &(temp_rec->next);
 			//free_record(data);

			++count;
			data = tracecmd_read_data(handle, cpu);
		}
		
		//data = tracecmd_read_cpu_first(handle, cpu);
		//while (data) {
			//*next = rec = g_malloc(sizeof(*rec));
			//g_assert(rec != NULL);
			//rec->offset = data->offset;
			//rec->ts = data->ts;
			//rec->next = NULL;
			//next = &rec->next;
			//free_record(data);
			//count++;
			//data = tracecmd_read_data(handle, cpu);
		//}
		
		total += count;
	}

	gettimeofday(&t2, NULL);
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
	elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;
	fprintf(stderr, "load time1 %f  ms, \n", elapsedTime);
	
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
	fprintf(stderr, "load time2 %f  ms, \n", elapsedTime);
    
	return total;
}

uint32_t kshark_find_row(uint64_t time,
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

