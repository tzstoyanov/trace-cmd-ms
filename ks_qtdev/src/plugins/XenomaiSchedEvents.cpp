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

#include "libkshark.h"
#include "plugins/xenomai_sched_events.h"
#include "KsPlotTools.hpp"

extern struct xenomai_context *xenomai_context_handler;

/*static*/ void openBox(KsPlot::Rectangle **rec, KsPlot::Bin *bin)
{
	if (!*rec)
		*rec = new KsPlot::Rectangle;

	(**rec)._drawContour = true;
	(**rec).setPoint(0, bin->_base._x - 1,
			    bin->_base._y - CPU_GRAPH_HEIGHT*.3);

	(**rec).setPoint(1, bin->_base._x - 1,
			    bin->_base._y - 1);
}

/*static*/ void closeBox(KsPlot::Rectangle **rec, KsPlot::Bin *bin, KsPlot::ShapeList *list)
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

static void xenomaiSwitchPluginDraw(KsTimeMap *histo,
				 KsPlot::Graph *graph,
				 int pid,
				 QList<KsPlot::Shape*> *shapes)
{
	if (pid == 0)
		return;

	struct xenomai_context *plugin_ctx = xenomai_context_handler;
	if (!plugin_ctx)
		return;

	struct kshark_entry *entryFront, *entryBack;
	KsPlot::Rectangle *rec = nullptr;

	size_t nBins = graph->size();
	for (size_t bin = 0; bin < nBins; ++bin) {
		/* Starting from the first element in this bin, go forward in time
		 * until you find a trace entry that satisfies the condition defined
		 * by kshark_check_pid. */
		entryFront = histo->getEntryFront(bin, pid, false, kshark_check_pid);

		/* Starting from the last element in this bin, go backward in time
		 * until you find a trace entry that satisfies the condition defined
		 * by cobalt_switch_check_pid. */
		entryBack = histo->getEntryBack(bin, pid, false, cobalt_switch_check_pid);
	
		if (entryBack &&
		    plugin_ctx->cobalt_switch_event &&
		    entryBack->event_id == plugin_ctx->cobalt_switch_event->id) {
			if (entryBack->pid != pid) {
				/* entryBack is a sched_switch_event. Open a red box here.*/
				openBox(&rec, &graph->_bins[bin]);
				rec->_color = KsPlot::Color(255, 222, 0); // Yellow
			}
		}

		if (entryFront &&
		    plugin_ctx->cobalt_switch_event &&
		    entryFront->event_id == plugin_ctx->cobalt_switch_event->id) {
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

static void xenomaiWakeupPluginDraw(KsTimeMap *histo,
				 KsPlot::Graph *graph,
				 int pid,
				 QList<KsPlot::Shape*> *shapes)
{
	if (pid == 0)
		return;

	struct xenomai_context *plugin_ctx = xenomai_context_handler;
	if (!plugin_ctx)
		return;

	struct kshark_entry *entryFront, *entryBack;
	KsPlot::Rectangle *rec = nullptr;
// 
	size_t nBins = graph->size();
	for (size_t bin = 0; bin < nBins; ++bin) {
		/* Starting from the first element in this bin, go forward in time
		 * until you find a trace entry that satisfies the condition defined
		 * by kshark_check_pid. */
		entryFront = histo->getEntryFront(bin, pid, false, kshark_check_pid);

		/* Starting from the last element in this bin, go backward in time
		 * until you find a trace entry that satisfies the condition defined
		 * by cobalt_wakeup_check_pid. */
		entryBack = histo->getEntryBack(bin, pid, false, cobalt_wakeup_check_pid);

		if (entryBack &&
		    plugin_ctx->cobalt_wakeup_event &&
		    entryBack->event_id == plugin_ctx->cobalt_wakeup_event->id) {
			struct pevent_record *record =
				tracecmd_read_at(plugin_ctx->handle, entryBack->offset, NULL);

			if (cobalt_get_wakeup_pid(plugin_ctx, record) == pid) {
				/* entryBack is a sched_wakeup_event. Open a green box here.*/
				openBox(&rec, &graph->_bins[bin]);
				rec->_color = KsPlot::Color(222, 0, 255); // Purple
			}
		}

		if (entryFront &&
		    plugin_ctx->cobalt_switch_event &&
		    entryFront->event_id == plugin_ctx->cobalt_switch_event->id) {
			struct pevent_record *record =
				tracecmd_read_at(plugin_ctx->handle, entryFront->offset, NULL);

			if (cobalt_get_next_pid(plugin_ctx, record) == pid) {
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

void xenomai_draw(void *histoPtr, void *graphPtr, void *shapeListPtr, int pid, int draw_action)
{
	if (draw_action != KSHARK_PLUGIN_TASK_DRAW)
		return;

	KsTimeMap *histo = static_cast<KsTimeMap*>(histoPtr);
	KsPlot::Graph *graph = static_cast<KsPlot::Graph*>(graphPtr);
	KsPlot::ShapeList *shapes = static_cast<KsPlot::ShapeList*>(shapeListPtr);

	xenomaiWakeupPluginDraw(histo, graph, pid, shapes);
	xenomaiSwitchPluginDraw(histo, graph, pid, shapes);
}
