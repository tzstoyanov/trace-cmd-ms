/*
 * Copyright (C) 2009, 2010 Red Hat Inc, Steven Rostedt <srostedt@redhat.com>
 *
 * Implemented a fixed row size to speed up list.
 *  Copyright (C) 2010 Darren Hart <dvhltc@us.ibm.com>
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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
// #include <gtk/gtk.h>

#include "trace-view-store.h"

#include "cpu.h"
// #include "trace-filter.h"


static int mask_cpu_isset(TraceViewStore *store, int cpu)
{
	return cpu_isset(store->cpu_mask, cpu);
}

static void mask_set_cpus(TraceViewStore *store, int cpus)
{
	set_cpus(store->cpu_mask, cpus);
}

/*****************************************************************************
 *
 *	merge_sort_rows_ts: Merge sort the data by time stamp
 *
 *****************************************************************************/

static void merge_sort_rows_ts(TraceViewStore *store)
{
	uint64_t ts;
	int next;
	unsigned int *indexes;
	unsigned int count = 0;
	int cpu;
	unsigned int i;

// 	indexes = g_new0(unsigned int, store->cpus);
	indexes = (unsigned int*) calloc(store->cpus, sizeof(unsigned int));

	/* Now sort these by timestamp */
	do {
		next = -1;
		ts = 0;
		for (cpu = 0; cpu < store->cpus; cpu++) {
			if (!store->all_cpus && !mask_cpu_isset(store, cpu))
				continue;

 try_again:
			if (indexes[cpu] == store->cpu_items[cpu])
				continue;

			i = indexes[cpu];

			if (!store->cpu_list[cpu][i].visible) {
				indexes[cpu]++;
				goto try_again;
			}

			if (!ts || store->cpu_list[cpu][i].timestamp < ts) {
				ts = store->cpu_list[cpu][i].timestamp;
				next = cpu;
			}
		}
		if (next >= 0) {
			i = indexes[next]++;
			store->rows[count] = &store->cpu_list[next][i];
			store->cpu_list[next][i].pos = count++;
		}
	} while (next >= 0);

	store->visible_rows = count;
	store->start_row = 0;
	store->pages = (count / store->rows_per_page) + 1;

	if (store->page > 1) {
		if (count < store->page * store->rows_per_page)
			store->page = store->pages;

		/* still greater? */
		if (store->page > 1) {
			store->start_row =
				(store->page - 1) * store->rows_per_page;
			assert(store->start_row < count);
		}
	}

	store->num_rows = count > (store->start_row + store->rows_per_page) ?
		store->rows_per_page :
		count - store->start_row;

// 	update_page(store);

// 	free(indexes);
	free(indexes);
}

/*****************************************************************************
 *
 *	trace_view_store_init: this is called everytime a new trace view store object
 *	instance is created (we do that in trace_view_store_new).
 *	Initialise the list structure's fields here.
 *
 *****************************************************************************/

static void trace_view_store_init (TraceViewStore *trace_view_store)
{
	trace_view_store->n_columns	= TRACE_VIEW_STORE_N_COLUMNS;

	assert (TRACE_VIEW_STORE_N_COLUMNS == 8);

	trace_view_store->num_rows = 0;
	trace_view_store->rows	= NULL;

	trace_view_store->page = 1;
	trace_view_store->pages = 1;
	trace_view_store->rows_per_page = TRACE_VIEW_DEFAULT_MAX_ROWS;
	trace_view_store->num_rows = 0;
	trace_view_store->start_row = 0;
	trace_view_store->visible_rows = 0;
	trace_view_store->actual_rows = 0;

	/* Set all columns visible */
	trace_view_store->visible_column_mask = (1 << TRACE_VIEW_STORE_N_COLUMNS) - 1;

	trace_view_store->stamp = rand();	/* Random int to check whether an iter belongs to our model */

}

/*****************************************************************************
 *
 *	trace_view_store_new:	This is what you use in your own code to create a
 *	new trace view store tree model for you to use.
 *
 *****************************************************************************/

TraceViewStore* trace_view_store_new (struct tracecmd_input *handle)
{
	TraceViewStore *newstore;
	struct pevent_record *data;
	int cpu, count, total=0;
	struct temp {
		uint64_t		offset;
		uint64_t		ts;
		struct temp	*next;
	} *list, **next, *rec;

// 	newstore = (TraceViewStore*) g_object_new (TRACE_VIEW_STORE_TYPE, NULL);
	newstore = (TraceViewStore*) malloc(sizeof(TraceViewStore));
	trace_view_store_init(newstore);

	assert( newstore != NULL );

	newstore->handle = handle;
	newstore->cpus = tracecmd_cpus(handle);
	tracecmd_ref(handle);
	newstore->event_filter = pevent_filter_alloc(tracecmd_get_pevent(handle));


// 	newstore->cpu_list = g_new(TraceViewRecord *, newstore->cpus);
	newstore->cpu_list = (TraceViewRecord **) malloc(newstore->cpus * sizeof(TraceViewRecord*));

	assert(newstore->cpu_list != NULL);

// 	newstore->cpu_items = g_new(int, newstore->cpus);
	newstore->cpu_items = (int*) malloc(newstore->cpus * sizeof(int));

	assert(newstore->cpu_items != NULL);

	newstore->all_cpus = 1;
	newstore->all_events = 1;

// 	newstore->cpu_mask = g_new0(uint64_t, (newstore->cpus >> 6) + 1);
	newstore->cpu_mask = (uint64_t*) calloc((newstore->cpus >> 6) + 1, sizeof(uint64_t));

	assert(newstore->cpu_mask != NULL);

	mask_set_cpus(newstore, newstore->cpus);

	for (cpu = 0; cpu < newstore->cpus; cpu++) {

		count = 0;
		list = NULL;
		next = &list;

		data = tracecmd_read_cpu_first(handle, cpu);
		while (data) {
			*next = rec = malloc(sizeof(*rec));
			assert(rec != NULL);
			rec->offset = data->offset;
			rec->ts = data->ts;
			rec->next = NULL;
			next = &rec->next;
			free_record(data);
			count++;
			data = tracecmd_read_data(handle, cpu);
		}

		if (count) {
			TraceViewRecord *trec;
			struct temp *t;
			int i;

			rec = list;

// 			trec = g_new(TraceViewRecord, count);
			trec = (TraceViewRecord*) malloc(count * sizeof(TraceViewRecord));

			assert(trec != NULL);

			for (i = 0; i < count; i++) {
				assert(rec != NULL);
				trec[i].cpu = cpu;
				trec[i].timestamp = rec->ts;
				trec[i].offset = rec->offset;
				trec[i].visible = 1;
				trec[i].pos = i;
				t = rec;
				rec = rec->next;
				free(t);
			}
			assert(rec == NULL);

			newstore->cpu_list[cpu] = trec;
		} else
			newstore->cpu_list[cpu] = NULL;

		newstore->cpu_items[cpu] = count;

		total += count;
	}

	newstore->actual_rows = total;
	newstore->rows = malloc(sizeof(*newstore->rows) * total + 1);

	merge_sort_rows_ts(newstore);

	return newstore;
}

/*****************************************************************************
 *
 *	trace_view_store_finalize: this is called just before a trace view store is
 *	destroyed. Free dynamically allocated memory here.
 *
 *****************************************************************************/

void trace_view_store_finalize (TraceViewStore *store)
{
	int cpu;
	/* free all records and free all memory used by the list */
	for (cpu = 0; cpu < store->cpus; cpu++)
		free(store->cpu_list[cpu]);

	free(store->cpu_list);
	free(store->cpu_mask);
	free(store->rows);
	free(store->cpu_items);

// 	filter_task_hash_free(store->task_filter);  !!!!!

	pevent_filter_free(store->event_filter);
	tracecmd_close(store->handle);
}
