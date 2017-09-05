#include <stdlib.h>
#include <assert.h>

#include "ks-view.h"

void ks_set_entry_values(	struct pevent			*pevent,
							struct pevent_record	*record,
							struct ks_entry			*entry)
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


struct ks_entry* ks_get_entry(	struct pevent			*pevent,
							struct pevent_record	*record)
{
	struct ks_entry *e = malloc(sizeof(struct ks_entry));
	ks_set_entry_values(pevent, record, e);
	return e;
}

void ks_clear_entry(struct ks_entry	*entry)
{
	free(entry->task);
	free(entry->latency);
	free(entry->event);
	free(entry->info);
}

void ks_free_entry(struct ks_entry	*entry)
{
	free(entry->task);
	free(entry->latency);
	free(entry->event);
	free(entry->info);
	free(entry);
	entry = NULL;
}

size_t ks_load_data_old(struct tracecmd_input *handle, struct ks_entry ***data_rows)
{
	int cpus = tracecmd_cpus(handle), cpu;
	size_t count, total=0;
	struct ks_entry **cpu_list = calloc(cpus, sizeof(struct ks_entry *));
	struct ks_entry *rec, **next;

	struct pevent_record	*data;
	struct pevent			*pevt;

	for (cpu = 0; cpu < cpus; ++cpu) {
		count = 0;
		cpu_list[cpu] = NULL;
		next = &cpu_list[cpu];

		data = tracecmd_read_cpu_first(handle, cpu);
		while (data) {
			*next = rec = malloc( sizeof(struct ks_entry) );
			assert(rec != NULL);

			pevt = tracecmd_get_pevent(handle);
			ks_set_entry_values(pevt, data, rec);
// 			rec.cpu_pos = count;
		
			rec->next = NULL;
			next = &(rec->next);
			free_record(data);
			
			++count;
			data = tracecmd_read_data(handle, cpu);
		}
		total += count;
	}

	struct ks_entry **rows;
	rows = calloc (total, sizeof(struct ks_entry *));

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


size_t ks_load_data(struct tracecmd_input *handle, struct pevent_record ***data_rows)
{
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
			*temp_next = temp_rec = malloc( sizeof(struct pevent_record) );
			assert(temp_rec != NULL);

// 			ks_set_entry_values(pevt, data, rec);
// 			rec.cpu_pos = count;

			temp_rec->rec = data;
			temp_rec->next = NULL;
			temp_next = &(temp_rec->next);
// 			free_record(data);

			++count;
			data = tracecmd_read_data(handle, cpu);
		}
		total += count;
	}

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
	return total;
}

