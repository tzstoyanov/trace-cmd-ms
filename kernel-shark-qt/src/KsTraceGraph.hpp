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

// KernelShark
#include "KsGLWidget.hpp"

class KsGraphScrollArea : public QScrollArea {
public:
	explicit KsGraphScrollArea(QWidget *parent = nullptr)
	: QScrollArea(parent) {}

	void wheelEvent(QWheelEvent *evt) {
		evt->ignore();
	}
};

class KsTraceGraph : public QWidget
{
	Q_OBJECT
public:
	explicit KsTraceGraph(QWidget *parent = nullptr);

	void loadData(KsDataStore *data);
	void setMarkerSM(KsDualMarkerSM *m);
	void reset();
	KsGLWidget *glPtr() {return &_glWindow;}

public slots:
	void addCpuPlot(int);
	void addTaskPlot(int);
	void zoomIn();
	void zoomOut();
	void scrollLeft();
	void scrollRight();
	void stopUpdating();
	void resetPointer(uint64_t ts, int cpu, int pid);
	void setPointerInfo(size_t);
	void markEntry(size_t);
	void cpuReDraw(QVector<int>);
	void taskReDraw(QVector<int>);
	void update(KsDataStore *data);
	void updateGeom();
	void updateGraphLegends();
	void updateTimeLegends();

private:
	void resizeEvent(QResizeEvent* event) override;
	bool eventFilter(QObject* obj, QEvent* evt) override;
	void selfUpdate();

	enum class GraphActions {
		ZoomIn,
		ZoomOut,
		ScrollLeft,
		ScrollRight
	};

	void updateGraphs(GraphActions action);
	void markerReDraw();

	QToolBar	_pointerBar;
	QToolBar	_navigationBar;
	QPushButton	_scrollLeftButton;
	QPushButton	_zoomInButton;
	QPushButton	_zoomOutButton;
	QPushButton	_scrollRightButton;

	QLabel	_labelP1, _labelP2,				  // Pointer
		_labelI1, _labelI2, _labelI3, _labelI4, _labelI5; // Process Info

	KsGraphScrollArea	_scrollArea;
	QWidget			_drawWindow;
	QWidget			_legendWindow;
	QWidget			_legendAxisX;
	QLabel			_labelXMin, _labelXMid, _labelXMax;
	KsGLWidget		_glWindow;

	QVBoxLayout		_layout;

	KsDualMarkerSM	*_mState;
	KsDataStore 	*_data;
	bool		 _keyPressed;
};

#endif // _KS_TRACEGRAPH_H