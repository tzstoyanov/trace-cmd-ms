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

#ifndef _KS_TRACEGRAPH_H
#define _KS_TRACEGRAPH_H

// Qt
#include <QRubberBand>
#include <QtCharts>
using namespace QtCharts;
QT_CHARTS_USE_NAMESPACE

// Kernel Shark 2
#include "KsUtils.h"
#include "KsModel.h"

class KsChartView : public QChartView
{
	Q_OBJECT
public:
	KsChartView(QWidget *parent = 0);
	KsChartView(QChart *chart, QWidget *parent = 0);

	int cpu() {return _cpuId;}

signals:
	void rangeBoundInit(int x, size_t);
	void rangeBoundStretched(int x, size_t);
	void rangeChanged(size_t, size_t);
	void zoomIn();
	void zoomOut();
	void scrollLeft();
	void scrollRight();
	void stopUpdating();
	void resetPointer();
	void select(int pos, bool mark);
	void deselect();
	void found(size_t);

protected:
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);

private:
	QGraphicsLineItem *_pointer;

	void init();
	QPointF mapToValue(QMouseEvent *event);
	bool find(QMouseEvent *event, size_t variance, size_t *row);
	bool findAndSelect(QMouseEvent *event);

	int _posMousePress;

public:
	int		 _cpuId;
	KsGraphModel	*_model;
};


class KsTraceGraph : public QWidget
{
	Q_OBJECT
public:
	KsTraceGraph(QWidget *parent = 0);
	virtual ~KsTraceGraph();

	void loadData(KsDataStore *data);
	void setMarkerSM(KsDualMarkerSM *m);
	void reset();

private slots:
	void rangeBoundInit(int x, size_t);
	void rangeBoundStretched(int x, size_t);
	void rangeChanged(size_t , size_t);
	void zoomIn();
	void zoomOut();
	void scrollLeft();
	void scrollRight();
	void stopUpdating() {_keyPressed = false;}
	void resetPointer();
	void setPointerInfo(size_t);
	void markEntry(size_t);
	void selectReceived(int bin, bool mark);
	void deselectReceived();
	void cpuReDraw(QVector<int>);
	void taskReDraw(QVector<int>);
	void update();

signals:
	void select(int, bool);
	void deselect();

//protected:
	//bool eventFilter(QObject *obj, QEvent *event);

private:
	void resizeEvent(QResizeEvent* event);

	enum class GraphActions {
		ZoomIn,
		ZoomOut,
		ScrollLeft,
		ScrollRight
	};

	void addCpu(int cpu);
	void drawGraphs(QVector<int> cpuMask, QVector<int> taskMask);
	void updateGeom();
	void updateGraphs(GraphActions action);
	void setAxisX();

	QToolBar	_pointerBar;
	QToolBar	_navigationBar;
	QPushButton	_scrollLeftButton;
	QPushButton	_zoomInButton;
	QPushButton	_zoomOutButton;
	QPushButton	_scrollRightButton;

	QLabel	_labelP1, _labelP2,				  // Pointer
		_labelI1, _labelI2, _labelI3, _labelI4, _labelI5; // Process Info

	QScrollArea	_scrollArea;
	QWidget		_drawWindow;

	KsGraphModel		_model;
	QMap<int, KsChartView*> _chartMap;
	QVBoxLayout		_layout;

	KsDualMarkerSM	*_mState;
	KsDataStore 	*_data;
	QVXYModelMapper	*_mapper;
	QRubberBand 	 _rubberBand;
	QPoint 		 _rubberBandOrigin;

	size_t _posMousePress;
	bool   _keyPressed;
	QVector<int> _cpuMask;
	QVector<int> _taskMask;
};

#endif // _KS_TRACEGRAPH_H
