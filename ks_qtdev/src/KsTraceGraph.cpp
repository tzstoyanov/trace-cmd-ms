/*
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
 
#include <chrono>
#include <iostream>
#include <iomanip>

#include "KsTraceGraph.h"
#include "KsUtils.h"

#define CPU_GRAPH_HEIGHT (SCREEN_HEIGHT/17)

KsTraceGraph::KsTraceGraph(QWidget *parent)
: QWidget(parent),
  _toolbar(this),
  _navigationBar(this),
  _scrollLeftButton("<", this),
  _zoomInButton("+", this),
  _zoomOutButton("-", this),
  _scrollRightButton(">", this),
  _labelP1("Pointer: ", this),
  _labelP2(" ", this),
  _labelI("", this),
  _labelA1("    Marker A: ", this),
  _labelA2(" ", this),
  _labelB1("    Marker B: ", this),
  _labelB2(" ", this),
  _labelD1("    A,B Delta: ", this),
  _labelD2(" ", this),
  _scrollArea(this),
  _drawWindow(this),
  _model(this),
  _data(nullptr),
  _axisChart(nullptr),
  _axisX(nullptr),
  _mapper(nullptr),
  _rubberBand(QRubberBand::Rectangle, this)
{
	_toolbar.setOrientation(Qt::Horizontal);

	auto makeLabel = [&](QLabel *l1, QLabel *l2, QToolBar *tb) {
		tb->addWidget(l1);
		l2->setFrameStyle(QFrame::Panel | QFrame::Sunken);
		l2->setStyleSheet("QLabel { background-color : white;}");
		l2->setFixedWidth(FONT_WIDTH*15);
		tb->addWidget(l2);
		tb->addSeparator();
	};
	
	makeLabel(&_labelP1, &_labelP2, &_toolbar);
	_toolbar.addWidget(&_labelI);
	

	_layout.addWidget(&_toolbar);

	_scrollArea.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	_scrollArea.setWidget(&_drawWindow);

	_layout.addWidget(&_scrollArea);

	//_scrollLeftButton.resize(20,20);
	_navigationBar.setMaximumHeight(FONT_HEIGHT*1.75);

	auto makeNavButton = [&](QPushButton *b) {
		b->setMaximumWidth(FONT_WIDTH*5);

		connect(b, SIGNAL(released()), this, SLOT(stopUpdating()));
		_navigationBar.addWidget(b);
	};

	connect(&_scrollLeftButton, SIGNAL(pressed()), this, SLOT(scrollLeft()));
	makeNavButton(&_scrollLeftButton);

	connect(&_zoomInButton, SIGNAL(pressed()), this, SLOT(zoomIn()));
	makeNavButton(&_zoomInButton);

	connect(&_zoomOutButton, SIGNAL(pressed()), this, SLOT(zoomOut()));
	makeNavButton(&_zoomOutButton);

	connect(&_scrollRightButton, SIGNAL(pressed()), this, SLOT(scrollRight()));
	makeNavButton(&_scrollRightButton);	
	
	makeLabel(&_labelA1, &_labelA2, &_navigationBar);
	makeLabel(&_labelB1, &_labelB2, &_navigationBar);
	makeLabel(&_labelD1, &_labelD2, &_navigationBar);

	_layout.addWidget(&_navigationBar);

	this->setLayout(&_layout);
}

KsTraceGraph::~KsTraceGraph() {}

void KsTraceGraph::updateGeom()
{
	if (_chartView.count() == 0)
		return;

	int hMin = 	_drawWindow.height() +
				_toolbar.height() +
				_navigationBar.height() +
				_layout.contentsMargins().top() +
				_layout.contentsMargins().bottom();

	if (hMin > SCREEN_HEIGHT/4) hMin = SCREEN_HEIGHT/4;

	setMinimumHeight(hMin);

	_scrollArea.resize(this->width(), 	this->height() -
										_toolbar.height() -
										_navigationBar.height() -
										_layout.spacing()*2 -
										_layout.contentsMargins().top() -
										_layout.contentsMargins().bottom());

	int marginsH = 	_layout.contentsMargins().left() +
					_layout.contentsMargins().right() +
					_drawWindow.layout()->contentsMargins().left() +
					_drawWindow.layout()->contentsMargins().right();

	_drawWindow.resize(	_scrollArea.width() - marginsH,
						_drawWindow.height());

}

void KsTraceGraph::resizeEvent(QResizeEvent* event)
{
	updateGeom();
}

void KsTraceGraph::reset()
{
	_model.reset(); 
	//_chart->removeAllSeries();
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

	connect(view, SIGNAL(zoomIn()),
			this, SLOT(zoomIn()));

	connect(view, SIGNAL(zoomOut()),
			this, SLOT(zoomOut()));

	connect(view, SIGNAL(scrollLeft()),
			this, SLOT(scrollLeft()));

	connect(view, SIGNAL(scrollRight()),
			this, SLOT(scrollRight()));

	connect(view, SIGNAL(stopUpdating()),
			this, SLOT(stopUpdating()));

	connect(view, SIGNAL(resetPointer()),
			this, SLOT(resetPointer()));

	connect(view, SIGNAL(select(int, bool)),
			this, SLOT(selectReceived(int, bool)));

	connect(view, SIGNAL(found(size_t)),
			this, SLOT(setPointerInfo(size_t)));

	QChart *chart = new QChart();
	chart->setAnimationOptions(QChart::NoAnimation);
	QLineSeries *series = new QLineSeries();
	//QPen pen(Qt::black);
	QPen pen(Qt::darkBlue);
	
	if (SCREEN_HEIGHT < 2000) pen.setWidthF(1.5);
	else pen.setWidthF(2.5);

	series->setPen(pen);
	//chart->installEventFilter(this);
	//series->installEventFilter(this);
	
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
	//chart->createDefaultAxes();

	QCategoryAxis *axisY = new QCategoryAxis(this);
	axisY->setRange(0 , 1);
	axisY->setTitleText(series->name());
	axisY->setTitleVisible(true);
	chart->setAxisY(axisY, series);

	view->setChart(chart);
	//_layout.addWidget(view);
	_drawWindow.layout()->addWidget(view);
	_chartView.append(view);
}

void KsTraceGraph::loadData(KsDataStore *data)
{
	hd_time t0 = GET_TIME;
	
	int nCpus = data->_pevt->cpus;
	_model.reset();	
	_model.setNCpus(nCpus);
 	_model.fill(data->_pevt, data->_rows, data->size());

	//drawGraphs(nCpus, 0x3);
	drawGraphs(nCpus);

	_data = data;
	double time2 = GET_DURATION(t0);
	std::cout <<"time: " << 1e3*time2 << " ms.\n";
}

void KsTraceGraph::drawGraphs(int nCpus, uint32_t cpuMask)
{
	if (_drawWindow.layout()) {
		QLayoutItem *child;
		while ((child = _drawWindow.layout()->takeAt(0)) != 0)
			delete child;

		delete _drawWindow.layout();
	}

	_drawWindow.setLayout(new QVBoxLayout());
	_drawWindow.layout()->setContentsMargins(0, 0, 0, 0);
	_drawWindow.layout()->setSpacing(SCREEN_HEIGHT/400);

	_chartView.clear();

	for (int cpu = 0; cpu <nCpus ; ++cpu) {
		if (0x1 & cpuMask >> cpu)
			addCpu(cpu);
	}

	int margins = 	_drawWindow.layout()->contentsMargins().top() +
					_drawWindow.layout()->contentsMargins().bottom();

	_drawWindow.resize(_drawWindow.width(), CPU_GRAPH_HEIGHT*_chartView.count() + margins);
	updateGeom();
}

void KsTraceGraph::setAxisX()
{
	if (_axisX)
		_axisChart->removeAxis(_axisX);

	_axisX = new QCategoryAxis(this);
	_axisX->setGridLineVisible(false);
	_axisX->setRange(0, (int)_model._visMap.size()-1);

	QString l1, l2, l3;

	l1 = QString::number(_model._visMap.binTime(0), 'f', 6);
	_axisX->append(l1, 250);

	l2 = QString::number(_model._visMap.binTime(_model._visMap.size()/2), 'f', 6);
	_axisX->append(l2, _model._visMap.size()-250);

	l3 = QString::number(_model._visMap.binTime(_model._visMap.size()-1), 'f', 6);
	_axisX->append(l3, _model._visMap.size());

	_axisChart->addAxis(_axisX, Qt::AlignBottom);
}

void KsTraceGraph::rangeBoundInit(int x, size_t posMA)
{
	_markerA = posMA;
	QString mA = QString::number(_model._visMap.binTime(posMA), 'f', 6);
	_labelA2.setText(QString("%1").arg(mA));

	int margins = 	_layout.contentsMargins().left() +
					_drawWindow.layout()->contentsMargins().left();

	_rubberBandOrigin.rx() = x + margins;
	_rubberBandOrigin.ry() = _toolbar.height() +
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
	QString pointer = QString::number(_model._visMap.binTime(posMB), 'f', 6);
	_labelP2.setText(QString("%1").arg(pointer));

	if(!_rubberBand.isVisible())
		return;

	QString mB = QString::number(_model._visMap.binTime(posMB), 'f', 6);
	_labelB2.setText(QString("%1").arg(mB));

	QString delta = QString::number(_model._visMap.binTime(posMB) -
									_model._visMap.binTime(_markerA),
									'f', 6);
	_labelD2.setText(QString("%1").arg(delta));

	QPoint pos;
	int margins = 	_layout.contentsMargins().left() +
					_drawWindow.layout()->contentsMargins().left();
	pos.rx() = x + margins;
	pos.ry() = _drawWindow.height() + _toolbar.height() +
				_drawWindow.layout()->contentsMargins().top();

	_rubberBand.setGeometry(QRect(_rubberBandOrigin, pos).normalized());
}

void KsTraceGraph::rangeChanged(size_t p0, size_t p1)
{
	if ( (p1 - p0) < 4) {
		_rubberBand.hide();
		return;
	}

	QString mA = QString::number(_model._visMap.binTime(p0), 'f', 6);
	_labelA2.setText(QString("%1").arg(mA));

	QString mB = QString::number(_model._visMap.binTime(p1), 'f', 6);
	_labelB2.setText(QString("%1").arg(mB));

	uint64_t min, max;
	min = _model._visMap.ts(p0);
	max = _model._visMap.ts(p1);

	if (max - min < KS_GRAPH_N_BINS) {
		_rubberBand.hide();
		return;
	}

	std::cout << "rangeChanged (" << min << ", " << max << ")\n";

	_model._visMap.setBining(KS_GRAPH_N_BINS, min, max);
	_model.fill(_data->_pevt, _data->_rows, _data->size(), false);

	//setAxisX();
	_rubberBand.hide();

	if (_mark.isSet()) {
		_mark.remove();
		if(_mark.reset(_data, &_model._visMap))
			_mark.draw();
	}

	for (size_t i = 0; i < KS_GRAPH_N_BINS; ++i) {
		if (_model._visMap[i] > 0) {
			emit select(_model._visMap[i], false);
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
	_mark.remove();
}

void KsTraceGraph::setPointerInfo(size_t i)
{
	ks_entry e;
	ks_set_entry_values(_data->_pevt, _data->_rows[i], &e);
	int str_size;
	const char* info_str = ks_dump_entry(&e, &str_size);
	QString info(info_str);

	_labelI.setText(info);
	_labelI.adjustSize();
}
	
void KsTraceGraph::markEntry(size_t pos)
{
	_mark.remove();

	int cpu = _data->_rows[pos]->cpu;
	_scrollArea.ensureVisible(	0,
								CPU_GRAPH_HEIGHT/2 +
								cpu*(CPU_GRAPH_HEIGHT + _drawWindow.layout()->spacing()/2),
								50,
								CPU_GRAPH_HEIGHT/2);
								
	_model.shiftTo(_data->_rows[pos]->ts);
	_mark.set(_data, &_model._visMap, pos);
	for (auto const &v: _chartView) {
		if (v->_cpuId == cpu)
			_mark.draw(v);
	}
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
			if (_mark.isSet())
				_model.zoomIn(k, _mark._bin);
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

		if (_mark.isSet()) {
			_mark.remove();
			if(_mark.reset(_data, &_model._visMap))
				_mark.draw();
		}

		QCoreApplication::processEvents();

		QString mA = QString::number(_model._visMap.binTime(_model._visMap._min), 'f', 6);
		_labelA2.setText(QString("%1").arg(mA));

		QString mB = QString::number(_model._visMap.binTime(_model._visMap._max), 'f', 6);
		_labelB2.setText(QString("%1").arg(mB));

		QString delta = QString::number(_model._visMap.binTime(_model._visMap._max) -
										_model._visMap.binTime(_model._visMap._min),
										'f', 6);
		_labelD2.setText(QString("%1").arg(delta));

		
		if (!_mark.isSet())
			for (size_t i = 0; i < KS_GRAPH_N_BINS; ++i) {
				if (_model._visMap[i] > 0) {
					emit select(_model._visMap[i], false);
					break;
				}
			}
	}

	//setAxisX();
}

//bool KsTraceGraph::eventFilter(QObject *obj, QEvent *event)
//{
    ////if (event->type() == QEvent::GraphicsSceneMouseDoubleClick)
    //{
		//qInfo() << "# Event " << event->type();
    //}
    //return QObject::eventFilter(obj, event);
//}
	
KsChartView::KsChartView(QWidget *parent)
: QChartView(parent),
  _markerA(-1), _pointer(nullptr)
{
	init();
}

KsChartView::KsChartView(QChart *chart, QWidget *parent)
: QChartView(chart, parent),
  _markerA(-1), _pointer(nullptr)
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
		_markerA = this->mapToValue(event).x();
		std::cerr << "1 @ " << _markerA << std::endl;	
		if (_markerA < 0 || _markerA > KS_GRAPH_N_BINS ) {
			_markerA = -1;
			return;
		}

		emit rangeBoundInit(event->pos().x(), _markerA);
	} else if (event->button() == Qt::RightButton)
		emit resetPointer();
}

bool KsChartView::find(QMouseEvent *event, size_t variance, size_t *row)
{
	size_t pos = this->mapToValue(event).x();
	if (pos < 0 || pos > KS_GRAPH_N_BINS ) {
		*row = 0;
		return false;
	}

	int64_t found = _model->_visMap.at(pos, _cpuId);
	if ( found >= 0) {
		*row = found;
		return true;
	}
		
	for (size_t i = 1; i < variance; ++i) {
		found = _model->_visMap.at(pos + i, _cpuId);
		if ( found >= 0) {
			*row = found;
			return true;
		}
		found = _model->_visMap.at(pos - i, _cpuId);
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
	if ( found)
		emit select(row, true);
}

void KsChartView::mouseMoveEvent(QMouseEvent *event)
{
	int markerB = this->mapToValue(event).x();
	if (markerB < 0 || markerB > KS_GRAPH_N_BINS )
		return;
	
	size_t row;
	bool status = find(event, 5, &row);
	if ( status)
		emit found(row);

	emit rangeBoundStretched(event->pos().x(), markerB);
}

void KsChartView::mouseDoubleClickEvent(QMouseEvent *event)
{
	if ( event->button() == Qt::LeftButton ) {
		findAndSelect(event);
	}
}

void KsChartView::mouseReleaseEvent(QMouseEvent *event)
{
	if ( event->button() == Qt::LeftButton ) {
		int markerB = this->mapToValue(event).x();
		std::cerr << "2 @ " << _markerA << " " << markerB << std::endl;
	
		if (_markerA < 0) {
			findAndSelect(event);
			return;
		}

		if (markerB < 0)
			markerB = 0;

		if (markerB > KS_GRAPH_N_BINS)
			markerB = KS_GRAPH_N_BINS;

		if (_markerA < markerB)
			emit rangeChanged(_markerA, markerB);
		else
			emit rangeChanged(markerB, _markerA);

		_markerA = -1;
	}
}

void KsChartView::keyPressEvent(QKeyEvent *event)
{
	if (event->isAutoRepeat())
		return;

	switch (event->key()) {
	case Qt::Key_Plus:
		emit zoomIn();
		break;

	case Qt::Key_Minus:
		emit zoomOut();
		break;

	case Qt::Key_Left:
		emit scrollLeft();
		break;

	case Qt::Key_Right:
		emit scrollRight();
		break;

	default:
		QGraphicsView::keyPressEvent(event);
		break;
	}
}

void KsChartView::keyReleaseEvent(QKeyEvent *event)
{
	if (event->isAutoRepeat())
		return;

	if(	event->key() == Qt::Key_Plus ||
		event->key() == Qt::Key_Minus ||
		event->key() == Qt::Key_Left ||
		event->key() == Qt::Key_Right)
		emit stopUpdating();
}

void KsChartView::setPointer(size_t bin)
{
	emit resetPointer();
	QPointF p0 = this->chart()->mapToPosition(QPoint(bin, 2));
	QPointF p1 = this->chart()->mapToPosition(QPoint(bin, -1));

	_pointer = new QGraphicsLineItem(p0.x(), p0.y(), p1.x(), p1.y());
	QPen pen(Qt::darkGreen);
	pen.setWidth(2);
	_pointer->setPen(pen);
	this->scene()->addItem(_pointer);
}
