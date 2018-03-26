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

#ifndef _KSHARK_JSON_H
#define _KSHARK_JSIN_H

// C
#include <json.h>

// Kernel Shark
#include "libkshark.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
	
struct json_object *kshark_filter_config_alloc();

void kshark_filter_to_json(struct pevent *pevt,
			   struct filter_id *filter,
			   const char *filter_name,
			   struct json_object *jobj);

void kshark_filter_from_json(struct pevent *pevt,
			     struct filter_id *filter,
			     const char *filter_name,
			     struct json_object *jobj);

struct json_object *kshark_all_filters_to_json(struct kshark_context *kshark_ctx);

void kshark_all_filters_from_json(struct kshark_context *kshark_ctx,
				  struct json_object *jobj);

void kshark_save_json_file(const char *filter_name,
			   struct json_object *jobj);

struct json_object *kshark_open_json_file(const char *filter_name,
					  const char *type);

#ifdef __cplusplus
}
#endif // __cplusplus
	
#endif
