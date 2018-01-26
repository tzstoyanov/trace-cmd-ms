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

#include "plugin_sched.h"
#include "KsPlotTools.hpp"

extern struct plugin_sched_context *plugin_sched_context_handler;

static void openBox(KsPlot::Rectangle **rec, KsPlot::Bin *bin)
{
	if (*rec)
		delete *rec;

	*rec = new KsPlot::Rectangle;
	(**rec)._color = KsPlot::Color(255, 0, 0);
	(**rec)._drawContour = true;
	(**rec).setPoint(0, bin->_base._x - 1,
			    bin->_base._y - CPU_GRAPH_HEIGHT*.3);

	(**rec).setPoint(1, bin->_base._x - 1,
			    bin->_base._y - 1);
}

static void closeBox(KsPlot::Rectangle **rec, KsPlot::Bin *bin, QList<KsPlot::Shape*> *list)
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
				 QList<KsPlot::Shape*> *shapes)
{
	if (pid == 0)
		return;

	struct plugin_sched_context *plugin_ctx = plugin_sched_context_handler;
	if (!plugin_ctx)
		return;

	struct kshark_entry *entry_front, *entry_back;
	KsPlot::Rectangle *rec = nullptr;

	size_t nBins = graph->size();
	for (size_t b = 0; b < nBins; ++b) {
		entry_front = histo->getEntryFront(b, pid, false, plugin_check_pid);
		entry_back  = histo->getEntryBack(b, pid, false, plugin_switch_check_pid);
	
		if (entry_back &&
		    plugin_ctx->sched_switch_event &&
		    entry_back->event_id == plugin_ctx->sched_switch_event->id) {
			if (entry_back->pid != pid) {
				openBox(&rec, &graph->_bins[b]);
				rec->_color = KsPlot::Color(255, 0, 0); // Red
			}
		}

		if (entry_front &&
		    plugin_ctx->sched_switch_event &&
		    entry_front->event_id == plugin_ctx->sched_switch_event->id) {
			if (entry_front->pid == pid)
				closeBox(&rec, &graph->_bins[b], shapes);
		}
	}

	if (rec)
		delete rec;

	return;
}

static void schedWakeupPluginDraw(KsTimeMap *histo,
				 KsPlot::Graph *graph,
				 int pid,
				 QList<KsPlot::Shape*> *shapes)
{
	if (pid == 0)
		return;

	struct plugin_sched_context *plugin_ctx = plugin_sched_context_handler;
	if (!plugin_ctx)
		return;

	struct kshark_entry *entry_front, *entry_back;

	KsPlot::Rectangle *rec = nullptr;

	size_t nBins = graph->size();
	for (size_t b = 0; b < nBins; ++b) {
		entry_front = histo->getEntryFront(b, pid, false, plugin_check_pid);
		entry_back = histo->getEntryBack(b, pid, false, plugin_wakeup_check_pid);

		if (entry_back &&
		    plugin_ctx->sched_wakeup_event &&
		    entry_back->event_id == plugin_ctx->sched_wakeup_event->id) {
			struct pevent_record *record =
				tracecmd_read_at(plugin_ctx->handle, entry_back->offset, NULL);

			if (plugin_get_wakeup_pid(plugin_ctx, record) == pid) {
				openBox(&rec, &graph->_bins[b]);
				rec->_color = KsPlot::Color(0, 255, 0); // Green
			}
		}

		if (entry_front &&
		    plugin_ctx->sched_switch_event &&
		    entry_front->event_id == plugin_ctx->sched_switch_event->id) {
			struct pevent_record *record =
				tracecmd_read_at(plugin_ctx->handle, entry_front->offset, NULL);

			if (plugin_get_next_pid(plugin_ctx, record) == pid)
				closeBox(&rec, &graph->_bins[b], shapes);
		}
	}

	if (rec)
		delete rec;

	return;
}

void plugin_draw(void *vHisto, void *vGraph, int pid, void *vShapes)
{
	KsTimeMap *histo = static_cast<KsTimeMap*>(vHisto);
	KsPlot::Graph *graph = static_cast<KsPlot::Graph*>(vGraph);
	KsPlot::ShapeList *shapes = static_cast<KsPlot::ShapeList*>(vShapes);

	schedWakeupPluginDraw(histo, graph, pid, shapes);
	schedSwitchPluginDraw(histo, graph, pid, shapes);
}
