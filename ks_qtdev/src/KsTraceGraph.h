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

#include <atomic>

// Qt
#include <QtWidgets>
#include <QRubberBand>
#include <QtCharts>
#include <QDebug>

using namespace QtCharts;
QT_CHARTS_USE_NAMESPACE

#include "KsModel.h"
#include "KsDataStore.h"

class KsChartView : public QChartView
{

	Q_OBJECT	

public:
	KsChartView(QWidget *parent = 0);
    KsChartView(QChart *chart, QWidget *parent = 0);


signals:
	void rangeBoundInit(int x, size_t);
	void rangeBoundStretched(int x, size_t);
	void rangeChanged(size_t, size_t);
	void zoomIn();
	void zoomOut();
	void scrollLeft();
	void scrollRight();
	void stopUpdating();
	void resetPointer();
	void select(int pos, bool mark);
	void found(size_t);

public slots:
	void setPointer(size_t);

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

private:
	void init();
	QPointF mapToValue(QMouseEvent *event);
	bool find(QMouseEvent *event, size_t variance, size_t *row);
	void findAndSelect(QMouseEvent *event);

	int 	_markerA;

public:
	int 			 _cpuId;
	QGraphicsLineItem* _pointer;
	KsGraphModel	*_model;
};

class KsGraphMark {
public:
	int					_bin;
	size_t				_pos;
	QGraphicsLineItem	*_mark;
	KsChartView 		*_graph;


	KsGraphMark()
	: _bin(-1), _pos(0), _mark(nullptr), _graph(nullptr) {}

	bool set(KsDataStore *data, KsTimeMap *map, size_t pos) {
		_pos = pos;
		size_t ts = data->_rows[_pos]->ts;
		if (ts > map->_max || ts < map->_min) {
			_bin = -1;
			return false;
		}
			
		_bin = (ts - map->_min)/map->_binSize;
		return true;
	}

	bool reset(KsDataStore *data, KsTimeMap *map) {
		return set(data, map, this->_pos);
	}
	bool isSet() {
		if (_mark)
			return true;

		return false;
	}

	void draw(KsChartView *graph) {
		_graph = graph;
		QPointF p0 = _graph->chart()->mapToPosition(QPoint(_bin, 2));
		QPointF p1 = _graph->chart()->mapToPosition(QPoint(_bin, -1));

		QPen pen(Qt::darkGreen);
		pen.setWidth(2);

		_mark = new QGraphicsLineItem(p0.x(), p0.y(), p1.x(), p1.y());
		_mark->setPen(pen);
		_graph->scene()->addItem(_mark);
	}

	void draw() {
		draw(this->_graph);
	}

	void remove() {
		if (_mark) {
			_graph->scene()->removeItem(_mark);
			delete _mark;
			_mark = nullptr;
			//_graph = nullptr;
		}
	}
};

class KsTraceGraph : public QWidget {

	Q_OBJECT

public:
	KsTraceGraph(QWidget *parent = 0);
	virtual ~KsTraceGraph();

	void loadData(KsDataStore *data);
	void reset();

private slots:
	void rangeBoundInit(int x, size_t);
	void rangeBoundStretched(int x, size_t);
	void rangeChanged(size_t, size_t);
	void zoomIn();
	void zoomOut();
	void scrollLeft();
	void scrollRight();
	void stopUpdating() {_keyPressed = false;}
	void resetPointer();
	void setPointerInfo(size_t);
	void markEntry(size_t);
	void selectReceived(int bin, bool mark);

signals:
	void select(int, bool);

//protected:
	//bool eventFilter(QObject *obj, QEvent *event);

private:
	void resizeEvent(QResizeEvent* event);

	enum class GraphActions {
		ZoomIn,
		ZoomOut,
		ScrollLeft,
		ScrollRight
	};

	void addCpu(int cpu);
	void drawGraphs(int nCpus, uint32_t cpuMask=0xffffffff);
	void updateGeom();
	void updateGraphs(GraphActions action);
	void setAxisX();

	QToolBar 		_toolbar;
	QToolBar 		_navigationBar;
	QPushButton     _scrollLeftButton;
	QPushButton     _zoomInButton;
	QPushButton     _zoomOutButton;
	QPushButton     _scrollRightButton;

	QLabel 			_labelP1, _labelP2,
					_labelI,
					_labelA1, _labelA2,
					_labelB1, _labelB2,
					_labelD1, _labelD2;

	QScrollArea _scrollArea;
	QWidget     _drawWindow;

	KsGraphModel		_model;
	QVector<KsChartView*>	_chartView;
	QVBoxLayout			_layout;

	KsDataStore 		*_data;
	QChart 				*_axisChart;
	QCategoryAxis 		*_axisX;
	QVXYModelMapper 	*_mapper;

	QRubberBand 	_rubberBand;
	QPoint 			_rubberBandOrigin;

	size_t _markerA;
    bool _keyPressed;

	KsGraphMark _mark;
};

#endif // KS_TRACEGRAPH_H



