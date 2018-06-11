/*
 * Copyright (C) 2018 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
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

#ifndef _LIB_KSHARK_COLLECTION_H
#define _LIB_KSHARK_COLLECTION_H

// C
#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct kshark_context;

struct kshark_entry;

#define COLLECTION_RESUME	0x1
#define COLLECTION_BREAK	0x2

typedef bool (matching_condition_func)(struct kshark_context*,
				       struct kshark_entry*,
				       int);

struct kshark_entry_collection {
	struct kshark_entry_collection *next;

	int val;
	matching_condition_func *cond;

	size_t *break_points;
	size_t *resume_points;
	uint8_t type;

	size_t size;
};

struct kshark_entry_request {
	struct kshark_entry_request *next;

	size_t first;
	size_t n;

	matching_condition_func *cond;
	int val;

	bool vis_only;
	int vis_mask;
};

struct kshark_entry_request *kshark_entry_request_alloc(size_t first,
							size_t n,
							matching_condition_func cond,
							int val,
							bool vis_only,
							int vis_mask);

void kshark_free_entry_request(struct kshark_entry_request *req);

struct kshark_entry_collection *
kshark_register_data_collection(struct kshark_context *kshark_ctx,
				struct kshark_entry **data, size_t n_rows,
				matching_condition_func cond, int val,
				size_t margin);

void kshark_unregister_data_collection(struct kshark_entry_collection **col,
				       matching_condition_func cond,
				       int val);

struct kshark_entry_collection *
kshark_find_data_collection(struct kshark_entry_collection *col,
			    matching_condition_func cond,
			    int val);

void kshark_reset_data_collection(struct kshark_entry_collection *col);

void kshark_free_collection_list(struct kshark_entry_collection *col);

struct kshark_entry *
kshark_get_collection_entry_front(struct kshark_entry_request **req,
				  struct kshark_entry **data,
				  const struct kshark_entry_collection *col);

struct kshark_entry *
kshark_get_collection_entry_back(struct kshark_entry_request **req,
				 struct kshark_entry **data,
				 const struct kshark_entry_collection *col);

#ifdef __cplusplus
}
#endif

#endif // _LIB_KSHARK_COLLECTION_H
