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
#include "plugins/xenomai_sched_events.h"
#include "KsPlotTools.hpp"
#include "KsPlugins.hpp"

extern struct xenomai_context *xenomai_context_handler;

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

	if (bin->_base._x - (**rec).getPoint(0)._x < 4) {
		/* This box is too small.
		 * Don't try to plot it. */
		delete *rec;
		*rec = nullptr;
		return;
	}

	(**rec).setPoint(3, bin->_base._x - 1,
			    bin->_base._y - CPU_GRAPH_HEIGHT*.3);

	(**rec).setPoint(2, bin->_base._x - 1,
			    bin->_base._y - 1);

	list->append(*rec);
	*rec = nullptr;
}

static void xenomaiWakeupPluginDraw(struct xenomai_context *plugin_ctx,
				    KsTimeMap *histo,
				    KsPlot::Graph *graph,
				    int pid,
				    QList<KsPlot::Shape*> *shapes)
{
	struct kshark_entry *entryFront, *entryBack;
	KsPlot::Rectangle *rec = nullptr;

	size_t nBins = graph->size();
	for (size_t bin = 0; bin < nBins; ++bin) {
		if (histo->binCount(bin) > 20)
			continue;

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
			if (cobalt_get_wakeup_pid_lazy(plugin_ctx, entryBack) == pid) {
				/* entryBack is a cobalt_wakeup_event.
				 * Open a dark cobalt blue box here. */
				openBox(&rec, &graph->_bins[bin]);
				rec->_color = KsPlot::Color(66, 126, 165); // Dark cobalt
			}
		}

		if (entryFront &&
		    plugin_ctx->cobalt_switch_event &&
		    entryFront->event_id == plugin_ctx->cobalt_switch_event->id &&
		    entryFront->pid == pid) {
			/* entryFront is a cobalt_switch_event.
			 * Close the box and add it to the list
			 * of shapes to be ploted. */
			closeBox(&rec, &graph->_bins[bin], shapes);
		}
	}

	if (rec)
		delete rec;

	return;
}

static void xenomaiSwitchPluginDraw(struct xenomai_context *plugin_ctx,
				    KsTimeMap *histo,
				    KsPlot::Graph *graph,
				    int pid,
				    QList<KsPlot::Shape*> *shapes)
{
	struct kshark_entry *entryFront, *entryBack;
	KsPlot::Rectangle *rec = nullptr;

	size_t nBins = graph->size();
	for (size_t bin = 0; bin < nBins; ++bin) {
		if (histo->binCount(bin) > 20)
			continue;

		/* Starting from the first element in this bin, go forward in time
		 * until you find a trace entry that satisfies the condition defined
		 * by kshark_check_pid. */
		entryFront = histo->getEntryFront(bin, pid, false, kshark_check_pid);

		/* Starting from the last element in this bin, go backward in time
		 * until you find a trace entry that satisfies the condition defined
		 * by cobalt_switch_check_pid. */
		entryBack = histo->getEntryBack(bin, pid, false, cobalt_switch_check_pid);

		if (entryBack && entryBack->pid != pid &&
		    entryBack->event_id == plugin_ctx->cobalt_switch_event->id) {
			/* entryBack is a cobalt_switch_event.
			 * Open a light cobalt blue box here. */
			openBox(&rec, &graph->_bins[bin]);
			rec->_color = KsPlot::Color(93, 184, 244); // Light cobalt blue
		}

		if (entryFront && entryFront->pid == pid &&
		    entryFront->event_id == plugin_ctx->cobalt_switch_event->id) {
			/* entryFront is cobalt_switch_event.
			 * Close the box and add it to the list
			 * of shapes to be ploted. */
			closeBox(&rec, &graph->_bins[bin], shapes);
		}
	}

	if (rec)
		delete rec;

	return;
}

void xenomai_draw(struct kshark_cpp_argv *argv_c, int pid, int draw_action)
{
	if (draw_action != KSHARK_PLUGIN_TASK_DRAW || pid == 0)
		return;

	struct xenomai_context *plugin_ctx = xenomai_context_handler;
	if (!plugin_ctx)
		return;

	KsCppArgV *argvCpp = KS_ARGV_TO_CPP(argv_c);

	xenomaiWakeupPluginDraw(plugin_ctx, argvCpp->_histo,
					    argvCpp->_graph,
					    pid,
					    argvCpp->_shapes);

	xenomaiSwitchPluginDraw(plugin_ctx, argvCpp->_histo,
					    argvCpp->_graph,
					    pid,
					    argvCpp->_shapes);
}
