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

#ifndef _GLWIDGET_H
#define _GLWIDGET_H

// Qt
#include <QColor>

// KernelShark
#include "KsModel.hpp"

class KsGLWidget;

namespace KsPlot {

void initOpenGl(int dpr);

void resizeOpenGL(int width, int height);

struct Color
{
	Color();
	Color(uint8_t r, uint8_t g, uint8_t b);
	Color(int rgb);

	void setRainbowColor(int n);
	static void setRainbowFrequency(float f) {_frequency = f;}
	static float getRainbowFrequency() {return _frequency;}

	uint8_t _r, _g, _b;
	static float _frequency;
};

typedef QHash<int, KsPlot::Color> ColorTable;

struct Shape
{
	Shape() : _visible(true), _size(2.) {}
	virtual ~Shape() {}

	void draw() const {
		if (_visible)
			_draw(_color, _size);
	}

	bool	_visible;
	Color	_color;
	float	_size;

private:
	virtual void _draw(const Color &c, float s) const = 0;
};

class Point : public Shape
{
	void _draw(const Color &c, float s = 1.) const override;

public:
	Point();
	Point(int x, int y);
	virtual ~Point() {}

	/** Cartesian coordinates. */
	int _x, _y;
};

class Line : public Shape
{
	void _draw(const Color &c, float s = 1.) const override;

public:
	Line();
	Line(Point *a, Point *b);
	virtual ~Line() {}

	Point *_a, *_b;
};

class Polygon : public Shape
{
	Polygon() = delete;

	void _draw(const Color &, float s = 1.) const override;
	size_t	  _nPoints;
	Point	*_points;

public:
	Polygon(size_t n);
	virtual ~Polygon();

	void setPoint(size_t i, int x, int y);
	void setPoint(size_t i, const Point &p);
	const Point &getPoint(size_t i) {return _points[i];}
	size_t pointCount() const {return _nPoints;}
	void makeLines();

	bool	  _drawContour;
};

struct Triangle : public Polygon
{
	Triangle() : Polygon(3) {}
};

struct Rectangle : public Polygon
{
	Rectangle() : Polygon(4) {}
};

class Mark : public Shape
{
	void _draw(const Color &c, float s = 1.) const override;
	Point *_a, *_b;
	Point _cpu, _task;

public:
	Mark();
	virtual ~Mark();

	void setMark(int bin, int cpuId, int taskId, KsGLWidget *w, int dpr);
};

struct Bin
{
	Bin();
	Bin(size_t id);

	void drawLine(float size = 1.);
	void drawVal(float size = 2.);

	int mod() {return _val._y - _base._y;}

	size_t	_id;
	Color	_color;
	int 	_pidFront, _pidBack;
	Point	_base, _val;
	uint8_t	_visMask;
};

class Graph
{
	Graph() = delete;

	size_t	_size;
	int	_hMargin;

public:
	Graph(size_t s);
	~Graph();

	size_t size();
	void fillCpuGraph(int cpu, const KsPlot::ColorTable &pidColors);
	void fillTaskGraph(int pid);
	void draw(const ColorTable &pidColors, float s = 1);

	void setBase(int b);
	void setBinValue(size_t bin, int val);
	void setBinPid(size_t bin, int pidF, int pidB);
	void setBinColor(size_t bin, const Color &c);
	void setBinVisMask(size_t bin, uint8_t m);
	void setBinIdle(size_t bin, bool idle);
	void setBin(size_t bin, int pidF, int pidB, const Color &c, uint8_t m);
	void deriveBin(int bin);

	void setHMargin(int hMargin);

	kshark_trace_histo	*_histo;
	kshark_entry_collection *_graphCollection;

	Bin	*_bins;
};

}; // KsPlot

#endif  /* _GLWIDGET_H */