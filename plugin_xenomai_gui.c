/*
 * Copyright (c) Siemens AG, 2017
 *
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
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
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include "trace-cmd.h"
#include "event-parse.h"
#include "kshark-plugin.h"
#include "kernel-shark.h"
#include "trace-view-store.h"
#include "trace-graph.h"

struct xenomai_context {
	struct shark_info *info;
	struct trace_view_store *store;
	struct pevent *pevent;

	struct event_format		*cobalt_switch_event;
	struct format_field		*cobalt_switch_next_field;
	struct format_field     *cobalt_switch_prev_state;
	struct format_field     *cobalt_switch_next_pid_field;
	struct format_field     *cobalt_switch_next_name_field;

	struct event_format		*cobalt_wakeup_event;
	struct format_field		*cobalt_wakeup_pid_field;
};

typedef struct xenomai_context  xenomai_context;
 
struct xenomai_context *xenomai_context_handler = NULL;

int xenomai_update_context (struct pevent *pevent, int task_id)
{
	xenomai_context *ctx = xenomai_context_handler;
	if (task_id & 0x1) {
		struct event_format	*event;
		event = pevent_find_event_by_name(	xenomai_context_handler->pevent,
											"cobalt_core",
											"cobalt_switch_context");
		ctx->cobalt_switch_event = event;

		event = pevent_find_event_by_name(	xenomai_context_handler->pevent,
											"cobalt_core",
											"cobalt_thread_resume");
		ctx->cobalt_wakeup_event = event;
	}
	
	if (!ctx->cobalt_switch_event ||
		!ctx->cobalt_wakeup_event)
	{
		return 0;
	}
	
	
	if (task_id & (0x1 << 1)) {
		if (ctx->cobalt_switch_event) {
			ctx->cobalt_switch_next_field = pevent_find_any_field(ctx->cobalt_switch_event, "next_pid");
		}

		if (ctx->cobalt_wakeup_event) {
			ctx->cobalt_wakeup_pid_field = pevent_find_any_field(ctx->cobalt_wakeup_event, "pid");
		}
	}

	if (task_id & (0x1 << 2)) {
		ctx->cobalt_switch_prev_state = pevent_find_field(ctx->cobalt_switch_event, "prev_state");
		ctx->cobalt_switch_next_pid_field = pevent_find_field(ctx->cobalt_switch_event, "next_pid");
		ctx->cobalt_switch_next_name_field = pevent_find_field(ctx->cobalt_switch_event, "next_name");
	}

	return 1;
}


static void xenomai_context_new(struct shark_info *info, struct trace_view_store *store)
{
	if (!xenomai_context_handler) {
		xenomai_context_handler = (xenomai_context*) calloc(1, sizeof(xenomai_context));
	}
		
	xenomai_context_handler->info = info;
	xenomai_context_handler->store = store;
	xenomai_context_handler->pevent = tracecmd_get_pevent(info->handle);

	

	xenomai_update_context(xenomai_context_handler->pevent, 3);
}


static int cobalt_get_next_pid(	xenomai_context *ctx,
								struct pevent_record *record,
								int *pid)
{
	long long unsigned int val;
	int status = pevent_read_number_field(	ctx->cobalt_switch_next_field,
											record->data, &val);
	if (pid)
		*pid = val;

	return status;
}


static int cobalt_get_prev_state(	xenomai_context *ctx,
								struct pevent_record *record,
								int *state)
{
	long long unsigned int val;
	pevent_read_number_field(	ctx->cobalt_switch_prev_state,
								record->data, &val);

	if (state)
		*state = val;

	return val & 0x00000008 ? 1 : 0;
}

static void cobalt_get_command(	xenomai_context *ctx,
								struct pevent_record *record,
								const char **comm)
{
	int offset = data2host4(ctx->pevent, record->data +
							ctx->cobalt_switch_next_name_field->offset);
	offset &= 0xffff;
	*comm = record->data + offset;
}

static int xenomai_switch_handler(	struct pevent_record *record,
									int event_id, int task_id,
									void *output)
{
	xenomai_context *ctx = xenomai_context_handler;
	switch (task_id) {
		case 0:
		{
			if (ctx->cobalt_switch_next_field &&
				event_id == ctx->cobalt_switch_event->id)
			{
				cobalt_get_next_pid(ctx, record, (int*) output);
				return 1;
			}

			return 0;
		}

		case 1:
		{
			if (ctx->cobalt_switch_prev_state
				&& event_id == ctx->cobalt_switch_event->id)
			{
				return cobalt_get_prev_state(ctx, record, (int*) output);
			}

			return 0;
		}

		case 2:
		{
			if (ctx->cobalt_switch_next_name_field &&
				event_id == ctx->cobalt_switch_event->id)
			{
				cobalt_get_command(ctx, record, (const char**) output);
				return 1;
			}

			return 0;
		}

		default:
			return 0;
	}
}


static int cobalt_get_wakeup_pid(	xenomai_context *ctx,
									struct pevent_record *record,
									int *pid)
{
	long long unsigned int val;
	int status = pevent_read_number_field(	ctx->cobalt_wakeup_pid_field,
											record->data, &val);
	if (pid)
		*pid = val;

	return status;
}

static int xenomai_wakeup_handler(	struct pevent_record *record,
									int event_id, int task_id,
									void *pid)
{
	xenomai_context *ctx = xenomai_context_handler;
	if (ctx->cobalt_wakeup_pid_field
		&& event_id == ctx->cobalt_wakeup_event->id)
	{
		cobalt_get_wakeup_pid(ctx, record, pid);
		return 1;
	}

	return 0;
}

void KSHARK_PLUGIN_LOADER(void *info, void *store)
{
	struct shark_info *ks_info = info;
	struct trace_view_store *ks_store = store;
	
	xenomai_context_new(ks_info, ks_store);

	trace_view_store_register_plugin_handlers(	ks_store,
												xenomai_switch_handler,
												xenomai_wakeup_handler,
												xenomai_update_context);

	trace_graph_register_plugin_handlers(	ks_info->ginfo,
											xenomai_switch_handler,
											xenomai_wakeup_handler,
											xenomai_update_context);
}

void KSHARK_PLUGIN_UNLOADER(void *info, void *store)
{
	free(xenomai_context_handler);
}

