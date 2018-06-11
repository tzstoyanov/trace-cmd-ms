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

#ifndef _LIB_KSHARK_MODEL_H
#define _LIB_KSHARK_MODEL_H

// KernelShark
#include "libkshark.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

enum OverflowBin {
	UPPER_OVERFLOW_BIN = -1,
	LOWER_OVERFLOW_BIN = -2,
};

struct kshark_trace_histo {
	struct kshark_entry	**data;
	size_t			data_size;
	ssize_t			*map;
	size_t			*bin_count;

	uint64_t		min;
	uint64_t		max;
	uint64_t		bin_size;
	size_t			n_bins;
};

void ksmodel_init(struct kshark_trace_histo *histo);

void ksmodel_clear(struct kshark_trace_histo *histo);

void ksmodel_set_bining(struct kshark_trace_histo *histo,
			size_t n, uint64_t min, uint64_t max);

void ksmodel_set_in_range_bining(struct kshark_trace_histo *histo,
				 size_t n, uint64_t min, uint64_t max);

size_t ksmodel_bin_count(struct kshark_trace_histo *histo, int bin);

void ksmodel_fill(struct kshark_trace_histo *histo,
		  struct kshark_entry **data, size_t n);

void ksmodel_shift_forward(struct kshark_trace_histo *histo, size_t n);

void ksmodel_shift_backward(struct kshark_trace_histo *histo, size_t n);

void ksmodel_jump_to(struct kshark_trace_histo *histo, size_t ts);

void ksmodel_zoom_out(struct kshark_trace_histo *histo,
		      double r, int mark);

void ksmodel_zoom_in(struct kshark_trace_histo *histo,
		     double r, int mark);

size_t ksmodel_first_index_at(struct kshark_trace_histo *histo, int bin);

ssize_t ksmodel_first_index_at_cpu(struct kshark_trace_histo *histo,
				   int bin, int cpu);

ssize_t ksmodel_first_index_at_pid(struct kshark_trace_histo *histo,
				   int bin, int pid);

struct kshark_entry *ksmodel_get_entry_front(struct kshark_trace_histo *histo,
					     int bin, bool vis_only,
					     matching_condition_func func, int val);

struct kshark_entry *ksmodel_get_entry_back(struct kshark_trace_histo *histo,
					    int bin, bool vis_only,
					    matching_condition_func func, int val);

struct kshark_entry *
ksmodel_get_collection_entry_front(struct kshark_trace_histo *histo,
				   int bin, bool vis_only,
				   matching_condition_func func, int val,
				   struct kshark_entry_collection *col);

struct kshark_entry *
ksmodel_get_collection_entry_back(struct kshark_trace_histo *histo,
				  int bin, bool vis_only,
				  matching_condition_func func, int val,
				  struct kshark_entry_collection *col);

int ksmodel_get_pid_front(struct kshark_trace_histo *histo,
			  int bin, int cpu, bool vis_only,
			  struct kshark_entry **e);

int ksmodel_get_pid_back(struct kshark_trace_histo *histo,
			 int bin, int cpu, bool vis_only,
			 struct kshark_entry **e);

int ksmodel_get_cpu(struct kshark_trace_histo *histo,
		    int bin, int pid, bool vis_only,
		    struct kshark_entry **e);

int ksmodel_get_collection_pid_front(struct kshark_trace_histo *histo,
				     int bin, int cpu, bool vis_only,
				     struct kshark_entry_collection *col,
				     struct kshark_entry **e);

int ksmodel_get_collection_pid_back(struct kshark_trace_histo *histo,
				    int bin, int cpu, bool vis_only,
				    struct kshark_entry_collection *col,
				    struct kshark_entry **e);

int ksmodel_get_collection_cpu(struct kshark_trace_histo *histo,
			       int bin, int pid, bool vis_only,
			       struct kshark_entry_collection *col,
			       struct kshark_entry **e);

static inline double ksmodel_bin_time(struct kshark_trace_histo *histo, int bin)
{
	return (histo->min + bin*histo->bin_size) * 1e-9;
}

static inline uint64_t ksmodel_ts(struct kshark_trace_histo *histo, int bin)
{
	return (histo->min + bin*histo->bin_size);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif
