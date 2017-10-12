/*
 * Copyright (c) Siemens AG, 2017
 *
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
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
#include <pthread.h>
#include <sched.h>
#include "event-parse.h"

#define XNOBJECT_NAME_LEN	32

#ifndef SCHED_SPORADIC
#define SCHED_SPORADIC          10
#define sched_ss_low_priority   sched_u.ss.__sched_low_priority
#define sched_ss_repl_period    sched_u.ss.__sched_repl_period
#define sched_ss_init_budget    sched_u.ss.__sched_init_budget
#define sched_ss_max_repl       sched_u.ss.__sched_max_repl
#endif  /* !SCHED_SPORADIC */

#ifndef SCHED_TP
#define SCHED_TP                11
#define sched_tp_partition      sched_u.tp.__sched_partition
#endif  /* !SCHED_TP */

#ifndef SCHED_QUOTA
#define SCHED_QUOTA             12
#define sched_quota_group       sched_u.quota.__sched_group
#endif  /* !SCHED_QUOTA */

#define SCHED_COBALT            42
#define SCHED_WEAK              43
#define __SCHED_CURRENT         44

struct __sched_ss_param {
        int __sched_low_priority;
        struct timespec __sched_repl_period;
        struct timespec __sched_init_budget;
        int __sched_max_repl;
};

#define sched_rr_quantum        sched_u.rr.__sched_rr_quantum

struct __sched_rr_param {
        struct timespec __sched_rr_quantum;
};

struct __sched_tp_param {
        int __sched_partition;
};

struct __sched_quota_param {
        int __sched_group;
};

struct sched_param_ex {
	int sched_priority;
	union {
		struct __sched_ss_param ss;
		struct __sched_rr_param rr;
		struct __sched_tp_param tp;
		struct __sched_quota_param quota;
	} sched_u;
};

static void register_name(struct format_field *field, void *data, int pid)
{
	char buf[XNOBJECT_NAME_LEN + 1];
	unsigned int offset;

	if (pid == 0)
		return;

	offset = data2host4(field->event->pevent, data + field->offset) & 0xffff;

	memcpy(buf, data + offset, XNOBJECT_NAME_LEN);
	buf[XNOBJECT_NAME_LEN] = 0;

	pevent_register_comm(field->event->pevent, buf, pid);
}

static int switch_handler(struct trace_seq *s, struct pevent_record *record,
			  struct event_format *event, void *context)
{
	struct format_field *field;
	unsigned long long pid;

	if (pevent_get_field_val(s, event, "prev_pid", record, &pid, 1))
		return -1;

	field = pevent_find_field(event, "prev_name");
	if (!field)
		return -1;

	register_name(field, record->data, pid);

	if (pevent_get_field_val(s, event, "next_pid", record, &pid, 1))
		return -1;

	field = pevent_find_field(event, "next_name");
	if (!field)
		return -1;

	register_name(field, record->data, pid);

//	pevent_print_fields(s, record->data, record->size, event);

	return 1;
}

static int wakeup_handler(struct trace_seq *s, struct pevent_record *record,
			  struct event_format *event, void *context)
{
	struct format_field *field;
	unsigned long long pid;

	if (pevent_get_field_val(s, event, "pid", record, &pid, 1))
		return -1;

	field = pevent_find_field(event, "name");
	if (!field)
		return -1;

	register_name(field, record->data, pid);

	return 1;
}

static unsigned long long
process_sched_params(struct trace_seq *p, unsigned long long *args)
{
	int policy = (int)args[1];
	const struct sched_param_ex *params = (void *)(unsigned long)args[2];

	switch (policy) {
	case SCHED_QUOTA:
		trace_seq_printf(p, "priority=%d, group=%d",
				 params->sched_priority,
				 params->sched_quota_group);
		break;
	case SCHED_TP:
		trace_seq_printf(p, "priority=%d, partition=%d",
				 params->sched_priority,
				 params->sched_tp_partition);
		break;
	case SCHED_OTHER:
		break;
	case SCHED_SPORADIC:
		trace_seq_printf(p, "priority=%d, low_priority=%d, "
				 "budget=(%ld.%09ld), period=(%ld.%09ld), "
				 "maxrepl=%d",
				 params->sched_priority,
				 params->sched_ss_low_priority,
				 params->sched_ss_init_budget.tv_sec,
				 params->sched_ss_init_budget.tv_nsec,
				 params->sched_ss_repl_period.tv_sec,
				 params->sched_ss_repl_period.tv_nsec,
				 params->sched_ss_max_repl);
		break;
	case SCHED_RR:
	case SCHED_FIFO:
	case SCHED_COBALT:
	case SCHED_WEAK:
	default:
		trace_seq_printf(p, "priority=%d", params->sched_priority);
		break;
	}
	trace_seq_putc(p, '\0');

	return 0;
}

int PEVENT_PLUGIN_LOADER(struct pevent *pevent)
{
	pevent_register_event_handler(pevent, -1, "cobalt_core",
				      "cobalt_switch_context",
				      switch_handler, NULL);

	pevent_register_event_handler(pevent, -1, "cobalt_core",
				      "cobalt_thread_resume",
				      wakeup_handler, NULL);

	pevent_register_print_function(pevent,
				       process_sched_params,
				       PEVENT_FUNC_ARG_STRING,
				       "cobalt_trace_parse_sched_params",
				       PEVENT_FUNC_ARG_PTR,
				       PEVENT_FUNC_ARG_INT,
				       PEVENT_FUNC_ARG_PTR,
				       PEVENT_FUNC_ARG_VOID);
	return 0;
}

void PEVENT_PLUGIN_UNLOADER(struct pevent *pevent)
{
	pevent_unregister_print_function(pevent, process_sched_params,
					 "cobalt_trace_parse_sched_params");

	pevent_unregister_event_handler(pevent, -1, "cobalt_core",
					"cobalt_thread_resume",
					wakeup_handler, NULL);

	pevent_unregister_event_handler(pevent, -1, "cobalt_core",
					"cobalt_switch_context",
					switch_handler, NULL);
}
