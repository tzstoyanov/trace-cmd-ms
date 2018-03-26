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

// Kernel Shark
#include "libkshark-json.h"

struct json_object *kshark_filter_config_alloc()
{
	struct json_object *jobj = json_object_new_object();

	/* Set the type of this Json document. */
	json_object_object_add(jobj,
			       "type",
			       json_object_new_string("kshark.filter.config"));

	return jobj;
}

void kshark_filter_to_json(struct pevent *pevt,
			   struct filter_id *filter,
			   const char *filter_name,
			   struct json_object *jobj)
{
	int i, evt;
	/* Get the array of Ids to be fitered. */
	int *ids = filter_ids(filter);
	/* Create a Json array and fill the Id values into it. */
	json_object *filter_data = json_object_new_array();
	for (i = 0; i < filter->count; ++i) {
		for (evt = 0; evt < pevt->nr_events; ++evt) {
			if (pevt->events[evt]->id == ids[i]) {
				json_object *jevent = json_object_new_object();

				json_object *system =
					json_object_new_string(pevt->events[evt]->system);

				json_object *name =
					json_object_new_string(pevt->events[evt]->name);

				json_object_object_add(jevent, "system", system);
				json_object_object_add(jevent, "name", name);
				json_object_array_add(filter_data, jevent);

				break;
			}
		}
	}

	/* Add the array of Ids to the filter config document. */
	json_object_object_add(jobj, filter_name, filter_data);
}

void kshark_filter_from_json(struct pevent *pevt,
			     struct filter_id *filter,
			     const char *filter_name,
			     struct json_object *jobj)
{
	struct json_object *json_filter, *json_event, *json_system, *json_event_name;
	int i, length;
	struct event_format *event;

	/* Use the name of the filter to find the array of events associated
	 * with this filter. Notice that the filter config document may contain
	 * no data for this particular filter. */
	json_object_object_get_ex(jobj, filter_name, &json_filter);
	if (!json_filter || json_object_get_type(json_filter) != json_type_array)
		return;

	/* Set the filter. */
	length = json_object_array_length(json_filter);
	for (i = 0; i < length; ++i) {
		json_event = json_object_array_get_idx(json_filter, i);

		json_object_object_get_ex(json_event, "system", &json_system);
		json_object_object_get_ex(json_event, "name", &json_event_name);
		event = pevent_find_event_by_name(pevt,
						  json_object_get_string(json_system),
						  json_object_get_string(json_event_name));
		
		if (event)
			filter_id_add(filter, event->id);
	}
}

struct json_object *kshark_all_filters_to_json(struct kshark_context *kshark_ctx)
{
	/*  Create a new Json document. */
	struct json_object *jobj = kshark_filter_config_alloc();

	/* Save the filter only if it contains Id values. */
	if (kshark_ctx->show_event_filter &&
	    kshark_ctx->show_event_filter->count)
		kshark_filter_to_json(kshark_ctx->pevt,
				      kshark_ctx->show_event_filter,
				      "show event filter",
				      jobj);

	if (kshark_ctx->hide_task_filter &&
	    kshark_ctx->hide_event_filter->count)
		kshark_filter_to_json(kshark_ctx->pevt,
				      kshark_ctx->hide_event_filter,
				      "hide event filter",
				      jobj);

// 	if (kshark_ctx->show_task_filter &&
// 	    kshark_ctx->show_task_filter->count)
// 		kshark_filter_to_json(kshark_ctx->pevt,
// 				      kshark_ctx->show_task_filter,
// 				      "show task filter",
// 				      jobj);
// 
// 	if (kshark_ctx->hide_task_filter &&
// 	    kshark_ctx->hide_task_filter->count)
// 		kshark_filter_to_json(kshark_ctx->pevt,
// 				      kshark_ctx->hide_task_filter,
// 				      "hide task filter",
// 				      jobj);

	return jobj;
}

void kshark_all_filters_from_json(struct kshark_context *kshark_ctx,
				  struct json_object *jobj)
{
	kshark_filter_from_json(kshark_ctx->pevt,
				kshark_ctx->hide_event_filter,
				"hide event filter",
				jobj);

	kshark_filter_from_json(kshark_ctx->pevt,
				kshark_ctx->show_event_filter,
				"show event filter",
				jobj);

// 	kshark_filter_from_json(kshark_ctx->pevt,
// 				kshark_ctx->hide_task_filter,
// 				"hide task filter",
// 				jobj);
// 
// 	kshark_filter_from_json(kshark_ctx->pevt,
// 				kshark_ctx->show_task_filter,
// 				"show task filter",
// 				jobj);
}

void kshark_save_json_file(const char *filter_name,
			   struct json_object *jobj)
{
	/* Save the file in a human-readable form. */
	json_object_to_file_ext(filter_name,
				jobj,
			        JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY);
}

struct json_object *kshark_open_json_file(const char *filter_name,
					  const char *type)
{
	struct json_object *jobj = json_object_from_file(filter_name);
	if (!jobj)
		return NULL;

	/* Get the type of the document. */
	struct json_object *var;
	json_object_object_get_ex(jobj, "type", &var);
	const char *type_var = json_object_get_string(var);

	if (strcmp(type, type_var) != 0) {
		/* The document has a wrong type. */
		fprintf(stderr,
			"Failed to open Json file %s. The document has a wrong type.\n",
			filter_name);

		json_object_put(jobj);
		return NULL;
	}

	return jobj;
}
