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
 
#ifndef _ks_view_h_included_
#define _ks_view_h_included_

#include <stdint.h>
#include "trace-cmd.h"

#ifdef __cplusplus
extern "C" {
#endif
	
enum {
	TRACE_VIEW_STORE_COL_INDEX,
	TRACE_VIEW_STORE_COL_CPU,
	TRACE_VIEW_STORE_COL_TS,
	TRACE_VIEW_STORE_COL_COMM,
	TRACE_VIEW_STORE_COL_PID,
	TRACE_VIEW_STORE_COL_LAT,
	TRACE_VIEW_STORE_COL_EVENT,
	TRACE_VIEW_STORE_COL_INFO,
	TRACE_VIEW_STORE_N_COLUMNS,
};

struct ks_entry {
	uint32_t	pos;
	uint8_t		cpu;
	uint64_t	timestamp;
	char* 		task;
	int32_t		pid;
	char* 		latency;
	char* 		event;
	char* 		info;

	struct ks_entry *next;
	// int visible;
};

void ks_set_entry_values(struct pevent *pevent,
			 struct pevent_record *record,
			 struct ks_entry *entry);

char* ks_dump_entry(struct ks_entry *entry, int *size);

struct ks_entry* ks_get_entry(struct pevent *pevent,
			      struct pevent_record *record);

size_t ks_load_data_old(struct tracecmd_input *handle,
			struct ks_entry ***data_rows);

void ks_clear_entry(struct ks_entry *entry);

void ks_free_entry(struct ks_entry *entry);

size_t ks_load_data(struct tracecmd_input *handle,
		    struct pevent_record ***data_rows);

uint32_t ks_find_row(uint64_t time,
		     struct pevent_record **data_rows,
		     uint32_t l, uint32_t h);

#ifdef __cplusplus
}
#endif

#endif // _ks_view_h_included_

