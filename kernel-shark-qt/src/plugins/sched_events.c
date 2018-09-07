// SPDX-License-Identifier: LGPL-2.1

/*
 * Copyright (C) 2018 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
 */

/**
 *  @file    sched_events.c
 *  @brief   Defines a callback function for Sched events used to registers the
 *	     "next" task (if not registered already) and to changes the value
 *	     of the "pid" field of the "sched_switch" entries such that, it
 *	     will be ploted as part of the "next" task.
 */

// C
#include <stdlib.h>
#include <stdio.h>

// KernelShark
#include "plugins/sched_events.h"

/** Plugin context instance. */
struct plugin_sched_context *plugin_sched_context_handler = NULL;

static bool plugin_sched_update_context(struct kshark_context *kshark_ctx)
{
	struct plugin_sched_context *plugin_ctx;
	struct event_format *event;

	if (!plugin_sched_context_handler) {
		plugin_sched_context_handler =
		 malloc(sizeof(*plugin_sched_context_handler));
	}

	plugin_ctx = plugin_sched_context_handler;
	plugin_ctx->handle = kshark_ctx->handle;
	plugin_ctx->pevent = kshark_ctx->pevent;

	event = tep_find_event_by_name(plugin_ctx->pevent,
				       "sched", "sched_switch");
	if (!event)
		return false;

	plugin_ctx->sched_switch_event = event;
	plugin_ctx->sched_switch_next_field =
		tep_find_any_field(event, "next_pid");

	plugin_ctx->sched_switch_comm_field =
		tep_find_field(event, "next_comm");

	event = tep_find_event_by_name(plugin_ctx->pevent,
				      "sched", "sched_wakeup");
	if (!event)
		return false;

	plugin_ctx->sched_wakeup_event = event;
	plugin_ctx->sched_wakeup_pid_field =
		tep_find_any_field(event, "pid");

	plugin_ctx->sched_wakeup_success_field =
		tep_find_field(event, "success");

	event = tep_find_event_by_name(plugin_ctx->pevent,
				       "sched", "sched_wakeup_new");
	if (!event)
		return false;

	plugin_ctx->sched_wakeup_new_event = event;
	plugin_ctx->sched_wakeup_new_pid_field =
		tep_find_any_field(event, "pid");

	plugin_ctx->sched_wakeup_new_success_field =
		tep_find_field(event, "success");

	return true;
}

/**
 * @brief Get the Process Id of the next scheduled task.
 *
 * @param record: Input location for a sched_switch record.
 */
int plugin_get_next_pid(struct tep_record *record)
{
	struct plugin_sched_context *plugin_ctx =
		plugin_sched_context_handler;
	unsigned long long val;

	tep_read_number_field(plugin_ctx->sched_switch_next_field,
			      record->data, &val);
	return val;
}

/**
 * @brief Get the Process Id of the task being woke up.
 *
 * @param record: Input location for a sched_wakeup record.
 */
int plugin_get_wakeup_pid(struct tep_record *record)
{
	struct plugin_sched_context *plugin_ctx =
		plugin_sched_context_handler;
	unsigned long long val;

	tep_read_number_field(plugin_ctx->sched_wakeup_pid_field,
			      record->data, &val);
	return val;
}

static void plugin_register_command(struct kshark_context *kshark_ctx,
				    struct tep_record *record,
				    int pid)
{
	struct plugin_sched_context *plugin_ctx =
		plugin_sched_context_handler;
	const char *comm;

	if (!plugin_ctx->sched_switch_comm_field)
		return;

	comm = record->data + plugin_ctx->sched_switch_comm_field->offset;

	if (!tep_pid_is_registered(kshark_ctx->pevent, pid))
			tep_register_comm(kshark_ctx->pevent, comm, pid);
}

static int plugin_get_wakeup_new_pid(struct tep_record *record)
{
	struct plugin_sched_context *plugin_ctx =
		plugin_sched_context_handler;
	unsigned long long val;

	tep_read_number_field(plugin_ctx->sched_wakeup_new_pid_field,
				 record->data, &val);

	return val;
}

/**
 * @brief Process Id matching function adapted for sched_wakeup and
 *	  sched_wakeup_new events.
 *
 * @param kshark_ctx: Input location for the session context pointer.
 * @param e: kshark_entry to be checked.
 * @param pid: Matching condition value.
 *
 * @returns True if the Pid of the entry matches the value of "pid".
 *	    Otherwise false.
 */
bool plugin_wakeup_match_pid(struct kshark_context *kshark_ctx,
			     struct kshark_entry *e,
			     int pid)
{
	struct plugin_sched_context *plugin_ctx;
	struct tep_record *record = NULL;
	unsigned long long val;
	int wakeup_pid = -1;

	if (e->pid == pid)
		return true;

	plugin_ctx = plugin_sched_context_handler;
	if (!plugin_ctx)
		return false;

	if (plugin_ctx->sched_wakeup_event &&
	    e->event_id == plugin_ctx->sched_wakeup_event->id) {
		record = kshark_read_at(kshark_ctx, e->offset);

		/* We only want those that actually woke up the task. */
		tep_read_number_field(plugin_ctx->sched_wakeup_success_field,
				      record->data, &val);

		if (val)
			wakeup_pid = plugin_get_wakeup_pid(record);
	}

	if (plugin_ctx->sched_wakeup_new_event &&
	    e->event_id == plugin_ctx->sched_wakeup_new_event->id) {
		record = kshark_read_at(kshark_ctx, e->offset);

		/* We only want those that actually woke up the task. */
		tep_read_number_field(plugin_ctx->sched_wakeup_new_success_field,
				      record->data, &val);

		if (val)
			wakeup_pid = plugin_get_wakeup_new_pid(record);
	}

	free(record);

	if (wakeup_pid >= 0 && wakeup_pid == pid)
		return true;

	return false;
}

/**
 * @brief Process Id matching function adapted for sched_switch events.
 *
 * @param kshark_ctx: Input location for the session context pointer.
 * @param e: kshark_entry to be checked.
 * @param pid: Matching condition value.
 *
 * @returns True if the Pid of the entry matches the value of "pid".
 *	    Otherwise false.
 */
bool plugin_switch_match_pid(struct kshark_context *kshark_ctx,
			     struct kshark_entry *e,
			     int pid)
{
	struct plugin_sched_context *plugin_ctx;
	struct tep_record *record = NULL;
	int switch_pid = -1;

	if (e->pid == pid)
		return true;

	plugin_ctx = plugin_sched_context_handler;

	if (plugin_ctx->sched_switch_event &&
	    e->event_id == plugin_ctx->sched_switch_event->id) {
		record = kshark_read_at(kshark_ctx, e->offset);

		switch_pid = tep_data_pid(plugin_ctx->pevent, record);
	}

	free(record);

	if (switch_pid >= 0 && switch_pid == pid)
		return true;

	return false;
}

static void plugin_sched_action(struct kshark_context *kshark_ctx,
				struct tep_record *rec,
				struct kshark_entry *entry)
{
	entry->pid = plugin_get_next_pid(rec);
	plugin_register_command(kshark_ctx, rec, entry->pid);
}

static void plugin_sched_load()
{
	struct kshark_context *kshark_ctx = NULL;
	struct plugin_sched_context *plugin_ctx;

	kshark_instance(&kshark_ctx);

	if (!plugin_sched_update_context(kshark_ctx)) {
		free(plugin_sched_context_handler);
		plugin_sched_context_handler = NULL;
		return;
	}

	plugin_ctx = plugin_sched_context_handler;

	kshark_register_event_handler(&kshark_ctx->event_handlers,
				      plugin_ctx->sched_switch_event->id,
				      plugin_sched_action,
				      plugin_draw);
}

static void plugin_sched_unload()
{
	struct kshark_context *kshark_ctx = NULL;
	struct plugin_sched_context *plugin_ctx;

	if (!plugin_sched_context_handler)
		return;

	plugin_ctx = plugin_sched_context_handler;
	kshark_instance(&kshark_ctx);

	if (kshark_ctx) {
		kshark_unregister_event_handler(&kshark_ctx->event_handlers,
						plugin_ctx->sched_switch_event->id,
						plugin_sched_action,
						plugin_draw);
	}

	free(plugin_ctx);
	plugin_sched_context_handler = NULL;
}

/** Load this plugin. */
void KSHARK_PLUGIN_LOADER()
{
	plugin_sched_load();
}

/** Reload this plugin. */
void KSHARK_PLUGIN_RELOADER()
{
	plugin_sched_unload();
	plugin_sched_load();
}

/** Unload this plugin. */
void KSHARK_PLUGIN_UNLOADER()
{
	plugin_sched_unload();
}
