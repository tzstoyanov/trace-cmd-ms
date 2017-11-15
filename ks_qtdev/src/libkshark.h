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

#ifdef __cplusplus
extern "C" {
#endif

struct kshark_context {
	struct tracecmd_input *handle;
	struct pevent *pevt;
};

int kshark_init(struct kshark_context **context);

struct kshark_entry {
	struct kshark_entry *next;

	size_t		offset;
	uint8_t		cpu;
	uint64_t	ts;
	int16_t		pid;
	
	int		event_id;
	uint8_t		visible;
};

const char *kshark_get_task(struct pevent *pevt, struct kshark_entry *entry);

char *kshark_get_latency(struct pevent *pevt,
			 struct pevent_record *record);

char *kshark_get_event_name(struct pevent *pevt, struct kshark_entry *entry);

char *kshark_get_info(struct pevent *pevt,
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

uint32_t kshark_find_entry_row(uint64_t time,
			       struct kshark_entry **data_rows,
			       uint32_t l, uint32_t h);

uint32_t kshark_find_record_row(uint64_t time,
				struct pevent_record **data_rows,
				uint32_t l, uint32_t h);

#ifdef __cplusplus
}
#endif

#endif // _LIB_KSHARK_H
