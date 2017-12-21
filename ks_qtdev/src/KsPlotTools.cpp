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

// Qt
#include <QMouseEvent>

// OpenGL
#include <iostream>
#include <GL/glut.h>
#include <GL/gl.h>

// Kernel Shark 2
#include "KsUtils.h"
#include "KsPlotTools.h"
#include "KsTraceGraph.h"

namespace KsPlot
{

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

Color& Color::operator ++()
{
	int rgb = 0;
	rgb |= _r;
	rgb |= (_g << 8);
	rgb |= (_b << 16);
	rgb = (217013*rgb+1531011);
	rgb = (rgb >> 7) & 0xFFFFFF;

	*this = Color(rgb);
	this->nice();
	return *this;
}

Color& Color::operator =(const QColor &c)
{
	_r = c.red();
	_g = c.green();
	_b = c.blue();

	return *this;
}

void Color::nice()
{
	int sum = _r + _g + _b;
	while (sum < 350 || sum > 650) {
		++(*this);
		sum = _r + _g + _b;
	}
}

void Color::setRainbowsColor(int n)
{
	float frequency = .3;

	_r = sin(frequency*n + 0) * 127 + 128;
	_g = sin(frequency*n + 2) * 127 + 128;
	_b = sin(frequency*n + 4) * 127 + 128;
}

Color Palette::_colors[20];

Point::Point()
: _x(0), _y(0), _size(2.f)
{}

Point::Point(int x, int y)
: _x(x), _y(y), _size(2.f)
{}

void Point::_draw(const Color &c) const
{
	glPointSize(_size);
	glBegin(GL_POINTS);
	glColor3ub(c._r, c._g, c._b);
	glVertex2i(_x, _y);
	glEnd();
}

Line::Line()
: _a(NULL), _b(NULL)
{}

Line::Line(Point *a, Point *b)
: _a(a), _b(b)
{}

void Line::_draw(const Color &c) const
{
	glBegin(GL_LINES);
	glColor3ub(c._r, c._g, c._b);
	glVertex2i(this->_a->_x, this->_a->_y);
	glVertex2i(this->_b->_x, this->_b->_y);
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
	average->_x = (b._x + a._x)/2;
	average->_y = (b._y + a._y)/2;
}

void Polygon::_draw(const Color &c) const
{
	if (_drawContour) {
		// Draw all lines.
		Line lines[_nPoints];
		lines[0]._a = &_points[_nPoints - 1];
		lines[0]._b = &_points[0];

		for(size_t i = 1; i < _nPoints; ++i) {
			lines[i]._a = &_points[i - 1];
			lines[i]._b = &_points[i];
		}

		for (size_t i = 0; i < _nPoints; ++i)
			lines[i].draw();

		glPushAttrib( GL_CURRENT_BIT );
	} else {
		Point inPoint;
		average(_points[0], _points[2], &inPoint);

		glBegin(GL_TRIANGLE_FAN);
		glColor3ub(c._r, c._g, c._b);
		glVertex2f(inPoint._x, inPoint._y);
		for (size_t i = 0; i<_nPoints; ++i)
			glVertex2f(_points[i]._x, _points[i]._y);

		glVertex2f(_points[0]._x, _points[0]._y);
		glEnd();
	}
}

Mark::Mark()
: _a(new Point(0, 0)),
  _b(new Point(0, 0)),
  _cpu(new Point(0, 0)),
  _task(new Point(0, 0))
{
	_visible = false;
	_cpu->_color = Color(255, 0, 0); // Red
	_cpu->_size = 6.f;
	_task->_color = Color(0, 255, 0); // Red
	_task->_size = 6.f;
}

Mark::~Mark()
{
	delete _a;
	delete _b;
	delete _cpu;
	delete _task;
}

void Mark::setMark(int b, int cpuhId, int taskId, KsGLWidget *w)
{
	_a->_x = b + w->_hMargin;
	_b->_x = _a->_x;
	_cpu->_x = _a->_x;
	_task->_x = _a->_x;

	_a->_y = w->_vMargin/2 + 2;
	_b->_y = w->height() - w->_vMargin;

	if (cpuhId >= 0) {
		_cpu->_y = w->_graphs[cpuhId]->_bins[0]._base._y;
		_cpu->_visible = true;
	} else {
		_cpu->_visible = false;
	}

	if (taskId >= 0) {
		_task->_y = w->_graphs[taskId]->_bins[0]._base._y;
		_task->_visible = true;
	} else {
		_task->_visible = false;
	}
}

void Mark::_draw(const Color &col) const
{
	Line l(_a, _b);
	l._color = col;
	l.draw();
	_cpu->draw();
	_task->draw();
}

Bin::Bin()
: _id(0), _color(), _pidFront(-1), _pidBack(-1)
{}

Bin::Bin(size_t id)
: _id(id), _color(), _pidFront(-1), _pidBack(-1)
{}

void Bin::drawLine()
{
	Line l(&_base, &_val);
	l._color = _color;
	l.draw();
}

void Bin::drawVal()
{
	_val.draw();
}

Graph::Graph(size_t s)
: _size(s), _hMargin(30), _bins(new Bin[s])
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
		_bins[i]._base._y = b;
		_bins[i]._val._y = b + 1;
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

void Graph::setBin(size_t bin, int pidF, int pidB, const Color &c)
{
	setBinPid(bin, pidF, pidB);
	setBinValue(bin, CPU_GRAPH_HEIGHT*.7);
	setBinColor(bin, c);
}

void Graph::draw(const QHash<int, KsPlot::Color> &pidColors)
{
	/* Start by drawing a line between the base points of the first
	 * and the last bin. */
	Line l(&_bins[0]._base, &_bins[_size - 1]._base);
	l.draw();

	/* Draw as lines all bins containing data. */
	for (size_t i = 0; i < _size; ++i)
		if (_bins[i]._pidFront >= 0)
			_bins[i].drawLine();

	/* Draw colored boxes.
	 * First find the first bin, which contains data and determine its Pid. */
	int lastPid = 0;
	Rectangle rec;
	size_t b = 0;
	while (b < _size) {
		if (_bins[b]._pidBack > 0) {
			lastPid = _bins[b]._pidFront;
			/* Initialize a box starting from this bin.
			 * The color of the box corresponds to the Pid
			 * of the process. */
			rec._color = Color(pidColors[lastPid]);
			rec.setPoint(0, _bins[b]._base._x,
					_bins[b]._base._y - CPU_GRAPH_HEIGHT*.3);
			rec.setPoint(1, _bins[b]._base._x,
					_bins[b]._base._y);
			break;
		}

		++b;
	}

	while (b < _size) {
		if (_bins[b]._pidFront == KS_EMPTY_BIN) {
			/* This bin is empty. If a colored box is already initialized,
			 * it will be extended. */
			++b;
			continue;
		}

		if ((_bins[b]._pidFront != _bins[b]._pidBack) ||
		    (lastPid != _bins[b]._pidFront)) {
			/* A new process starts here. */
			if (lastPid > 0 && b > 0) {
				/* There is another process running up to this point. Close
				 * its colored box here and draw. */
				rec.setPoint(3, _bins[b - 1]._base._x,
						_bins[b - 1]._base._y - CPU_GRAPH_HEIGHT*.3);
				rec.setPoint(2, _bins[b - 1]._base._x,
						_bins[b - 1]._base._y);
				rec.draw();
			}

			if (_bins[b]._pidBack > 0) {
				/* This is a regular process. Initialize colored box
				 * starting from this bin. */
				rec._color = Color(pidColors[_bins[b]._pidBack]);
				rec.setPoint(0, _bins[b]._base._x,
						_bins[b]._base._y - CPU_GRAPH_HEIGHT*.3);
				rec.setPoint(1, _bins[b]._base._x,
						_bins[b]._base._y);
			}

			lastPid = _bins[b]._pidBack;
		}
		++b;
	}

	if (lastPid > 0) {
		/* This is the end of the Graph and we have a process running.
		 * Close its colored box and draw. */
		rec.setPoint(3, _bins[_size - 1]._base._x,
				_bins[_size - 1]._base._y - CPU_GRAPH_HEIGHT*.3);
		rec.setPoint(2, _bins[_size - 1]._base._x,
				_bins[_size - 1]._base._y);

		rec.draw();
	}
}

}; // KsPlot
