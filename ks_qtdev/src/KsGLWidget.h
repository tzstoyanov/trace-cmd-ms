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

#ifndef _KS_GLWIDGET_H
#define _KS_GLWIDGET_H

// Qt
#include <QRubberBand>

// Kernel Shark 2
#include "KsUtils.h"
#include "KsPlotTools.h"
#include "KsModel.h"

class KsGLWidget : public QOpenGLWidget
{
	Q_OBJECT
public:
	KsGLWidget(QWidget *parent = NULL);

	void loadData(KsDataStore *data);
	void loadColors();
	void setMarkerSM(KsDualMarkerSM *m);
	KsGraphModel *model() {return &_model;}
	
	size_t chartCount() {return _cpuList.count() + _taskList.count();}
	int height() {return (_cpuList.count() + _taskList.count())*(CPU_GRAPH_HEIGHT + _vSpacing) + _vMargin*2;}

signals:
	void found(size_t pos);
	void zoomIn();
	void zoomOut();
	void scrollLeft();
	void scrollRight();
	void stopUpdating();
	void select(size_t pos);
	void updateView(size_t pos, bool mark);
	void deselect();

protected:
	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void paintGL() override;

	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);
	void wheelEvent(QWheelEvent * event);
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);

	void updateGraphs();
	void drawAxisX();
	void drawGraphs(QVector<int> cpuMask, QVector<int> taskMask);
	void addCpu(int cpu);
	void addTask(int pid);

	int posInRange(int x);
	int getCpu(int y);
	int getPid(int y);

	void rangeBoundInit(int x);
	void rangeBoundStretched(int x);
	void rangeChanged(int binMin, int binMax);

	bool find(QMouseEvent *event, int variance, size_t *row);
	bool findAndSelect(QMouseEvent *event);

public:
	QVector<KsPlot::Graph*>		_graphs;
	QVector<KsPlot::Shape*>		_shapes;
	QVector<int>			_cpuList;
	QVector<int>			_taskList;
	QHash<int, KsPlot::Color> 	_pidColors;

	int		_hMargin, _vMargin;
	unsigned int	_vSpacing;

private:
	KsGraphModel	 _model;
	KsDualMarkerSM	*_mState;
	KsDataStore	*_data;

	QRubberBand	_rubberBand;
	QPoint		_rubberBandOrigin;

	size_t		_posMousePress;
	bool		_keyPressed;
};

#endif
