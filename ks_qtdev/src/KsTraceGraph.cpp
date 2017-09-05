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
  _data(nullptr),
  _model(nullptr),
  _chartView(nullptr),
  _chart(nullptr),
  _axisX(nullptr),
  _mapper(nullptr)
{

	_chartView = new KsChartView(this);
	//_chartView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	//_chartView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	_chartView->setRubberBand(QChartView::HorizontalRubberBand);
	_chartView->setRenderHint(QPainter::Antialiasing);
	_chartView->setMinimumSize(640, 400);

	connect(_chartView, SIGNAL(rangeChanged(size_t, size_t)),
			this, SLOT(rangeChanged(size_t, size_t)));
	
	//_axisX = new QCategoryAxis(this);
	//_axisX->setGridLineVisible(false);

	QVBoxLayout *layout = new QVBoxLayout();
	layout->addWidget(_chartView);
	this->setLayout(layout);
}

KsTraceGraph::~KsTraceGraph() {}

void KsTraceGraph::init(int nCpus)
{
	//if (_model)
		//delete _model;

	_model = new KsGraphModel(nCpus, this);

	if (!_chart)
		_chart = new QChart();
	else
		_chart->removeAllSeries();

	//_chart->setAnimationOptions(QChart::AllAnimations);

	//addSeries("CPU 0", 0, 2);
	//addSeries("CPU 1", 0, 3);

	for (int cpu = 0; cpu <nCpus ; ++cpu) {
		addSeries(QString("CPU %1").arg(cpu), 0, cpu + 2);
	}

	_chartView->setChart(_chart);
}

void KsTraceGraph::reset()
{
	_model->reset(); 
	_chart->removeAllSeries();
}

void KsTraceGraph::addSeries(const QString &name, int colX, int colY)
{
	QLineSeries *series = new QLineSeries();
	series->setName(name);
	series->setUseOpenGL(true);
	_mapper = new QVXYModelMapper(this);
	_mapper->setXColumn(colX);
	_mapper->setYColumn(colY);
	_mapper->setSeries(series);
	_mapper->setModel(_model);

	_chart->addSeries(series);
	_series.append(series);
}

void KsTraceGraph::loadData(KsDataStore *data)
{
	hd_time t0 = GET_TIME;

	int nCpus = data->_pevt->cpus;
	init(nCpus);

 	_model->fill(data->_pevt, data->_rows, data->size());
	//_model->fill(data->_pevt, data->_rows, 1024*8);

	_chart->createDefaultAxes();
	setAxisX();
	for (auto const &s: _chart->series())
		_chart->setAxisX(_axisX, s);
	
	//_chart->axisX()->setRange( (double)_model->_map->_min*1e-8, (double) _model->_map->_max*1e-8);
	_chart->axisY()->setRange(- nCpus +1 , 1);

	_data = data;
	double time2 = GET_DURATION(t0);
	std::cout <<"time: " << 1e3*time2 << " ms.\n";
}

void KsTraceGraph::setAxisX()
{
	_axisX = new QCategoryAxis(this);
	_axisX->setGridLineVisible(false);
	_axisX->setRange(0, (int)_model->_map->size()-1);

	QString l1, l2, l3;

	l1 = QString::number(_model->_map->binTime(0), 'f', 6);
	_axisX->append(l1, 250);

	l2 = QString::number(_model->_map->binTime(_model->_map->size()/2), 'f', 6);
	_axisX->append(l2, _model->_map->size()-250);

	l3 = QString::number(_model->_map->binTime(_model->_map->size()-1), 'f', 6);
	_axisX->append(l3, _model->_map->size());
	
}

void KsTraceGraph::rangeChanged(size_t p0, size_t p1)
{
	uint64_t min, max;
	min = _model->_map->ts(p0);
	max = _model->_map->ts(p1);
	std::cout << "rangeChanged (" << min << ", " << max << ")\n";
	_model->_map->setBining(1024*2, min, max);
	_model->fill(_data->_pevt, _data->_rows, _data->size(), false);
	setAxisX();
	for (auto const &s: _chart->series())
		_chart->setAxisX(_axisX, s);
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
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	setRubberBand(QChartView::HorizontalRubberBand);
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

