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

#define GET_TIME std::chrono::high_resolution_clock::now()

#define GET_DURATION(t0) std::chrono::duration_cast<std::chrono::duration<double>>( \
std::chrono::high_resolution_clock::now()-t0).count()

typedef std::chrono::high_resolution_clock::time_point  hd_time;


#include "KsTraceGraph.h"

KsTraceGraph::KsTraceGraph(QWidget *parent)
: QWidget(parent),
  _model(this),
  _data(nullptr),
  //_chartView(nullptr),
  _chart(nullptr),
  _axisX(nullptr),
  _mapper(nullptr)
{
	//scrollarea = QScrollArea(parent.widget())
	//QScrollArea *scrollArea = new QScrollArea(this);
	//scrollArea->setWidget(&_layout);
	//layout->setContentsMargins(0, 0, 0, 0);
	_layout.setSpacing(0);
	//_layout.setSizeConstraint(QLayout::SetMinAndMaxSize);
	
	this->setLayout(&_layout);

	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred );
	//setMinimumHeight(400);
	setMinimumWidth(800);

	//QVBoxLayout *layout = new QVBoxLayout();

	//for (auto const &c: _chartView)
		//layout->addWidget(c);

	//this->setLayout(layout);
}

KsTraceGraph::~KsTraceGraph() {}

//void KsTraceGraph::init(int nCpus)
//{
	//if (!_chart)
		//_chart = new QChart();
	//else
		//_chart->removeAllSeries();

	////_chart->setAnimationOptions(QChart::AllAnimations);

	//addSeries("CPU 0", 0, 2);
	////addSeries("CPU 1", 0, 3);

	////for (int cpu = 0; cpu <nCpus ; ++cpu) {
		////addSeries(QString("CPU %1").arg(cpu), 0, cpu + 2);
	////}

	//_chart->createDefaultAxes();
	//setAxisX();

	//for (auto const &s: _chart->series())
		//_chart->setAxisX(_axisX, s);

	////_chart->axisY()->setRange(- nCpus +1 , 1);
	//_chart->axisY()->setRange(0 , 1);

	//_chartView[0]->setChart(_chart);
//}

//void KsTraceGraph::init()
//{
	//QVBoxLayout *layout = new QVBoxLayout();

	////layout->setContentsMargins(0, 0, 0, 0);
	//layout->setSpacing(0);
	//layout->setSizeConstraint(QLayout::SetMinAndMaxSize);
	//this->setLayout(layout);
//}

void KsTraceGraph::reset()
{
	_model.reset(); 
	//_chart->removeAllSeries();
}

//void KsTraceGraph::resetTimeAxis()
//{
	//QChart *chart = new QChart();
	
//}

void KsTraceGraph::addCpu(int cpu)
{
	KsChartView *view = new KsChartView(this);

	connect(view, SIGNAL(rangeChanged(size_t, size_t)),
			this, SLOT(rangeChanged(size_t, size_t)));

	QChart *chart = new QChart();
	QLineSeries *series = new QLineSeries();
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
	_layout.addWidget(view);
	_chartView.append(view);
}

void KsTraceGraph::addSeries(const QString &name, int colX, int colY)
{
	//QLineSeries *series = new QLineSeries();
	//series->setName(name);
	//series->setUseOpenGL(true);

	//_mapper = new QVXYModelMapper(this);
	//_mapper->setXColumn(colX);
	//_mapper->setYColumn(colY);
	//_mapper->setSeries(series);
	//_mapper->setModel(&_model);

	//_chart->addSeries(series);
	//_series.append(series);
}

void KsTraceGraph::loadData(KsDataStore *data)
{
	hd_time t0 = GET_TIME;

	int nCpus = data->_pevt->cpus;
	_model.setNCpus(nCpus);
 	_model.fill(data->_pevt, data->_rows, data->size());

	//drawGraphs(nCpus, 0xf);
	drawGraphs(nCpus);

	setMinimumHeight(70*_chartView.count());
	_data = data;

	double time2 = GET_DURATION(t0);
	std::cout <<"time: " << 1e3*time2 << " ms.\n";
}

void KsTraceGraph::drawGraphs(int nCpus, uint32_t cpuMask)
{
	QLayoutItem *child;
	while ((child = _layout.takeAt(0)) != 0) {
		//child->hide();
		delete child;
	}
	
	_chartView.clear();
	
	//nCpus = 1;
	for (int cpu = 0; cpu <nCpus ; ++cpu) {
		if (0x1 & cpuMask >> cpu)
			addCpu(cpu);
	}
}

void KsTraceGraph::setAxisX()
{
	_axisX = new QCategoryAxis(this);
	_axisX->setGridLineVisible(false);
	_axisX->setRange(0, (int)_model._map->size()-1);

	QString l1, l2, l3;

	l1 = QString::number(_model._map->binTime(0), 'f', 6);
	_axisX->append(l1, 250);

	l2 = QString::number(_model._map->binTime(_model._map->size()/2), 'f', 6);
	_axisX->append(l2, _model._map->size()-250);

	l3 = QString::number(_model._map->binTime(_model._map->size()-1), 'f', 6);
	_axisX->append(l3, _model._map->size());
}

void KsTraceGraph::rangeChanged(size_t p0, size_t p1)
{
	uint64_t min, max;
	min = _model._map->ts(p0);
	max = _model._map->ts(p1);
	
	if (max - min < KS_GRAPH_N_BINS)
		return;

	std::cout << "rangeChanged (" << min << ", " << max << ")\n";

	_model._map->setBining(KS_GRAPH_N_BINS, min, max);
	_model.fill(_data->_pevt, _data->_rows, _data->size(), false);

	setAxisX();
	//for (auto const &s: _chartView[0]->chart()->series())
		//_chartView[0]->chart()->setAxisX(_axisX, s);
}

KsChartView::KsChartView(QWidget *parent) :
    QChartView(parent),
    m_isTouching(false)
{
	init();
}

KsChartView::KsChartView(QChart *chart, QWidget *parent) :
    QChartView(chart, parent),
    m_isTouching(false)
{
	init();
}

void KsChartView::init()
{
	//setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	//setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Maximum );
	//setRubberBand(QChartView::HorizontalRubberBand);
	setRenderHint(QPainter::Antialiasing);
	setMaximumHeight(70);
	//setMinimumWidth(600);
}

bool KsChartView::viewportEvent(QEvent *event)
{
    if (event->type() == QEvent::TouchBegin) {
        // By default touch events are converted to mouse events. So
        // after this event we will get a mouse event also but we want
        // to handle touch events as gestures only. So we need this safeguard
        // to block mouse events that are actually generated from touch.
        m_isTouching = true;

        // Turn off animations when handling gestures they
        // will only slow us down.
        chart()->setAnimationOptions(QChart::NoAnimation);
    }

    return QChartView::viewportEvent(event);
}

QPointF KsChartView::mapToValue(QMouseEvent *event)
{
	QPointF qScree = event->screenPos(), qChart, qVal;
	qChart = this->chart()->mapFromScene(qScree);
	qVal = this->chart()->mapToValue(qChart);

	return qVal;
}
void KsChartView::mousePressEvent(QMouseEvent *event)
{
    if (m_isTouching) {
        return;
	}

	_p0 = this->mapToValue(event).x();
	std::cerr << "1@ " << _p0 << std::endl;
	
    //QChartView::mousePressEvent(event);
}

void KsChartView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isTouching)
        return;

    QChartView::mouseMoveEvent(event);
}

void KsChartView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_isTouching) {
        m_isTouching = false;
	}

    // Because we disabled animations when touch event was detected
    // we must put them back on.
    chart()->setAnimationOptions(QChart::SeriesAnimations);

    //QChartView::mouseReleaseEvent(event);

	size_t p1 = this->mapToValue(event).x();
	std::cerr << "2@ " << _p0 << " " << p1 << std::endl;
	
	if (_p0 < p1)
		emit rangeChanged(_p0, p1);
	else
		emit rangeChanged(p1, _p0);
	
}

void KsChartView::keyPressEvent(QKeyEvent *event)
{
	switch (event->key()) {
	case Qt::Key_Plus:
	{
		chart()->zoomIn();
		break;
	}
	case Qt::Key_Minus:
		chart()->zoomOut();
		break;

	case Qt::Key_Left:
		chart()->scroll(-10, 0);
		break;

	case Qt::Key_Right:
		chart()->scroll(10, 0);
		break;

	case Qt::Key_Up:
		chart()->scroll(0, 10);
		break;

	case Qt::Key_Down:
		chart()->scroll(0, -10);
		break;

	default:
		QGraphicsView::keyPressEvent(event);
		break;
	}
}

