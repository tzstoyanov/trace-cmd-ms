/*
 * Copyright (C) 2017 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License (not later!)
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not,  see <http://www.gnu.org/licenses>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#ifndef _LIB_KSHARK_H
#define _LIB_KSHARK_H

// C
#include <stdint.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

// trace-cmd
#include "trace-cmd.h"
#include "event-parse.h"
#include "trace-filter-hash.h"

// KernelShark
#include "libkshark-plugin.h"
#include "libkshark-collection.h"

#define TASK_HASH_SIZE 1024

#define KS_EMPTY_BIN		-1
#define KS_FILTERED_BIN		-2

#define KS_LAST_BIN		-4

enum kshark_filter_masks {
	KS_VIEW_FILTER_MASK		= (1 << 0),
	KS_GRAPH_FILTER_MASK		= (1 << 1),
	KS_EVENT_FILTER_MASK		= (1 << 2),
	KS_PLUGIN_UNTOUCHED_MASK	= (1 << 7)
};

struct kshark_entry {
	uint8_t		visible;
	uint8_t		cpu;

	int16_t		pid;

	int		event_id;

	uint64_t	offset;
	uint64_t	ts;

	struct kshark_entry *next;
};

struct task_list {
	struct task_list	*next;
	int			 pid;
};

struct kshark_entry_collection;

struct kshark_context {
	struct tracecmd_input	*handle;
	struct pevent		*pevt;

	uint8_t			filter_mask;

	struct task_list	*tasks[TASK_HASH_SIZE];
	struct filter_id	*show_task_filter;  /* hash of tasks to filter on */
	struct filter_id	*hide_task_filter;  /* hash of tasks to not display */

	struct filter_id	*show_event_filter;  /* hash of events to filter on */
	struct filter_id	*hide_event_filter;  /* hash of events to not display */

	struct event_filter	*advanced_event_filter;
	bool			adv_filter_is_set;

	struct plugin_list	*plugins;

	struct gui_event_handler *event_handlers;

	struct kshark_entry_collection *collections;

	pthread_mutex_t input_mutex;
};

int kshark_instance(struct kshark_context **kshark_ctx);

bool kshark_open(struct kshark_context *kshark_ctx, const char *file);

size_t kshark_load_data_entries(struct kshark_context *kshark_ctx,
				struct kshark_entry ***data_rows);

size_t kshark_load_data_records(struct kshark_context *kshark_ctx,
				struct pevent_record ***data_rows);

size_t kshark_load_data_matrix(struct kshark_context *kshark_ctx,
			       uint64_t	**offset_array,
			       uint8_t	**cpu_array,
			       uint64_t	**ts_array,
			       uint16_t	**pid_array,
			       int	**event_array,
			       uint8_t	**vis_array);

void kshark_close(struct kshark_context *kshark_ctx);

void kshark_free(struct kshark_context *kshark_ctx);

int kshark_get_pid_lazy(struct kshark_entry *entry);

const char *kshark_get_task(struct pevent *pevt,
			    struct pevent_record *record);

const char *kshark_get_task_lazy(struct kshark_entry *entry);

const char *kshark_get_latency(struct pevent *pevt,
			       struct pevent_record *record);

const char *kshark_get_latency_lazy(struct kshark_entry *entry);

const char *kshark_get_event_name(struct pevent *pevt,
				  struct pevent_record *record);

const char *kshark_get_event_name_lazy(struct kshark_entry *entry);

const char *kshark_get_info(struct pevent *pevt,
			    struct pevent_record *record,
			    struct kshark_entry *entry);

const char *kshark_get_info_lazy(struct kshark_entry *entry);

void kshark_set_entry_values(struct kshark_context *kshark_ctx,
			     struct pevent_record *record,
			     struct kshark_entry *entry);

char* kshark_dump_entry(struct kshark_entry *entry);

enum kshark_filter_type {
	SHOW_EVENT_FILTER,
	HIDE_EVENT_FILTER,
	SHOW_TASK_FILTER,
	HIDE_TASK_FILTER,
};

void kshark_filter_add_id(struct kshark_context *kshark_ctx, int filter_id, int id);

void kshark_filter_clear(struct kshark_context *kshark_ctx, int filter_id);

bool kshark_filter_is_set(struct kshark_context *kshark_ctx);

size_t kshark_filter_entries(struct kshark_context *kshark_ctx,
			     struct kshark_entry **data_rows,
			     size_t n_entries);

size_t kshark_find_entry_row(uint64_t time,
			     struct kshark_entry **data_rows,
			     size_t l, size_t h);

size_t kshark_find_record_row(uint64_t time,
			      struct pevent_record **data_rows,
			      size_t l, size_t h);

void kshark_convert_nano(uint64_t time, uint64_t *sec, uint64_t *usec);

int kshark_get_plugins(char ***plugins);

struct pevent *kshark_local_events();

char **kshark_get_event_format_fields(struct event_format *event);

bool kshark_check_pid(struct kshark_context *kshark_ctx,
		      struct kshark_entry *e, int pid);

bool kshark_check_cpu(struct kshark_context *kshark_ctx,
		      struct kshark_entry *e, int cpu);

bool kshark_check_cpu_visible(struct kshark_context *kshark_ctx,
			      struct kshark_entry *e, int cpu);

struct kshark_entry *kshark_get_entry_front(const struct kshark_entry_request *req,
					    struct kshark_entry **data);

struct kshark_entry *kshark_get_entry_back(const struct kshark_entry_request *req,
					   struct kshark_entry **data);

#ifdef __cplusplus
}
#endif

#endif // _LIB_KSHARK_H