/*
 * Copyright (C) 2018 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
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

#ifndef _KS_PLUGINS_H
#define _KS_PLUGINS_H

// Kernel Shark 2
#include "KsUtils.hpp"
#include "KsPlotTools.hpp"
#include "KsModel.hpp"

struct KsCppArgV {
	KsTimeMap *_histo;
	KsPlot::Graph *_graph;
	KsPlot::ShapeList *_shapes;

	kshark_cpp_argv *toC()
	{
		return reinterpret_cast<kshark_cpp_argv*>(this);
	}
};

#define KS_ARGV_TO_CPP(a) (reinterpret_cast<KsCppArgV*>(a))

#endif
