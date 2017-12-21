
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

// Kernel Shark 2
#include "KsModel.h"
#include "KsUtils.h"

class KsGLWidget;

namespace KsPlot {

struct Color
{
	Color();
	Color(uint8_t r, uint8_t g, uint8_t b);
	Color(int rgb);

	Color& operator ++();
	Color& operator =(const QColor &c);

	void nice();
	void setRainbowsColor(int n);
	uint8_t _r, _g, _b;
};

struct Palette
{
	static Color &colot(size_t i) {return _colors[i];}
	static Color _colors[20];
};

struct Shape
{
	Shape() : _visible(true) {}
	void draw() {if (_visible) _draw(_color);}

	bool	_visible;
	Color	_color;

private:
	virtual void _draw(const Color &c) const = 0;
};

class Point : public Shape
{
	void _draw(const Color &c) const override;

public:
	Point();
	Point(int x, int y);
	virtual ~Point() {}

	/** Cartesian coordinates. */
	int _x, _y;
	float _size;
};

class Line : public Shape
{
	void _draw(const Color &c) const override;

public:
	Line();
	Line(Point *a, Point *b);
	virtual ~Line() {}

	Point *_a, *_b;
};

class Polygon : public Shape
{
	Polygon() = delete;

	void _draw(const Color &c) const override;
	size_t	  _nPoints;
	Point	*_points;

public:
	Polygon(size_t n);
	virtual ~Polygon();

	void setPoint(size_t i, int x, int y);
	void setPoint(size_t i, const Point &p);
	size_t pointCount() {return _nPoints;}
	void makeLines();

	bool	  _drawContour;
};

class Triangle : public Polygon
{
public:
	Triangle() : Polygon(3) {}
};

class Rectangle : public Polygon
{
public:
	Rectangle() : Polygon(4) {}
};

class Mark : public Shape
{
	void _draw(const Color &c) const override;
	Point *_a, *_b, *_cpu, *_task;

public:
	Mark();
	virtual ~Mark();

	void setMark(int bin, int cpuId, int taskId, KsGLWidget *w);
};

class Bin
{
public:
	Bin();
	Bin(size_t id);

	void drawLine();
	void drawVal();

	int mod() {return _val._y - _base._y;}

	size_t	_id;
	Color	_color;
	int 	_pidFront, _pidBack;
	Point	_base, _val;
};

class Graph
{
	Graph() = delete;
	size_t	 _size;
	int	 _hMargin;

public:
	Graph(size_t s);
	~Graph();

	size_t size();
	void draw(const QHash<int, KsPlot::Color> &pidColors);

	void setBase(int b);
	void setBinValue(size_t bin, int val);
	void setBinPid(size_t bin, int pidF, int pidB);
	void setBinColor(size_t bin, const Color &c);
	void setBinIdle(size_t bin, bool idle);
	void setBin(size_t bin, int pidF, int pidB, const Color &c);
	void deriveBin(int bin);

	void setHMargin(int hMargin);

	Bin	*_bins;
};

}; // KsPlot

#endif  /* _GLWIDGET_H */
