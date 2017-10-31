/*
 * Copyright (C) 2017 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
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

#ifndef KS_TRACEGRAPH_H
#define KS_TRACEGRAPH_H 1

// Qt
#include <QtWidgets>
#include <QRubberBand>
#include <QtCharts>
#include <QDebug>
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
	void found(size_t);

public slots:
	void setPointer(size_t);

protected:
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);

private:
	void init();
	QPointF mapToValue(QMouseEvent *event);
	bool find(QMouseEvent *event, size_t variance, size_t *row);
	void findAndSelect(QMouseEvent *event);

	int 	_posMarkA;

public:
	int		   _cpuId;
	QGraphicsLineItem *_pointer;
	KsGraphModel	  *_model;
};

class KsTraceGraph : public QWidget
{
	Q_OBJECT
public:
	KsTraceGraph(QWidget *parent = 0);
	virtual ~KsTraceGraph();

	void loadData(KsDataStore *data);
	void reset();

private slots:
	void rangeBoundInit(int x, size_t);
	void rangeBoundStretched(int x, size_t);
	void rangeChanged(size_t, size_t);
	void zoomIn();
	void zoomOut();
	void scrollLeft();
	void scrollRight();
	void stopUpdating() {_keyPressed = false;}
	void resetPointer();
	void setPointerInfo(size_t);
	void markEntry(size_t);
	void selectReceived(int bin, bool mark);
	void cpuReDraw(QVector<Qt::CheckState>);

signals:
	void select(int, bool);

//protected:
	//bool eventFilter(QObject *obj, QEvent *event);

private:
	void resizeEvent(QResizeEvent* event);
	void setMarkLabels(size_t mA, size_t  mB);

	enum class GraphActions {
		ZoomIn,
		ZoomOut,
		ScrollLeft,
		ScrollRight
	};

	void addCpu(int cpu);
	void drawGraphs(int nCpus, uint32_t cpuMask=0xffffffff);
	void updateGeom();
	void updateGraphs(GraphActions action);
	void setAxisX();

	QToolBar 	_pointerBar;
	QToolBar 	_navigationBar;
	QPushButton     _scrollLeftButton;
	QPushButton     _zoomInButton;
	QPushButton     _zoomOutButton;
	QPushButton     _scrollRightButton;
	KsMarkerState   _mState;


	QLabel	_labelP1, _labelP2, 				  // Pointer
		_labelI1, _labelI2, _labelI3, _labelI4, _labelI5, // Process Info
		_labelA1, _labelA2,				  // Marker A
		_labelB1, _labelB2,				  // Marker B
		_labelD1, _labelD2;				  // Delta

	QScrollArea _scrollArea;
	QWidget     _drawWindow;

	KsGraphModel		_model;
	QVector<KsChartView*>	_chartView;
	QVBoxLayout		_layout;

	KsDataStore 	*_data;
	QChart 		*_axisChart;
	QCategoryAxis 	*_axisX;
	QVXYModelMapper *_mapper;

	QRubberBand 	_rubberBand;
	QPoint 		_rubberBandOrigin;

	size_t _posMarkA;
	bool _keyPressed;

	KsGraphMark _markA, _markB;
};

#endif // KS_TRACEGRAPH_H



