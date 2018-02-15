/*
 *  Copyright (C) 2018 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
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

// Kernel Shark 2
#include "libkshark.h"
#include "plugins/sched_events.h"
#include "KsPlotTools.hpp"

extern struct plugin_sched_context *plugin_sched_context_handler;

static void openBox(KsPlot::Rectangle **rec, KsPlot::Bin *bin)
{
	if (!*rec)
		*rec = new KsPlot::Rectangle;

	(**rec)._drawContour = true;
	(**rec).setPoint(0, bin->_base._x - 1,
			    bin->_base._y - CPU_GRAPH_HEIGHT*.3);

	(**rec).setPoint(1, bin->_base._x - 1,
			    bin->_base._y - 1);
}

static void closeBox(KsPlot::Rectangle **rec, KsPlot::Bin *bin, KsPlot::ShapeList *list)
{
	if (*rec == nullptr)
		return;

	(**rec).setPoint(3, bin->_base._x - 1,
			    bin->_base._y - CPU_GRAPH_HEIGHT*.3);

	(**rec).setPoint(2, bin->_base._x - 1,
			    bin->_base._y - 1);

	list->append(*rec);
	*rec = nullptr;
}

static void schedSwitchPluginDraw(KsTimeMap *histo,
				 KsPlot::Graph *graph,
				 int pid,
				 KsPlot::ShapeList *shapes)
{
	if (pid == 0)
		return;

	struct plugin_sched_context *plugin_ctx = plugin_sched_context_handler;
	if (!plugin_ctx)
		return;

	struct kshark_entry *entryFront, *entryBack;
	KsPlot::Rectangle *rec = nullptr;

	size_t nBins = graph->size();
	for (size_t bin = 0; bin < nBins; ++bin) {
		/* Starting from the first element in this bin, go forward in time
		 * until you find a trace entry that satisfies the condition defined
		 * by plugin_check_pid. */
		entryFront = histo->getEntryFront(bin, pid, false, kshark_check_pid);

		/* Starting from the last element in this bin, go backward in time
		 * until you find a trace entry that satisfies the condition defined
		 * by plugin_switch_check_pid. */
		entryBack = histo->getEntryBack(bin, pid, false, plugin_switch_check_pid);
	
		if (entryBack &&
		    plugin_ctx->sched_switch_event &&
		    entryBack->event_id == plugin_ctx->sched_switch_event->id) {
			if (entryBack->pid != pid) {
				/* entryBack is a sched_switch_event. Open a red box here.*/
				openBox(&rec, &graph->_bins[bin]);
				rec->_color = KsPlot::Color(255, 0, 0); // Red
			}
		}

		if (entryFront &&
		    plugin_ctx->sched_switch_event &&
		    entryFront->event_id == plugin_ctx->sched_switch_event->id) {
			if (entryFront->pid == pid) {
				/* entryFront is sched_switch_event. Close the box and add it
				 * to the list of shapes to be ploted. */
				closeBox(&rec, &graph->_bins[bin], shapes);
			}
		}
	}

	if (rec)
		delete rec;

	return;
}

static void schedWakeupPluginDraw(KsTimeMap *histo,
				 KsPlot::Graph *graph,
				 int pid,
				 KsPlot::ShapeList *shapes)
{
	if (pid == 0)
		return;

	struct plugin_sched_context *plugin_ctx = plugin_sched_context_handler;
	if (!plugin_ctx)
		return;

	struct kshark_entry *entryFront, *entryBack;
	KsPlot::Rectangle *rec = nullptr;

	size_t nBins = graph->size();
	for (size_t bin = 0; bin < nBins; ++bin) {
		/* Starting from the first element in this bin, go forward in time
		 * until you find a trace entry that satisfies the condition defined
		 * by plugin_check_pid. */
		entryFront = histo->getEntryFront(bin, pid, false, kshark_check_pid);

		/* Starting from the last element in this bin, go backward in time
		 * until you find a trace entry that satisfies the condition defined
		 * by plugin_wakeup_check_pid. */
		entryBack = histo->getEntryBack(bin, pid, false, plugin_wakeup_check_pid);

		if (entryBack &&
		    plugin_ctx->sched_wakeup_event &&
		    entryBack->event_id == plugin_ctx->sched_wakeup_event->id) {
			struct pevent_record *record =
				tracecmd_read_at(plugin_ctx->handle, entryBack->offset, NULL);

			if (plugin_get_wakeup_pid(plugin_ctx, record) == pid) {
				/* entryBack is a sched_wakeup_event. Open a green box here.*/
				openBox(&rec, &graph->_bins[bin]);
				rec->_color = KsPlot::Color(0, 255, 0); // Green
			}
		}

		if (entryFront &&
		    plugin_ctx->sched_switch_event &&
		    entryFront->event_id == plugin_ctx->sched_switch_event->id) {
			struct pevent_record *record =
				tracecmd_read_at(plugin_ctx->handle, entryFront->offset, NULL);

			if (plugin_get_next_pid(plugin_ctx, record) == pid) {
				/* entryFront is sched_switch_event. Close the box and add it
				 * to the list of shapes to be ploted. */
				closeBox(&rec, &graph->_bins[bin], shapes);
			}
		}
	}

	if (rec)
		delete rec;

	return;
}

void plugin_draw(void *histoPtr, void *graphPtr, void *shapeListPtr, int pid, int draw_action)
{
	if (draw_action != KSHARK_PLUGIN_TASK_DRAW)
		return;

	KsTimeMap *histo = static_cast<KsTimeMap*>(histoPtr);
	KsPlot::Graph *graph = static_cast<KsPlot::Graph*>(graphPtr);
	KsPlot::ShapeList *shapes = static_cast<KsPlot::ShapeList*>(shapeListPtr);

	schedWakeupPluginDraw(histo, graph, pid, shapes);
	schedSwitchPluginDraw(histo, graph, pid, shapes);
}
