/*
 * Copyright (C) 2017 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
 *
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

// C++
#include <iostream>

//Qt
#include <QFrame>

// Kernel Shark 2
#include "KsUtils.h"
#include "KsDeff.h"
#include "KsTraceGraph.h"

KsMessageDialog::KsMessageDialog(QString message, QWidget *parent)
: QDialog(parent),
  _text(message, this),
  _close_button("Close", this)
{
	this->resize(200, 50);
	_layout.addWidget(&_text);
	_layout.addWidget(&_close_button);
	connect(&_close_button,  SIGNAL(pressed()), this, SLOT(close()));

	this->setLayout(&_layout);
}

KsCheckBoxDialog::KsCheckBoxDialog(const QString &n, QWidget *parent)
: QDialog(parent),
  _all_cb("all", this),
  _cb_layout(nullptr),
  _cb_widget(nullptr),
  _top_layout(this),
  _scrollArea(this),
  _name(n),
  _cansel_button("Cansel", this),
  _apply_button("Apply", this)
{
	this->resize(SCREEN_WIDTH/10, SCREEN_HEIGHT/2.5);
	this->setWindowTitle(_name);

	connect(&_apply_button, SIGNAL(pressed()), this, SLOT(applyPress()));
	connect(&_apply_button, SIGNAL(pressed()), parentWidget(), SLOT(reload()) );

	connect(&_cansel_button,  SIGNAL(pressed()), this, SLOT(close()));

	connect(&_all_cb, SIGNAL(stateChanged(int)), this, SLOT(chechAll(int)));

	_button_layout.addWidget(&_apply_button,  1, Qt::AlignBottom);
	_button_layout.addWidget(&_cansel_button, 1, Qt::AlignBottom);

	_cb_widget = new QWidget(this);
	_cb_layout = new QVBoxLayout();
	_cb_widget->setLayout(_cb_layout);
	_scrollArea.setMinimumHeight(this->height() - _apply_button.height()*5.5);
	_scrollArea.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	_scrollArea.setWidget(_cb_widget);
					  
	_top_layout.addWidget(&_all_cb);
	_top_layout.addWidget(&_scrollArea);
	_top_layout.addLayout(&_button_layout, 1);

	this->setLayout(&_top_layout);
	_all_cb.setCheckState(Qt::Checked);
	this->show();
}

KsCheckBoxDialog::~KsCheckBoxDialog() {}

void KsCheckBoxDialog::applyPress()
{
	QVector<Qt::CheckState> vec;
	vec.resize(_cb.size());
	int i = 0;
	for (auto &c: _cb) {
		vec[i] = c->checkState();
		++i;
	}

	emit apply(vec);
	close();
}

void KsCheckBoxDialog::chechAll(int st)
{
	Qt::CheckState state = Qt::Unchecked;
	if (st) state = Qt::Checked;

	for (auto &c: _cb) {
		c->setCheckState(state);
	}
}

KSCpuCheckBoxDialog::KSCpuCheckBoxDialog(struct pevent *pe, QWidget *parent)
: KsCheckBoxDialog("Filter CPUs", parent)
{
	int i=0;
	_cb.resize(pe->cpus);
	for (auto &c: _cb) {
		c = new QCheckBox(QString("CPU %1").arg(i++), this);
		c->setCheckState(Qt::Checked);
		_cb_layout->addWidget(c);
	}
	_cb_widget->resize(this->width(), _cb[0]->height()*_cb.size()*1.8);
}

KSTasksCheckBoxDialog::KSTasksCheckBoxDialog(struct pevent *pe, QWidget *parent)
: KsCheckBoxDialog("Tasks", parent)
{

}

KSEventsCheckBoxDialog::KSEventsCheckBoxDialog(struct pevent *pe, QWidget *parent)
: KsCheckBoxDialog("Events", parent)
{
	int i=0;
	_cb.resize(pe->nr_events);

	struct event_format **events;
	
	events = pevent_list_events(pe, EVENT_SORT_NAME);
	if (!events)
		return;

	for (auto &c: _cb) {
		c = new QCheckBox(QString(pe->events[i]->name), this);
		c->setCheckState(Qt::Checked);
		_cb_layout->addWidget(c);
		++i;
	}
	_cb_widget->resize(this->width(), _cb[0]->height()*_cb.size()*2);
}


KsDataStore::~KsDataStore()
{
	clear();
}

void KsDataStore::loadData(const QString& file)
{
	hd_time t0 = GET_TIME;

	clear();
	_handle = tracecmd_open( file.toStdString().c_str() );

	if(!_handle) {
		std::cerr << "ERROR Loading file " << file.toStdString() << std::endl;
		return;
	}

	/* The handle should have loaded the file by now.
	 * Try to turn off function trace indent and turn on show parent
	 * if possible.
	 */
	trace_util_add_option("ftrace:parent", "1");
	trace_util_add_option("ftrace:indent", "0");

	/* Also, show the function name in the tail for function graph */
	trace_util_add_option("fgraph:tailprint", "1");

	loadData(_handle);

	double time2 = GET_DURATION(t0);
	std::cout <<"entries: " << _data_size << "  time: " << 1e3*time2 << " ms.\n";
}


void KsDataStore::loadData(struct tracecmd_input *handle)
{
	_data_size = ks_load_data(handle, &_rows);
	_pevt = tracecmd_get_pevent(handle);
}

void KsDataStore::clear()
{
	if (_data_size) {
		for (size_t r = 0; r < _data_size; ++r)
			free_record( _rows[r] );

		free(_rows);
		_rows = nullptr;
		_data_size = 0;
	}

	_pevt = nullptr;

	if (_handle)
	{
		tracecmd_close(_handle);
		_handle = nullptr;
	}
}


bool KsGraphMark::set(KsDataStore *data, KsTimeMap *map, size_t pos) {
	_pos = pos;
	size_t ts = data->_rows[_pos]->ts;
	if (ts > map->_max || ts < map->_min) {
		_bin = -1;
		return false;
	}
		
	_bin = (ts - map->_min)/map->_binSize;
	return true;
}

bool KsGraphMark::reset(KsDataStore *data, KsTimeMap *map) {
	return set(data, map, this->_pos);
}

bool KsGraphMark::isSet() {
	if (_mark)
		return true;

	return false;
}

void KsGraphMark::draw(KsChartView *graph) {
	_graph = graph;
	QPointF p0 = _graph->chart()->mapToPosition(QPoint(_bin, 4));
	QPointF p1 = _graph->chart()->mapToPosition(QPoint(_bin, -3));

	QPen pen(_color);
	if (SCREEN_HEIGHT < 2000)
		pen.setWidthF(2);
	else
		pen.setWidthF(4.5);

	_mark = new QGraphicsLineItem(p0.x(), p0.y(), p1.x(), p1.y());
	_mark->setPen(pen);
	_graph->scene()->addItem(_mark);
}

void KsGraphMark::draw() {
	draw(this->_graph);
}

void KsGraphMark::remove() {
	if (_mark) {
		_graph->scene()->removeItem(_mark);
		delete _mark;
		_mark = nullptr;
	}
}


KsMarkerState::KsMarkerState(QWidget *parent)
 : QWidget(parent), _buttonA("Marker A"), _buttonB("Marker B")
{
	_buttonA.setFixedWidth(STRING_WIDTH(" Marker A "));
	_buttonA.setFixedHeight(FONT_HEIGHT*1.2);
	_buttonB.setFixedWidth(STRING_WIDTH(" Marker B "));
	_buttonB.setFixedHeight(FONT_HEIGHT*1.2);

	_stateA = new QState;
	_stateA->setObjectName("A");
	_stateA->assignProperty(&_buttonA, "styleSheet", "background-color: green;");
	_stateA->assignProperty(&_buttonB, "styleSheet", "");

	_stateB = new QState;
	_stateB->setObjectName("B");
	_stateB->assignProperty(&_buttonA, "styleSheet", "");
	_stateB->assignProperty(&_buttonB, "styleSheet", "background-color: darkCyan;");

	_stateB->addTransition(&_buttonA, SIGNAL(clicked()), _stateA);
	connect(&_buttonA, SIGNAL(clicked()), this, SLOT(setStateA()));

	_stateA->addTransition(&_buttonB, SIGNAL(clicked()), _stateB);
	connect(&_buttonB, SIGNAL(clicked()), this, SLOT(setStateB()));
	
	_machine.addState(_stateA);
	_machine.addState(_stateB);
	_machine.setInitialState(_stateA);
	_markState = STATE_A;
	_machine.start();
	
	this->setLayout(new QHBoxLayout);
	this->layout()->addWidget(&_buttonA);
	this->layout()->addWidget(&_buttonB);
}

void KsMarkerState::setStateA()
{
	_markState = STATE_A;
	std::cerr << _markState << std::endl;
}

void KsMarkerState::setStateB()
{
	_markState = STATE_B;
	std::cerr << _markState << std::endl;
}


