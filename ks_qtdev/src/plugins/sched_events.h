/*
 *  Copyright (C) 2018 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
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

#ifndef _KS_PLUGIN_SHED_H
#define _KS_PLUGIN_SHED_H

// trace-cmd
#include "event-parse.h"

// Kernel Shark 2
#include "libkshark.h"

#ifdef __cplusplus
extern "C" {
#endif


struct plugin_sched_context {
	struct tracecmd_input	*handle;
	struct pevent		*pevt;

	struct event_format	*sched_switch_event;
	struct format_field	*sched_switch_next_field;
	struct event_format	*sched_wakeup_event;
	struct format_field	*sched_wakeup_pid_field;
// 	struct event_format	*sched_wakeup_new_event;
// 	struct format_field	*sched_wakeup_new_pid_field;
};

int plugin_get_next_pid(struct plugin_sched_context *ctx, struct pevent_record *record);

int plugin_get_wakeup_pid(struct plugin_sched_context *ctx, struct pevent_record *record);

bool plugin_wakeup_check_pid(struct kshark_context *ctx, struct kshark_entry *e, int pid);

bool plugin_switch_check_pid(struct kshark_context *ctx, struct kshark_entry *e, int pid);

void plugin_draw(void *histoPtr, void *graphPtr, void *shapeListPtr, int pid, int draw_action);

#ifdef __cplusplus
}
#endif

#endif
