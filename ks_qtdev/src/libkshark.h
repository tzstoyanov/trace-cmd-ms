/*
 * Copyright (C) 2009, 2010 Red Hat Inc, Steven Rostedt <srostedt@redhat.com>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License (not later!)
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not,  see <http://www.gnu.org/licenses>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
 
#ifndef _LIB_KSHARK_H
#define _LIB_KSHARK_H

// C
#include <stdint.h>

// trace-cmd
#include "trace-cmd.h"
#include "trace-filter-hash.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TASK_HASH_SIZE 1024
struct task_list {
	struct task_list	*next;
	int			 pid;
};

struct kshark_context {
	struct tracecmd_input	*handle;
	struct pevent		*pevt;

	struct task_list	*tasks[TASK_HASH_SIZE];
	struct filter_task	*show_task_filter;  /* hash of tasks to filter on */
	struct filter_task	*hide_task_filter;  /* hash of tasks to not display */

	struct filter_task	*show_event_filter;  /* hash of events to filter on */
	struct filter_task	*hide_event_filter;  /* hash of events to not display */
};

int kshark_instance(struct kshark_context **ctx);

void kshark_close(struct kshark_context *ctx);

struct kshark_entry {
	struct kshark_entry *next;

	uint64_t	offset;
	uint8_t		cpu;
	uint64_t	ts;
	int16_t		pid;

	int		event_id;
	bool		visible;
};

const char *kshark_get_task(struct pevent *pevt,
			    struct kshark_entry *entry);

const char *kshark_get_task_lazy(int16_t pid);

char *kshark_get_latency(struct pevent *pevt,
			 struct pevent_record *record);

char *kshark_get_latency_lazy(struct kshark_entry *entry);

char *kshark_get_event_name(struct pevent *pevt,
			    struct kshark_entry *entry);

char *kshark_get_event_name_lazy(struct kshark_entry *entry);

char *kshark_get_info(struct pevent *pevt,
		      struct pevent_record *record,
		      struct kshark_entry *entry);

char *kshark_get_info_lazy(struct kshark_entry *entry);

void kshark_set_entry_values(struct pevent *pevt,
			     struct pevent_record *record,
			     struct kshark_entry *entry);

void kshark_set_entry_values(struct pevent *pevt,
			     struct pevent_record *record,
			     struct kshark_entry *entry);

char* kshark_dump_entry(struct kshark_entry *entry, int *size);

struct kshark_entry* kshark_get_entry(struct pevent *pevt,
				      struct pevent_record *record);

size_t kshark_load_data_entries(struct tracecmd_input *handle,
				struct kshark_entry ***data_rows);

size_t kshark_load_data_records(struct tracecmd_input *handle,
				struct pevent_record ***data_rows);

size_t kshark_load_data_matrix(struct tracecmd_input *handle,
			       uint64_t **offset_array,
			       uint8_t **cpu_array,
			       uint64_t **ts_array,
			       uint16_t **pid_array,
			       int **event_array,
			       uint8_t **vis_array);

uint32_t kshark_find_entry_row(uint64_t time,
			       struct kshark_entry **data_rows,
			       uint32_t l, uint32_t h);

uint32_t kshark_find_record_row(uint64_t time,
				struct pevent_record **data_rows,
				uint32_t l, uint32_t h);

enum kshark_filter_type {
	SHOW_EVENT_FILTER,
	HIDE_EVENT_FILTER,
	SHOW_TASK_FILTER,
	HIDE_TASK_FILTER,
};

void kshark_filter_add_pid(struct kshark_context *ctx, int filter_id, int pid);

void kshark_filter_clear(struct kshark_context *ctx, int filter_id);

size_t kshark_filter_entries(struct kshark_context *ctx,
			     struct kshark_entry **data_rows,
			     size_t n_entries);

#ifdef __cplusplus
}
#endif

#endif // _LIB_KSHARK_H
