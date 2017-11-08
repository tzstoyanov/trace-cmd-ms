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

// Kernel Shark 2
#include "KsTraceGraph.h"
#include "KsUtils.h"

#define CPU_GRAPH_HEIGHT (SCREEN_HEIGHT/17)

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
  _drawWindow(this),
  _model(this),
  _mState(nullptr),
  _data(nullptr),
  _mapper(nullptr),
  _rubberBand(QRubberBand::Rectangle, this)
{
	auto makeNavButton = [&](QPushButton *b) {
		b->setMaximumWidth(FONT_WIDTH*5);

		connect(b, SIGNAL(released()), this, SLOT(stopUpdating()));
		_navigationBar.addWidget(b);
	};

	auto makeLabel = [&](QLabel *l1, QLabel *l2, QToolBar *tb) {
		tb->addWidget(l1);
		l2->setFrameStyle(QFrame::Panel | QFrame::Sunken);
		l2->setStyleSheet("QLabel { background-color : white;}");
		l2->setFixedWidth(FONT_WIDTH*15);
		tb->addWidget(l2);
		tb->addSeparator();
	};

	_pointerBar.setOrientation(Qt::Horizontal);
	makeLabel(&_labelP1, &_labelP2, &_pointerBar);

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
}

KsTraceGraph::~KsTraceGraph() {}

void KsTraceGraph::updateGeom()
{
	if (_chartMap.count() == 0)
		return;

	int hMin = _drawWindow.height() +
		   _pointerBar.height() +
		   _navigationBar.height() +
		   _layout.contentsMargins().top() +
		   _layout.contentsMargins().bottom();

	if (hMin > SCREEN_HEIGHT/4) hMin = SCREEN_HEIGHT/4;

	setMinimumHeight(hMin);

	_scrollArea.resize(this->width(), this->height() -
					  _pointerBar.height() -
					  _navigationBar.height() -
					  _layout.spacing()*2 -
					  _layout.contentsMargins().top() -
					  _layout.contentsMargins().bottom());

	int marginsH = _layout.contentsMargins().left() +
		       _layout.contentsMargins().right() +
		       _drawWindow.layout()->contentsMargins().left() +
		       _drawWindow.layout()->contentsMargins().right();

	if (_drawWindow.height() > _scrollArea.height())
		marginsH += style()->pixelMetric(QStyle::PM_ScrollBarExtent);

	_drawWindow.resize(_scrollArea.width() - marginsH,
			   _drawWindow.height());
}

void KsTraceGraph::resizeEvent(QResizeEvent* event)
{
	updateGeom();
}

void KsTraceGraph::reset()
{
	_model.reset(); 
}

void KsTraceGraph::addCpu(int cpu)
{
	KsChartView *view = new KsChartView(this);
	view->_cpuId = cpu;
	view->_model = &_model;

	connect(view, SIGNAL(rangeBoundInit(int, size_t)), 
			this, SLOT(rangeBoundInit(int, size_t)));

	connect(view, SIGNAL(rangeBoundStretched(int, size_t)),
			this, SLOT(rangeBoundStretched(int, size_t)));
	
	connect(view, SIGNAL(rangeChanged(size_t, size_t)),
			this, SLOT(rangeChanged(size_t, size_t)));

	connect(view, SIGNAL(zoomIn()), this, SLOT(zoomIn()));
	connect(view, SIGNAL(zoomOut()), this, SLOT(zoomOut()));
	connect(view, SIGNAL(scrollLeft()), this, SLOT(scrollLeft()));
	connect(view, SIGNAL(scrollRight()), this, SLOT(scrollRight()));
	connect(view, SIGNAL(stopUpdating()), this, SLOT(stopUpdating()));
	connect(view, SIGNAL(resetPointer()), this, SLOT(resetPointer()));
	connect(view, SIGNAL(select(int, bool)), this, SLOT(selectReceived(int, bool)));
	connect(view, SIGNAL(found(size_t)), this, SLOT(setPointerInfo(size_t)));

	QChart *chart = new QChart();
	chart->setAnimationOptions(QChart::NoAnimation);
	QLineSeries *series = new QLineSeries();
	QPen pen(Qt::darkBlue);
	
	if (SCREEN_HEIGHT < 2000)
		pen.setWidthF(1.5);
	else
		pen.setWidthF(3.5);

	series->setPen(pen);
	series->setName(QString("CPU %1").arg(cpu));
	series->setUseOpenGL(true);

	QVXYModelMapper *mapper = new QVXYModelMapper(this);
	mapper->setXColumn(0);
	mapper->setYColumn(cpu + 2);
	mapper->setSeries(series);
	mapper->setModel(&_model);

	chart->addSeries(series);
	chart->legend()->hide();
	chart->layout()->setContentsMargins(0, 0, 0, 0);
	chart->setBackgroundRoundness(0);

	QCategoryAxis *axisY = new QCategoryAxis(this);
	axisY->setRange(0, 1);
	axisY->setTitleText(series->name());
	axisY->setTitleVisible(true);
	chart->setAxisY(axisY, series);

	view->setChart(chart);
	_drawWindow.layout()->addWidget(view);
	_chartMap[cpu] = view;
}

void KsTraceGraph::loadData(KsDataStore *data)
{
	hd_time t0 = GET_TIME;
	
	int nCpus = data->_pevt->cpus;
	_model.reset();	
	_model.setNCpus(nCpus);
 	_model.fill(data->_pevt, data->_rows, data->size());

	drawGraphs(nCpus);
	_data = data;

	double time = GET_DURATION(t0);
	qInfo() <<"Graph loading time: " << 1e3*time << " ms.";
}

void KsTraceGraph::setMarkerSM(KsDualMarkerSM *m)
{
	_mState = m;

	_navigationBar.addSeparator();
	_mState->placeInToolBar(&_navigationBar);
}

void KsTraceGraph::drawGraphs(int nCpus, QVector<Qt::CheckState> cpuMask)
{
	/* The Very first thing to do is to clean up. */
	if (_drawWindow.layout()) {
		QLayoutItem *child;
		while ((child = _drawWindow.layout()->takeAt(0)) != 0) {
			delete child;
		}
		delete _drawWindow.layout();

		for (auto const &m: _chartMap)
			delete m;

		_chartMap.clear();
	}

	/*Now start on a new layout. */
	_drawWindow.setLayout(new QVBoxLayout());
	_drawWindow.layout()->setContentsMargins(0, 0, 0, 0);
	_drawWindow.layout()->setSpacing(SCREEN_HEIGHT/400);

	if (cpuMask.size() != nCpus)
		cpuMask.fill(Qt::Checked, nCpus);

	for (int cpu = 0; cpu <nCpus ; ++cpu) {
		if (cpuMask[cpu] == Qt::Checked)
			addCpu(cpu);
	}

	int margins = _drawWindow.layout()->contentsMargins().top() +
		      _drawWindow.layout()->contentsMargins().bottom();

	_drawWindow.resize(_drawWindow.width(),
			   CPU_GRAPH_HEIGHT*_chartMap.count() + margins);
	updateGeom();
}

void KsTraceGraph::cpuReDraw(QVector<Qt::CheckState> cpuMask)
{
	int nCpus = _data->_pevt->cpus;
	this->drawGraphs(nCpus, cpuMask);
}

void KsTraceGraph::rangeBoundInit(int x, size_t posMC)
{
	_posMousePress = posMC;
	int margins = _layout.contentsMargins().left() +
		      _drawWindow.layout()->contentsMargins().left();

	_rubberBandOrigin.rx() = x + margins;
	_rubberBandOrigin.ry() = _pointerBar.height() +
				 _navigationBar.height() +
				 _layout.spacing()/2 +
				 _layout.contentsMargins().top() +
				 _layout.contentsMargins().bottom() +
				 _drawWindow.layout()->contentsMargins().top();

	_rubberBand.setGeometry(_rubberBandOrigin.x(),
				_rubberBandOrigin.y(),
				0,0);

	_rubberBand.show();
}

void KsTraceGraph::rangeBoundStretched(int x, size_t posMB)
{
	QString pointer = QString::number(_model.histo().binTime(posMB), 'f', 7);
	_labelP2.setText(QString("%1").arg(pointer));

	if(!_rubberBand.isVisible())
		return;

	QPoint pos;
	int margins = _layout.contentsMargins().left() +
		      _drawWindow.layout()->contentsMargins().left();
	pos.rx() = x + margins;

	int dwHeight = _drawWindow.height() +
		       _pointerBar.height() +
		       _navigationBar.height() +
		       _layout.spacing()/2 +
		       _drawWindow.layout()->spacing()*(_chartMap.size() - 2);
		       _layout.contentsMargins().top();

	int saHeight = _scrollArea.height() +
		       _pointerBar.height() +
		       _navigationBar.height() +
		       _layout.spacing()/2 +
		       _layout.contentsMargins().top() + _layout.spacing();

	pos.ry() = (dwHeight < saHeight)? dwHeight : saHeight;

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

void KsTraceGraph::rangeChanged(size_t p0, size_t p1)
{
	if ( (p1 - p0) < 4) {
		_rubberBand.hide();
		return;
	}

	uint64_t min, max;
	min = _model.histo().ts(p0);
	max = _model.histo().ts(p1);

	if (max - min < KS_GRAPH_N_BINS) {
		_rubberBand.hide();
		return;
	}

	_model.histoPtr()->setBining(KS_GRAPH_N_BINS, min, max);
	_model.fill(_data->_pevt, _data->_rows, _data->size(), false);

	_rubberBand.hide();

	_mState->updateMarkers(*_data, _model.histo());

	for (size_t i = 0; i < KS_GRAPH_N_BINS; ++i) {
		if (_model.histo()[i] > 0) {
			emit select(_model.histo()[i], false);
			break;
		}
	}
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

void KsTraceGraph::setPointerInfo(size_t i)
{
	kshark_entry e;
	kshark_set_entry_values(_data->_pevt, _data->_rows[i], &e);
	int str_size;
	char* info_str = kshark_dump_entry(&e, &str_size);
	char *next, *pch = strchr(info_str,';');
	char *buff = strndup(info_str, pch - info_str);
	_labelI1.setText(QString(buff));
	_labelI1.adjustSize();

	for (auto const &l: {&_labelI2, &_labelI3, &_labelI4, &_labelI5}) {
		next = strchr(pch + 1,';');
		buff = strndup(pch + 2, next - pch - 2);
		pch = next;
		l->setText(QString(buff));
		l->adjustSize();
		free(buff);
	}
	
	free(info_str);
}
	
void KsTraceGraph::markEntry(size_t pos)
{
	_mState->activeMarker().remove();

	int cpu = _data->_rows[pos]->cpu;
	_scrollArea.ensureVisible(0,
				  CPU_GRAPH_HEIGHT/2 +
				  cpu*(CPU_GRAPH_HEIGHT +
				  _drawWindow.layout()->spacing()/2),
				  50,
				  CPU_GRAPH_HEIGHT/2);
								
	_model.shiftTo(_data->_rows[pos]->ts);
	_mState->activeMarker().set(*_data, _model.histo(), pos);

	_mState->activeMarker().draw(_chartMap[cpu]);
	_mState->updateLabels(_model.histo());
}

void KsTraceGraph::selectReceived(int pos, bool mark)
{
	emit select(pos, mark);
	markEntry(pos);
}

void KsTraceGraph::updateGraphs(GraphActions action)
{
	_keyPressed = true;
	double k = .05;
	while (_keyPressed) {
		switch (action) {
		case GraphActions::ZoomIn:
			if (_mState->activeMarker().isSet())
				_model.zoomIn(k, _mState->activeMarker().bin());
			else
				_model.zoomIn(k);

			k *= 1.1;
			break;

		case GraphActions::ZoomOut:
			_model.zoomOut(k);
			k *= 1.1;
			break;

		case GraphActions::ScrollLeft:
			_model.shiftBackward(32);
			break;

		case GraphActions::ScrollRight:
			_model.shiftForward(32);
			break;
		}

		_mState->updateMarkers(*_data, _model.histo());
		QCoreApplication::processEvents();
	}
}
	
KsChartView::KsChartView(QWidget *parent)
: QChartView(parent), _pointer(nullptr), _posMousePress(-1)
{
	init();
}

KsChartView::KsChartView(QChart *chart, QWidget *parent)
: QChartView(chart, parent), _pointer(nullptr), _posMousePress(-1)
{
	init();
}

void KsChartView::init()
{
	setRenderHint(QPainter::Antialiasing);
	setMaximumHeight(CPU_GRAPH_HEIGHT);
}

QPointF KsChartView::mapToValue(QMouseEvent *event)
{
	QPointF qPos = event->pos(), qChart, qVal;
	qVal = this->chart()->mapToValue(qPos);

	return qVal;
}
void KsChartView::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		_posMousePress = this->mapToValue(event).x();
		qInfo() << "1 @ " << _posMousePress;
		if (_posMousePress < 0 || _posMousePress > KS_GRAPH_N_BINS) {
			_posMousePress = -1;
			return;
		}

		emit rangeBoundInit(event->pos().x(), _posMousePress);
	} else if (event->button() == Qt::RightButton) {
		emit resetPointer();
	}
}

bool KsChartView::find(QMouseEvent *event, size_t variance, size_t *row)
{
	size_t pos = this->mapToValue(event).x();
	if (pos < 0 || pos > KS_GRAPH_N_BINS) {
		*row = 0;
		return false;
	}

	int64_t found = _model->histo().at(pos, _cpuId);
	if ( found >= 0) {
		*row = found;
		return true;
	}
		
	for (size_t i = 1; i < variance; ++i) {
		found = _model->histo().at(pos + i, _cpuId);
		if ( found >= 0) {
			*row = found;
			return true;
		}

		found = _model->histo().at(pos - i, _cpuId);
		if ( found >= 0) {
			*row = found;
			return true;
		}
	}

	*row = 0;
	return false;
}

void KsChartView::findAndSelect(QMouseEvent *event)
{
	emit resetPointer();
	size_t row;
	bool found = find(event, 10, &row);
	if (found)
		emit select(row, true);
}

void KsChartView::mouseMoveEvent(QMouseEvent *event)
{
	int markerB = this->mapToValue(event).x();

	if (markerB < 0 || markerB > KS_GRAPH_N_BINS)
		return;
	
	size_t row;
	bool status = find(event, 5, &row);
	if (status)
		emit found(row);

	emit rangeBoundStretched(event->pos().x(), markerB);
}

void KsChartView::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		findAndSelect(event);
	}
}

void KsChartView::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		int posMouseRel = this->mapToValue(event).x();
		qInfo() << "2 @ " << _posMousePress << " " << posMouseRel;
	
		if (_posMousePress < 0) {
			findAndSelect(event);
			return;
		}

		if (posMouseRel < 0)
			posMouseRel = 0;

		if (posMouseRel > KS_GRAPH_N_BINS)
			posMouseRel = KS_GRAPH_N_BINS;

		if (_posMousePress < posMouseRel)
			emit rangeChanged(_posMousePress, posMouseRel);
		else
			emit rangeChanged(posMouseRel, _posMousePress);

		_posMousePress = -1;
	}
}

void KsChartView::keyPressEvent(QKeyEvent *event)
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
		QGraphicsView::keyPressEvent(event);
		return;
	}
}

void KsChartView::keyReleaseEvent(QKeyEvent *event)
{
	if (event->isAutoRepeat())
		return;

	if(event->key() == Qt::Key_Plus ||
	   event->key() == Qt::Key_Minus ||
	   event->key() == Qt::Key_Left ||
	   event->key() == Qt::Key_Right)
		emit stopUpdating();
}




