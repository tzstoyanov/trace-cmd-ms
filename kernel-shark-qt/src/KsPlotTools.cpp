/*
 * Copyright (C) 2017 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
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

// OpenGL
#include <GL/glut.h>
#include <GL/gl.h>

// KernelShark
#include "KsUtils.hpp"
#include "KsPlotTools.hpp"
#include "KsTraceGraph.hpp"

namespace KsPlot
{

float Color::_frequency = .75;

Color::Color()
: _r(0), _g(0), _b(0)
{}

Color::Color(uint8_t r, uint8_t g, uint8_t b)
: _r(r), _g(g), _b(b)
{}

Color::Color(int rgb)
{
	_r = rgb & 0xFF;
	_g = (rgb >> 8) & 0xFF;
	_b = (rgb >> 16) & 0xFF;
}

Color& Color::operator =(const QColor &c)
{
	_r = c.red();
	_g = c.green();
	_b = c.blue();

	return *this;
}

void Color::setRainbowColor(int n)
{
	_r = sin(_frequency * n + 0) * 127 + 128;
	_g = sin(_frequency * n + 2) * 127 + 128;
	_b = sin(_frequency * n + 4) * 127 + 128;
}

Point::Point()
: _x(0), _y(0)
{}

Point::Point(int x, int y)
: _x(x), _y(y)
{}

void Point::_draw(const Color &c, float s) const
{
	glPointSize(s);
	glBegin(GL_POINTS);
	glColor3ub(c._r, c._g, c._b);
	glVertex2i(_x, _y);
	glEnd();
}

Line::Line()
: _a(nullptr), _b(nullptr)
{}

Line::Line(Point *a, Point *b)
: _a(a), _b(b)
{}

void Line::_draw(const Color &c, float s) const
{
	glLineWidth(s);
	glBegin(GL_LINES);
	glColor3ub(c._r, c._g, c._b);
	glVertex2i(_a->_x, _a->_y);
	glVertex2i(_b->_x, _b->_y);
	glEnd();
}

Polygon::Polygon(size_t n)
: _nPoints(n),
  _points(new Point[n]),
  _drawContour(false)
{}

Polygon::~Polygon()
{
	delete[] _points;
}

void Polygon::setPoint(size_t i, int x, int y)
{
	_points[i]._x = x;
	_points[i]._y = y;
}

void Polygon::setPoint(size_t i, const Point &p)
{
	this->setPoint(i, p._x, p._y);
}

void average(const Point &a, const Point &b, Point *average)
{
	average->_x = (b._x + a._x) / 2;
	average->_y = (b._y + a._y) / 2;
}

void Polygon::_draw(const Color &c, float s) const
{
	if (_drawContour) {
		// Draw all lines.
		Line lines[_nPoints];
		lines[0]._a = &_points[_nPoints - 1];
		lines[0]._b = &_points[0];
		lines[0]._size = _size;
		lines[0]._color = this->_color;
		lines[0].draw();

		for(size_t i = 1; i < _nPoints; ++i) {
			lines[i]._a = &_points[i - 1];
			lines[i]._b = &_points[i];
			lines[i]._size = _size;
			lines[i]._color = this->_color;
			lines[i].draw();
		}

		glPushAttrib(GL_CURRENT_BIT);
	} else {
		Point inPoint;
		average(_points[0], _points[2], &inPoint);

		glBegin(GL_TRIANGLE_FAN);
		glColor3ub(c._r, c._g, c._b);
		glVertex2i(inPoint._x, inPoint._y);
		for (size_t i = 0; i < _nPoints; ++i)
			glVertex2i(_points[i]._x, _points[i]._y);

		glVertex2i(_points[0]._x, _points[0]._y);
		glEnd();
	}
}

Mark::Mark()
: _a(new Point),
  _b(new Point)
{
	_visible = false;
	_cpu._color = Color(225, 255, 100);
	_cpu._size = 5.5f;
	_task._color = Color(0, 255, 0);
	_task._size = 5.5f;
}

Mark::~Mark()
{
	delete _a;
	delete _b;
}

void Mark::setMark(int bin, int cpuhId, int taskId, KsGLWidget *w, int dpr)
{
	_size = 1.5*dpr;
	_task._size =_cpu._size = 1.5 + 4.f * dpr;
	_a->_x = bin + w->_hMargin;
	_b->_x = _a->_x;
	_cpu._x = _a->_x;
	_task._x = _a->_x;

	_a->_y = w->_vMargin / 2 + 2;
	_b->_y = w->height() - w->_vMargin;

	if (cpuhId >= 0) {
		_cpu._y = w->_graphs[cpuhId]->_bins[0]._base._y;
		_cpu._visible = true;
	} else {
		_cpu._visible = false;
	}

	if (taskId >= 0) {
		_task._y = w->_graphs[taskId]->_bins[0]._base._y;
		_task._visible = true;
	} else {
		_task._visible = false;
	}
}

void Mark::_draw(const Color &col, float s) const
{
	Line l(_a, _b);
	l._color = col;
	l._size = s;
	l.draw();
	_cpu.draw();
	_task.draw();
}

Bin::Bin()
: _id(0), _color(), _pidFront(-1), _pidBack(-1)
{}

Bin::Bin(size_t id)
: _id(id), _color(), _pidFront(-1), _pidBack(-1)
{}

void Bin::drawLine(float size)
{
	Line l(&_base, &_val);
	l._color = _color;
	l._size = size;
	l.draw();
}

void Bin::drawVal(float size)
{
	_val._size = size;
	_val.draw();
}

Graph::Graph(size_t s)
: _size(s),
  _hMargin(30),
  _histo(nullptr),
  _graphCollection(nullptr),
  _bins(new Bin[s])
{
	for (size_t i = 0; i < _size; ++i) {
		_bins[i]._base._x = i + _hMargin;
		_bins[i]._base._y = 0;
		_bins[i]._val._x = _bins[i]._base._x;
		_bins[i]._val._y = _bins[i]._base._y;
	}
}

Graph::~Graph()
{
	delete[] _bins;
}

size_t Graph::size()
{
	return _size;
}

void Graph::setBase(int b)
{
	if (!_size)
		return;

	if (b == _bins[0]._base._y) // Nothing to do.
		return;

	for (size_t i = 0; i < _size; ++i) {
		int mod = _bins[i].mod();
		_bins[i]._base._y = b;
		_bins[i]._val._y = b + mod;
	}
}

void Graph::setHMargin(int hMargin)
{
	if (!_size)
		return;

	if (hMargin == _bins[0]._base._x) // Nothing to do.
		return;

	for (size_t i = 0; i < _size; ++i) {
		_bins[i]._base._x = i + hMargin;
		_bins[i]._val._x = _bins[i]._base._x;
	}
}

void Graph::setBinValue(size_t bin, int val)
{
	_bins[bin]._val._y = _bins[bin]._base._y - val;
}

void Graph::setBinPid(size_t bin, int pidF, int pidB)
{
	_bins[bin]._pidFront = pidF;
	_bins[bin]._pidBack = pidB;
}

void Graph::setBinColor(size_t bin, const Color &c)
{
	_bins[bin]._color = c;
}

void Graph::setBinVisMask(size_t bin, uint8_t m)
{
	_bins[bin]._visMask = m;
}
void Graph::setBin(size_t bin, int pidF, int pidB, const Color &c, uint8_t m)
{
	setBinPid(bin, pidF, pidB);
	setBinValue(bin, CPU_GRAPH_HEIGHT * .7);
	setBinColor(bin, c);
	setBinVisMask(bin, m);
}

void Graph::fillCpuGraph(int cpu, const KsPlot::ColorTable &pidColors)
{
// 	hd_time t0 = GET_TIME;

	int pidFront(0), pidBack(0);
	struct kshark_entry *eFront, *eBack;
	int nBins = _histo->n_bins;
	uint8_t visMask;

	auto set_bin = [&] (int bin) {
		if (pidFront != KS_EMPTY_BIN) {
			/* This is a regular process. */
			setBin(bin, pidFront, pidBack, pidColors[pidFront], 0);
		} else {
			/*
			 * The bin contens no data from this Cpu or the data has
			 * been filtered. Whene the graph is ploter the Process Id
			 * of the bin will be derived from the previous bin.
			 */
			setBinPid(bin, KS_EMPTY_BIN, KS_EMPTY_BIN);
		}
	};

	auto get_pid = [&] (int bin) {
		if (_graphCollection && _graphCollection->size) {
			pidFront = ksmodel_get_collection_pid_front(_histo, bin, cpu, true,
								_graphCollection, &eFront);
			pidBack = ksmodel_get_collection_pid_back(_histo, bin, cpu, true,
							       _graphCollection, &eBack);
			int pidBackNoFilter = ksmodel_get_collection_pid_back(_histo, bin, cpu, false,
							       _graphCollection, NULL);
			if (pidBack != pidBackNoFilter)
				pidBack = KS_FILTERED_BIN;
		} else {
			pidFront = ksmodel_get_pid_front(_histo, bin, cpu, true, &eFront);
			pidBack = ksmodel_get_pid_back(_histo, bin, cpu, true, &eBack);
		}
	};

	/* Check the content of the very firs bin and see if the Cpu is active. */
	int bin(0);
	get_pid(bin);
	if (pidFront >= 0) {
		/* The Cpu is active and this is a regular process. Set this bin. */
		set_bin(bin);
	} else {
		/*
		 * No data from this Cpu in the very firs bin. Use the Lower
		 * Overflow Bin to retrieve the Process Id (if any). First
		 * get the Pid back, ignoring the filters.
		 */
		int pidBackNoFilter = ksmodel_get_pid_back(_histo,
							LOWER_OVERFLOW_BIN,
							cpu,
							false,
							NULL);

		/* Now get the Pid back, applying filters. */
		pidBack = ksmodel_get_pid_back(_histo,
					    LOWER_OVERFLOW_BIN,
					    cpu,
					    true,
					    NULL);

		if (pidBack != pidBackNoFilter) {
			/* The Lower Overflow Bin ends with filtered data. */
			setBinPid(bin, KS_FILTERED_BIN, KS_FILTERED_BIN);
		} else {
			/*
			 * The Lower Overflow Bin ends with data which has
			 * to be plotted.
			 */
			setBinPid(bin, pidBack, pidBack);
		}
	}

	/* The first bin is already processed. The loop starts from the second bin. */
	for (bin = 1; bin < nBins; ++bin) {
		/*
		 * Check the content of this bin and see if the Cpu is active.
		 * If yes, retrieve the Process Id. If not, derive from the
		 * previous bin.
		 */
		get_pid(bin);
		set_bin(bin);
	}

// 	double time = GET_DURATION(t0);
// 	qInfo() << "Cpu Graph (" << cpu << "): " << 1e3 * time << " ms.";
}

void Graph::fillTaskGraph(int pid)
{
// 	hd_time t0 = GET_TIME;

	int cpu, pidFront(0), pidBack(0), lastCpu(-1);
// 	struct kshark_entry *eFront, *eBack;
	int nBins = _histo->n_bins;

	bool useCollection = false;
	if (_graphCollection && _graphCollection->size)
		useCollection = true;

	auto set_bin = [&] (int bin) {
		if (cpu >= 0) {
			KsPlot::Color col;
			col.setRainbowColor(cpu);

			/* Data from thе Task has been found in this bin. */
			if (pid == pidFront && pid == pidBack) {
				/* No data from other tasks in this bin. */
				setBin(bin, pid, pid, col, 0);
			} else if (pid != pidFront && pid != pidBack) {
				/*
				 * There is some data from another tasks at both
				 * front and back sides of this bin. But we still
				 * want to see this bin drawn.
				 */
				setBin(bin, pid, KS_FILTERED_BIN, col, 0);
			} else {
				if (pidFront != pid) {
					/*
					 * There is some data from another task
					 * at the front side of this bin.
					 */
					pidFront = KS_FILTERED_BIN;
				}

				if (pidBack != pid) {
					/*
					 * There is some data from another task
					 * at the back side of this bin.
					 */
					pidBack = KS_FILTERED_BIN;
				}

				setBin(bin, pidFront, pidBack, col, 0);
			}

			lastCpu = cpu;
		} else {
			/*
			 * No data from thе Task in this bin. Check the Cpu, previously used
			 * by the task.
			 */
			int cpuPid = ksmodel_get_pid_back(_histo, bin, lastCpu, false, NULL);
			if (cpuPid != KS_EMPTY_BIN) {
				/*
				 * If the Cpu is active and works on another task break the
				 * graph here.
				 */
				setBinPid(bin, KS_FILTERED_BIN, KS_EMPTY_BIN);
			} else {
				/* No data from this Cpu in the bin. Continue the graph. */
				setBinPid(bin, KS_EMPTY_BIN, KS_EMPTY_BIN);
			}
		}
	};

	auto get_pid_cpu = [&] (int bin) {
		if (useCollection) {
			/*
			 * A data collection exists. Use this collection to speed up the
			 * processing of the graph. First get the CPU used by this task.
			 */
			cpu = ksmodel_get_collection_cpu(_histo, bin, pid, false,
							 _graphCollection, NULL);

			/* Get the process Id at the begining and at the end bin. */
			pidFront = ksmodel_get_collection_pid_front(_histo, bin, cpu, false,
								_graphCollection, NULL);
			pidBack = ksmodel_get_collection_pid_back(_histo, bin, cpu, false,
							       _graphCollection, NULL);
		} else {
			/* Get the CPU used by this task. */
			cpu = ksmodel_get_cpu(_histo, bin, pid, false, NULL);

			/* Get the process Id at the begining and at the end bin. */
			pidFront = ksmodel_get_pid_front(_histo, bin, cpu, false, NULL);
			pidBack = ksmodel_get_pid_back(_histo, bin, cpu, false, NULL);
		}
	};

	/* Check the content of the very firs bin and see if the Task is active. */
	int bin = 0;
	get_pid_cpu(bin);

	if (cpu >= 0) {
		/* The Task is active. Set this bin. */
		set_bin(bin);
	} else {
		/*
		 * No data from this Task in the very firs bin. Use the Lower
		 * Overflow Bin to retrieve the Cpu used by the task (if any).
		 */
		if (useCollection)
			cpu = ksmodel_get_collection_cpu(_histo, LOWER_OVERFLOW_BIN, pid, false,
							 _graphCollection, NULL);
		else
			cpu = ksmodel_get_cpu(_histo, LOWER_OVERFLOW_BIN, pid, false, NULL);
		if (cpu >= 0) {
			/*
			 * The Lower Overflow Bin contains data from this Task. Now
			 * look again in the Lower Overflow Bin and find the Pid of the
			 * last active task on the same Cpu.
			 */
			int pidCpu;
			if (useCollection) {
				pidCpu = ksmodel_get_collection_pid_back(_histo,
									 LOWER_OVERFLOW_BIN,
									 cpu, false,
									 _graphCollection,
									 NULL);
			}else {
				pidCpu = ksmodel_get_pid_back(_histo,
							      LOWER_OVERFLOW_BIN,
							      cpu, false,
							      NULL);
			}

			if (pidCpu == pid) {
				/*
				 * The Task is the last one running on this Cpu. Set the Pid
				 * of the bin. In this case the very firs bin is empty but
				 * we derive the Process Id from the Lower Overflow Bin.
				 */
				setBinPid(bin, pid, pid);
				lastCpu = cpu;
			}
		}
	}

	/* The first bin is already processed. The loop starts from the second bin. */
	for (bin = 1; bin < nBins; ++bin) {
		get_pid_cpu(bin);

		/* Set the bin accordingly. */
		set_bin(bin);
	}

// 	double time = GET_DURATION(t0);
// 	qInfo() << "Task Graph ( pid:" << pid << " col:" << _graphCollection->size << "): " << 1e3 * time << " ms.";
}

void Graph::draw(const ColorTable &pidColors, float s)
{
	/*
	 * Start by drawing a line between the base points of the first
	 * and the last bin.
	 */
	Line l(&_bins[0]._base, &_bins[_size - 1]._base);
	l._size = s;
	l.draw();

	/* Draw as lines all bins containing data. */
	for (size_t i = 0; i < _size; ++i)
		if (_bins[i]._pidFront >= 0 || _bins[i]._pidBack >= 0)
// 			if (_bins[i]._visMask)
				_bins[i].drawLine(s);

	/*
	 * Draw colored boxes. First find the first bin, which contains data and
	 * determine its Pid.
	 */
	int lastPid = 0;
	Rectangle rec;
	size_t b = 0;
	while (b < _size) {
		if (_bins[b]._pidBack > 0) {
			lastPid = _bins[b]._pidFront;
			/*
			 * Initialize a box starting from this bin.
			 * The color of the box corresponds to the Pid
			 * of the process.
			 */
			rec._color = Color(pidColors[lastPid]);
			rec.setPoint(0, _bins[b]._base._x,
					_bins[b]._base._y - CPU_GRAPH_HEIGHT * .3);
			rec.setPoint(1, _bins[b]._base._x,
					_bins[b]._base._y);
			break;
		}

		++b;
	}

	while (b < _size) {
		if (_bins[b]._pidFront == KS_EMPTY_BIN &&
		    _bins[b]._pidBack == KS_EMPTY_BIN) {
			/*
			 * This bin is empty. If a colored box is already initialized,
			 * it will be extended.
			 */
			++b;
			continue;
		}

		if (_bins[b]._pidFront != _bins[b]._pidBack ||
		    _bins[b]._pidFront != lastPid ||
		    _bins[b]._pidBack  != lastPid) {
			/* A new process starts here. */
			if (lastPid > 0 && b > 0) {
				/*
				 * There is another process running up to this point. Close
				 * its colored box here and draw.
				 */
				rec.setPoint(3, _bins[b]._base._x - 1,
						_bins[b]._base._y - CPU_GRAPH_HEIGHT * .3);
				rec.setPoint(2, _bins[b]._base._x - 1,
						_bins[b]._base._y);
				rec.draw();
			}

			if (_bins[b]._pidBack > 0) {
				/*
				 * This is a regular process. Initialize colored box
				 * starting from this bin.
				 */
				rec._color = Color(pidColors[_bins[b]._pidBack]);
				rec.setPoint(0, _bins[b]._base._x - 1,
						_bins[b]._base._y - CPU_GRAPH_HEIGHT * .3);
				rec.setPoint(1, _bins[b]._base._x - 1,
						_bins[b]._base._y);
			}

			lastPid = _bins[b]._pidBack;
		}
		++b;
	}

	if (lastPid > 0) {
		/*
		 * This is the end of the Graph and we have a process running.
		 * Close its colored box and draw.
		 */
		rec.setPoint(3, _bins[_size - 1]._base._x,
				_bins[_size - 1]._base._y - CPU_GRAPH_HEIGHT * .3);
		rec.setPoint(2, _bins[_size - 1]._base._x,
				_bins[_size - 1]._base._y);
		rec.draw();
	}
}

}; // KsPlot
