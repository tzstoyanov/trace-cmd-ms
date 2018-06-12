/*
 * Copyright (C) 2018 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
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

#ifndef _KS_PLUGIN_SHED_H
#define _KS_PLUGIN_SHED_H

// trace-cmd
#include "event-parse.h"

// KernelShark
#include "libkshark.h"

#ifdef __cplusplus
extern "C" {
#endif

struct xenomai_context {
	struct tracecmd_input	*handle;
	struct pevent 		*pevt;

	struct event_format	*cobalt_switch_event;
	struct format_field     *cobalt_switch_next_pid_field;
	struct format_field     *cobalt_switch_prev_state_field;
	struct format_field     *cobalt_switch_next_name_field;

	struct event_format	*cobalt_wakeup_event;
	struct format_field	*cobalt_wakeup_pid_field;
};

int cobalt_get_next_pid(struct pevent_record *record);

int cobalt_get_wakeup_pid(struct pevent_record *record);

int cobalt_get_wakeup_pid_lazy(struct xenomai_context *plugin_ctx, struct kshark_entry *e);

bool cobalt_switch_check_pid(struct kshark_context *ctx, struct kshark_entry *e, int pid);

bool cobalt_wakeup_check_pid(struct kshark_context *ctx, struct kshark_entry *e, int pid);

void xenomai_draw(struct kshark_cpp_argv *argv, int pid, int draw_action);

#ifdef __cplusplus
}
#endif

#endif