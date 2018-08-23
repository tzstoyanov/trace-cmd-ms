// SPDX-License-Identifier: LGPL-2.1

/*
 * Copyright (C) 2017 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
 */

 /**
  *  @file    libkshark-plugin.c
  *  @brief   KernelShark plugins.
  */

// C
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <errno.h>

// KernelShark
#include "libkshark-plugin.h"
#include "libkshark.h"

static struct kshark_event_handler *
gui_event_handler_alloc(int event_id,
			kshark_plugin_event_handler_func evt_func,
			kshark_plugin_draw_handler_func dw_func)
{
	struct kshark_event_handler *handler = malloc(sizeof(*handler));

	if (!handler) {
		fprintf(stderr,
			"failed to allocate memory for gui eventhandler");
		return NULL;
	}

	handler->next = NULL;
	handler->id = event_id;
	handler->event_func = evt_func;
	handler->draw_func = dw_func;

	return handler;
}

/**
 * @brief Search the list of event handlers for a handle associated with a given event type.
 *
 * @param handlers: Input location for the Event handler list.
 * @param event_id: Event Id to search for.
 */
struct kshark_event_handler *
find_event_handler(struct kshark_event_handler *handlers, int event_id)
{
	for (; handlers; handlers = handlers->next)
		if (handlers->id == event_id)
			return handlers;

	return NULL;
}

/**
 * @brief Add new event handler to an existing list of handlers.
 *
 * @param handlers: Input location for the Event handler list.
 * @param event_id: Event Id.
 * @param evt_func: Input location for an Event action provided by the plugin.
 * @param dw_func: Input location for a Draw action provided by the plugin.
 *
 * @returns Zero on success, or a negative error code on failure.
 */
int kshark_register_event_handler(struct kshark_event_handler **handlers,
				  int event_id,
				  kshark_plugin_event_handler_func evt_func,
				  kshark_plugin_draw_handler_func dw_func)
{
	struct kshark_event_handler *handler =
		gui_event_handler_alloc(event_id, evt_func, dw_func);

	if(!handler)
		return -ENOMEM;

	handler->next = *handlers;
	*handlers = handler;
	return 0;
}

/**
 * @brief Search the list for a specific plugin handle. If such a plugin handle
 *	  exists, unregister (remove and free) this handle from the list.
 *
 * @param handlers: Input location for the Event handler list.
 * @param event_id: Event Id of the plugin handler to be unregistered.
 * @param evt_func: Event action function of the handler to be unregistered.
 * @param dw_func: Draw action function of the handler to be unregistered.
 */
void kshark_unregister_event_handler(struct kshark_event_handler **handlers,
				     int event_id,
				     kshark_plugin_event_handler_func evt_func,
				     kshark_plugin_draw_handler_func dw_func)
{
	struct kshark_event_handler **last = handlers;
	struct kshark_event_handler *list;

	for (list = *handlers; list; list = list->next) {
		if (list->id == event_id &&
		    list->event_func == evt_func &&
		    list->draw_func == dw_func) {
			*last = list->next;
			free(list);
			return;
		}

		last = &list->next;
	}

// 	struct kshark_event_handler **last;
// 
// 	for (*last = *handlers; *last; last = &(*last)->next) {
// 		printf("%i  %i\n", (*last)->id, event_id);
// 		if ((*last)->id == event_id &&
// 		    (*last)->event_func == evt_func &&
// 		    (*last)->draw_func == dw_func) {
// 			struct kshark_event_handler *list;
// 			list = *last;
// 			*last = list->next;
// 			free(list);
// 			return;
// 		}
// 	}
}

/**
 * @brief Free all Event handlers in a given list.
 *
 * @param handlers: Input location for the Event handler list.
 */
void kshark_free_event_handler_list(struct kshark_event_handler *handlers)
{
	struct kshark_event_handler *last;

	while (handlers) {
		last = handlers;
		handlers = handlers->next;
		free(last);
	}
}

/**
 * @brief Allocate memory for a new plugin. Add this plugin to the list of
 *	  plugins used by the session.
 *
 * @param kshark_ctx: Input location for the session context pointer.
 * @param file: The plugin object file to load.
 *
 * @returns Zero on success, or a negative error code on failure.
 */
int kshark_register_plugin(struct kshark_context *kshark_ctx,
			   const char *file)
{
	struct kshark_plugin_list *plugin = kshark_ctx->plugins;
	struct stat st;
	int ret;

	while (plugin) {
		if (strcmp(plugin->file, file) == 0)
			return -EEXIST;

		plugin = plugin->next;
	}

	ret = stat(file, &st);
	if (ret < 0) {
		fprintf(stderr, "plugin %s not found\n", file);
		return -ENODEV;
	}

	plugin = calloc(sizeof(struct kshark_plugin_list), 1);
	if (!plugin) {
		fprintf(stderr, "failed to allocate memory for plugin\n");
		return -ENOMEM;
	}

	if (asprintf(&plugin->file, "%s", file) <= 0) {
		fprintf(stderr,
			"failed to allocate memory for plugin file name");
		return -ENOMEM;
	}

	plugin->handle = NULL;

	plugin->next = kshark_ctx->plugins;
	kshark_ctx->plugins = plugin;

	return 0;
}

/**
 * @brief Register a new plugin..
 *
 * @param kshark_ctx: Input location for context pointer.
 * @param file: The plugin object file to load.
 */
void kshark_unregister_plugin(struct kshark_context *kshark_ctx,
			      const char *file)
{
	struct kshark_plugin_list **last = &kshark_ctx->plugins;
	struct kshark_plugin_list *list;

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

/**
 * @brief Free all plugins in a given list.
 *
 * @param plugins: Input location for the plugins list.
 */
void kshark_free_plugin_list(struct kshark_plugin_list *plugins)
{
	struct kshark_plugin_list *last;

	while (plugins) {
		last = plugins;
		plugins = plugins->next;
		free(last->file);
		free(last);
	}
}

/**
 * @brief Use this function to load/reload/unload all registered plugins.
 *
 * @param kshark_ctx: Input location for context pointer.
 * @param task_id: Action identifier specifying the action to be executed.
 *
 * @returns The number of successful operations on success, or a negative error
 *	    code on failure.
 */
int kshark_handle_plugins(struct kshark_context *kshark_ctx,
			  int task_id)
{
	kshark_plugin_load_func func;
	struct kshark_plugin_list *plugin;
	int plugin_count = 0;
	char* func_name;

	switch (task_id) {
	case KSHARK_PLUGIN_LOAD:
		func_name = KSHARK_PLUGIN_LOADER_NAME;
		break;

	case KSHARK_PLUGIN_RELOAD:
		func_name =KSHARK_PLUGIN_RELOADER_NAME;
		break;

	case KSHARK_PLUGIN_UNLOAD:
		func_name = KSHARK_PLUGIN_UNLOADER_NAME;
		break;

	default:
		return -EINVAL;
	}

	for (plugin = kshark_ctx->plugins; plugin; plugin = plugin->next) {
		if (task_id == KSHARK_PLUGIN_LOAD) {
			plugin->handle =
				dlopen(plugin->file, RTLD_NOW | RTLD_GLOBAL);

			if (!plugin->handle) {
				fprintf(stderr,
					"cannot load plugin '%s'\n%s\n",
					plugin->file,
					dlerror());

				continue;
			}
		}

		if (plugin->handle) {
			func = dlsym(plugin->handle, func_name);
			if (!func) {
				fprintf(stderr,
					"cannot find func '%s' in plugin '%s'\n%s\n",
					func_name,
					plugin->file,
					dlerror());

				dlclose(plugin->handle);
				plugin->handle = NULL;
				continue;
			}

			func();
			++plugin_count;

			if (task_id == KSHARK_PLUGIN_UNLOAD) {
				dlclose(plugin->handle);
				plugin->handle = NULL;
			}
		}
	}

	return plugin_count;
}
