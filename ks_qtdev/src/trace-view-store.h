/*
 * Copyright (C) 2009, 2010 Red Hat Inc, Steven Rostedt <srostedt@redhat.com>
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
#ifndef _trace_view_store_h_included_
#define _trace_view_store_h_included_

#include <stdint.h>
#include "trace-cmd.h"

#ifdef __cplusplus
extern "C" {
#endif


/* The data columns that we export via the tree model interface */

enum
{
	TRACE_VIEW_STORE_COL_INDEX,
	TRACE_VIEW_STORE_COL_CPU,
	TRACE_VIEW_STORE_COL_TS,
	TRACE_VIEW_STORE_COL_COMM,
	TRACE_VIEW_STORE_COL_PID,
	TRACE_VIEW_STORE_COL_LAT,
	TRACE_VIEW_STORE_COL_EVENT,
	TRACE_VIEW_STORE_COL_INFO,
	TRACE_VIEW_STORE_N_COLUMNS,
} ;


typedef struct trace_view_record		TraceViewRecord;
typedef struct trace_view_store			TraceViewStore;



/* TraceViewRecord: this structure represents a row */

struct trace_view_record
{
	/* What we need from the record */
	uint64_t		timestamp;
	uint64_t		offset;
	int		cpu;

	/* admin stuff used by the trace view store model */
	int		visible;
	unsigned int		pos;	/* pos within the array */
};



/* TraceViewStore: this structure contains everything we need for our
 *             model implementation. You can add extra fields to
 *             this structure, e.g. hashtables to quickly lookup
 *             rows or whatever else you might need, but it is
 *             crucial that 'parent' is the first member of the
 *             structure.                                          */

struct trace_view_store
{
	unsigned int			start_row; /* row to start at */
	unsigned int			num_rows; /* number of rows that we have showing   */
	unsigned int			visible_rows; /* number of rows defined */
	unsigned int			actual_rows; /* size of rows array */
	TraceViewRecord		**rows;	/* a dynamically allocated array of pointers to
					 *   the TraceViewRecord structure for each row    */

	unsigned int			visible_column_mask;
	int			n_columns;		/* number of columns visible */

// 	GType			column_types[TRACE_VIEW_STORE_N_COLUMNS];

	/* Tracecmd specific info */
	struct tracecmd_input 	*handle;
	struct event_format		*sched_switch_event;
	struct format_field		*sched_switch_next_field;
	struct event_format		*sched_wakeup_event;
	struct format_field		*sched_wakeup_pid_field;
	struct event_format		*sched_wakeup_new_event;
	struct format_field		*sched_wakeup_new_pid_field;
	int			cpus;

	TraceViewRecord		**cpu_list;
	int			*cpu_items;

	int			page;
	int			pages;
	int			rows_per_page;

	/* filters */
	int			all_events; /* set 1 when all events are enabled */
						/* else */
	struct event_filter	*event_filter; /* Filtered events */
	struct filter_task	*task_filter;	/* hash of tasks to filter on */
	struct filter_task	*hide_tasks;	/* hash of tasks to not display */

	int			all_cpus;   /* set 1 when all cpus are enabled */
						/* else */
	uint64_t			*cpu_mask;  /* cpus that are enabled */

	int		stamp;	/* Random integer to check whether an iter belongs to our model */
};



TraceViewStore	*trace_view_store_new (struct tracecmd_input *handle);
void trace_view_store_finalize (TraceViewStore *store);

#define TRACE_VIEW_DEFAULT_MAX_ROWS 1000000


#ifdef __cplusplus
}
#endif


#endif /* _trace_view_store_h_included_ */
