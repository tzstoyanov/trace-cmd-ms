/* SPDX-License-Identifier: LGPL-2.1 */

/*
 * Copyright (C) 2017 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
 */

/**
 *  @file    rename_sched_events.c
 *  @brief   A plugin to deal with renamed threads.
 */

#ifndef _KS_PLUGIN_SHED_RENAME_H
#define _KS_PLUGIN_SHED_RENAME_H

// trace-cmd
#include "event-parse.h"

// KernelShark
#include "libkshark.h"
#include "libkshark-model.h"

#ifdef __cplusplus
extern "C" {
#endif

/** True if the job is done. */
static bool done = false;

/** Structure representing a plugin-specific context. */
struct plugin_sched_context {
	/** Input handle for the trace data file. */
	struct tracecmd_input	*handle;

	/** Page event used to parse the page. */
	struct tep_handle	*pevent;

	/** Pointer to the sched_switch_event object. */
	struct event_format	*sched_switch_event;

	/** Pointer to the sched_switch_next_field format descriptor. */
	struct format_field	*sched_switch_next_field;

	/** Pointer to the sched_switch_comm_field format descriptor. */
	struct format_field	*sched_switch_comm_field;
};

/** Plugin context instance. */
static struct plugin_sched_context **plugin_sched_context_handler = NULL;

static bool plugin_sched_update_context(struct kshark_context *kshark_ctx)
{
	struct plugin_sched_context *plugin_ctx;
	struct kshark_data_stream *stream;
	struct event_format *event;
	int *stream_ids, sd;
	int i, count = 0;

	if (!plugin_sched_context_handler) {
		plugin_sched_context_handler =
			calloc(KS_MAX_NUM_STREAMS,
			       sizeof(*plugin_sched_context_handler));
// 		 malloc(sizeof(*plugin_sched_context_handler));
	}

	stream_ids = kshark_all_streams(kshark_ctx);

	for (i = 0; i < kshark_ctx->n_streams; ++i) {
		plugin_ctx = malloc(sizeof(*plugin_sched_context_handler));
		sd = stream_ids[i];
		stream = kshark_get_data_stream(kshark_ctx, sd);

		plugin_ctx->handle = stream->handle;
		plugin_ctx->pevent = stream->pevent;

		event = tep_find_event_by_name(plugin_ctx->pevent,
					       "sched", "sched_switch");
		if (!event)
			continue;

		plugin_ctx->sched_switch_event = event;
		plugin_ctx->sched_switch_next_field =
			tep_find_any_field(event, "next_pid");

		plugin_ctx->sched_switch_comm_field =
			tep_find_field(event, "next_comm");

		++count;

		plugin_sched_context_handler[sd] = plugin_ctx;
	}

	free(stream_ids);

	return count;
}

static void plugin_nop(struct kshark_context *kshark_ctx,
		       struct tep_record *rec,
		       struct kshark_entry *entry)
{}

static int plugin_get_next_pid(struct tep_record *record, int sd)
{
	unsigned long long val;
	struct plugin_sched_context *plugin_ctx =
		plugin_sched_context_handler[sd];

	tep_read_number_field(plugin_ctx->sched_switch_next_field,
			      record->data, &val);
	return val;
}

static bool plugin_sched_switch_match_pid(struct kshark_context *kshark_ctx,
					  struct kshark_entry *e,
					  int sd, int pid)
{
	struct plugin_sched_context *plugin_ctx;
	struct tep_record *record = NULL;
	int switch_pid;

	plugin_ctx = plugin_sched_context_handler[e->stream_id];
	if (plugin_ctx &&
	    plugin_ctx->sched_switch_event &&
	    e->stream_id == sd &&
	    e->event_id == plugin_ctx->sched_switch_event->id) {
		record = kshark_read_at(kshark_ctx, e->stream_id, e->offset);
		switch_pid = plugin_get_next_pid(record, e->stream_id);
		free(record);

		if (switch_pid == pid)
			return true;
	}

	return false;
}

static void plugin_rename(struct kshark_cpp_argv *argv,
			  int pid, int draw_action)
{
	struct plugin_sched_context *plugin_ctx;
	struct kshark_context *kshark_ctx;
	const struct kshark_entry *entry;
	struct kshark_entry_request req;
	struct tep_record *record;
	int *stream_ids, sd;
	int *pids, n_tasks;
	const char *comm;
	ssize_t index;
	int r;

	if (done)
		return;

	req.first = argv->histo->data_size - 1;
	req.n = argv->histo->data_size;
	req.cond = plugin_sched_switch_match_pid;
	req.vis_only = false;

	kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	stream_ids = kshark_all_streams(kshark_ctx);

	for (int i = 0; i < kshark_ctx->n_streams; ++i) {
		sd = stream_ids[i];
		n_tasks = kshark_get_task_pids(kshark_ctx, sd, &pids);
		printf("stream %i  nt: %i\n", sd, n_tasks);
		for (r = 0; r < n_tasks; ++r) {
			req.val = pids[r];
			req.sd = sd;
			entry = kshark_get_entry_back(&req, argv->histo->data,
						      &index);
			if (!entry)
				continue;

			plugin_ctx =
				plugin_sched_context_handler[entry->stream_id];

			record = kshark_read_at(kshark_ctx, entry->stream_id,
							    entry->offset);

			comm = record->data +
			       plugin_ctx->sched_switch_comm_field->offset;

			printf("%li task: %s  pid: %i\n", index, comm, pids[r]);
		}
	}

	done = true;
	free(pids);
}


static void plugin_rename_sched_load()
{
	struct kshark_context *kshark_ctx = NULL;
	struct plugin_sched_context *plugin_ctx;
	int *stream_ids, sd;

	kshark_instance(&kshark_ctx);

	if (!plugin_sched_update_context(kshark_ctx)) {
		free(plugin_sched_context_handler);
		plugin_sched_context_handler = NULL;
		return;
	}

	stream_ids = kshark_all_streams(kshark_ctx);

	for (int i = 0; i < kshark_ctx->n_streams; ++i) {
		sd = stream_ids[i];
		plugin_ctx = plugin_sched_context_handler[sd];

		kshark_register_event_handler(&kshark_ctx->event_handlers,
					      plugin_ctx->sched_switch_event->id,
					      sd,
					      plugin_nop,
					      plugin_rename);
	}
}

static void plugin_rename_sched_unload()
{
	struct kshark_context *kshark_ctx = NULL;
	struct plugin_sched_context *plugin_ctx;
	int *stream_ids, sd;

	if (!plugin_sched_context_handler)
		return;

	if (!kshark_instance(&kshark_ctx))
		return;

	stream_ids = kshark_all_streams(kshark_ctx);

	for (int i = 0; i < kshark_ctx->n_streams; ++i) {
		sd = stream_ids[i];
		if (!(plugin_ctx = plugin_sched_context_handler[sd]))
			continue;

		kshark_unregister_event_handler(&kshark_ctx->event_handlers,
						plugin_ctx->sched_switch_event->id,
						sd,
						plugin_nop,
						plugin_rename);

		free(plugin_ctx);
	}

	free(plugin_sched_context_handler);
	plugin_sched_context_handler = NULL;
}

/** Load this plugin. */
void KSHARK_PLUGIN_LOADER()
{
	plugin_rename_sched_load();
}

/** Reload this plugin. */
void KSHARK_PLUGIN_RELOADER()
{
	plugin_rename_sched_unload();
	plugin_rename_sched_load();
}

/** Unload this plugin. */
void KSHARK_PLUGIN_UNLOADER()
{
	plugin_rename_sched_unload();
}

#ifdef __cplusplus
}
#endif

#endif
