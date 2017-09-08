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

#ifndef KS_TRACEGRAPH_H
#define KS_TRACEGRAPH_H 1

// Qt
#include <QtWidgets>
#include <QtCharts>
using namespace QtCharts;

#include "KsModel.h"
#include "KsDataStore.h"

QT_CHARTS_USE_NAMESPACE

class KsChartView : public QChartView
{

	Q_OBJECT	

public:
	KsChartView(QWidget *parent = 0);
    KsChartView(QChart *chart, QWidget *parent = 0);

signals:
	void rangeChanged(size_t, size_t);

protected:
    bool viewportEvent(QEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);

private:
	void init();
	QPointF mapToValue(QMouseEvent *event);

    bool m_isTouching;
	size_t _p0;
};

class KsTraceGraph : public QWidget {

	Q_OBJECT

public:
	KsTraceGraph(QWidget *parent = 0);
	virtual ~KsTraceGraph();

	void loadData(KsDataStore *data);
	void reset();

private slots:
	void rangeChanged(size_t, size_t);

private:
	//void init(int nCpus);
	void addSeries(const QString &name, int colX, int colY);
	void addCpu(int cpu);
	void drawGraphs(int nCpus, uint32_t cpuMask=0xffffffff);
	void setAxisX();

	KsGraphModel		_model;
	QVector<KsChartView*>	_chartView;
	QVBoxLayout			_layout;

	KsDataStore 		*_data;
	QChart 				*_chart;
	QCategoryAxis 		*_axisX;
	QVXYModelMapper 	*_mapper;

	QList<QLineSeries*> 	 _series;
};

#endif



