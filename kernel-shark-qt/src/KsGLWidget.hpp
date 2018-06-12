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

// KernelShark
#include "KsUtils.hpp"
#include "KsPlotTools.hpp"
#include "KsModel.hpp"

typedef QList<KsPlot::Shape*> ShapeList;

class KsGLWidget : public QOpenGLWidget
{
	Q_OBJECT

	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void paintGL() override;

public:
	explicit KsGLWidget(QWidget *parent = NULL);

	void loadData(KsDataStore *data);
	void loadColors();
	void setMarkerSM(KsDualMarkerSM *m);
	KsGraphModel *model() {return &_model;}

	int cpuGraphCount()	const;
	int taskGraphCount()	const;
	int graphCount()	const;
	int height()		const;
	int dpr()		const;

	void findGraphIds(const kshark_entry &e, int *graphCpu, int *graphTask);

	QVector<KsPlot::Graph*>	_graphs;
	ShapeList		_shapes;
	QVector<int>		_cpuList;
	QVector<int>		_taskList;
	KsPlot::ColorTable	_pidColors;

	int		_hMargin, _vMargin;
	unsigned int	_vSpacing;

signals:
	void found(size_t pos);
	void notFound(uint64_t ts, int cpu, int pid);
	void zoomIn();
	void zoomOut();
	void scrollLeft();
	void scrollRight();
	void stopUpdating();
	void select(size_t pos);
	void updateView(size_t pos, bool mark);
	void deselect();

protected:
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);
	void wheelEvent(QWheelEvent * event);
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);

private:
	void updateGraphs();
	void drawAxisX();
	void makeGraphs(QVector<int> cpuMask, QVector<int> taskMask);
	KsPlot::Graph *newCpuGraph(int cpu);
	KsPlot::Graph *newTaskGraph(int pid);
	void makePluginShapes(QVector<int> cpuMask, QVector<int> taskMask);

	int posInRange(int x);
	int getCpu(int y);
	int getPid(int y);

	void rangeBoundInit(int x);
	void rangeBoundStretched(int x);
	void rangeChanged(int binMin, int binMax);

	bool find(QMouseEvent *event, int variance, size_t *row);
	bool find(int bin, int cpu, int pid, int variance, size_t *row);
	bool findAndSelect(QMouseEvent *event);

	KsGraphModel	 _model;
	KsDualMarkerSM	*_mState;
	KsDataStore	*_data;

	QRubberBand	_rubberBand;
	QPoint		_rubberBandOrigin;

	size_t		_posMousePress;
	bool		_keyPressed;
	int 		_dpr;
};

#endif
