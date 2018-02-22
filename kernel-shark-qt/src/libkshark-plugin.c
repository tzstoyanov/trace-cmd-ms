/*
 * Copyright (C) 2017 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
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

// C
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dlfcn.h>

// trace-cmd
// #include "event-utils.h"

// Kernel shark 2
#include "libkshark-plugin.h"
#include "libkshark.h"

static struct gui_event_handler *gui_event_handler_alloc(int event_id,
							 kshark_plugin_event_handler_func evt_func,
							 kshark_plugin_draw_handler_func dw_func,
							 kshark_plugin_context_update_func ctx_func)
{
	struct gui_event_handler *handler = malloc(sizeof(struct gui_event_handler));

	if (!handler) {
		fprintf(stderr, "failed to allocate memory for gui eventhandler");
		return NULL;
	}

	handler->next = NULL;
	handler->id = event_id;
	handler->event_func = evt_func;
	handler->draw_func = dw_func;
	handler->context_func = ctx_func;

	return handler;
}

struct gui_event_handler *find_gui_event_handler(struct gui_event_handler *handlers,
						 int event_id)
{
	while (handlers) {
		if (handlers->id == event_id)
			return handlers;

		handlers = handlers->next;
	}

	return NULL;
}

void kshark_register_event_handler(struct gui_event_handler **handlers,
				int event_id,
				kshark_plugin_event_handler_func evt_func,
				kshark_plugin_draw_handler_func dw_func,
				kshark_plugin_context_update_func ctx_func)
{
	struct gui_event_handler *handler =
		gui_event_handler_alloc(event_id, evt_func, dw_func, ctx_func);

	if(!handler)
		return;

	handler->next = *handlers;
	*handlers = handler;
}

void kshark_unregister_event_handler(struct gui_event_handler **handlers,
				  int event_id,
				  kshark_plugin_event_handler_func evt_func,
				  kshark_plugin_draw_handler_func dw_func,
				  kshark_plugin_context_update_func ctx_func)
{
	struct gui_event_handler **last = handlers;
	struct gui_event_handler *list;

	for (list = *handlers; list; list = list->next) {
		if (list->id == event_id &&
		    list->event_func == evt_func &&
		    list->draw_func == dw_func &&
		    list->context_func == ctx_func) {
			*last = list->next;
			free(list);
			return;
		}

		last = &list->next;
	}
}

void kshark_free_event_handler_list(struct gui_event_handler *handlers)
{
	struct gui_event_handler *last;

	while (handlers) {
		last = handlers;
		handlers = handlers->next;
		free(last);
	}
}

void kshark_register_plugin(struct kshark_context *kshark_ctx, const char *file)
{
	struct stat st;
	int ret;
	struct plugin_list *plugin = kshark_ctx->plugins;

	while (plugin) {
		if (strcmp(plugin->file, file) == 0)
			return;

		plugin = plugin->next;
	}

	ret = stat(file, &st);
	if (ret < 0) {
		fprintf(stderr, "plugin %s not found\n", file);
		return;
	}

	plugin = calloc(sizeof(struct plugin_list), 1);
	if (!plugin) {
		fprintf(stderr, "failed to allocate memory for plugin\n");
		return;
	}

	asprintf(&plugin->file, "%s", file);
	plugin->next = kshark_ctx->plugins;
	kshark_ctx->plugins = plugin;
}

void kshark_unregister_plugin(struct kshark_context *kshark_ctx, const char *file)
{
	struct plugin_list **last = &kshark_ctx->plugins;
	struct plugin_list *list;

	for (list = kshark_ctx->plugins; list; list = list->next) {
		if (strcmp(list->file, file) == 0) {
			*last = list->next;
			free(list->file);
			free(list);
			return;
		}

		last = &list->next;
	}
}

void kshark_free_plugin_list(struct plugin_list *plugins)
{
	struct plugin_list *last;

	while (plugins) {
		last = plugins;
		plugins = plugins->next;
		free(last->file);
		free(last);
	}
}

void kshark_handle_plugins(struct kshark_context *kshark_ctx, int task_id)
{
	kshark_plugin_load_func func;
	struct plugin_list *plugin;
	void *handle;
	char* func_name;
	int fn_size = 0;

	switch (task_id) {
		case KSHARK_PLUGIN_LOAD:
			fn_size = asprintf(&func_name, KSHARK_PLUGIN_LOADER_NAME);
			break;

		case KSHARK_PLUGIN_RELOAD:
			fn_size = asprintf(&func_name, KSHARK_PLUGIN_RELOADER_NAME);
			break;

		case KSHARK_PLUGIN_UNLOAD:
			fn_size = asprintf(&func_name, KSHARK_PLUGIN_UNLOADER_NAME);
			break;

		default:
			return;
	}

	if (fn_size <= 0) {
		fprintf(stderr, "failed to allocate memory for plugin function name");
		return;
	}

	for (plugin = kshark_ctx->plugins; plugin; plugin = plugin->next) {
		handle = dlopen(plugin->file, RTLD_NOW | RTLD_GLOBAL);

		if (!handle) {
			fprintf(stderr, "cannot load plugin '%s'\n%s\n",
				plugin->file, dlerror());
			continue;
		}

		func = dlsym(handle, func_name);
		if (!func) {
			fprintf(stderr, "cannot find func '%s' in plugin '%s'\n%s\n",
				func_name, plugin->file, dlerror());
			continue;
		}

		func();
	}

// 	if (task_id == KSHARK_PLUGIN_UNLOAD) {
// 		while ((plugin = kshark_ctx->plugins)) {
// 			kshark_ctx->plugins = plugin->next;
// 			free(plugin);
// 		}
// 
// 		plugin_next = &plugins;
// 	}

	free(func_name);
}
