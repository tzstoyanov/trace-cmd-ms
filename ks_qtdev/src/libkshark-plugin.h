/*
 * Copyright (C) 2016 Red Hat Inc, Steven Rostedt <srostedt@redhat.com>
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

#ifndef _KSHARK_PLUGIN_H
#define _KSHARK_PLUGIN_H

// C
#include <limits.h>

// trace-cmd
#include "event-parse.h"


#ifdef __cplusplus
extern "C" {
#endif

#define KSHARK_PLUGIN_LOADER kshark_plugin_loader
#define KSHARK_PLUGIN_RELOADER kshark_plugin_reloader
#define KSHARK_PLUGIN_UNLOADER kshark_plugin_unloader

#define _MAKE_STR(x)	#x
#define MAKE_STR(x)	_MAKE_STR(x)
#define KSHARK_PLUGIN_LOADER_NAME MAKE_STR(KSHARK_PLUGIN_LOADER)
#define KSHARK_PLUGIN_RELOADER_NAME MAKE_STR(KSHARK_PLUGIN_RELOADER)
#define KSHARK_PLUGIN_UNLOADER_NAME MAKE_STR(KSHARK_PLUGIN_UNLOADER)

struct kshark_context;
struct kshark_entry;

typedef int (*kshark_plugin_load_func)();

typedef void (*kshark_plugin_draw_handler_func)(void *histo,
						void *graph,
						void *shapes,
						int   val,
					        int   draw_action);

typedef void (*kshark_plugin_event_handler_func)(struct kshark_context *kshark_ctx,
						 struct pevent_record *rec,
						 struct kshark_entry *e);

typedef bool (*kshark_plugin_context_update_func)(struct kshark_context *kshark_ctx);

enum gui_plugin_actions {
	KSHARK_PLUGIN_LOAD,
	KSHARK_PLUGIN_RELOAD,
	KSHARK_PLUGIN_UNLOAD,
	KSHARK_PLUGIN_TASK_DRAW,
	KSHARK_PLUGIN_CPU_DRAW,
};

struct gui_event_handler {
	struct gui_event_handler		*next;
	int					id;
	int					type;
	kshark_plugin_event_handler_func	event_func;
	kshark_plugin_draw_handler_func		draw_func;
	kshark_plugin_context_update_func	context_func;
};

struct gui_event_handler *find_gui_event_handler(struct gui_event_handler *handlers,
						 int event_id);

void kshark_register_event_handler(struct gui_event_handler **handlers,
				   int event_id,
				   kshark_plugin_event_handler_func	evt_func,
				   kshark_plugin_draw_handler_func	dw_func,
				   kshark_plugin_context_update_func	ctx_func);

void kshark_unregister_event_handler(struct gui_event_handler **handlers,
				  int event_id,
				  kshark_plugin_event_handler_func	evt_func,
				  kshark_plugin_draw_handler_func	dw_func,
				  kshark_plugin_context_update_func	ctx_func);

void kshark_free_event_handler_list(struct gui_event_handler *handlers);

struct plugin_list {
	struct plugin_list	*next;
	char			*file;
};

void kshark_register_plugin(struct kshark_context *kshark_ctx, char *file);

void kshark_unregister_plugin(struct kshark_context *kshark_ctx, char *file);

void kshark_free_plugin_list(struct plugin_list *plugins);

void kshark_handle_plugins(struct kshark_context *kshark_ctx, int task_id);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

class KsTimeMap;
typedef void (*kshark_plugin_draw_func)(KsTimeMap *histo, int pid);

#endif

#endif
