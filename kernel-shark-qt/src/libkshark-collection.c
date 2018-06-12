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

//C
#include <assert.h>
#include <stdbool.h>

// KernelShark
#include "libkshark-collection.h"
#include "libkshark.h"

extern struct kshark_entry dummy_entry;

struct kshark_entry_request *kshark_entry_request_alloc(size_t first,
							size_t n,
							matching_condition_func cond,
							int val,
							bool vis_only,
							int vis_mask)
{
	struct kshark_entry_request *req = malloc(sizeof(struct kshark_entry_request));
	req->next = NULL;
	req->first = first;
	req->n = n;
	req->cond = cond;
	req->val = val;
	req->vis_only = vis_only;
	req->vis_mask = vis_mask;

	return req;
}

void kshark_free_entry_request(struct kshark_entry_request *req)
{
	struct kshark_entry_request *last;

	while (req) {
		last = req;
		req = req->next;
		free(last);
	}
}

struct entry_list {
	size_t			index;
	struct entry_list	*next;
	uint8_t			type;
};

static void kshark_add_entry(struct entry_list **list,
			     size_t i, uint8_t type)
{
	if ((*list)->type != 0) {
		(*list)->next = malloc(sizeof(**list));
		assert((*list)->next != NULL);
		*list = (*list)->next;
	}

	(*list)->index = i;
	(*list)->type = type;
}

static struct kshark_entry_collection *
kshark_data_collection_alloc(struct kshark_context *kshark_ctx,
			     struct kshark_entry **data,
			     size_t first,
			     size_t n_rows,
			     matching_condition_func cond,
			     int val,
			     size_t margin)
{
	size_t i, j;
	size_t resume_count = 0, break_count = 0;
	size_t last_added = 0;
	bool good = false;

	struct kshark_entry *last_vis_entry = NULL;
	struct entry_list *col_list = malloc(sizeof(*col_list));
	struct entry_list *temp = col_list;

	if (margin != 0) {
		temp->index = first;
		temp->type = COLLECTION_RESUME;
		++resume_count;

		kshark_add_entry(&temp, first + margin - 1, COLLECTION_BREAK);
		++break_count;
	} else {
		temp->index = first;
		temp->type = 0;
	}

	size_t end = first + n_rows - margin;
	for (i = first + margin; i < end; ++i) {
		if (!good && !cond(kshark_ctx, data[i], val)) {
			/*
			 * The entry is irrelevant for this collection.
			 * Do nothing.
			 */
			continue;
		}

		if (!good && cond(kshark_ctx, data[i], val)) {
			/*
			 * Resume the collection here. Add some margin data
			 * in front of the data of interest.
			 */
			good = true;
			if (last_added == 0 || last_added < i - margin) {
				kshark_add_entry(&temp, i - margin, COLLECTION_RESUME);
				++resume_count;
			} else {
				--break_count;
			}
		} else if (good &&
			   cond(kshark_ctx, data[i], val) &&
			   data[i]->next &&
			   !cond(kshark_ctx, data[i]->next, val)) {
			/*
			 * Brack the collection here. Add some margin data
			 * after the data of interest.
			 */
			good = false;
			last_vis_entry = data[i];

			/* Keep adding entries until the "next" record. */
			j = i;
			do {
				++j;
				if (j == end)
					break;
			} while  (last_vis_entry->next != data[j]);

			/*
			 * If the number of added entryes is smaller then the number
			 * of margin entries requested, keep adding until you fill
			 * the margin.
			 */
			if (i + margin < j)
				i = j;
			else
				i += margin;

			last_added = i;
			kshark_add_entry(&temp, i, COLLECTION_BREAK);
			++break_count;
		}
	}

	if (good) {
		kshark_add_entry(&temp, i, COLLECTION_BREAK);
		++break_count;
	}

	if (margin != 0) {
		kshark_add_entry(&temp, first + n_rows - margin, COLLECTION_RESUME);
		++resume_count;

		kshark_add_entry(&temp, first + n_rows, COLLECTION_BREAK);
		++break_count;
	}

	/* Create the collection. */
	struct kshark_entry_collection *col_ptr;
	col_ptr = malloc(sizeof(struct kshark_entry_collection));
	col_ptr->next = NULL;

	col_ptr->resume_points = calloc(resume_count, sizeof(*col_ptr->resume_points));
	col_ptr->break_points = calloc(break_count, sizeof(*col_ptr->break_points));

	col_ptr->cond = cond;
	col_ptr->val = val;

	assert(break_count == resume_count);
	col_ptr->size = resume_count;
	for (i = 0; i < col_ptr->size; ++i) {
		assert(col_list->type == COLLECTION_RESUME);
		col_ptr->resume_points[i] = col_list->index;
		temp = col_list;
		col_list = col_list->next;
		free(temp);

		assert(col_list->type == COLLECTION_BREAK);
		col_ptr->break_points[i] = col_list->index;
		temp = col_list;
		col_list = col_list->next;
		free(temp);
	}

	return col_ptr;
}

static ssize_t
map_collection_index_from_source(const struct kshark_entry_collection *col,
				 size_t source_index)
{
	if (!col->size || source_index > col->break_points[col->size - 1])
		return KS_EMPTY_BIN;

	size_t l = 0, h = col->size - 1, mid;
	if (source_index < col->resume_points[0])
		return l;

	if (source_index > col->resume_points[col->size - 1])
		return KS_LAST_BIN;

	while (h - l > 1) {
		mid = (l + h) / 2;
		if (source_index > col->resume_points[mid])
			l = mid;
		else
			h = mid;
	}

	return h;
}

// static void dump_collection(const struct kshark_entry_collection *col)
// {
// 	for (size_t i = 0; i < col->size; ++i)
// 		printf("%lu  (%lu, %lu)\n", i, col->resume_points[i], col->break_points[i]);
// }

static int
map_collection_back_request(const struct kshark_entry_collection *col,
			    struct kshark_entry_request **req)
{
	struct kshark_entry_request *req_tmp = *req;
	size_t req_end = req_tmp->first - req_tmp->n + 1;

	/*
	 * Find the first Resume Point of the collection which is equal or greater than
	 * the first index of this request.
	 */
	ssize_t col_index = map_collection_index_from_source(col, req_tmp->first);

	/* The value of "col_index" is ambiguous. Deal with all possible cases. */
	if (col_index == KS_EMPTY_BIN) {
		/*
		 * No overlap between the request and the collection.
		 * Do nothing.
		 */
		kshark_free_entry_request(*req);
		*req = NULL;
		return 0;
	}

	if (col_index == 0) {
		if (req_tmp->first == col->resume_points[col_index]) {
			/*
			 * This is a special case. Because this is Back Request,
			 * if the beginning of this request is at the Resume Point
			 * of the interval, then there is only one possible entry,
			 * to look into.
			 */
			req_tmp->n = 1;
			return 1;
		}

		/*
		 * No overlap between the request and the collection.
		 * Do nothing.
		 */
		kshark_free_entry_request(*req);
		*req = NULL;
		return 0;
	} else if (col_index > 0) {
		/*
		 * This is Back Request, so the "col_index" interval of the collection
		 * is guaranteed to be outside the requested data, except in one
		 * special case.
		 */
		if (req_tmp->first == col->resume_points[col_index]) {
			/*
			 * We still have to check the very first entry of the "col_index"
			 * interval.
			 */
			if (req_end > col->break_points[col_index - 1]) {
				/* There is only one possible entry, to look into. */
				req_tmp->n = 1;
				return 1;
			}
		}  else {
			/* Move to the previous interval of the collection. */
			--col_index;

			if (req_tmp->first > col->break_points[col_index]) {
				req_tmp->first = col->break_points[col_index];
			}
		}
	} else if (col_index == KS_LAST_BIN) {
		col_index = col->size - 1;
	}

	/*
	 * Now loop over the intervals of the collection going backwords
	 * and create a separate request for each of those interest.
	 */
	int req_count = 1;
	while (req_end <= col->break_points[col_index] &&
	       col_index >= 0) {
		if (req_end >= col->resume_points[col_index]) {
			/*
			 * The last entry of the original request is inside the
			 * "col_index" collection interval. Close the collection
			 * request here and return.
			 */
			req_tmp->n = req_tmp->first - req_end + 1;
			break;
		}

		/*
		 * The last entry of the original request is outside this collection
		 * interval (col_index). Close the collection request at the end of
		 * the interval and move to the next interval. Try to make another
		 * request there.
		 */
		req_tmp->n = req_tmp->first - col->resume_points[col_index] + 1;
		--col_index;

		if (req_end > col->break_points[col_index]) {
			/*
			 * The last entry of the original request comes berofe the
			 * next collection interval. Stop here.
			 */
			break;
		}

		if (col_index > 0) {
			/* Make a new request. */
			req_tmp->next = malloc(sizeof(struct kshark_entry_request));
			req_tmp = req_tmp->next;
			req_tmp->next = NULL;
			req_tmp->first = col->break_points[col_index];
			++req_count;
		}
	}

	return req_count;
}

static int
map_collection_front_request(const struct kshark_entry_collection *col,
			     struct kshark_entry_request **req)
{
	struct kshark_entry_request *req_tmp = *req;
	size_t req_end = req_tmp->first + req_tmp->n - 1;

	/*
	 * Find the first Resume Point of the collection which is equal or greater than
	 * the first index of this request.
	 */
	ssize_t col_index = map_collection_index_from_source(col, req_tmp->first);

	/* The value of "col_index" is ambiguous. Deal with all possible cases. */
	if (col_index == KS_EMPTY_BIN) {
		/*
		 * No overlap between the request and the collection.
		 * Do nothing.
		 */
		kshark_free_entry_request(*req);
		*req = NULL;
		return 0;
	}

	if (col_index == 0) {
		if (col->resume_points[col_index] > req_end) {
			/*
			 * No overlap between the request and the collection.
			 * Do nothing.
			 */
			kshark_free_entry_request(*req);
			*req = NULL;
			return 0;
		}

		/*
		 * The request overlaps with the "col_index" interval of the
		 * collection. Start from here, using the Resume Point of the
		 * interval as begining of the request.
		 */
		req_tmp->first = col->resume_points[col_index];
	} else if (col_index > 0) {
		if (req_end < col->resume_points[col_index] &&
		    req_tmp->first > col->break_points[col_index - 1]) {
			/*
			 * No overlap between the request and the collection.
			 * Do nothing.
			 */
			kshark_free_entry_request(*req);
			*req = NULL;
			return 0;
		}

		if (req_tmp->first <= col->break_points[col_index - 1]) {
			/*
			 * The begining of this request is inside the previous interval
			 * of the collection. Start from there, keeping the original
			 * begining point.
			 */
			--col_index;
		} else {
			/*
			 * The request overlaps with the "col_index" interval of the
			 * collection. Start from here, using the Resume Point of the
			 * interval as begining of the request.
			 */
			req_tmp->first = col->resume_points[col_index];
		}
	} else if (col_index == KS_LAST_BIN) {
		col_index = col->size - 1;
	}

	/*
	 * Now loop over the intervals of the collection going forwards
	 * and create a separate request for each of those interest.
	 */
	int req_count = 1;
	while (req_end >= col->resume_points[col_index] &&
	       col_index < col->size) {
		if (req_end <= col->break_points[col_index]) {
			/*
			 * The last entry of the original request is inside the
			 * "col_index" collection interval. Close the collection
			 * request here and return.
			 */
			req_tmp->n = req_end - req_tmp->first + 1;
			break;
		}

		/*
		 * The last entry of the original request is outside this collection
		 * interval (col_index). Close the collection request at the end of
		 * the interval and move to the next interval. Try to make another
		 * request there.
		 */
		req_tmp->n = col->break_points[col_index] - req_tmp->first + 1;
		++col_index;
		
		if (req_end < col->resume_points[col_index]) {
			/*
			 * The last entry of the original request comes berofe the
			 * begining of next collection interval. Stop here.
			 */
			break;
		}

		if (col_index < col->size) {
			/* Make a new request. */
			req_tmp->next =
				kshark_entry_request_alloc(col->resume_points[col_index],
							   0,
							   req_tmp->cond,
							   req_tmp->val,
							   req_tmp->vis_only,
							   req_tmp->vis_mask);
			req_tmp = req_tmp->next;
			++req_count;
		}
	}

// 	printf("map_collection_front_request --> end\n");
	return req_count;
}

struct kshark_entry *
kshark_get_collection_entry_front(struct kshark_entry_request **req,
				  struct kshark_entry **data,
				  const struct kshark_entry_collection *col)
{
	struct kshark_entry *entry = NULL;
	map_collection_front_request(col, req);

	while (*req) {
		entry = kshark_get_entry_front(*req, data);
		if (entry) {
			break;
		}

		*req = (*req)->next;
	}

	return entry;
}

struct kshark_entry *
kshark_get_collection_entry_back(struct kshark_entry_request **req,
				 struct kshark_entry **data,
				 const struct kshark_entry_collection *col)
{
	struct kshark_entry *entry = NULL;
	map_collection_back_request(col, req);

	while (*req) {
		entry = kshark_get_entry_back(*req, data);
		if (entry) {
			break;
		}

		*req = (*req)->next;
	}

	return entry;
}

struct kshark_entry_collection *
kshark_find_data_collection(struct kshark_entry_collection *col,
			    matching_condition_func cond,
			    int val)
{
	while (col) {
		if (col->cond == cond && col->val == val)
			return col;

		col = col->next;
	}

	return NULL;
}

void kshark_reset_data_collection(struct kshark_entry_collection *col)
{
	free(col->resume_points);
	col->resume_points = NULL;

	free(col->break_points);
	col->break_points = NULL;

	col->size = 0;
}

static void kshark_free_data_collection(struct kshark_entry_collection *col)
{
	if (col->size) {
		free(col->resume_points);
		free(col->break_points);
	}

	free(col);
}

struct kshark_entry_collection *
kshark_register_data_collection(struct kshark_context *kshark_ctx,
				struct kshark_entry **data,
				size_t n_rows,
				matching_condition_func cond,
				int val,
				size_t margin)
{
	struct kshark_entry_collection *col =
		kshark_data_collection_alloc(kshark_ctx, data,
					     0, n_rows,
					     cond, val,
					     margin);

	if(!col)
		return NULL;

	col->next = kshark_ctx->collections;
	kshark_ctx->collections = col;

	return col;
}

void kshark_unregister_data_collection(struct kshark_entry_collection **col,
				       matching_condition_func cond,
				       int val)
{
	struct kshark_entry_collection **last = col;
	struct kshark_entry_collection *list;

	for (list = *col; list; list = list->next) {
		if (list->cond == cond && list->val == val) {
			*last = list->next;
			kshark_free_data_collection(list);
			return;
		}

		last = &list->next;
	}
}

void kshark_free_collection_list(struct kshark_entry_collection *col)
{
	struct kshark_entry_collection *last;

	while (col) {
		last = col;
		col = col->next;
		kshark_free_data_collection(last);
	}
}
