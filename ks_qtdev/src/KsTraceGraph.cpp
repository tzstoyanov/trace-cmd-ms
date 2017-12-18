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

// OpenGL
#include <GL/glut.h>
#include <GL/gl.h>

// Kernel Shark 2
#include "KsTraceGraph.h"
#include "KsUtils.h"

KsGLWidget::KsGLWidget(QWidget *parent)
: QOpenGLWidget(parent),
  _hMargin(20),
  _vMargin(30),
  _vSpacing(20),
  _mState(nullptr),
  _data(nullptr),
  _model(),
  _rubberBand(QRubberBand::Rectangle, this),
  _rubberBandOrigin(0, 0)
{
// 	printf("KsGLWidget \n");
	setMouseTracking(true);
// 	grabKeyboard();
	connect(&_model, SIGNAL(modelReset()), this, SLOT(update()));
}

void KsGLWidget::initializeGL()
{
// 	printf("initializeGL \n");
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_COLOR_MATERIAL);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_POLYGON_SMOOTH);
	glLineWidth(1.5);
	glPointSize(2.0);
	glClearColor(1, 1, 1, 1);
}

void KsGLWidget::resizeGL(int w, int h)
{
// 	printf("\n\nresizeGL w: %i  h: %i \n", w, h);
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, w, h, 0); // Set origin to top left corner. "Y" coordinate is inverted.
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if(_data)
		updateGraphs();
}

void KsGLWidget::paintGL()
{
// 	printf("paintGL \n");
// 	hd_time t0 = GET_TIME;
	
	glClear(GL_COLOR_BUFFER_BIT);

	drawGraphs(_cpuList, _taskList);
	for (auto const &g: _graphs)
		g->draw();

	for (auto const &s: _shapes)
		s->draw();

	_mState->updateMarkers(*_data, _model.histo());

// 	double time = GET_DURATION(t0);
// 	qInfo() <<"GL paint time: " << 1e3*time << " ms.";
}

void KsGLWidget::loadData(KsDataStore *data)
{
// 	hd_time t0 = GET_TIME;
	_data = data;

	int nCpus = _data->_pevt->cpus;
	_model.reset();
	_model.setNCpus(nCpus);

	/** From the size of the widget, calculate the number of bins.
	 *  One bin will correspond to one pixel. */
	int nBins = width() - _hMargin*2;

	/* Now load the entire set of trace data. */
	uint64_t tMin = _data->_rows[0]->ts;
	uint64_t tMax = _data->_rows[_data->size() - 1]->ts;
	_model.histoPtr()->setBining(nBins, tMin, tMax);
	_model.fill(_data->_pevt, _data->_rows, _data->size());

	/* Make a default Cpu list. All Cpus will be plotted. */
	_cpuList = {};
	for (int i = 0; i < nCpus; ++i)
		_cpuList.append(i);

	/* Make a default task list. No tasks will be plotted. */
	_taskList = {};

	drawGraphs(_cpuList, _taskList);

// 	double time = GET_DURATION(t0);
// 	qInfo() <<"Graph loading time: " << 1e3*time << " ms.";
}

void KsGLWidget::setMarkerSM(KsDualMarkerSM *m)
{
	_mState = m;
	_shapes.append(m->markerA().markPtr());
	_shapes.append(m->markerB().markPtr());
}

void KsGLWidget::updateGraphs()
{
	/** From the size of the widget, calculate the number of bins.
	 *  One bin will correspond to one pixel. */
	int nBins = width() - _hMargin*2;

	/* Reload the data. The range of the histogram is the same
	 * but the number of bins changes. */
	_model.histoPtr()->setBining(nBins,
				     _model.histo()._min,
				     _model.histo()._max);

	_model.fill(_data->_pevt, _data->_rows, _data->size());
}

void KsGLWidget::drawAxisX() {
	KsPlot::Point a0(_hMargin, _vMargin/4), a1(_hMargin, _vMargin/2);
	KsPlot::Point b0(width()/2, _vMargin/4), b1(width()/2, _vMargin/2);
	KsPlot::Point c0(width() - _hMargin, _vMargin/4), c1(width() - _hMargin, _vMargin/2);
	KsPlot::Line a(&a0, &a1), b(&b0, &b1), c(&c0, &c1), ac(&a0, &c0);

	a0.draw();
	c0.draw();
	a.draw();
	b.draw();
	c.draw();
	ac.draw();
}

void KsGLWidget::drawGraphs(QVector<int> cpuList, QVector<int> taskList)
{
	/* The very first thing to do is to clean up. */
	for (auto &g: _graphs)
		delete g;
	_graphs.resize(0);

	drawAxisX();
	
	/* Create Cpu graphs according to the cpuList. */
	for (auto const &cpu: cpuList)
		addCpu(cpu);

	/* Create Task graphs taskList to the taskList. */
	for (auto const &pid: taskList)
		addTask(pid);
}

void KsGLWidget::addCpu(int cpu)
{
	int pid;
	int nBins = _model.histo().size();
	KsPlot::Graph *graph = new KsPlot::Graph(nBins);
	graph->setHMargin(_hMargin);

	/* Calculate the base level of the Cpu graph inside the widget.
	 * Remember that the "Y" coordinate is inverted. */
	int base = _vMargin + _vSpacing*_graphs.count();
	base += CPU_GRAPH_HEIGHT*(_graphs.count() + 1);
	graph->setBase(base);

	auto set_bin = [&] (int b) {
		if (pid != KS_EMPTY_BIN) {
			// This is a regular process.
			graph->setBin(b, pid, {pid*0xae0d});
		} else {
			/* The bin contens no data from this Cpu.
			 * Derive the Process Id from the previous bin. */
			graph->setBinPid(b, pid);
		}
	};
	
	/* Check the content of the very firs bin and see if the Cpu is active. */
	int b = 0;
	pid = _model.histo().getPid(b, cpu, true);
	if (pid >= 0) {
		/* The Cpu is active. Set this bin. */
		set_bin(b);
	} else {
		/* No data from this Cpu in the very firs bin. Use the Lower
		 * Overflow Bin to retrieve the Process Id (if any). */
		pid = _model.histo().getPid(KsTimeMap::OverflowBin::Lower, cpu, true);
		graph->setBinPid(b, pid);
	}

	for (b = 1; b < nBins; ++b) {
		/* Check the content of this bin and see if the Cpu is active.
		 * If yes, retrieve the Process Id. If not, derive from the
		 * previous bin. */
		pid = _model.histo().getPid(b, cpu, true);
		set_bin(b);
	}

	_graphs.append(graph);
}

void KsGLWidget::addTask(int pid)
{
	int cpu, lastCpu = -1;
	int nBins = _model.histo().size();
	KsPlot::Graph *graph = new KsPlot::Graph(nBins);
	graph->setHMargin(_hMargin);

	/* Calculate the base level of the Cpu graph inside the widget.
	 * Remember that the "Y" coordinate is inverted. */
	int base = _vMargin + _vSpacing*_graphs.count();
	base += CPU_GRAPH_HEIGHT*(_graphs.count() + 1);
	graph->setBase(base);

	auto set_bin = [&] (int b) {
		if (cpu >= 0) {
			graph->setBin(b, pid, {(cpu + 1)*0xae0d});
			lastCpu = cpu;
		} else {
			/* No data from thе Task in this bin. Check the Cpu, previously used
			 * by the task. If it is active and works on another task break the
			 * graph here. */
			int cpuPid = _model.histo().getPid(b, lastCpu, false);
			if (cpuPid == KS_EMPTY_BIN)
				graph->setBinPid(b, KS_EMPTY_BIN);
			else
				graph->setBinPid(b, KS_FILTERED_BIN);
		}
	};

	/* Check the content of the very firs bin and see if the Task is active. */
	int b = 0;
	cpu = _model.histo().getCpu(b, pid, true);
	if (cpu >= 0) {
		/* The Task is active. Set this bin. */
		set_bin(b);
	} else {
		/* No data from this Task in the very firs bin. Use the Lower
		 * Overflow Bin to retrieve the Cpu used by the task (if any). */
		cpu = _model.histo().getCpu(KsTimeMap::OverflowBin::Lower, pid, true);
		if (cpu >= 0) {
			/* The Lower Overflow Bin contains data from this Task. Now 
			 * look again in the Lower Overflow Bin and find the Pid of the
			 * last active task on the same Cpu. */
			int pidCpu = _model.histo().getPid(KsTimeMap::OverflowBin::Lower,
							   cpu, false);
			if (pidCpu == pid) {
				graph->setBinPid(b, pid);
				lastCpu = cpu;
			}
		}
	}

	for (int b = 1; b < nBins; ++b) {
		cpu = _model.histo().getCpu(b, pid, false);
		set_bin(b);
	}

	_graphs.append(graph);
}

bool KsGLWidget::find(QMouseEvent *event, int variance, size_t *row)
{
	/* Get the bin and cpu numbers. Remember that one bin corresponds
	 * to one pixel. */
	int bin = event->pos().x() - _hMargin;
	int cpu = getCpu(event->pos().y());
	int pid = getPid(event->pos().y());
	int hSize = _model.histo().size();
	if (bin < 0 || bin > hSize || (cpu < 0 && pid < 0)) {
		/* The click is outside of the range of the histogram.
		 * Do nothing. */
		*row = 0;
		return false;
	}

	auto get_entry_by_cpu = [&] (int b) {
		/* Get the first data entry in this bin. */
		int64_t found = _model.histo().atCpu(b, cpu);
		if (found < 0) {
			/* The bin is empty or the entire connect of the bin
			 * has been filtered. */
			return false;
		}
		*row = found;
		return true;
	};

	auto get_entry_by_pid = [&] (int b) {
		/* Get the first data entry in this bin. */
		int64_t found = _model.histo().atPid(b, pid);
		if (found < 0) {
			/* The bin is empty or the entire connect of the bin
			 * has been filtered. */
			return false;
		}
		*row = found;
		return true;
	};

	if (cpu >= 0) {
		/* First try the exact match. */
		if (get_entry_by_cpu(bin))
			return true;

		/* Now look for a match, nearby the position of the click. */
		for (int i = 1; i < variance; ++i) {
			if (bin + i <= hSize &&
			    get_entry_by_cpu(bin + i))
				return true;

			if (bin - i >= 0 &&
			    get_entry_by_cpu(bin - i))
				return true;
		}
	} 
	
	if (pid >= 0) {
		/* First try the exact match. */
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
	emit deselect();
	size_t row;
	bool found = find(event, 10, &row);
	if (found) {
		emit select(row);
		emit updateView(row, true);
	}
	return found;
}

void KsGLWidget::rangeBoundInit(int x)
{
	/* Set the origin of the rubber band that shows the new range. Only the X
	 * coordinate of the origin matters. The Y coordinate will be set to zero. */
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
	if(!_rubberBand.isVisible())
		return;

	QPoint pos;
	pos.rx() = x;
	pos.ry() = this->height();

	/* Stretch the rubber band between the origin position and the current position
	 * of the mouse. The Y coordinate will be the height of the widget. */
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
		/* Most likely this is an accidental click.
		 * Do nothing. */
		return;
	}

	/* Calculate the new range of the histogram. The number of bins will
	 * stay the same. */
	uint64_t min, max;
	size_t nBins = _model.histo().size();
	int binMark = _mState->activeMarker().bin();

	min = _model.histo().ts(binMin);
	max = _model.histo().ts(binMax);
	if (max - min < nBins) {
		/* The range cannot be smaller than the number of bins.
		 * Do nothing. */
		return;
	}

	/* Recalculate the model and update the markers. */
	_model.histoPtr()->setBining(nBins, min, max);
	_model.fill(_data->_pevt, _data->_rows, _data->size());
	_mState->updateMarkers(*_data, _model.histo());

	/* If the Marker is inside the new range, make sure that it will
	 * be visible. Note that for this check we use the bin number of
	 * the marker, retrieved before its update. */
	if (_mState->activeMarker().isSet() &&
	    binMark < binMax && binMark > binMin) {
		emit updateView(_mState->activeMarker().row(), true);
		return;
	}

	/* Find the first bin which contains data and send a signal
	 * to the View widget to make this data visible. */
	for (size_t i = 0; i < _model.histo().size(); ++i) {
		if (_model.histo()[i] != KS_EMPTY_BIN) {
			emit updateView(_model.histo()[i], false);
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
	int cpuId = (y - _vMargin + _vSpacing/2)/(_vSpacing + CPU_GRAPH_HEIGHT);

	if (cpuId < 0 || cpuId > _cpuList.count())
		return -1;

	return _cpuList[cpuId];
}

int KsGLWidget::getPid(int y)
{
	int pidId = (y - _vMargin -
			 _cpuList.count()*(CPU_GRAPH_HEIGHT + _vSpacing) +
			 _vSpacing/2)/(_vSpacing + CPU_GRAPH_HEIGHT);

	if (pidId < 0 || pidId > _taskList.count())
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
	size_t row;
	bool status = find(event, 5, &row);
	if (status)
		emit found(row);

	rangeBoundStretched(posInRange(event->pos().x()));
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

		emit rangeChanged(min, max);
	}
}

void KsGLWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		findAndSelect(event);
	}
}

void KsGLWidget::wheelEvent(QWheelEvent * event)
{
	int zoomFocus = _model.histo().size()/2;
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

KsTraceGraph::KsTraceGraph(QWidget *parent)
: QWidget(parent),
  _pointerBar(this),
  _navigationBar(this),
  _scrollLeftButton("<", this),
  _zoomInButton("+", this),
  _zoomOutButton("-", this),
  _scrollRightButton(">", this),
  _labelP1("Pointer: ", this),
  _labelP2("", this),
  _labelI1("", this),
  _labelI2("", this),
  _labelI3("", this),
  _labelI4("", this),
  _labelI5("", this),
  _scrollArea(this),
  _drawWindow(&_scrollArea),
  _legendWindow(&_drawWindow),
  _legendAxisX(&_drawWindow),
  _labelXMin("", &_legendAxisX),
  _labelXMid("", &_legendAxisX),
  _labelXMax("", &_legendAxisX),
  _glWindow(&_drawWindow),
  _mState(nullptr),
  _data(nullptr),
  _keyPressed(false)
{
	auto makeNavButton = [&](QPushButton *b) {
		b->setMaximumWidth(FONT_WIDTH*5);

		connect(b, SIGNAL(released()), this, SLOT(stopUpdating()));
		_navigationBar.addWidget(b);
	};
	_pointerBar.setOrientation(Qt::Horizontal);
	_pointerBar.addWidget(&_labelP1);
	_labelP2.setFrameStyle(QFrame::Panel | QFrame::Sunken);
	_labelP2.setStyleSheet("QLabel { background-color : white;}");
	_labelP2.setTextInteractionFlags(Qt::TextSelectableByMouse);
	_labelP2.setFixedWidth(FONT_WIDTH*16);
	_pointerBar.addWidget(&_labelP2);
	_pointerBar.addSeparator();

	_labelI1.setStyleSheet("QLabel {color : blue;}");
	_labelI2.setStyleSheet("QLabel {color : green;}");
	_labelI3.setStyleSheet("QLabel {color : red;}");
	_labelI4.setStyleSheet("QLabel {color : blue;}");
	_labelI5.setStyleSheet("QLabel {color : green;}");

	_pointerBar.addWidget(&_labelI1);
	_pointerBar.addSeparator();
	_pointerBar.addWidget(&_labelI2);
	_pointerBar.addSeparator();
	_pointerBar.addWidget(&_labelI3);
	_pointerBar.addSeparator();
	_pointerBar.addWidget(&_labelI4);
	_pointerBar.addSeparator();
	_pointerBar.addWidget(&_labelI5);

	_legendAxisX.setFixedHeight(FONT_HEIGHT*1.5);
	_legendAxisX.setLayout(new QHBoxLayout);
	_legendAxisX.layout()->setSpacing(0);
	_legendAxisX.layout()->setContentsMargins(0, 0, FONT_WIDTH, 0);

	_labelXMin.setAlignment(Qt::AlignLeft);
	_labelXMid.setAlignment(Qt::AlignHCenter);
	_labelXMax.setAlignment(Qt::AlignRight);

	_legendAxisX.layout()->addWidget(&_labelXMin);
	_legendAxisX.layout()->addWidget(&_labelXMid);
	_legendAxisX.layout()->addWidget(&_labelXMax);
	_drawWindow.setMinimumSize(100, 100);
	_drawWindow.setStyleSheet("QWidget {background-color : white;}");
	QGridLayout *drawLayout = new QGridLayout;
	_drawWindow.setLayout(drawLayout);
	_drawWindow.installEventFilter(this);

	drawLayout->setContentsMargins(0, 0, 0, 0);
	drawLayout->setSpacing(0);
	drawLayout->addWidget(&_legendAxisX, 0, 1);
	drawLayout->addWidget(&_legendWindow, 1, 0);
	drawLayout->addWidget(&_glWindow, 1, 1);

	connect(&_glWindow, SIGNAL(select(size_t)), this, SLOT(markEntry(size_t)));
	connect(&_glWindow, SIGNAL(found(size_t)), this, SLOT(setPointerInfo(size_t)));
	connect(&_glWindow, SIGNAL(zoomIn()), this, SLOT(zoomIn()));
	connect(&_glWindow, SIGNAL(zoomOut()), this, SLOT(zoomOut()));
	connect(&_glWindow, SIGNAL(scrollLeft()), this, SLOT(scrollLeft()));
	connect(&_glWindow, SIGNAL(scrollRight()), this, SLOT(scrollRight()));
	connect(&_glWindow, SIGNAL(stopUpdating()), this, SLOT(stopUpdating()));

	connect(&_glWindow._model, SIGNAL(modelReset()), this, SLOT(updateTimeLegends()));

	_scrollArea.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	_scrollArea.setWidget(&_drawWindow);

	_navigationBar.setMaximumHeight(FONT_HEIGHT*1.75);
	_navigationBar.setOrientation(Qt::Horizontal);

	connect(&_scrollLeftButton, SIGNAL(pressed()), this, SLOT(scrollLeft()));
	makeNavButton(&_scrollLeftButton);

	connect(&_zoomInButton, SIGNAL(pressed()), this, SLOT(zoomIn()));
	makeNavButton(&_zoomInButton);

	connect(&_zoomOutButton, SIGNAL(pressed()), this, SLOT(zoomOut()));
	makeNavButton(&_zoomOutButton);

	connect(&_scrollRightButton, SIGNAL(pressed()), this, SLOT(scrollRight()));
	makeNavButton(&_scrollRightButton);	

	_layout.addWidget(&_pointerBar);
	_layout.addWidget(&_navigationBar);
	_layout.addWidget(&_scrollArea);
	this->setLayout(&_layout);
	updateGeom();
}

bool KsTraceGraph::eventFilter(QObject* obj, QEvent* evt)
{
	if (obj == &_drawWindow && evt->type() == QEvent::Enter) {
		_glWindow.grabKeyboard();
	}

	if (obj == &_drawWindow && evt->type() == QEvent::Leave) {
		_glWindow.releaseKeyboard();
	}

	return QWidget::eventFilter(obj, evt);
}

void KsTraceGraph::updateTimeLegends()
{
	uint64_t sec, usec;
	kshark_convert_nano(_glWindow._model.histo()._min, &sec, &usec);
	QString tMin;
	tMin.sprintf("%llu.%llu", (unsigned long long)sec, (unsigned long long)usec);
	_labelXMin.setText(tMin);

	uint64_t tsMid = (_glWindow._model.histo()._min + _glWindow._model.histo()._max)/2;
	kshark_convert_nano(tsMid, &sec, &usec);
	QString tMid;
	tMid.sprintf("%llu.%llu", (unsigned long long)sec, (unsigned long long)usec);
	_labelXMid.setText(tMid);

	kshark_convert_nano(_glWindow._model.histo()._max, &sec, &usec);
	QString tMax;
	tMax.sprintf("%llu.%llu", (unsigned long long)sec, (unsigned long long)usec);
	_labelXMax.setText(tMax);
}

void KsTraceGraph::updateGraphLegends()
{
	if (_legendWindow.layout()) {
		QLayoutItem *child;
		while ((child = _legendWindow.layout()->takeAt(0)) != 0) {
			delete child->widget();
			delete child;
		}
		delete _legendWindow.layout();
	}

	QString graphLegends;
	QString graphName;
	int width = 0;

	QVBoxLayout *layout = new QVBoxLayout;
	layout->setContentsMargins(FONT_WIDTH, 0, 0, 0);
	layout->setSpacing(_glWindow._vSpacing);
	layout->setAlignment(Qt::AlignTop);
	layout->addSpacing(_glWindow._vMargin);

	auto make_name = [&] {
		if (width < STRING_WIDTH(graphName))
			width = STRING_WIDTH(graphName);

		QLabel *name = new QLabel(graphName);
		name->setAlignment(Qt::AlignBottom);
		name->setStyleSheet("QLabel {background-color : white;}");
		name->setFixedHeight(CPU_GRAPH_HEIGHT);
		layout->addWidget(name);
	};
	
	for (auto const &cpu: _glWindow._cpuList) {
		graphName = QString("cpu %1").arg(cpu);
		make_name();
	}

	for (auto const &pid: _glWindow._taskList) {
		graphName = QString(pevent_data_comm_from_pid(_data->_pevt, pid));
		graphName.append(QString("-%1").arg(pid));
		make_name();
	}

	_legendWindow.setLayout(layout);
	_legendWindow.setMaximumWidth(width + FONT_WIDTH);
}

void KsTraceGraph::updateGeom()
{
	/* Set the minimum height of the Graph widget. */
	int hMin = _drawWindow.height() +
		   _pointerBar.height() +
		   _navigationBar.height() +
		   _layout.contentsMargins().top() +
		   _layout.contentsMargins().bottom();

	if (hMin > SCREEN_HEIGHT/5) hMin = SCREEN_HEIGHT/5;

	setMinimumHeight(hMin);

	/* Set the size of the Scroll Area. */
	int saWidth = width() - _layout.contentsMargins().left() -
				_layout.contentsMargins().right();

	int saHeight = height() - _pointerBar.height() -
				  _navigationBar.height() -
				  _layout.spacing()*2 -
				  _layout.contentsMargins().top() -
				  _layout.contentsMargins().bottom();
		
	_scrollArea.resize(saWidth, saHeight);

	/* Calculate the width of the Draw Window, taking into account the size
	 * of the scroll bar. */
	int dwWidth = _scrollArea.width();
	if (_glWindow.height() > _scrollArea.height())
		dwWidth -= qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);

	/* Set the height of the Draw Window according to the number of
	 * plotted graphs. */
// 	if (_glWindow._graphs.count())
		_drawWindow.resize(dwWidth, _glWindow.height() + _legendAxisX.height());

	/* Now use the height of the Draw Window to fix the maximum height
	 * of the Graph widget. */
	setMaximumHeight(_drawWindow.height() +
			 _pointerBar.height() +
			 _navigationBar.height() +
			 _layout.spacing()*2 +
			 _layout.contentsMargins().top() +
			 _layout.contentsMargins().bottom() +
			 2);	// Just a little bit of extra space. This will allow
				// the scroll bar to disappear when extended to maximum.
}

void KsTraceGraph::resizeEvent(QResizeEvent* event)
{
	updateGeom();
}

void KsTraceGraph::reset()
{
	_glWindow._cpuList = {};
	_glWindow._taskList = {};
	update();
}

void KsTraceGraph::loadData(KsDataStore *data)
{
	_data = data;
	_glWindow.loadData(data);
	updateGraphLegends();
	updateGeom();
}

void KsTraceGraph::setMarkerSM(KsDualMarkerSM *m)
{
	_mState = m;
	_navigationBar.addSeparator();
	_mState->placeInToolBar(&_navigationBar);
	_glWindow.setMarkerSM(m);
}

void KsTraceGraph::cpuReDraw(QVector<int> v)
{
	_mState->reset();
	_glWindow._cpuList = v;
	update();
}

void KsTraceGraph::taskReDraw(QVector<int> v)
{
	_mState->reset();
	_glWindow._taskList = v;
	update();
}

void KsTraceGraph::zoomIn()
{
	updateGraphs(GraphActions::ZoomIn);
}

void KsTraceGraph::zoomOut()
{
	updateGraphs(GraphActions::ZoomOut);
}

void KsTraceGraph::scrollLeft()
{
	updateGraphs(GraphActions::ScrollLeft);
}

void KsTraceGraph::scrollRight()
{
	updateGraphs(GraphActions::ScrollRight);
}

void KsTraceGraph::resetPointer()
{
	_mState->activeMarker().remove();
}

void KsTraceGraph::updateGraphs(GraphActions action)
{
	/* Set the "Key Pressed" flag. The flag will stay set as long as the user
	 * keeps the corresponding action button pressed. */
	_keyPressed = true;

	/* Initialize the zooming factor with a small value. */
	double k = .01;
	while (_keyPressed) {
		switch (action) {
		case GraphActions::ZoomIn:
			if (_mState->activeMarker().isSet() &&
			    _mState->activeMarker().isVisible()) {
				/* Use the position of the active marker as a focus point
				 * for the zoom. */ 
				_glWindow._model.zoomIn(k, _mState->activeMarker().bin());
			} else {
				/* The default focus point is the center of the range
				 * interval of the model. */
				_glWindow._model.zoomIn(k);
			}
			break;

		case GraphActions::ZoomOut:
			_glWindow._model.zoomOut(k);
			break;

		case GraphActions::ScrollLeft:
			_glWindow._model.shiftBackward(10);
			break;

		case GraphActions::ScrollRight:
			_glWindow._model.shiftForward(10);
			break;
		}

		/* As long as the action button is pressed, the zooming factor will grow
		 * smoothly until it reaches a maximum value. This will have a visible
		 * effect of an accelerating zoom. */
		if (k < .25)
			k *= 1.02;

		_mState->updateMarkers(*_data, _glWindow._model.histo());
		QCoreApplication::processEvents();
	}
}

void KsTraceGraph::setPointerInfo(size_t i)
{
	kshark_entry *e = _data->_rows[i];
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	pevent_record *record = tracecmd_read_at(kshark_ctx->handle,
						 e->offset,
						 NULL);
	uint64_t sec, usec;
	kshark_convert_nano(e->ts, &sec, &usec);
	QString pointer;
	pointer.sprintf("%llu.%llu", (unsigned long long)sec, (unsigned long long)usec);
	_labelP2.setText(pointer);

	QString comm(kshark_get_task(kshark_ctx->pevt, e));
	comm.append("-");
	comm.append(QString("%1").arg(e->pid));
	_labelI1.setText(comm);

	_labelI2.setText(QString("%1").arg(e->cpu));

	QString lat(kshark_get_latency(kshark_ctx->pevt, record));
	_labelI3.setText(lat);

	QString event(kshark_get_event_name(kshark_ctx->pevt, e));
	_labelI4.setText(event);

	QString info(kshark_get_info(kshark_ctx->pevt, record, e));
	_labelI5.setText(info);

	free_record(record);
}

void KsTraceGraph::markEntry(size_t pos)
{
	int cpu = _data->_rows[pos]->cpu;
	int pid = _data->_rows[pos]->pid;
	int graph = 0, graphCpu = -1, graphTask = -1;
	bool found = false;

	for (auto const &c: _glWindow._cpuList) {
		if (c == cpu) {
			found = true;
			break;
		}
		++graph;
	}

	
	if (found)
		graphCpu = graph;
	
	graph = 0;
	found = false;

	for (auto const &p: _glWindow._taskList) {
		if (p == pid) {
			found = true;
			break;
		}
		++graph;
	}

	if (found)
		graphTask = graph + _glWindow._cpuList.count();
	else
		graph = graphCpu;

	_scrollArea.ensureVisible(0,
				  _glWindow._vMargin + CPU_GRAPH_HEIGHT/2 +
				  graph*(CPU_GRAPH_HEIGHT + _glWindow._vSpacing),
				  50,
				  CPU_GRAPH_HEIGHT/2 + _glWindow._vSpacing/2);

	_glWindow._model.shiftTo(_data->_rows[pos]->ts);
	_mState->activeMarker().set(*_data, _glWindow._model.histo(), pos, graphCpu, graphTask);
	_mState->updateLabels(_glWindow._model.histo());
	_mState->updateMarkers(*_data, _glWindow._model.histo());
}

void KsTraceGraph::update()
{
	_glWindow._model.update();
	updateGraphLegends();
	updateTimeLegends();
	updateGeom();
}
