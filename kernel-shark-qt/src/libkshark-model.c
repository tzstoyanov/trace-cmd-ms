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

// C
#include <stdlib.h>

// KernelShark
#include "libkshark-model.h"

void ksmodel_init(struct kshark_trace_histo *histo)
{
	/*
	 * Initialize an empty histo. The histo will have
	 * no bins and will contain no data.
	 */
	histo->bin_size = 0;
	histo->min = 0;
	histo->max = 0;
	histo->n_bins = 0;
}

void ksmodel_clear(struct kshark_trace_histo *histo)
{
	/*
	 * Reset the histo. It will have no bins and will contain
	 * no data.
	 */
	histo->bin_size = 0;
	histo->min = 0;
	histo->max = 0;

	if (histo->n_bins == 0)
		return;

	histo->n_bins = 0;

	if (histo->map)
		free(histo->map);

	if (histo->bin_count)
		free(histo->bin_count);
}

static void ksmodel_reset_bins(struct kshark_trace_histo *histo,
			       size_t first, size_t last)
{
	/* Reset the content of the bins. */
	size_t r;
	for (r = first; r <= last; ++r) {
		histo->map[r] = KS_EMPTY_BIN;
		histo->bin_count[r] = 0;
	}
}

void ksmodel_set_bining(struct kshark_trace_histo *histo,
			size_t n, uint64_t min, uint64_t max)
{
	/* Clear the existing content of the histo. */
	ksmodel_clear(histo);

	uint64_t range = max - min;
	/* The size of the bin must be >= 1, hence the range must be >= n. */
	if (n == 0 || range < n)
		return;

	histo->n_bins = n;

	if (range % (histo->n_bins) == 0) {
		/*
		 * The range is multiple of the number of bin and needs no
		 * adjustment. This is very unlikely to happen but still ...
		 */
		histo->min = min;
		histo->max = max;
		histo->bin_size = range/histo->n_bins;
	} else {
		/*
		 * The range needs adjustment. The new range will be slightly
		 * bigger, compared to the requested one.
		 */
		histo->bin_size = range/histo->n_bins + 1;
		uint64_t corrected_range = histo->bin_size * histo->n_bins;
		size_t delta_range = corrected_range - range;
		histo->min = min - delta_range/2;
		histo->max = histo->min + corrected_range;
	}

	/* Create bins. Two overflow bins are added. */
	histo->map = calloc(n, sizeof(ssize_t) + 2);
	histo->bin_count = calloc(n, sizeof(size_t) + 2);

	/* Reset the content of all bins to zero. */
	ksmodel_reset_bins(histo, 0, histo->n_bins + 1);
}

void ksmodel_set_in_range_bining(struct kshark_trace_histo *histo,
				 size_t n, uint64_t min, uint64_t max)
{
	/* Clear the existing content of the histo. */
	ksmodel_clear(histo);

	uint64_t range = max - min;
	/* The size of the bin must be >= 1, hence the range must be >= n. */
	if (n == 0 || range < n)
		return;

	histo->n_bins = n;

	if (range % (histo->n_bins) == 0) {
		/*
		 * The range is multiple of the number of bin and needs no
		 * adjustment. This is very unlikely to happen but still ...
		 */
		histo->min = min;
		histo->max = max;
		histo->bin_size = range / histo->n_bins;
	} else {
		/*
		 * The range needs adjustment. The new range will be slightly
		 * bigger, compared to the requested one.
		 */
		histo->bin_size = range / histo->n_bins + 1;
		uint64_t corrected_range = histo->bin_size * histo->n_bins;
		size_t delta_range = corrected_range - range;
		histo->min = min - delta_range / 2;
		histo->max = histo->min + corrected_range;

		/*
		 * Make sure that the new range doesn't go outside of the
		 * time interval of the dataset.
		 */
		if (histo->min < histo->data[0]->ts) {
			histo->min = histo->data[0]->ts;
			histo->max = histo->min + corrected_range;
		} else if (histo->max > histo->data[histo->data_size - 1]->ts) {
			histo->max = histo->data[histo->data_size - 1]->ts;
			histo->min = histo->max - corrected_range;
		}
	}

	/* Create bins. Two overflow bins are added. */
	histo->map = calloc(n, sizeof(ssize_t) + 2);
	histo->bin_count = calloc(n, sizeof(size_t) + 2);

	/* Reset the content of all bins to zero. */
	ksmodel_reset_bins(histo, 0, histo->n_bins + 1);
}

static size_t ksmodel_set_lower_edge(struct kshark_trace_histo *histo)
{
	/*
	 * Find the index of the first entry inside
	 * the range (timestamp > min).
	 */
	size_t row = kshark_find_entry_row(histo->min,
					   histo->data,
					   0,
					   histo->data_size - 1);

	if (row != 0) {
		/*
		 * The first entry inside the range is not the first entry
		 *of the dataset. This means that the Lower Overflow bin
		 * contains data.
		 */

		// Lower Overflow bin starts at "0".
		histo->map[histo->n_bins + 1] = 0;

		/*
		 * The number of entries inside the Lower Overflow bin is
		 * equal to the index of the first entry inside the range.
		 */
		histo->bin_count[histo->n_bins + 1] = row;
	}  else {
		/*
		 * Lower Overflow bin is empty. The number of entries is
		 * already set to "0".
		 */
		histo->map[histo->n_bins + 1] = KS_EMPTY_BIN;
	}

	/*
	 * Now check if the first entry inside the range falls into the
	 * first bin.
	 */
	if (histo->data[row]->ts  < histo->min + histo->bin_size) {
		/*
		 * It is inside the fisrs bin. Set the beginning
		 * of the fisrs bin.
		 */
		histo->map[0] = row;
	} else {
		/* The fisrs bin is empty. */
		histo->map[0] = KS_EMPTY_BIN;
	}

	return row;
}

static size_t ksmodel_set_upper_edge(struct kshark_trace_histo *histo)
{
	/*
	 * Find the index of the first entry outside
	 * the range (timestamp > max).
	 */
	size_t row = kshark_find_entry_row(histo->max,
					   histo->data,
					   0,
					   histo->data_size - 1);

	if (row < histo->data_size - 1 ||
	    (row == histo->data_size - 1 &&
	     histo->data[histo->data_size - 1]->ts > histo->max)) {
		/*
		 * The Upper Overflow bin contains data. Set its beginning
		 * and the number of entries.
		 */
		histo->map[histo->n_bins] = row;
		histo->bin_count[histo->n_bins] = histo->data_size - row;
	}  else {
		/*
		 * Upper Overflow bin is empty. The number of entries is
		 * already set to "0".
		 */
		histo->map[histo->n_bins] = KS_EMPTY_BIN;
	}

	return row;
}

static void ksmodel_set_next_bin_edge(struct kshark_trace_histo *histo,
				      size_t bin)
{
	size_t next_bin = bin + 1;

	/* Calculate the beginning of the next bin. */
	size_t time = histo->min + next_bin * histo->bin_size;

	/*
	 * Find the index of the first entry inside
	 * the next bin (timestamp > time).
	 */
	size_t row = kshark_find_entry_row(time,
					   histo->data,
					   0,
					   histo->data_size - 1);

	/*
	 * The timestamp of the very last entry of the dataset can be exactly
	 * equal to the value of the upper edge of the range. This is very
	 * likely to happen when we use ksmodel_set_in_range_bining().
	 * In this case we have to increase the size of the very last bin
	 * in order to make sure that the last entry of the dataset will
	 * fall into it.
	 */
	if (next_bin == histo->n_bins - 1)
		++time;

	if (histo->data[row]->ts  >= time + histo->bin_size) {
		/* The bin is empty. */
		histo->map[next_bin] = KS_EMPTY_BIN;
		return;
	}

	/* Set the index of the first entry. */
	histo->map[next_bin] = row;
}

static void ksmodel_set_bin_counts(struct kshark_trace_histo *histo)
{
	size_t i = 0, prev_not_empty;

	/*
	 * Find the first bin which contain data. Start by checking the
	 * Lower Overflow bin.
	 */
	if (histo->map[histo->n_bins + 1] != KS_EMPTY_BIN) {
		prev_not_empty = histo->n_bins + 1;
	} else {
		while (histo->map[i] < 0) {
			++i;
		}

		prev_not_empty = i++;
	}

	/*
	 * Starting from the first not empty bin, loop over all bins and
	 * calculate the number of entries.
	 */
	while (i < histo->n_bins) {
		if (histo->map[i] != KS_EMPTY_BIN) {
			histo->bin_count[prev_not_empty] = histo->map[i] -
							   histo->map[prev_not_empty];
			prev_not_empty = i;
		}
		++i;
	}

	/* Check if the Upper Overflow bin contains data. */
	if (histo->map[histo->n_bins] == KS_EMPTY_BIN) {
		/*
		 * The Upper Overflow bin is empty. Use the size of the dataset
		 * to calculate the content of the previouse not empty bin.
		 */
		histo->bin_count[prev_not_empty] = histo->data_size -
						   histo->map[prev_not_empty];
	} else {
		/*
		 * Use the index of the first entry inside the Upper Overflow bin
		 * to calculate the content of the previouse not empty bin.
		 */
		histo->bin_count[prev_not_empty] = histo->map[histo->n_bins] -
						   histo->map[prev_not_empty];
	}
}

size_t ksmodel_bin_count(struct kshark_trace_histo *histo, int bin)
{
	if (bin >= 0 && bin < (int) histo->n_bins)
		return histo->bin_count[bin];

	if (bin == UPPER_OVERFLOW_BIN)
		return histo->bin_count[histo->n_bins];

	if (bin == LOWER_OVERFLOW_BIN)
		return histo->bin_count[histo->n_bins + 1];

	return 0;
}

void ksmodel_fill(struct kshark_trace_histo *histo,
		  struct kshark_entry **data, size_t n)
{
	histo->data = data;
	histo->data_size = n;

	if (histo->n_bins == 0 || histo->bin_size == 0 || histo->data_size == 0) {
		/*
		 * Something is wrong with this histo.
		 * Most likely the binning is not set.
		 */
		ksmodel_clear(histo);
		return;
	}

	/* Set the beginning of the Lower Overflow bin */
	ksmodel_set_lower_edge(histo);
	size_t bin = 0;

	/* Loop over the dataset and set the beginning of all individual bins. */
	while (bin < histo->n_bins) {
		ksmodel_set_next_bin_edge(histo, bin);
		++bin;
	}

	/* Set the beginning of the Upper Overflow bin. */
	ksmodel_set_upper_edge(histo);

	/* Calculate the number of entries in each bin. */
	ksmodel_set_bin_counts(histo);
}

void ksmodel_shift_forward(struct kshark_trace_histo *histo, size_t n)
{
	if (!histo->data_size)
		return;

	if (ksmodel_bin_count(histo, UPPER_OVERFLOW_BIN) == 0) {
		/*
		 * The Upper Overflow bin is empty. This means that we are at
		 * the upper edge of the dataset already. Do nothing in this case.
		 */
		return;
	}

	/* Calculate the new range. */
	histo->min += n * histo->bin_size;
	histo->max += n * histo->bin_size;

	if (n >= histo->n_bins) {
		/*
		 * No overlap between the new and the old ranges. Recalculate all
		 * bins from scratch.
		 */
		ksmodel_set_bining(histo, histo->n_bins, histo->min, histo->max);
		ksmodel_fill(histo, histo->data, histo->data_size);
		return;
	}

	/* Set the beginning of the new Lower Overflow bin */
	ksmodel_set_lower_edge(histo);

	/*
	 * Copy the content of all overlaping bins starting from bin "0"
	 * of the new histo.
	 */
	size_t bin = 0;
	while(bin <  histo->n_bins - n) {
		histo->map[bin] = histo->map[bin + n];
		histo->bin_count[bin] = histo->bin_count[bin + n];
		++bin;
	}

	histo->map[bin] = histo->map[bin + n];
	histo->bin_count[bin] = 0;

	/* Calculate only the content of the new (non-overlapping) bins. */
	ksmodel_reset_bins(histo, bin + 1, histo->n_bins);
	while (bin < histo->n_bins) {
		ksmodel_set_next_bin_edge(histo, bin);
		++bin;
	}

	/*
	 * Set the beginning of the Upper Overflow bin and calculate
	 * the number of entries in each bin.
	 */
	ksmodel_set_upper_edge(histo);
	ksmodel_set_bin_counts(histo);
}

void ksmodel_shift_backward(struct kshark_trace_histo *histo, size_t n)
{
	if (!histo->data_size)
		return;

	if (ksmodel_bin_count(histo, LOWER_OVERFLOW_BIN) == 0) {
		/*
		 * The Lower Overflow bin is empty. This means that we are at
		 * the Lower edge of the dataset already. Do nothing in this case.
		 */
		return;
	}

	/* Calculate the new range. */
	histo->min -= n * histo->bin_size;
	histo->max -= n * histo->bin_size;

	if (n >= histo->n_bins) {
		/*
		 * No overlap between the new and the old range. Recalculate all bins
		 * from scratch.
		 */
		ksmodel_set_bining(histo, histo->n_bins, histo->min, histo->max);
		ksmodel_fill(histo, histo->data, histo->data_size);
		return;
	}

	/*
	 * Copy the content of all overlaping bins starting from the last bin
	 * of the new histo.
	 */
	size_t bin = histo->n_bins - 1;
	while(1) {
		if (bin > histo->n_bins - n && histo->map[bin] >= 0) {
			histo->map[histo->n_bins] = histo->map[bin];
			histo->bin_count[histo->n_bins] += histo->bin_count[bin];
		}

		histo->map[bin] = histo->map[bin-n];
		histo->bin_count[bin] = histo->bin_count[bin-n];
		if (bin == n)
			break;

		--bin;
	}

	/* Reset all new bins. */
	ksmodel_reset_bins(histo, 0, bin);
	ksmodel_reset_bins(histo, histo->n_bins, histo->n_bins + 1);

	/* Set the beginning of the new Lower Overflow bin */
	ksmodel_set_lower_edge(histo);

	/* Calculate only the content of the new (non-overlapping) bins. */
	bin = 0;
	while (bin < n) {
		ksmodel_set_next_bin_edge(histo, bin);
		++bin;
	}

	/*
	 * Set the beginning of the Upper Overflow bin and calculate
	 * the number of entries in each bin.
	 */
	ksmodel_set_upper_edge(histo);
	ksmodel_set_bin_counts(histo);
}

void ksmodel_jump_to(struct kshark_trace_histo *histo, size_t ts)
{
	if (ts > histo->min && ts < histo->max) {
		/*
		 * The new position is already inside the range.
		 * Do nothing in this case.
		 */
		return;
	}

	/*
	 * Calculate the new range without changing the size and the number
	 * of bins.
	 */
	size_t min = ts - histo->n_bins * histo->bin_size / 2;
	if (min < histo->data[0]->ts)
		min = histo->data[0]->ts;

	if (min > histo->data[histo->data_size - 1]->ts - histo->n_bins * histo->bin_size) {
		min = histo->data[histo->data_size - 1]->ts -
		      histo->n_bins * histo->bin_size;
	}

	size_t max = min + histo->n_bins * histo->bin_size;

	/* Use the new range to recalculate all bins from scratch. */
	ksmodel_set_bining(histo, histo->n_bins, min, max);
	ksmodel_fill(histo, histo->data, histo->data_size);
}

void ksmodel_zoom_out(struct kshark_trace_histo *histo,
		      double r, int mark)
{
	if (!histo->data_size) 
		return;

	/*
	 * If the marker is not set, assume that the focus point of the zoom
	 * is the center of the range.
	 */
	if (mark < 0)
		mark = histo->n_bins / 2;

	/*
	 * Calculate the new range of the histo. Use the bin of the marker
	 * as a focus point for the zoomout. With this the maker will stay inside
	 * the same bin in the new histo.
	 */
	size_t range = histo->max - histo->min;
	double delta_tot = range * r;
	size_t delta_min = delta_tot * mark / histo->n_bins;

	size_t min = histo->min - delta_min;
	size_t max = histo->max + (size_t) delta_tot - delta_min;

	/* Make sure the new range doesn't go outside of the dataset. */
	if (min < histo->data[0]->ts)
		min = histo->data[0]->ts;

	if (max > histo->data[histo->data_size - 1]->ts)
		max = histo->data[histo->data_size - 1]->ts;

	/* Use the new range to recalculate all bins from scratch. */
	ksmodel_set_in_range_bining(histo, histo->n_bins, min, max);
	ksmodel_fill(histo, histo->data, histo->data_size);
}

void ksmodel_zoom_in(struct kshark_trace_histo *histo,
		     double r, int mark)
{
	if (!histo->data_size)
		return;

	/*
	 * If the marker is not set, assume that the focus point of the zoom
	 * is the center of the range.
	 */
	if (mark < 0)
		mark = histo->n_bins / 2;

	size_t range = histo->max - histo->min;

	/* Avoid overzooming. */
	if (range < histo->n_bins * 4)
		return;

	/*
	 * Calculate the new range of the histo. Use the bin of the marker
	 * as a focus point for the zoomin. With this the maker will stay inside
	 * the same bin in the new histo.
	 */
	double delta_tot =  range * r;
	size_t delta_min;

	if (mark == (int)histo->n_bins - 1)
		delta_min = delta_tot;
	else if (mark == 0)
		delta_min = 0;
	else
		delta_min = delta_tot * mark / histo->n_bins;

	size_t min = histo->min + delta_min;
	size_t max = histo->max - (size_t) delta_tot + delta_min;

	/* Use the new range to recalculate all bins from scratch. */
	ksmodel_set_in_range_bining(histo, histo->n_bins, min, max);
	ksmodel_fill(histo, histo->data, histo->data_size);
}

size_t ksmodel_first_index_at(struct kshark_trace_histo *histo, int bin)
{
	if (bin >= 0 && bin < (int) histo->n_bins)
		return histo->map[bin];

	if (bin == UPPER_OVERFLOW_BIN)
		return histo->map[histo->n_bins];

	if (bin == LOWER_OVERFLOW_BIN)
		return histo->map[histo->n_bins + 1];

	return KS_EMPTY_BIN;
}

ssize_t ksmodel_first_index_at_cpu(struct kshark_trace_histo *histo,
				   int bin, int cpu)
{
	size_t found = KS_EMPTY_BIN;
	size_t n = ksmodel_bin_count(histo, bin);
	if (!n)
		return found;

	ssize_t pos = histo->map[bin];
	size_t i;
	for (i = pos; i < pos + n; ++i) {
		if (histo->data[i]->cpu == cpu) {
			if ((histo->data[i]->visible & KS_GRAPH_FILTER_MASK) &&
			    (histo->data[i]->visible & KS_EVENT_FILTER_MASK))
				return i;
			else
				found = KS_FILTERED_BIN;
		}
	}

	return found;
}

ssize_t ksmodel_first_index_at_pid(struct kshark_trace_histo *histo,
				   int bin, int pid)
{
	size_t found = KS_EMPTY_BIN;
	size_t n = ksmodel_bin_count(histo, bin);
	if (!n)
		return found;

	ssize_t pos = histo->map[bin];
	size_t i;
	for (i = pos; i < pos + n; ++i) {
		if (histo->data[i]->pid == pid) {
			if ((histo->data[i]->visible & KS_GRAPH_FILTER_MASK) &&
			    (histo->data[i]->visible & KS_EVENT_FILTER_MASK))
				return i;
			else
				found = KS_FILTERED_BIN;
		}
	}

	return found;
}

static struct kshark_entry_request *
ksmodel_entry_front_request_alloc(struct kshark_trace_histo *histo,
				  int bin, bool vis_only,
				  matching_condition_func func, int val)
{
	/* Get the number of entries in this bin. */
	size_t n = ksmodel_bin_count(histo, bin);
	if (!n)
		return NULL;

	size_t first = ksmodel_first_index_at(histo, bin);
	struct kshark_entry_request *req;
	req = kshark_entry_request_alloc(first, n,
					 func, val,
					 vis_only, KS_GRAPH_FILTER_MASK);

	return req;
}

static struct kshark_entry_request *
ksmodel_entry_back_request_alloc(struct kshark_trace_histo *histo,
				 int bin, bool vis_only,
				 matching_condition_func func, int val)
{
	/* Get the number of entries in this bin. */
	size_t n = ksmodel_bin_count(histo, bin);
	if (!n)
		return NULL;

	size_t first = ksmodel_first_index_at(histo, bin) + n - 1;
	struct kshark_entry_request *req;
	req = kshark_entry_request_alloc(first, n,
					 func, val,
					 vis_only, KS_GRAPH_FILTER_MASK);

	return req;
}

struct kshark_entry *ksmodel_get_entry_front(struct kshark_trace_histo *histo,
					     int bin, bool vis_only,
					     matching_condition_func func, int val)
{
	struct kshark_entry_request *req;
	struct kshark_entry *entry;

	/* Set the position at the beginning of the bin and go forward. */
	req = ksmodel_entry_front_request_alloc(histo, bin, vis_only, func, val);
	if (!req)
		return NULL;

	entry = kshark_get_entry_front(req, histo->data);
	kshark_free_entry_request(req);

	return entry;
}

struct kshark_entry *ksmodel_get_entry_back(struct kshark_trace_histo *histo,
					    int bin, bool vis_only,
					    matching_condition_func func, int val)
{
	struct kshark_entry_request *req;
	struct kshark_entry *entry;

	/* Set the position at the end of the bin and go backwards. */
	req = ksmodel_entry_back_request_alloc(histo, bin, vis_only, func, val);
	if (!req)
		return NULL;

	entry = kshark_get_entry_back(req, histo->data);
	kshark_free_entry_request(req);

	return entry;
}

struct kshark_entry *
ksmodel_get_collection_entry_front(struct kshark_trace_histo *histo,
				   int bin, bool vis_only,
				   matching_condition_func func, int val,
				   struct kshark_entry_collection *col)
{
	struct kshark_entry_request *req;
	struct kshark_entry *entry;

	/* Set the position at the beginning of the bin and go forward. */
	req = ksmodel_entry_front_request_alloc(histo, bin, vis_only, func, val);
	if (!req)
		return NULL;

	entry = kshark_get_collection_entry_front(&req, histo->data, col);
	kshark_free_entry_request(req);

	return entry;
}

struct kshark_entry *
ksmodel_get_collection_entry_back(struct kshark_trace_histo *histo,
				  int bin, bool vis_only,
				  matching_condition_func func, int val,
				  struct kshark_entry_collection *col)
{
	struct kshark_entry_request *req;
	struct kshark_entry *entry;

	/* Set the position at the end of the bin and go backwards. */
	req = ksmodel_entry_back_request_alloc(histo, bin, vis_only, func, val);
	if (!req)
		return NULL;

	entry = kshark_get_collection_entry_back(&req, histo->data, col);
	kshark_free_entry_request(req);

	return entry;
}


static bool ksmodel_is_visible(struct kshark_entry *entry)
{
	if (!entry || !entry->visible) // No visible entry has been found.
		return false;
	return true;
}

static int ksmodel_get_entry_pid(struct kshark_entry *entry)
{
	if (!entry) // No data has been found.
		return KS_EMPTY_BIN;

	if (!entry->visible) // Some data has been found, but it is filtered.
		return KS_FILTERED_BIN;

	return entry->pid;
}

static int ksmodel_get_entry_cpu(struct kshark_entry *entry)
{
	if (!entry) // No data has been found.
		return KS_EMPTY_BIN;

	if (!entry->visible)  // Some data has been found, but it is filtered.
		return KS_FILTERED_BIN;

	return entry->cpu;
}

int ksmodel_get_pid_front(struct kshark_trace_histo *histo,
			  int bin, int cpu, bool vis_only,
			  struct kshark_entry_collection *col,
			  struct kshark_entry **e)
{
	struct kshark_entry_request *req;
	struct kshark_entry *entry;

	/* Set the position at the beginning of the bin and go forward. */
	req = ksmodel_entry_front_request_alloc(histo,
						bin, vis_only,
						kshark_check_cpu, cpu);
	if (!req)
		return KS_EMPTY_BIN;

	if (col && col->size)
		entry = kshark_get_collection_entry_front(&req, histo->data, col);
	else
		entry = kshark_get_entry_front(req, histo->data);

	kshark_free_entry_request(req);

	if (e)
		*e = entry;

	return ksmodel_get_entry_pid(entry);
}

int ksmodel_get_pid_back(struct kshark_trace_histo *histo,
			 int bin, int cpu, bool vis_only,
			 struct kshark_entry_collection *col,
			 struct kshark_entry **e)
{
	struct kshark_entry_request *req;
	struct kshark_entry *entry;

	/* Set the position at the end of the bin and go backwards. */
	req = ksmodel_entry_back_request_alloc(histo,
					       bin, vis_only,
					       kshark_check_cpu, cpu);
	if (!req)
		return KS_EMPTY_BIN;

	if (col && col->size)
		entry = kshark_get_collection_entry_back(&req, histo->data, col);
	else
		entry = kshark_get_entry_back(req, histo->data);

	kshark_free_entry_request(req);

	if (e)
		*e = entry;

	return ksmodel_get_entry_pid(entry);
}

int ksmodel_get_cpu(struct kshark_trace_histo *histo,
		    int bin, int pid, bool vis_only,
		    struct kshark_entry_collection *col,
		    struct kshark_entry **e)
{
	/* Get the number of entries in this bin. */
	size_t n = ksmodel_bin_count(histo, bin);
	if (!n)
		return KS_EMPTY_BIN;

	struct kshark_entry_request *req;
	struct kshark_entry *entry;

	/* Create an entry request but keep the starting position unset. */
	req = kshark_entry_request_alloc(0, n,
					 kshark_check_pid, pid,
					 vis_only, KS_GRAPH_FILTER_MASK);

	if (bin == UPPER_OVERFLOW_BIN) {
		/*
		 * Set the position at the end of the Lower Overflow bin and go
		 * backwards.
		 */
		req->first = ksmodel_bin_count(histo, bin) - 1;

		if (col && col->size)
			entry =  kshark_get_collection_entry_back(&req, histo->data, col);
		else
			entry =  kshark_get_entry_back(req, histo->data);
	} else {
		/* Set the position at the beginning of the bin and go forward. */
		req->first = ksmodel_first_index_at(histo, bin);

		if (col && col->size)
			entry =  kshark_get_collection_entry_front(&req, histo->data, col);
		else
			entry = kshark_get_entry_front(req, histo->data);
	}

	kshark_free_entry_request(req);

	if (e)
		*e = entry;

	return ksmodel_get_entry_cpu(entry);
}

bool ksmodel_cpu_visible_event_exist(struct kshark_trace_histo *histo,
				     int bin,
				     int cpu,
				     struct kshark_entry_collection *col,
				     struct kshark_entry **e)
{
	struct kshark_entry_request *req;
	struct kshark_entry *entry;

	/* Set the position at the beginning of the bin and go forward. */
	req = ksmodel_entry_front_request_alloc(histo,
						bin, true,
						kshark_check_cpu, cpu);
	if (!req)
		return false;

	req->vis_mask = KS_EVENT_FILTER_MASK;

	if (col && col->size)
		entry = kshark_get_collection_entry_front(&req, histo->data, col);
	else
		entry = kshark_get_entry_front(req, histo->data);

	kshark_free_entry_request(req);

	if (e)
		*e = entry;

	return ksmodel_is_visible(entry);
}

bool ksmodel_task_visible_event_exist(struct kshark_trace_histo *histo,
				      int bin,
				      int pid,
				      struct kshark_entry_collection *col,
				      struct kshark_entry **e)
{
	struct kshark_entry_request *req;
	struct kshark_entry *entry;

	/* Set the position at the beginning of the bin and go forward. */
	req = ksmodel_entry_front_request_alloc(histo,
						bin, true,
						kshark_check_pid, pid);
	if (!req)
		return false;

	req->vis_mask = KS_EVENT_FILTER_MASK;

	if (col && col->size)
		entry = kshark_get_collection_entry_front(&req, histo->data, col);
	else
		entry = kshark_get_entry_front(req, histo->data);

	kshark_free_entry_request(req);

	if (e)
		*e = entry;

	return ksmodel_is_visible(entry);
}
