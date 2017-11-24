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

//Qt
#include <QFrame>

// Kernel Shark 2
#include "KsDeff.h"
#include "KsUtils.h"
#include "KsTraceGraph.h"

KsMessageDialog::KsMessageDialog(QString message, QWidget *parent)
: QDialog(parent),
  _text(message, this),
  _close_button("Close", this)
{
	this->resize(SCREEN_WIDTH/10, SCREEN_HEIGHT/5);
	_layout.addWidget(&_text);
	_layout.addWidget(&_close_button);
	connect(&_close_button,  SIGNAL(pressed()), this, SLOT(close()));

	this->setLayout(&_layout);
}

KsCheckBoxDialog::KsCheckBoxDialog(const QString &n, bool cond, QWidget *parent)
: QDialog(parent),
  _positiveCond(cond),
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
	connect(&_apply_button, SIGNAL(pressed()), parentWidget(), SLOT(reload()));

	connect(&_cansel_button,  SIGNAL(pressed()), this, SLOT(close()));

	connect(&_all_cb, SIGNAL(clicked(bool)), this, SLOT(chechAll(bool)));

	_button_layout.addWidget(&_apply_button,  1, Qt::AlignBottom);
	_button_layout.addWidget(&_cansel_button, 1, Qt::AlignBottom);

	_cb_widget = new QWidget(this);
	_cb_layout = new QVBoxLayout();
	_cb_widget->setLayout(_cb_layout);
	_scrollArea.setMinimumHeight(this->height() -
				   FONT_HEIGHT*3 -
				   _apply_button.height());

	_scrollArea.setStyleSheet("background-color : rgb(240, 240, 240)");
	_scrollArea.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	_scrollArea.setWidget(_cb_widget);

	_top_layout.addWidget(&_all_cb);
	_top_layout.addWidget(&_scrollArea);
	_top_layout.addLayout(&_button_layout, 1);

	this->setLayout(&_top_layout);
	_all_cb.setCheckState(Qt::Checked);
	this->show();
}

void KsCheckBoxDialog::setDefault(bool st)
{
	Qt::CheckState state = Qt::Unchecked;
	if (st)
		state = Qt::Checked;

	_all_cb.setCheckState(state);
	chechAll(state);

}

void KsCheckBoxDialog::resizeEvent(QResizeEvent* event)
{
	QDialog::resizeEvent(event);
	_scrollArea.resize(_scrollArea.width(),
			   this->height() - FONT_HEIGHT*5);
}

KsCheckBoxTableDialog::KsCheckBoxTableDialog(const QString &name, bool cond, QWidget *parent)
: KsCheckBoxDialog(name, cond, parent),
  _table(this)
{}

void KsCheckBoxTableDialog::initTable(QStringList headers, int size)
{
	_table.setColumnCount(headers.count());
	_table.setRowCount(size);
	_table.setShowGrid(false);
	_table.setHorizontalHeaderLabels(headers);
	_table.horizontalHeader()->setStretchLastSection(true);
	_table.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	_table.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	_table.verticalHeader()->setVisible(false);

	_cb.resize(size);

	for (int i = 0; i < size; ++i) {
		QWidget *cbWidget = new QWidget();
		_cb[i] = new QCheckBox(cbWidget);
		connect(_cb[i], SIGNAL(clicked(bool)),
			this, SLOT(update(bool)));

		QHBoxLayout *cbLayout = new QHBoxLayout(cbWidget);
		cbLayout->addWidget(_cb[i]);
		cbLayout->setAlignment(Qt::AlignCenter);
		cbLayout->setContentsMargins(0, 0, 0, 0);

		cbWidget->setLayout(cbLayout);
		_table.setCellWidget(i, 0, cbWidget);
	}

	_cb_layout->setContentsMargins(1, 1, 1, 1);
	_cb_layout->addWidget(&_table);
}

void KsCheckBoxTableDialog::adjustSize()
{
	_table.setVisible(false);
	_table.resizeColumnsToContents();
	_table.setVisible(true);

	int width = _table.horizontalHeader()->length() + 5;

	if(_table.verticalHeader()->length() > _cb_widget->height())
		width += style()->pixelMetric(QStyle::PM_ScrollBarExtent);

	_cb_widget->resize(width, FONT_HEIGHT*1.9*_table.rowCount());

	setMinimumWidth(_cb_widget->width() +
			_cb_layout->contentsMargins().left() +
			_cb_layout->contentsMargins().right() +
			_top_layout.contentsMargins().left() +
			_top_layout.contentsMargins().right());
}

void  KsCheckBoxTableDialog::update(bool state)
{
	if (!state)
		_all_cb.setCheckState(Qt::Unchecked);
}

void KsCheckBoxTableDialog::applyPress()
{
	QVector<int> vec;
	int n = _cb.size();
	for (int i = 0; i < n; ++i)
		if (_cb[i]->checkState() == Qt::Checked)
			vec.append(_id[i]);

	emit apply(vec);
	close();
}

void KsCheckBoxTableDialog::chechAll(bool st)
{
	Qt::CheckState state = Qt::Unchecked;
	if (st) state = Qt::Checked;

	for (auto &c: _cb) {
		c->setCheckState(state);
	}
}

KsCheckBoxTreeDialog::KsCheckBoxTreeDialog(const QString &name, bool cond, QWidget *parent)
: KsCheckBoxDialog(name, cond, parent),
  _tree(this)
{}

void KsCheckBoxTreeDialog::initTree()
{
	_tree.setColumnCount(1);
	_tree.setHeaderHidden(true);
	_tree.setSelectionMode(QAbstractItemView::SingleSelection);
	_tree.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	connect(&_tree, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
	this, SLOT(update(QTreeWidgetItem*, int)));

	_cb_layout->setContentsMargins(1, 1, 1, 1);
	_cb_layout->addWidget(&_tree);
}

void KsCheckBoxTreeDialog::adjustSize(int size)
{
	int width, height;
	int n = _tree.topLevelItemCount();
        for (int i = 0; i < n; ++i)
		_tree.topLevelItem(i)->setExpanded(true);

	_tree.resizeColumnToContents(0);

	if (_tree.topLevelItem(0)->child(0)) {
		width = _tree.visualItemRect(_tree.topLevelItem(0)->child(0)).width();
	} else {
		width = _tree.visualItemRect(_tree.topLevelItem(0)).width();
	}

	width += FONT_WIDTH*7;

        for (int i = 0; i < n; ++i)
		_tree.topLevelItem(i)->setExpanded(false);

	height = _tree.visualItemRect(_tree.topLevelItem(0)).height()*size;
	height += height/10;

	int heightSe = _scrollArea.height() -
		       _cb_layout->contentsMargins().top() -
		       _cb_layout->contentsMargins().bottom();

	int widthSe = _scrollArea.width() -
		      _cb_layout->contentsMargins().left() -
		      _cb_layout->contentsMargins().right();

	if (height < heightSe)
		height = heightSe;

	if (height > _cb_widget->height())
		width += style()->pixelMetric(QStyle::PM_ScrollBarExtent);

	if (width < widthSe)
		width = widthSe;

	_cb_widget->resize(width, height);

	setMinimumWidth(_cb_widget->width() +
			_cb_layout->contentsMargins().left() +
			_cb_layout->contentsMargins().right() +
			_top_layout.contentsMargins().left() +
			_top_layout.contentsMargins().right());
}

void update_r(QTreeWidgetItem *item, Qt::CheckState state)
{
	item->setCheckState(0, state);

	int n = item->childCount();
        for (int i = 0; i < n; ++i)
		update_r(item->child(i), state);
}

void KsCheckBoxTreeDialog::update(QTreeWidgetItem *item, int column)
{
	Qt::CheckState state = item->checkState(0);
	if (state != Qt::Checked) {
		_all_cb.setCheckState(Qt::Unchecked);
		QTreeWidgetItem *parent = item->parent();
		if (parent)
			parent->setCheckState(0, Qt::Unchecked);
	}

	update_r(item, state);
}

void KsCheckBoxTreeDialog::chechAll(bool st)
{
	Qt::CheckState state = Qt::Unchecked;
	if (st)
		state = Qt::Checked;

	int n = _tree.topLevelItemCount();
        for (int i = 0; i < n; ++i)
		update_r(_tree.topLevelItem(i), state);
}

void KsCheckBoxTreeDialog::applyPress()
{
	QVector<int> vec;
	int n = _id.size();
	for (int i = 0; i < n; ++i) {
		if (_cb[i]->checkState(0) == Qt::Checked)
			vec.append(_id[i]);
	}

	emit apply(vec);
	close();
}

KsCpuCheckBoxDialog::KsCpuCheckBoxDialog(struct pevent *pe, bool cond, QWidget *parent)
: KsCheckBoxTreeDialog("CPUs", cond, parent)
{
	int n = pe->cpus;
	if (!n)
		return;

	initTree();
	_id.resize(n);
	_cb.resize(n);

	for (int i = 0; i < n; ++i) {
		QTreeWidgetItem *cpuItem = new QTreeWidgetItem;
		cpuItem->setText(0, QString("CPU %1").arg(i));
		cpuItem->setCheckState(0, Qt::Checked);
		_tree.addTopLevelItem(cpuItem);
		_id[i] = i;
		_cb[i] = cpuItem;
	}

	adjustSize(n);
}

KsEventsCheckBoxDialog::KsEventsCheckBoxDialog(struct pevent *pe, bool cond, QWidget *parent)
: KsCheckBoxTreeDialog("Events", cond, parent)
{
	int n = pe->nr_events;
	if (!n)
		return;

	struct event_format **events;
	events = pevent_list_events(pe, EVENT_SORT_NAME);
	if (!events)
		return;

	initTree();
	_id.resize(n);
	_cb.resize(n);

	int i(0);
	while (i < n) {
		QString sysName(pe->events[i]->system);
		QTreeWidgetItem *sysItem = new QTreeWidgetItem;
		sysItem->setText(0, sysName);
		sysItem->setCheckState(0, Qt::Checked);
		_tree.addTopLevelItem(sysItem);

		while (sysName == pe->events[i]->system) {
			QTreeWidgetItem *evtItem = new QTreeWidgetItem;
			QString evtName(pe->events[i]->name);
			evtItem->setText(0, evtName);
			evtItem->setFlags(evtItem->flags() | Qt::ItemIsUserCheckable);
			evtItem->setCheckState(0, Qt::Checked);
			sysItem->addChild(evtItem);
			_id[i] = pe->events[i]->id;
			_cb[i] = evtItem;
			if (++i == n)
				break;
		}
	}

	adjustSize(n);
}

KsTasksCheckBoxDialog::KsTasksCheckBoxDialog(struct pevent *pe, bool cond, QWidget *parent)
: KsCheckBoxTableDialog("Tasks", cond, parent)
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	struct task_list *list;

	for (int i = 0; i < TASK_HASH_SIZE; ++i) {
		list = kshark_ctx->tasks[i];
		while (list) {
			_id.append(list->pid);
			list = list->next;
		}
	}

	int n = _id.size();
	if (!n)
		return;

	qSort(_id);

	QStringList headers;
	if (_positiveCond)
		headers << "Show" << "Pid" << "Task";
	else
		headers << "Hide" << "Pid" << "Task";

	initTable(headers, n);

	for (int i = 0; i < n; ++i) {
		QTableWidgetItem *pidItem
			= new QTableWidgetItem(tr("%1").arg(_id[i]));
		_table.setItem(i, 1, pidItem);
		QTableWidgetItem *comItem
			= new QTableWidgetItem(tr(pevent_data_comm_from_pid(kshark_ctx->pevt,
									    _id[i])));
		_table.setItem(i, 2, comItem);
	}

	adjustSize();
}

KsDataStore::KsDataStore()
: _handle(nullptr), _data_size(0), _rows(nullptr), _pevt(nullptr)
{}

KsDataStore::~KsDataStore()
{
	clear();
}

void KsDataStore::loadData(const QString &file)
{
	hd_time t0 = GET_TIME;

	clear();
	_handle = tracecmd_open( file.toStdString().c_str() );

	if(!_handle) {
		qCritical() << "ERROR Loading file " << file;
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

	double time = GET_DURATION(t0);
	qInfo() <<"Data loading time: " << 1e3*time << " ms.   entries: " << _data_size;
}

void KsDataStore::loadData(struct tracecmd_input *handle)
{
	//_data_size = kshark_load_data_records(handle, &_rows);
	_data_size = kshark_load_data_entries(handle, &_rows);
	_pevt = tracecmd_get_pevent(handle);
}

void KsDataStore::clear()
{
	if (_data_size) {
		for (size_t r = 0; r < _data_size; ++r)
			//free_record(_rows[r]);
			free(_rows[r]);

		free(_rows);
		_rows = nullptr;
		_data_size = 0;
	}

	_pevt = nullptr;

	if (_handle) {
		tracecmd_close(_handle);
		_handle = nullptr;
	}
}

void KsDataStore::applyPosTaskFilter(QVector<int> vec)
{
	hd_time t0 = GET_TIME;

	struct kshark_context *ctx = NULL;
	kshark_instance(&ctx);

	kshark_filter_clear(ctx, SHOW_TASK_FILTER);
	kshark_filter_clear(ctx, HIDE_TASK_FILTER);
	for (auto const &pid: vec)
		kshark_filter_add_pid(ctx, SHOW_TASK_FILTER, pid);

	kshark_filter_entries(ctx, _rows, _data_size);
	emit updateView();
	emit updateGraph();

	double time = GET_DURATION(t0);
	qInfo() <<"Filtering time: " << 1e3*time << " ms.";
}

void KsDataStore::applyNegTaskFilter(QVector<int> vec)
{
	hd_time t0 = GET_TIME;

	struct kshark_context *ctx = NULL;
	kshark_instance(&ctx);

	kshark_filter_clear(ctx, SHOW_TASK_FILTER);
	kshark_filter_clear(ctx, HIDE_TASK_FILTER);
	for (auto const &pid: vec)
		kshark_filter_add_pid(ctx, HIDE_TASK_FILTER, pid);

	kshark_filter_entries(ctx, _rows, _data_size);
	emit updateView();
	emit updateGraph();

	double time = GET_DURATION(t0);
	qInfo() <<"Filtering time: " << 1e3*time << " ms.";
}

void KsDataStore::applyPosEventFilter(QVector<int> vec)
{
	hd_time t0 = GET_TIME;

	struct kshark_context *ctx = NULL;
	kshark_instance(&ctx);

	kshark_filter_clear(ctx, SHOW_EVENT_FILTER);
	kshark_filter_clear(ctx, HIDE_EVENT_FILTER);

	for (auto const &pid: vec)
		kshark_filter_add_pid(ctx, SHOW_EVENT_FILTER, pid);

	kshark_filter_entries(ctx, _rows, _data_size);
	emit updateView();
	emit updateGraph();

	double time = GET_DURATION(t0);
	qInfo() <<"Filtering time: " << 1e3*time << " ms.";
}

void KsDataStore::applyNegEventFilter(QVector<int> vec)
{
	hd_time t0 = GET_TIME;

	struct kshark_context *ctx = NULL;
	kshark_instance(&ctx);

	kshark_filter_clear(ctx, SHOW_EVENT_FILTER);
	kshark_filter_clear(ctx, HIDE_EVENT_FILTER);
	for (auto const &pid: vec)
		kshark_filter_add_pid(ctx, HIDE_EVENT_FILTER, pid);

	kshark_filter_entries(ctx, _rows, _data_size);
	emit updateView();
	emit updateGraph();

	double time = GET_DURATION(t0);
	qInfo() <<"Filtering time: " << 1e3*time << " ms.";
}

KsGraphMark::KsGraphMark(DualMarkerState s)
: _state(s), _bin(-1), _pos(0), _color(Qt::darkGreen), _mark(nullptr), _graph(nullptr)
{}

KsGraphMark::KsGraphMark(DualMarkerState s, QColor col)
: _state(s), _bin(-1), _pos(0), _color(col), _mark(nullptr), _graph(nullptr)
{}

bool KsGraphMark::set(const KsDataStore &data, const  KsTimeMap &histo, size_t pos)
{
	_pos = pos;
	size_t ts = data._rows[_pos]->ts;
	if (ts > histo._max || ts < histo._min) {
		_bin = -1;
		return false;
	}
		
	_bin = (ts - histo._min)/histo._binSize;
	return true;
}

bool KsGraphMark::reset(const KsDataStore &data, const KsTimeMap &histo)
{
	return set(data, histo, this->_pos);
}

bool KsGraphMark::isSet()
{
	if (_mark)
		return true;

	return false;
}

void KsGraphMark::draw(KsChartView *graph)
{
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

void KsGraphMark::draw()
{
	draw(this->_graph);
}

void KsGraphMark::remove()
{
	if (_mark) {
		_graph->scene()->removeItem(_mark);
		delete _mark;
		_mark = nullptr;
	}
}

DualMarkerState operator !(const DualMarkerState &state)
{
	if (state == DualMarkerState::B)
		return DualMarkerState::A;

	return DualMarkerState::B;
}

KsDualMarkerSM::KsDualMarkerSM(QWidget *parent)
: QWidget(parent),
  _buttonA("Marker A", this),
  _buttonB("Marker B", this),
  _labelDeltaDescr("    A,B Delta: ", this),
  _markA(DualMarkerState::A, Qt::darkGreen),
  _markB(DualMarkerState::B, Qt::darkCyan)
{
	_buttonA.setFixedWidth(STRING_WIDTH(" Marker A "));
	_buttonA.setFixedHeight(FONT_HEIGHT*1.2);
	_buttonB.setFixedWidth(STRING_WIDTH(" Marker B "));
	_buttonB.setFixedHeight(FONT_HEIGHT*1.2);

	for (auto const &l: {&_labelMA, &_labelMB, &_labelDelta}) {
		l->setFrameStyle(QFrame::Panel | QFrame::Sunken);
		l->setStyleSheet("QLabel {background-color : white;}");
		l->setFixedHeight(FONT_HEIGHT*1.2);
		l->setFixedWidth(FONT_WIDTH*16);
	}

	_stateA = new QState;
	_stateA->setObjectName("A");
	_stateA->assignProperty(&_buttonA,
				"styleSheet",
				"background : darkGreen; color : white");

	_stateA->assignProperty(&_buttonB,
				"styleSheet",
				"color : rgb(70, 70, 70)");

	_stateB = new QState;
	_stateB->setObjectName("B");
	_stateB->assignProperty(&_buttonA,
				"styleSheet",
				"color : rgb(70, 70, 70)");

	_stateB->assignProperty(&_buttonB,
				"styleSheet",
				"background : darkCyan; color : white");

	_stateB->addTransition(&_buttonA, SIGNAL(clicked()), _stateA);
	connect(&_buttonA, SIGNAL(clicked()), this, SLOT(setStateA()));

	_stateA->addTransition(&_buttonB, SIGNAL(clicked()), _stateB);
	connect(&_buttonB, SIGNAL(clicked()), this, SLOT(setStateB()));

	_machine.addState(_stateA);
	_machine.addState(_stateB);
	_machine.setInitialState(_stateA);
	_markState = DualMarkerState::A;
	_machine.start();
}

void KsDualMarkerSM::setStateA()
{
	_markState = DualMarkerState::A;
	emit markSwitch();
}

void KsDualMarkerSM::setStateB()
{
	_markState = DualMarkerState::B;
	emit markSwitch();
}

KsGraphMark &KsDualMarkerSM::getMarker(DualMarkerState s)
{
	if (s == DualMarkerState::A)
		return _markA;

	return _markB;
}

KsGraphMark &KsDualMarkerSM::activeMarker()
{
	return this->getMarker(_markState);
}

KsGraphMark &KsDualMarkerSM::markerA()
{
	return _markA;
}

KsGraphMark &KsDualMarkerSM::markerB()
{
	return _markB;
}

void KsDualMarkerSM::placeInToolBar(QToolBar *tb)
{
	tb->addWidget(new QLabel("   "));
	tb->addWidget(&_buttonA);
	tb->addWidget(&_labelMA);
	tb->addSeparator();
	tb->addWidget(new QLabel("   "));
	tb->addWidget(&_buttonB);
	tb->addWidget(&_labelMB);
	tb->addSeparator();
	tb->addWidget(&_labelDeltaDescr);
	tb->addWidget(&_labelDelta);
}

void KsDualMarkerSM::updateMarkers(const KsDataStore &data, const KsTimeMap &histo)
{
	if (_markA.isSet()) {
		_markA.remove();
		if(_markA.reset(data, histo))
			_markA.draw();
	}

	if (_markB.isSet()) {
		_markB.remove();
		if(_markB.reset(data, histo))
			_markB.draw();
	}
}

void KsDualMarkerSM::updateLabels(const KsTimeMap &histo)
{
	// Marker A
	if (_markA.isSet()) {
		QString markA = QString::number(histo.binTime(_markA.bin()), 'f', 7);
		_labelMA.setText(QString("%1").arg(markA));
	} else {
		_labelMA.setText("");
	}

	// Marker B
	if (_markB.isSet()) {
		QString markB = QString::number(histo.binTime(_markB.bin()), 'f', 7);
		_labelMB.setText(QString("%1").arg(markB));
	} else {
		_labelMB.setText("");
	}

	// Delta
	if (_markA.isSet() && _markB.isSet()) {
		QString delta = QString::number(histo.binTime(_markB.bin()) -
						histo.binTime(_markA.bin()),
						'f', 7);
		_labelDelta.setText(QString("%1").arg(delta));
	} else {
		_labelDelta.setText("");
	}
}
