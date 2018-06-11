/*
 *  Copyright (C) 2017 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
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

// C++11
#include <future>

// OpenGL
#include <GL/glut.h>
#include <GL/gl.h>

// KernelShark
#include "KsGLWidget.hpp"
#include "KsUtils.hpp"
#include "KsPlugins.hpp"

void KsGLWidget::initializeGL()
{
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_COLOR_MATERIAL);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_POLYGON_SMOOTH);
	_dpr = QApplication::desktop()->devicePixelRatio();
	glLineWidth(1.5*_dpr);
	glPointSize(2.5*_dpr);
	glClearColor(1, 1, 1, 1);
}

void KsGLWidget::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, w, h, 0);		// Set origin to top left corner.
					// "Y" coordinate is inverted.
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if(_data)
		updateGraphs();
}

void KsGLWidget::paintGL()
{
// 	hd_time t0 = GET_TIME;

	/* Clear the canvas and prepare for a new drawing. */
	glClear(GL_COLOR_BUFFER_BIT);

	loadColors();

	/* Draw the time axis. */
	if(_data)
		drawAxisX();

	/* Process and draw all graphs by using the built-in logic. */
	makeGraphs(_cpuList, _taskList);
	for (auto const &g: _graphs)
		g->draw(_pidColors, 1.5 * _dpr);

	/* Process and draw all plugin-specific shapes. */
	makePluginShapes(_cpuList, _taskList);
	for (auto const &s: _shapes) {
		s->draw();
		delete s;
	}
	_shapes.clear();

	/*
	 * Update and draw the markers. Make sure that the active marker
	 * is drawn on top.
	 */
	_mState->updateMarkers(*_data, _model.histo());
	_mState->passiveMarker().markPtr()->draw();
	_mState->activeMarker().markPtr()->draw();

// 	double time = GET_DURATION(t0);
// 	qInfo() <<"GL paint time: " << 1e3*time << " ms.";
}

KsGLWidget::KsGLWidget(QWidget *parent)
: QOpenGLWidget(parent),
  _hMargin(20),
  _vMargin(30),
  _vSpacing(20),
  _mState(nullptr),
  _data(nullptr),
  _rubberBand(QRubberBand::Rectangle, this),
  _rubberBandOrigin(0, 0),
  _dpr(1)
{
	setMouseTracking(true);

	/* Using the old Signal-Slot syntax because QWidget::update has overloads. */
	connect(&_model, SIGNAL(modelReset()), this, SLOT(update()));
}

void KsGLWidget::loadData(KsDataStore *data)
{
// 	hd_time t0 = GET_TIME;
	_data = data;

	int nCpus = _data->_pevt->cpus;
	_model.reset();
	_model.setNCpus(nCpus);

	/*
	 * From the size of the widget, calculate the number of bins.
	 * One bin will correspond to one pixel.
	 */
	int nBins = width() - _hMargin * 2;

	/* Now load the entire set of trace data. */
	uint64_t tMin = _data->_rows[0]->ts;
	uint64_t tMax = _data->_rows[_data->size() - 1]->ts;
	ksmodel_set_bining(_model.histo(), nBins, tMin, tMax);
	_model.fill(_data->_pevt, _data->_rows, _data->size());

	/* Make a default Cpu list. All Cpus will be plotted. */
	_cpuList = {};
	for (int i = 0; i < nCpus; ++i)
		_cpuList.append(i);

	/* Make a default task list. No tasks will be plotted. */
	_taskList = {};

	makeGraphs(_cpuList, _taskList);

// 	double time = GET_DURATION(t0);
// 	qInfo() <<"Graph loading time: " << 1e3*time << " ms.";
}

void KsGLWidget::loadColors()
{
	_pidColors.clear();
	QVector<int> pids;
	int n = KsUtils::getPidList(&pids);
	KsPlot::Color col;

	for (int i = 0; i < n; ++i) {
		_pidColors[pids[i]] = col;
		col.setRainbowColor(i);
	}
}

void KsGLWidget::setMarkerSM(KsDualMarkerSM *m)
{
	_mState = m;
}

int KsGLWidget::cpuGraphCount() const
{
	return _cpuList.count();
}

int KsGLWidget::taskGraphCount() const
{
	return _taskList.count();
}

int KsGLWidget::graphCount() const
{
	return _cpuList.count() + _taskList.count();
}

int KsGLWidget::height() const
{
	return graphCount()*(CPU_GRAPH_HEIGHT + _vSpacing) + _vMargin * 2;
}

int KsGLWidget::dpr() const
{
	return _dpr;
}

void KsGLWidget::findGraphIds(const kshark_entry &e, int *graphCpu, int *graphTask)
{
	int graph(0);
	bool cpuFound(false), taskFound(false);

	/*
	 * Loop over all Cpu graphs and try to find the one that
	 * contains the entry.
	 */
	for (auto const &c: _cpuList) {
		if (c == e.cpu) {
			cpuFound = true;
			break;
		}
		++graph;
	}

	if (cpuFound)
		*graphCpu = graph;
	else
		*graphCpu = -1;

	/*
	 * Loop over all Task graphs and try to find the one that
	 * contains the entry.
	 */
	graph = _cpuList.count();
	for (auto const &p: _taskList) {
		if (p == e.pid) {
			taskFound = true;
			break;
		}
		++graph;
	}

	if (taskFound)
		*graphTask = graph;
	else
		*graphTask = -1;
}

void KsGLWidget::updateGraphs()
{
	/*
	 * From the size of the widget, calculate the number of bins.
	 * One bin will correspond to one pixel.
	 */
	int nBins = width() - _hMargin * 2;

	/*
	 * Reload the data. The range of the histogram is the same
	 * but the number of bins changes.
	 */
	ksmodel_set_bining(_model.histo(),
			   nBins,
			   _model.histo()->min,
			   _model.histo()->max);

	_model.fill(_data->_pevt, _data->_rows, _data->size());
}

void KsGLWidget::drawAxisX()
{
	KsPlot::Point a0(_hMargin, _vMargin / 4), a1(_hMargin, _vMargin / 2);
	KsPlot::Point b0(width()/2, _vMargin / 4), b1(width() / 2, _vMargin / 2);
	KsPlot::Point c0(width() - _hMargin, _vMargin / 4),
			 c1(width() - _hMargin, _vMargin / 2);
	KsPlot::Line a(&a0, &a1), b(&b0, &b1), c(&c0, &c1), ac(&a0, &c0);

	a0._size = c0._size = _dpr;
	a._size = b._size = c._size = ac._size = 1.5 * _dpr;

	a0.draw();
	c0.draw();
	a.draw();
	b.draw();
	c.draw();
	ac.draw();
}

void KsGLWidget::makeGraphs(QVector<int> cpuList, QVector<int> taskList)
{
	/* The very first thing to do is to clean up. */
	for (auto &g: _graphs)
		delete g;
	_graphs.resize(0);

	if (!_data || !_data->size())
		return;

	auto add_gpaph = [&] (KsPlot::Graph *graph) {
		/*
		* Calculate the base level of the Cpu graph inside the widget.
		* Remember that the "Y" coordinate is inverted.
		*/
		int base = _vMargin +
			   _vSpacing * _graphs.count() +
			   CPU_GRAPH_HEIGHT*(_graphs.count() + 1);

		graph->setBase(base);
		_graphs.append(graph);
	};

	/* Create Cpu graphs according to the cpuList. */
	for (auto const &cpu: cpuList)
		add_gpaph(newCpuGraph(cpu));


// 	std::vector< std::future<KsPlot::Graph *> > futureGraphs;
// 	for (auto const &cpu: cpuList)
// 		futureGraphs.push_back(std::async(&KsGLWidget::newCpuGraph, this, cpu));
// 
// 	for (auto &f: futureGraphs)
// 		add_gpaph(f.get());

	/* Create Task graphs taskList to the taskList. */
	for (auto const &pid: taskList)
		add_gpaph(newTaskGraph(pid));
}

void KsGLWidget::makePluginShapes(QVector<int> cpuList, QVector<int> taskList)
{
	kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);
	gui_event_handler *evt_handlers;

	KsCppArgV cppArgv;
	cppArgv._histo = _model.histo();
	cppArgv._shapes = &_shapes;

	for (int g = 0; g < cpuList.count(); ++g) {
		cppArgv._graph = _graphs[g];
		evt_handlers = kshark_ctx->event_handlers;
		while (evt_handlers) {
			evt_handlers->draw_func(cppArgv.toC(),
						cpuList[g],
						KSHARK_PLUGIN_CPU_DRAW);

			evt_handlers = evt_handlers->next;
		}
	}

	for (int g = 0; g < taskList.count(); ++g) {
		cppArgv._graph = _graphs[cpuList.count() + g];
		evt_handlers = kshark_ctx->event_handlers;
		while (evt_handlers) {
			evt_handlers->draw_func(cppArgv.toC(),
						taskList[g],
						KSHARK_PLUGIN_TASK_DRAW);

			evt_handlers = evt_handlers->next;
		}
	}
}

KsPlot::Graph *KsGLWidget::newCpuGraph(int cpu)
{
	int nBins = _model.histo()->n_bins;
	KsPlot::Graph *graph = new KsPlot::Graph(nBins);
	graph->setHMargin(_hMargin);
	graph->_histo = _model.histo();

	kshark_context *kshark_ctx = NULL;
	kshark_entry_collection *col;

	kshark_instance(&kshark_ctx);
	col = kshark_find_data_collection(kshark_ctx->collections,
					  kshark_check_cpu_visible,
					  cpu);

	graph->_graphCollection = col;
	graph->fillCpuGraph(cpu, _pidColors);

	return graph;
}

KsPlot::Graph *KsGLWidget::newTaskGraph(int pid)
{
	int nBins = _model.histo()->n_bins;
	KsPlot::Graph *graph = new KsPlot::Graph(nBins);
	graph->setHMargin(_hMargin);
	graph->_histo = _model.histo();

	kshark_context *kshark_ctx = NULL;
	kshark_entry_collection *col;

	kshark_instance(&kshark_ctx);
	col = kshark_find_data_collection(kshark_ctx->collections,
					  kshark_check_pid, pid);
	if (!col) {
		/*
		 * If a data collection for this task does not exist,
		 * register a new one.
		 */
		col = kshark_register_data_collection(kshark_ctx,
						      _data->_rows, _data->size(),
						      kshark_check_pid, pid,
						      25);
	}

	/*
	 * Data collections are efficient only when used on graphs, having a lot
	 * of empty bins.
	 * TODO: Determine the optimal criteria to decide whether to use or not
	 * use data collection for this graph.
	 */
	if (_data->size() < 1e6 &&
	    col && col->size &&
	    _data->size() / col->size < 100) {
		/*
		 * No need to use collection in this case. Free the collection data,
		 * but keep the collection registered. This will prevent from recalculating
		 * the same collection next time when this task is ploted.
		 */
		kshark_reset_data_collection(col);
	}

	graph->_graphCollection = col;
	graph->fillTaskGraph(pid);

	return graph;
}

bool KsGLWidget::find(QMouseEvent *event, int variance, size_t *row)
{
	/*
	 * Get the bin, pid and cpu numbers.
	 * Remember that one bin corresponds to one pixel.
	 */
	int bin = event->pos().x() - _hMargin;

	int cpu = getCpu(event->pos().y());
	int pid = getPid(event->pos().y());

	return find(bin, cpu, pid, variance, row);
}

bool KsGLWidget::find(int bin, int cpu, int pid, int variance, size_t *row)
{
	int hSize = _model.histo()->n_bins;
	if (bin < 0 || bin > hSize || (cpu < 0 && pid < 0)) {
		/*
		 * The click is outside of the range of the histogram.
		 * Do nothing.
		 */
		*row = 0;
		return false;
	}

	auto get_entry_by_cpu = [&] (int b) {
		/* Get the first data entry in this bin. */
		ssize_t found = ksmodel_first_index_at_cpu(_model.histo(), b, cpu);
		if (found < 0) {
			/*
			 * The bin is empty or the entire connect of the bin
			 * has been filtered.
			 */
			return false;
		}

		*row = found;
		return true;
	};

	auto get_entry_by_pid = [&] (int b) {
		/* Get the first data entry in this bin. */
		ssize_t found = ksmodel_first_index_at_pid(_model.histo(), b, pid);
		if (found < 0) {
			/*
			 * The bin is empty or the entire connect of the bin
			 * has been filtered.
			 */
			return false;
		}

		*row = found;
		return true;
	};

	if (cpu >= 0) {
		/* The click is over the Cpu graphs. First try the exact match. */
		if (get_entry_by_cpu(bin))
			return true;

		/* Now look for a match, nearby the position of the click. */
		for (int i = 1; i < variance; ++i) {
			if (bin + i <= hSize && get_entry_by_cpu(bin + i))
				return true;

			if (bin - i >= 0 && get_entry_by_cpu(bin - i))
				return true;
		}
	}

	if (pid >= 0) {
		/* The click is over the Task graphs. First try the exact match. */
		if (get_entry_by_pid(bin))
			return true;

		/* Now look for a match, nearby the position of the click. */
		for (int i = 1; i < variance; ++i) {
			if ((bin + i <= hSize) && get_entry_by_pid(bin + i))
				return true;

			if ((bin - i >= 0) && get_entry_by_pid(bin - i))
				return true;
		}
	}

	*row = 0;
	return false;
}

bool KsGLWidget::findAndSelect(QMouseEvent *event)
{
	size_t row;
	bool found = find(event, 10, &row);

	emit deselect();
	if (found) {
		emit select(row);
		emit updateView(row, true);
	}

	return found;
}

void KsGLWidget::rangeBoundInit(int x)
{
	/*
	 * Set the origin of the rubber band that shows the new range. Only the X
	 * coordinate of the origin matters. The Y coordinate will be set to zero.
	 */
	_rubberBandOrigin.rx() = x;
	_rubberBandOrigin.ry() = 0;

	_rubberBand.setGeometry(_rubberBandOrigin.x(),
				_rubberBandOrigin.y(),
				0, 0);

	/* Make the rubber band visible, although its size is zero. */
	_rubberBand.show();
}

void KsGLWidget::rangeBoundStretched(int x)
{
	QPoint pos;
	pos.rx() = x;
	pos.ry() = this->height();

	/*
	 * Stretch the rubber band between the origin position and the current
	 * position of the mouse. Only the X coordinate matters. The Y coordinate
	 * will be the height of the widget.
	 */
	if (_rubberBandOrigin.x() < pos.x()) {
		_rubberBand.setGeometry(QRect(_rubberBandOrigin.x(),
					      _rubberBandOrigin.y(),
					      pos.x() - _rubberBandOrigin.x(),
					      pos.y() - _rubberBandOrigin.y()));
	} else {
		_rubberBand.setGeometry(QRect(pos.x(),
					      _rubberBandOrigin.y(),
					      _rubberBandOrigin.x() - pos.x(),
					      pos.y() - _rubberBandOrigin.y()));
	}
}

void KsGLWidget::rangeChanged(int binMin, int binMax)
{
	/* The rubber band is no longer needed. Make it invisible. */
	_rubberBand.hide();

	if ( (binMax - binMin) < 4) {
		/* Most likely this is an accidental click. Do nothing. */
		return;
	}

	/*
	 * Calculate the new range of the histogram. The number of bins will
	 * stay the same.
	 */
	uint64_t min, max;
	size_t nBins = _model.histo()->n_bins;
	int binMark = _mState->activeMarker().bin();

	min = ksmodel_ts(_model.histo(), binMin);
	max = ksmodel_ts(_model.histo(), binMax);
	if (max - min < nBins) {
		/* The range cannot be smaller than the number of bins.
		 * Do nothing. */
		return;
	}

	/* Recalculate the model and update the markers. */
	ksmodel_set_bining(_model.histo(), nBins, min, max);
	_model.fill(_data->_pevt, _data->_rows, _data->size());
	_mState->updateMarkers(*_data, _model.histo());

	/*
	 * If the Marker is inside the new range, make sure that it will
	 * be visible in the table. Note that for this check we use the
	 * bin number of the marker, retrieved before its update.
	 */
	if (_mState->activeMarker().isSet() &&
	    binMark < binMax && binMark > binMin) {
		emit updateView(_mState->activeMarker().row(), true);
		return;
	}

	/*
	 * Find the first bin which contains unfiltered data and send a signal
	 * to the View widget to make this data visible.
	 */
	for (size_t i = 0; i < _model.histo()->n_bins; ++i) {
		int64_t row = ksmodel_first_index_at(_model.histo(), i);
		if (row != KS_EMPTY_BIN &&
		    (_data->_rows[row]->visible & KS_VIEW_FILTER_MASK)) {
			emit updateView(ksmodel_first_index_at(_model.histo(), i), false);
			return;
		}
	}
}

int KsGLWidget::posInRange(int x)
{
	int posX;
	if (x < _hMargin)
		posX = _hMargin;
	else if (x > (width() - _hMargin))
		posX = width() - _hMargin;
	else
		posX = x;

	return posX;
}

int KsGLWidget::getCpu(int y)
{
	if (_cpuList.count() == 0)
		return -1;

	int cpuId = (y - _vMargin + _vSpacing / 2) / (_vSpacing + CPU_GRAPH_HEIGHT);
	if (cpuId < 0 || cpuId >= _cpuList.count())
		return -1;

	return _cpuList[cpuId];
}

int KsGLWidget::getPid(int y)
{
	if (_taskList.count() == 0)
		return -1;

	int pidId = (y - _vMargin -
			 _cpuList.count()*(CPU_GRAPH_HEIGHT + _vSpacing) +
			 _vSpacing / 2) / (_vSpacing + CPU_GRAPH_HEIGHT);

	if (pidId < 0 || pidId >= _taskList.count())
		return -1;

	return _taskList[pidId];
}

void KsGLWidget::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		_posMousePress = posInRange(event->pos().x());
		rangeBoundInit(_posMousePress);
	} else if (event->button() == Qt::RightButton) {
		emit deselect();
		_mState->activeMarker().remove();
		_mState->updateLabels(_model.histo());
		_model.update();
	}
}

void KsGLWidget::mouseMoveEvent(QMouseEvent *event)
{
	if (_rubberBand.isVisible())
		rangeBoundStretched(posInRange(event->pos().x()));

	size_t row;
	int bin = event->pos().x() - _hMargin;
	int cpu = getCpu(event->pos().y());
	int pid = getPid(event->pos().y());

	bool status = find(bin, cpu, pid, 5, &row);
	if (status) {
		emit found(row);
	} else {
		emit notFound(ksmodel_ts(_model.histo(), bin), cpu, pid);
	}
}

void KsGLWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		size_t posMouseRel = posInRange(event->pos().x());
		int min, max;
		if (_posMousePress < posMouseRel) {
			min = _posMousePress - _hMargin;
			max = posMouseRel - _hMargin;
		} else {
			max = _posMousePress - _hMargin;
			min = posMouseRel - _hMargin;
		}

		rangeChanged(min, max);
	}
}

void KsGLWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
		findAndSelect(event);
}

void KsGLWidget::wheelEvent(QWheelEvent * event)
{
	int zoomFocus = _model.histo()->n_bins / 2;
	if (_mState->activeMarker().isSet() &&
	    _mState->activeMarker().isVisible())
		zoomFocus = _mState->activeMarker().bin();

	if (event->delta() > 0) {
		_model.zoomIn(.05, zoomFocus);
	} else {
		_model.zoomOut(.05);
	}

	_mState->updateMarkers(*_data, _model.histo());
}

void KsGLWidget::keyPressEvent(QKeyEvent *event)
{
	if (event->isAutoRepeat())
		return;

	switch (event->key()) {
	case Qt::Key_Plus:
		emit zoomIn();
		return;

	case Qt::Key_Minus:
		emit zoomOut();
		return;

	case Qt::Key_Left:
		emit scrollLeft();
		return;

	case Qt::Key_Right:
		emit scrollRight();
		return;

	default:
		QOpenGLWidget::keyPressEvent(event);
		return;
	}
}

void KsGLWidget::keyReleaseEvent(QKeyEvent *event)
{
	if (event->isAutoRepeat())
		return;

	if(event->key() == Qt::Key_Plus ||
	   event->key() == Qt::Key_Minus ||
	   event->key() == Qt::Key_Left ||
	   event->key() == Qt::Key_Right) {
		emit stopUpdating();
		return;
	}

	QOpenGLWidget::keyPressEvent(event);
	return;
}
