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
#include "plugins/sched_events.h"

struct plugin_sched_context	*plugin_sched_context_handler = NULL;

static bool plugin_sched_update_context(struct kshark_context *kshark_ctx)
{
	struct event_format 		*event;
	struct plugin_sched_context 	*plugin_ctx;

	if (!plugin_sched_context_handler) {
		plugin_sched_context_handler =
		(struct plugin_sched_context*) malloc(sizeof(struct plugin_sched_context));
	}

	plugin_ctx = plugin_sched_context_handler;
	plugin_ctx->handle = kshark_ctx->handle;
	plugin_ctx->pevt = kshark_ctx->pevt;

	event = pevent_find_event_by_name(plugin_ctx->pevt, "sched", "sched_switch");
	if (!event)
		return false;
	
	plugin_ctx->sched_switch_event = event;
	plugin_ctx->sched_switch_next_field = pevent_find_any_field(event, "next_pid");

	event = pevent_find_event_by_name(plugin_ctx->pevt, "sched", "sched_wakeup");
	if (!event)
		return false;

	plugin_ctx->sched_wakeup_event = event;
	plugin_ctx->sched_wakeup_pid_field = pevent_find_any_field(event, "pid");

// 	event = pevent_find_event_by_name(plugin_ctx->pevt, "sched", "sched_wakeup_new");
// 	if (!event)
// 		return false;
// 
// 	plugin_ctx->sched_wakeup_new_event = event;
// 	plugin_ctx->sched_wakeup_new_pid_field = pevent_find_any_field(event, "pid");

	return true;
}

int plugin_get_next_pid(struct pevent_record *record)
{
	unsigned long long val;
	struct plugin_sched_context *plugin_ctx = plugin_sched_context_handler;

	pevent_read_number_field(plugin_ctx->sched_switch_next_field, record->data, &val);
	return val;
}

int plugin_get_wakeup_pid(struct pevent_record *record)
{
	unsigned long long val;
	struct plugin_sched_context *plugin_ctx = plugin_sched_context_handler;

	pevent_read_number_field(plugin_ctx->sched_wakeup_pid_field, record->data, &val);
	return val;
}

int plugin_get_wakeup_pid_lazy(struct plugin_sched_context *plugin_ctx,
			       struct kshark_entry *e)
{
	unsigned long long val;
	struct pevent_record *record = tracecmd_read_at(plugin_ctx->handle,
							e->offset,
							NULL);

	pevent_read_number_field(plugin_ctx->sched_wakeup_pid_field, record->data, &val);
	return val;
}

// static int plugin_get_wakeup_new_pid(struct kshark_context *ctx, struct pevent_record *record)
// {
// 	unsigned long long val;
// 
// 	pevent_read_number_field(ctx->sched_wakeup_new_pid_field, record->data, &val);
// 
// 	return val;
// }

void plugin_sched_edit_entry(struct kshark_context *ctx,
			   struct pevent_record *rec,
			   struct kshark_entry *entry)
{
	entry->pid = plugin_get_next_pid(rec);
}

bool plugin_wakeup_check_pid(struct kshark_context *ctx, struct kshark_entry *e, int pid)
{
	if (e->pid == pid)
		return true;

	struct plugin_sched_context *plugin_ctx = plugin_sched_context_handler;
	
	if (plugin_ctx &&
	    plugin_ctx->sched_wakeup_event &&
	    e->event_id == plugin_ctx->sched_wakeup_event->id) {
		struct pevent_record *record = tracecmd_read_at(plugin_ctx->handle,
								e->offset,
								NULL);

		if (plugin_get_wakeup_pid(record) == pid)
			return true;
	}

	return false;
}

bool plugin_switch_check_pid(struct kshark_context *ctx, struct kshark_entry *e, int pid)
{
	if (e->pid == pid)
		return true;

	struct plugin_sched_context *plugin_ctx = plugin_sched_context_handler;

	if (plugin_ctx->sched_switch_event &&
	    e->event_id == plugin_ctx->sched_switch_event->id) {
		struct pevent_record *record = tracecmd_read_at(plugin_ctx->handle,
								e->offset,
								NULL);

		if (pevent_data_pid(plugin_ctx->pevt, record) == pid)
			return true;
	}

	return false;
}

static void plugin_sched_load()
{
	printf("## plugin_sched_load @@\n");

	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	if (!plugin_sched_update_context(kshark_ctx)) {
		free(plugin_sched_context_handler);
		plugin_sched_context_handler = NULL;
		return;
	}

	struct plugin_sched_context *plugin_ctx =
		plugin_sched_context_handler;

	kshark_register_event_handler(&kshark_ctx->event_handlers,
				      plugin_ctx->sched_switch_event->id,
				      plugin_sched_edit_entry,
				      plugin_draw,
				      plugin_sched_update_context);
}

static void plugin_sched_unload()
{
	printf("@@ plugin_sched_unload ##\n");

	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	if (!plugin_sched_context_handler)
		return;

	struct plugin_sched_context *plugin_ctx = plugin_sched_context_handler;

	if (kshark_ctx) {
		kshark_unregister_event_handler(&kshark_ctx->event_handlers,
						plugin_ctx->sched_switch_event->id,
						plugin_sched_edit_entry,
						plugin_draw,
						plugin_sched_update_context);
	}

	free(plugin_ctx);
	plugin_sched_context_handler = NULL;
}

void KSHARK_PLUGIN_LOADER()
{
	plugin_sched_load();
}

void KSHARK_PLUGIN_RELOADER()
{
	plugin_sched_unload();
	plugin_sched_load();
}

void KSHARK_PLUGIN_UNLOADER()
{
	plugin_sched_unload();
}
