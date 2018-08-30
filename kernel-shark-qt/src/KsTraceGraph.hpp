/* SPDX-License-Identifier: LGPL-2.1 */

/*
 * Copyright (C) 2017 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
 */

/**
 *  @file    KsTraceGraph.hpp
 *  @brief   KernelShark Trace Graph.
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
