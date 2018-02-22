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

// C
#include <stdlib.h>
#include <stdio.h>

// Kernel Shark 2
#include "plugins/xenomai_sched_events.h"

struct xenomai_context	*xenomai_context_handler = NULL;

#define COBALT_PREV_STATE_BIT		(1 << 3)
#define COBALT_COMM_OFFSET_MASK		0xFFFF

static bool xenomai_update_context(struct kshark_context *kshark_ctx)
{
	struct event_format *event;
	struct xenomai_context *plugin_ctx = xenomai_context_handler;

	if (!plugin_ctx)
		return false;

	event = pevent_find_event_by_name(plugin_ctx->pevt,
					  "cobalt_core",
					  "cobalt_switch_context");
	if (!event)
		return false;

	plugin_ctx->cobalt_switch_event = event;

	event = pevent_find_event_by_name(plugin_ctx->pevt,
					  "cobalt_core",
					  "cobalt_thread_resume");
	if (!event)
		return false;

	plugin_ctx->cobalt_wakeup_event = event;

	plugin_ctx->cobalt_switch_next_pid_field =
		pevent_find_field(plugin_ctx->cobalt_switch_event, "next_pid");

	plugin_ctx->cobalt_switch_prev_state_field =
		pevent_find_field(plugin_ctx->cobalt_switch_event, "prev_state");

	plugin_ctx->cobalt_switch_next_name_field =
		pevent_find_field(plugin_ctx->cobalt_switch_event, "next_name");

	plugin_ctx->cobalt_wakeup_pid_field =
		pevent_find_field(plugin_ctx->cobalt_wakeup_event, "pid");

	return true;
}

static void xenomai_context_new(struct kshark_context *kshark_ctx)
{
	if (!xenomai_context_handler) {
		xenomai_context_handler =
		(struct xenomai_context*) malloc(sizeof(struct xenomai_context));
	}

	xenomai_context_handler->handle = kshark_ctx->handle;
	xenomai_context_handler->pevt = kshark_ctx->pevt;

	int status = xenomai_update_context(kshark_ctx);
	if (status == false) {
		free(xenomai_context_handler);
		xenomai_context_handler = NULL;
	}
}

int cobalt_get_next_pid(struct pevent_record *record)
{
	long long unsigned int val;
	struct xenomai_context *plugin_ctx = xenomai_context_handler;

	pevent_read_number_field(plugin_ctx->cobalt_switch_next_pid_field,
					      record->data, &val);

	return val;
}

bool cobalt_switch_check_pid(struct kshark_context *kshark_ctx, struct kshark_entry *e, int pid)
{
	if (e->pid == pid)
		return true;

	struct xenomai_context *plugin_ctx = xenomai_context_handler;

	if (plugin_ctx->cobalt_switch_event &&
	    e->event_id == plugin_ctx->cobalt_switch_event->id) {
		struct pevent_record *record = tracecmd_read_at(plugin_ctx->handle,
								e->offset,
								NULL);

		if (pevent_data_pid(plugin_ctx->pevt, record) == pid)
			return true;
	}

	return false;
}

// static int cobalt_get_prev_state(struct xenomai_context *ctx,
// 				 struct pevent_record *record,
// 				 int *state)
// {
// 	long long unsigned int val;
// 	pevent_read_number_field(ctx->cobalt_switch_prev_state_field,
// 				 record->data, &val);
// 
// 	if (state)
// 		*state = val;
// 
// 	return (val & COBALT_PREV_STATE_BIT) ? 1 : 0;
// }
// 
// static void cobalt_get_command(struct xenomai_context *ctx,
// 			       struct pevent_record *record,
// 			       const char **comm)
// {
// 	int offset =
// 	data2host4(ctx->pevt, record->data + ctx->cobalt_switch_next_name_field->offset);
// 
// 	offset &= COBALT_COMM_OFFSET_MASK;
// 	*comm = record->data + offset;
// }

int cobalt_get_wakeup_pid(struct pevent_record *record)
{
	long long unsigned int val;
	struct xenomai_context *plugin_ctx = xenomai_context_handler;

	pevent_read_number_field(plugin_ctx->cobalt_wakeup_pid_field,
				 record->data, &val);
	return val;
}

int cobalt_get_wakeup_pid_lazy(struct xenomai_context *plugin_ctx, struct kshark_entry *e)
{
	long long unsigned int val;
	struct pevent_record *record = tracecmd_read_at(plugin_ctx->handle,
							e->offset,
							NULL);

	pevent_read_number_field(plugin_ctx->cobalt_wakeup_pid_field,
				 record->data, &val);
	return val;
}

bool cobalt_wakeup_check_pid(struct kshark_context *kshark_ctx,
			     struct kshark_entry *e, int pid)
{
	if (e->pid == pid)
		return true;

	struct xenomai_context *plugin_ctx = xenomai_context_handler;
	
	if (plugin_ctx &&
	    plugin_ctx->cobalt_wakeup_event &&
	    e->event_id == plugin_ctx->cobalt_wakeup_event->id) {
		struct pevent_record *record = tracecmd_read_at(plugin_ctx->handle,
								e->offset,
								NULL);

		if (cobalt_get_wakeup_pid(record) == pid)
			return true;
	}

	return false;
}


void cobalt_edit_entry(struct kshark_context *kshark_ctx,
		       struct pevent_record *rec,
		       struct kshark_entry *entry)
{
	entry->pid = cobalt_get_next_pid(rec);
}

static void plugin_xenomai_load()
{
	printf("$$ plugin_xenomai_load **\n");

	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	xenomai_context_new(kshark_ctx);
	if (!xenomai_context_handler)
		return;

	struct xenomai_context *plugin_ctx = xenomai_context_handler;

	kshark_register_event_handler(&kshark_ctx->event_handlers,
				      plugin_ctx->cobalt_switch_event->id,
				      cobalt_edit_entry,
				      xenomai_draw,
				      xenomai_update_context);
}

static void plugin_xenomai_unload()
{
	printf("** plugin_xenomai_unload $$ \n");

	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	if (!xenomai_context_handler)
		return;

	struct xenomai_context *plugin_ctx = xenomai_context_handler;

	if (kshark_ctx) {
		kshark_unregister_event_handler(&kshark_ctx->event_handlers,
						plugin_ctx->cobalt_switch_event->id,
						cobalt_edit_entry,
						xenomai_draw,
						xenomai_update_context);
	}

	free(plugin_ctx);
	xenomai_context_handler = NULL;
}

void KSHARK_PLUGIN_LOADER()
{
	plugin_xenomai_load();
}

void KSHARK_PLUGIN_RELOADER()
{
	plugin_xenomai_unload();
	plugin_xenomai_load();
}

void KSHARK_PLUGIN_UNLOADER()
{
	plugin_xenomai_unload();
}
