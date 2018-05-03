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

// C
#define _GNU_SOURCE
#include <stdio.h>

// Kernel Shark
#include "libkshark-json.h"

struct json_object *kshark_config_alloc(const char *type)
{
	struct json_object *jobj = json_object_new_object();

	/* Set the type of this Json document. */
	json_object_object_add(jobj,
			       "type",
			       json_object_new_string(type));

	return jobj;
}

struct json_object *kshark_record_config_alloc()
{
	return kshark_config_alloc("kshark.record.config");
}

struct json_object *kshark_filter_config_alloc()
{
	return kshark_config_alloc("kshark.filter.config");;
}

void kshark_event_filter_to_json(struct pevent *pevt,
				 struct filter_id *filter,
				 const char *filter_name,
				 struct json_object *jobj)
{
	int i, evt;
	/* Get the array of Ids to be fitered. */
	int *ids = filter_ids(filter);

	/* Create a Json array and fill the Id values into it. */
	json_object *jfilter_data = json_object_new_array();
	for (i = 0; i < filter->count; ++i) {
		for (evt = 0; evt < pevt->nr_events; ++evt) {
			if (pevt->events[evt]->id == ids[i]) {
				json_object *jevent = json_object_new_object();

				json_object *jsystem =
					json_object_new_string(pevt->events[evt]->system);

				json_object *jname =
					json_object_new_string(pevt->events[evt]->name);

				json_object_object_add(jevent, "system", jsystem);
				json_object_object_add(jevent, "name", jname);
				json_object_array_add(jfilter_data, jevent);

				break;
			}
		}
	}

	/* Add the array of Ids to the filter config document. */
	json_object_object_add(jobj, filter_name, jfilter_data);
}

void kshark_event_filter_from_json(struct pevent *pevt,
				   struct filter_id *filter,
				   const char *filter_name,
				   struct json_object *jobj)
{
	struct json_object *jfilter, *jevent, *jsystem, *jname;
	int i, length;
	struct event_format *event;

	/*
	 * Use the name of the filter to find the array of events associated
	 * with this filter. Notice that the filter config document may contain
	 * no data for this particular filter.
	 */
	json_object_object_get_ex(jobj, filter_name, &jfilter);
	if (!jfilter || json_object_get_type(jfilter) != json_type_array)
		return;

	/* Set the filter. */
	length = json_object_array_length(jfilter);
	for (i = 0; i < length; ++i) {
		jevent = json_object_array_get_idx(jfilter, i);

		json_object_object_get_ex(jevent, "system", &jsystem);
		json_object_object_get_ex(jevent, "name", &jname);
		event = pevent_find_event_by_name(pevt,
						  json_object_get_string(jsystem),
						  json_object_get_string(jname));
		
		if (event)
			filter_id_add(filter, event->id);
	}
}


void kshark_task_filter_to_json(struct pevent *pevt,
				struct filter_id *filter,
				const char *filter_name,
				struct json_object *jobj)
{
	int i;
	/* Get the array of Ids to be fitered. */
	int *ids = filter_ids(filter);

	/* Create a Json array and fill the Id values into it. */
	json_object *jfilter_data = json_object_new_array();
	for (i = 0; i < filter->count; ++i) {
		struct json_object *jpid = json_object_new_int(ids[i]);
		json_object_array_add(jfilter_data, jpid);
	}

	/* Add the array of Ids to the filter config document. */
	json_object_object_add(jobj, filter_name, jfilter_data);
}

void kshark_task_filter_from_json(struct pevent *pevt,
				  struct filter_id *filter,
				  const char *filter_name,
				  struct json_object *jobj)
{
	struct json_object *jfilter, *jpid;
	int i, length;

	/*
	 * Use the name of the filter to find the array of events associated
	 * with this filter. Notice that the filter config document may contain
	 * no data for this particular filter.
	 */
	json_object_object_get_ex(jobj, filter_name, &jfilter);
	if (!jfilter || json_object_get_type(jfilter) != json_type_array)
		return;

	/* Set the filter. */
	length = json_object_array_length(jfilter);
	for (i = 0; i < length; ++i) {
		jpid = json_object_array_get_idx(jfilter, i);

		filter_id_add(filter, json_object_get_int(jpid));
	}
}

void kshark_adv_filters_to_json(struct kshark_context *kshark_ctx,
				struct json_object *jobj)
{
	if (!kshark_ctx->adv_filter_is_set)
		return;

	/* Create a Json array and fill the Id values into it. */
	json_object *jfilter_data = json_object_new_array();

		struct event_format **events = pevent_list_events(kshark_ctx->pevt,
								  EVENT_SORT_SYSTEM);
	char *str;
	int i;
	for (i = 0; events[i]; i++) {
		str = pevent_filter_make_string(kshark_ctx->advanced_event_filter,
						events[i]->id);
		if (!str)
			continue;

		json_object *jevent = json_object_new_object();

		json_object *jsystem = json_object_new_string(events[i]->system);
		json_object *jname = json_object_new_string(events[i]->name);
		json_object *jfilter = json_object_new_string(str);

		json_object_object_add(jevent, "system", jsystem);
		json_object_object_add(jevent, "name", jname);
		json_object_object_add(jevent, "condition", jfilter);

		json_object_array_add(jfilter_data, jevent);
	}

	/* Add the array of advanced filters to the filter config document. */
	json_object_object_add(jobj, "adv event filter", jfilter_data);
}

void kshark_adv_filters_from_json(struct kshark_context *kshark_ctx,
				  struct json_object *jobj)
{
	struct json_object *jfilter, *jsystem, *jname, *jcond;
	int i, length, ret;

	/*
	 * Use the name of the filter to find the array of events associated
	 * with this filter. Notice that the filter config document may contain
	 * no data for this particular filter.
	 */
	json_object_object_get_ex(jobj, "adv event filter", &jfilter);
	if (!jfilter || json_object_get_type(jfilter) != json_type_array) {
		kshark_ctx->adv_filter_is_set = false;
		return;
	}

	/* Set the filter. */
	kshark_ctx->adv_filter_is_set = true;
	length = json_object_array_length(jfilter);
	for (i = 0; i < length; ++i) {
		jfilter = json_object_array_get_idx(jfilter, i);
		json_object_object_get_ex(jfilter, "system", &jsystem);
		json_object_object_get_ex(jfilter, "name", &jname);
		json_object_object_get_ex(jfilter, "condition", &jcond);

		char *filter;
		asprintf(&filter, "%s/%s:%s",  json_object_get_string(jsystem),
					       json_object_get_string(jname),
					       json_object_get_string(jcond));

		ret = pevent_filter_add_filter_str(kshark_ctx->advanced_event_filter, filter);

		if (ret < 0) {
			char error_str[200];
			pevent_strerror(kshark_ctx->pevt, ret, error_str, sizeof(error_str));
			fprintf(stderr, "filter failed due to: %s\n", error_str);
			free(filter);
			return;
		}

		free(filter);
	}
}

struct json_object *kshark_all_event_filters_to_json(struct kshark_context *kshark_ctx)
{
	/*  Create a new Json document. */
	struct json_object *jobj = kshark_filter_config_alloc();

	/* Save a filter only if it contains Id values. */
	if (kshark_ctx->show_event_filter &&
	    kshark_ctx->show_event_filter->count)
		kshark_event_filter_to_json(kshark_ctx->pevt,
					    kshark_ctx->show_event_filter,
					    "show event filter",
					    jobj);

	if (kshark_ctx->hide_task_filter &&
	    kshark_ctx->hide_event_filter->count)
		kshark_event_filter_to_json(kshark_ctx->pevt,
					    kshark_ctx->hide_event_filter,
					    "hide event filter",
					    jobj);

	return jobj;
}

struct json_object *kshark_all_task_filters_to_json(struct kshark_context *kshark_ctx)
{
	/*  Create a new Json document. */
	struct json_object *jobj = kshark_filter_config_alloc();

	/* Save a filter only if it contains Id values. */
	if (kshark_ctx->show_task_filter &&
	    kshark_ctx->show_task_filter->count)
		kshark_task_filter_to_json(kshark_ctx->pevt,
					   kshark_ctx->show_task_filter,
					   "show task filter",
					   jobj);

	if (kshark_ctx->hide_task_filter &&
	    kshark_ctx->hide_task_filter->count)
		kshark_task_filter_to_json(kshark_ctx->pevt,
					   kshark_ctx->hide_task_filter,
					   "hide task filter",
					   jobj);

	return jobj;
}

struct json_object *kshark_all_filters_to_json(struct kshark_context *kshark_ctx)
{
	/*  Create a new Json document. */
	struct json_object *jobj = kshark_filter_config_alloc();

	/* Save a filter only if it contains Id values. */
	if (kshark_ctx->show_event_filter &&
	    kshark_ctx->show_event_filter->count)
		kshark_event_filter_to_json(kshark_ctx->pevt,
					    kshark_ctx->show_event_filter,
					    "show event filter",
					    jobj);

	if (kshark_ctx->hide_task_filter &&
	    kshark_ctx->hide_event_filter->count)
		kshark_event_filter_to_json(kshark_ctx->pevt,
					    kshark_ctx->hide_event_filter,
					    "hide event filter",
					    jobj);

	if (kshark_ctx->show_task_filter &&
	    kshark_ctx->show_task_filter->count)
		kshark_task_filter_to_json(kshark_ctx->pevt,
					   kshark_ctx->show_task_filter,
					   "show task filter",
					   jobj);

	if (kshark_ctx->hide_task_filter &&
	    kshark_ctx->hide_task_filter->count)
		kshark_task_filter_to_json(kshark_ctx->pevt,
					   kshark_ctx->hide_task_filter,
					   "hide task filter",
					   jobj);

	kshark_adv_filters_to_json(kshark_ctx, jobj);

	return jobj;
}

void kshark_all_filters_from_json(struct kshark_context *kshark_ctx,
				  struct json_object *jobj)
{
	kshark_event_filter_from_json(kshark_ctx->pevt,
				      kshark_ctx->hide_event_filter,
				      "hide event filter",
				      jobj);

	kshark_event_filter_from_json(kshark_ctx->pevt,
				      kshark_ctx->show_event_filter,
				      "show event filter",
				      jobj);

	kshark_task_filter_from_json(kshark_ctx->pevt,
				     kshark_ctx->hide_task_filter,
				     "hide task filter",
				     jobj);

	kshark_task_filter_from_json(kshark_ctx->pevt,
				     kshark_ctx->show_task_filter,
				     "show task filter",
				     jobj);

	kshark_adv_filters_from_json(kshark_ctx, jobj);
}

void kshark_all_event_filters_from_json(struct kshark_context *kshark_ctx,
					struct json_object *jobj)
{
	kshark_event_filter_from_json(kshark_ctx->pevt,
				      kshark_ctx->hide_event_filter,
				      "hide event filter",
				      jobj);

	kshark_event_filter_from_json(kshark_ctx->pevt,
				      kshark_ctx->show_event_filter,
				      "show event filter",
				      jobj);
}

void kshark_all_task_filters_from_json(struct kshark_context *kshark_ctx,
				       struct json_object *jobj)
{
	kshark_task_filter_from_json(kshark_ctx->pevt,
				     kshark_ctx->hide_task_filter,
				     "hide task filter",
				     jobj);

	kshark_task_filter_from_json(kshark_ctx->pevt,
				     kshark_ctx->show_task_filter,
				     "show task filter",
				     jobj);
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
			"Failed to open Json file %s\n. The document has a wrong type.\n",
			filter_name);

		json_object_put(jobj);
		return NULL;
	}

	return jobj;
}
