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
#include "KsUtils.hpp"
#include "KsPlotTools.hpp"
#include "KsTraceGraph.hpp"

KsDataProgressBar::KsDataProgressBar(QWidget *parent)
: QWidget(parent), _sb(this), _pb(&_sb) {
	this->resize(FONT_WIDTH*45, FONT_HEIGHT*5);
	this->setWindowTitle("Loading data");
	this->setLayout(new QVBoxLayout);
	_pb.setOrientation(Qt::Horizontal);
	_pb.setTextVisible(false);
	_pb.setRange(0, 200);
	_pb.setValue(1);
	_sb.addPermanentWidget(&_pb, 1);
	this->layout()->addWidget(new QLabel("Loadong trace data ..."));
	this->layout()->addWidget(&_sb);
	setWindowFlags(Qt::WindowStaysOnTopHint);
	this->show();
}

void KsDataProgressBar::setValue(int i) {
	_pb.setValue(i);
	QApplication::processEvents();
}

KsMessageDialog::KsMessageDialog(QString message, QWidget *parent)
: QDialog(parent),
  _text(message, this),
  _close_button("Close", this)
{
	this->resize(SCREEN_WIDTH/10, FONT_HEIGHT*8);
	_layout.addWidget(&_text);
	_layout.addWidget(&_close_button);
	connect(&_close_button, SIGNAL(pressed()), this, SLOT(close()));

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

KsCheckBoxTableDialog::KsCheckBoxTableDialog(const QString &name,
					     bool cond,
					     QWidget *parent)
: KsCheckBoxDialog(name, cond, parent),
  _table(this)
{}


void KsCheckBoxTableDialog::set(QVector<bool> v)
{
	int n = (v.size() <=  _cb.size())? v.size() : _cb.size();
	Qt::CheckState state;
	_all_cb.setCheckState(Qt::Checked);
	for (int i = 0; i < n; ++i) {
		if (v[i]) {
			state = Qt::Checked;
		} else {
			state = Qt::Unchecked;
			_all_cb.setCheckState(state);
		}
		_cb[i]->setCheckState(state);
	}
}

void KsCheckBoxTableDialog::initTable(QStringList headers, int size)
{
	_table.setColumnCount(headers.count());
	_table.setRowCount(size);
	_table.setShowGrid(false);
	_table.setHorizontalHeaderLabels(headers);
	_table.horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
	_table.horizontalHeader()->setStretchLastSection(true);
	_table.setSelectionBehavior(QAbstractItemView::SelectRows);
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

	int height = _table.horizontalHeader()->height();
	if (_table.rowCount())
		height += _table.verticalHeader()->length() + FONT_HEIGHT;

	_cb_widget->resize(width, height);

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
	disconnect(&_apply_button, SIGNAL(pressed()), this, SLOT(applyPress()));
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

KsCheckBoxTreeDialog::KsCheckBoxTreeDialog(const QString &name,
					   bool cond,
					   QWidget *parent)
: KsCheckBoxDialog(name, cond, parent),
  _tree(this)
{}

void KsCheckBoxTreeDialog::set(QVector<bool> v)
{
	int n = (v.size() <=  _cb.size())? v.size() : _cb.size();
	Qt::CheckState state;
	_all_cb.setCheckState(Qt::Checked);
	for (int i = 0; i < n; ++i) {
		if (v[i]) {
			state = Qt::Checked;
		} else {
			state = Qt::Unchecked;
			_all_cb.setCheckState(state);
		}
		_cb[i]->setCheckState(0, state);
	}
}

void KsCheckBoxTreeDialog::initTree()
{
	_tree.setColumnCount(2);
	_tree.setHeaderHidden(true);
	_tree.setSelectionMode(QAbstractItemView::SingleSelection);
	_tree.setSelectionBehavior(QAbstractItemView::SelectRows);
	_tree.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	connect(&_tree, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
	this, SLOT(update(QTreeWidgetItem*, int)));

	_cb_layout->setContentsMargins(1, 1, 1, 1);
	_cb_layout->addWidget(&_tree);
}

void KsCheckBoxTreeDialog::adjustSize(int size)
{
	if (size == 0)
		return;

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

static void update_r(QTreeWidgetItem *item, Qt::CheckState state)
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
	disconnect(&_apply_button, SIGNAL(pressed()), this, SLOT(applyPress()));
	close();
}

KsCpuCheckBoxDialog::KsCpuCheckBoxDialog(struct pevent *pe, bool cond, QWidget *parent)
: KsCheckBoxTreeDialog("CPUs", cond, parent)
{
	int nCpus = 0;
	if (pe) {
		nCpus = pe->cpus;
		QString style = QString("QTreeView::item { height: %1 ;}").arg(FONT_HEIGHT*1.5);
		_tree.setStyleSheet(style);
		initTree();
	}

	_id.resize(nCpus);
	_cb.resize(nCpus);

	KsPlot::Color cpuCol;
	for (int i = 0; i < nCpus; ++i) {
		cpuCol.setRainbowsColor(1 + i*2);
		QTreeWidgetItem *cpuItem = new QTreeWidgetItem;
		cpuItem->setText(0, "  ");
		cpuItem->setText(1, QString("CPU %1").arg(i));
		cpuItem->setCheckState(0, Qt::Checked);
		cpuItem->setBackgroundColor(0, QColor(cpuCol._r, cpuCol._g, cpuCol._b));
		_tree.addTopLevelItem(cpuItem);
		_id[i] = i;
		_cb[i] = cpuItem;
	}

	adjustSize(nCpus);
}

KsEventsCheckBoxDialog::KsEventsCheckBoxDialog(struct pevent *pe,
					       bool cond,
					       QWidget *parent)
: KsCheckBoxTreeDialog("Events", cond, parent)
{
	int nEvts = 0;
	if (pe) {
		nEvts = pe->nr_events;
	}

	initTree();
	_id.resize(nEvts);
	_cb.resize(nEvts);

	int i(0);
	while (i < nEvts) {
		QString sysName(pe->events[i]->system);
		QTreeWidgetItem *sysItem = new QTreeWidgetItem;
		sysItem->setText(0, sysName);
		sysItem->setCheckState(0, Qt::Checked);
		_tree.addTopLevelItem(sysItem);

		while (sysName == pe->events[i]->system) {
			QTreeWidgetItem *evtItem = new QTreeWidgetItem;
			QString evtName(pe->events[i]->name);
			evtItem->setText(0, evtName);
// 			evtItem->setText(1, QString::number(pe->events[i]->id);
			evtItem->setFlags(evtItem->flags() | Qt::ItemIsUserCheckable);
			evtItem->setCheckState(0, Qt::Checked);
			sysItem->addChild(evtItem);
			_id[i] = pe->events[i]->id;
			_cb[i] = evtItem;
			if (++i == nEvts)
				break;
		}
	}

	adjustSize(nEvts);
}

int getPidList(QVector<int> *pids)
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	struct task_list *list;

	for (int i = 0; i < TASK_HASH_SIZE; ++i) {
		list = kshark_ctx->tasks[i];
		while (list) {
			pids->append(list->pid);
			list = list->next;
		}
	}

	qSort(*pids);
	return pids->size();
}

int getPluginList(QStringList *pl)
{
	*pl = plugins.split(";");
	return pl->count();
}

KsTasksCheckBoxDialog::KsTasksCheckBoxDialog(struct pevent *pe,
					     bool cond,
					     QWidget *parent)
: KsCheckBoxTableDialog("Tasks", cond, parent)
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);
	int n = getPidList(&_id);

	QStringList headers;
	if (_positiveCond)
		headers << "Show" << "Pid" << "Task";
	else
		headers << "Hide" << "Pid" << "Task";

	initTable(headers, n);

	KsPlot::Color pidCol;
	for (int i = 0; i < n; ++i) {
		QTableWidgetItem *pidItem
			= new QTableWidgetItem(tr("%1").arg(_id[i]));
		_table.setItem(i, 1, pidItem);
		QTableWidgetItem *comItem
			= new QTableWidgetItem(tr(kshark_get_comm_from_pid(kshark_ctx->pevt,
									   _id[i])));
		pidItem->setBackgroundColor(QColor(pidCol._r, pidCol._g, pidCol._b));
		if (_id[i] == 0)
			pidItem->setTextColor(Qt::white);
		_table.setItem(i, 2, comItem);
// 		++pidCol;
		pidCol.setRainbowsColor(i);
	}

	adjustSize();
}

KsPluginCheckBoxDialog::KsPluginCheckBoxDialog(struct pevent *pe,
					       QStringList pluginList,
					       bool cond,
					       QWidget *parent)
: KsCheckBoxTableDialog("Plugins", cond, parent)
{
	QStringList headers;
	headers << "Load" << "Name" << "Info";

	int n = pluginList.count();
	initTable(headers, n);
	_id.resize(n);

	for (int i = 0; i < n; ++i) {
		QTableWidgetItem *nameItem = new QTableWidgetItem(pluginList[i]);
		_table.setItem(i, 1, nameItem);
		QTableWidgetItem *infoItem = new QTableWidgetItem(" -- ");
		_table.setItem(i, 2, infoItem);
		_id[i] = i;
	}

	adjustSize();
}

KsDataStore::KsDataStore()
: _handle(nullptr), _dataSize(0), _rows(nullptr), _pevt(nullptr)
{}

KsDataStore::~KsDataStore()
{
	clear();
}

void KsDataStore::loadData(const QString &file)
{
	hd_time t0 = GET_TIME;

	clear();

	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	bool status = kshark_open(kshark_ctx, file.toStdString().c_str());
	if (!status)
		return;

	_handle = kshark_ctx->handle;
	_pevt = kshark_ctx->pevt;

	if (!_handle || !_pevt) {
		qCritical() << "ERROR Loading file " << file;
		return;
	}

	if (kshark_ctx->event_handlers == NULL)
		kshark_handle_plugins(kshark_ctx, KSHARK_PLUGIN_LOAD);
	else
		kshark_handle_plugins(kshark_ctx, KSHARK_PLUGIN_RELOAD);

	_dataSize = kshark_load_data_entries(kshark_ctx, &_rows);

	double time = GET_DURATION(t0);
	qInfo() <<"Data loading time: " << 1e3*time << " ms.   entries: " << _dataSize;
}

void KsDataStore::reload()
{
	hd_time t0 = GET_TIME;

	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	for (size_t r = 0; r < _dataSize; ++r)
		free(_rows[r]);

	free(_rows);
	_rows = nullptr;

	_dataSize = kshark_load_data_entries(kshark_ctx, &_rows);

	emit updateView(this);
	emit updateGraph(this);

	double time = GET_DURATION(t0);
	qInfo() <<"Data reloading time: " << 1e3*time << " ms.   entries: " << _dataSize;
}

void KsDataStore::clear()
{
	if (_dataSize) {
		for (size_t r = 0; r < _dataSize; ++r)
			//free_record(_rows[r]);
			free(_rows[r]);

		free(_rows);
		_rows = nullptr;
		_dataSize = 0;
	}

	_pevt = nullptr;

	if (_handle) {
		struct kshark_context *kshark_ctx = NULL;
		kshark_instance(&kshark_ctx);
		kshark_close(kshark_ctx);
		_handle = nullptr;
	}
}

void KsDataStore::applyPosTaskFilter(QVector<int> vec)
{
	hd_time t0 = GET_TIME;

	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	kshark_filter_clear(kshark_ctx, SHOW_TASK_FILTER);
	kshark_filter_clear(kshark_ctx, HIDE_TASK_FILTER);
	for (auto const &pid: vec)
		kshark_filter_add_id(kshark_ctx, SHOW_TASK_FILTER, pid);

	kshark_filter_entries(kshark_ctx, _rows, _dataSize);

	emit updateView(this);
	emit updateGraph(this);

	double time = GET_DURATION(t0);
	qInfo() <<"Filtering time (PTF): " << 1e3*time << " ms.";
}

void KsDataStore::applyNegTaskFilter(QVector<int> vec)
{
	hd_time t0 = GET_TIME;

	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	kshark_filter_clear(kshark_ctx, SHOW_TASK_FILTER);
	kshark_filter_clear(kshark_ctx, HIDE_TASK_FILTER);
	for (auto const &pid: vec)
		kshark_filter_add_id(kshark_ctx, HIDE_TASK_FILTER, pid);

	kshark_filter_entries(kshark_ctx, _rows, _dataSize);

	emit updateView(this);
	emit updateGraph(this);

	double time = GET_DURATION(t0);
	qInfo() <<"Filtering time (NTF): " << 1e3*time << " ms.";
}

void KsDataStore::applyPosEventFilter(QVector<int> vec)
{
	hd_time t0 = GET_TIME;

	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	kshark_filter_clear(kshark_ctx, SHOW_EVENT_FILTER);
	kshark_filter_clear(kshark_ctx, HIDE_EVENT_FILTER);

	for (auto const &pid: vec)
		kshark_filter_add_id(kshark_ctx, SHOW_EVENT_FILTER, pid);

	kshark_filter_entries(kshark_ctx, _rows, _dataSize);

	emit updateView(this);
	emit updateGraph(this);

	double time = GET_DURATION(t0);
	qInfo() <<"Filtering time (PEF): " << 1e3*time << " ms.";
}

void KsDataStore::applyNegEventFilter(QVector<int> vec)
{
	hd_time t0 = GET_TIME;

	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	kshark_filter_clear(kshark_ctx, SHOW_EVENT_FILTER);
	kshark_filter_clear(kshark_ctx, HIDE_EVENT_FILTER);
	for (auto const &pid: vec)
		kshark_filter_add_id(kshark_ctx, HIDE_EVENT_FILTER, pid);

	kshark_filter_entries(kshark_ctx, _rows, _dataSize);

	emit updateView(this);
	emit updateGraph(this);

	double time = GET_DURATION(t0);
	qInfo() <<"Filtering time (NEF): " << 1e3*time << " ms.";
}

KsPluginManager::KsPluginManager()
{
	int nPlugins = getPluginList(&_pluginList);
	_registeredPlugins.resize(nPlugins);

	for (int i = 0; i < nPlugins; ++i) {
		if (_pluginList[i].contains(" default", Qt::CaseInsensitive)) {
			_pluginList[i].remove(" default", Qt::CaseInsensitive);
			registerPlugin(_pluginList[i]);
		} else {
			_registeredPlugins[i] = false;
		}
	}
}

void KsPluginManager::registerPlugin(QString plugin)
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);
	char *lib;

	for (int i = 0; i < _pluginList.count(); ++i) {
		if (_pluginList[i] == plugin) {
			asprintf(&lib, "%s/lib/plugin-%s.so", KS_DIR,
							      plugin.toStdString().c_str());

			kshark_register_plugin(kshark_ctx, lib);
			_registeredPlugins[i] = true;
			return;
		} else if (plugin.contains("/lib/plugin-" +
			                   _pluginList[i], Qt::CaseInsensitive)) {
			asprintf(&lib, "%s", plugin.toStdString().c_str());

			kshark_register_plugin(kshark_ctx, lib);
			_registeredPlugins[i] = true;
			return;
		}
	}

	/** No plugin with this name in the list.
	 * Try to add it anyway.  */
	asprintf(&lib, "%s", plugin.toStdString().c_str());
	kshark_register_plugin(kshark_ctx, lib);
	_pluginList.append(plugin);
	_registeredPlugins.append(true);
}

void KsPluginManager::unregisterPlugin(QString plugin)
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	for (int i = 0; i < _pluginList.count(); ++i) {
		char *lib;
		if (_pluginList[i] == plugin) {
			asprintf(&lib, "%s/lib/plugin-%s.so", KS_DIR,
							      plugin.toStdString().c_str());

			kshark_unregister_plugin(kshark_ctx, lib);
			_registeredPlugins[i] = false;
			return;
		} else if  (plugin.contains("/lib/plugin-" +
			                   _pluginList[i], Qt::CaseInsensitive)) {
			asprintf(&lib, "%s", plugin.toStdString().c_str());

			kshark_unregister_plugin(kshark_ctx, lib);
			_registeredPlugins[i] = false;
			return;
		}
	}
}

void KsPluginManager::updatePlugins(QVector<int> pluginId)
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	kshark_handle_plugins(kshark_ctx, KSHARK_PLUGIN_UNLOAD);
	kshark_free_plugin_list(kshark_ctx->plugins);
	kshark_ctx->plugins = NULL;
	kshark_free_event_handler_list(kshark_ctx->event_handlers);

	for (auto &p: _registeredPlugins)
		p = false;

	for (auto const &p: pluginId)
		registerPlugin(_pluginList[p]);

	kshark_handle_plugins(kshark_ctx, KSHARK_PLUGIN_LOAD);

	emit dataReload();
}

KsGraphMark::KsGraphMark(DualMarkerState s)
: _state(s), _isSet(false), _bin(-1), _cpu(-1), _task(-1), _pos(0), _color(Qt::darkGreen)
{
	_mark = new KsPlot::Mark;
	_mark->_color = _color;
}

KsGraphMark::KsGraphMark(DualMarkerState s, QColor col)
: _state(s), _isSet(false), _bin(-1), _cpu(-1), _task(-1), _pos(0), _color(col)
{
	_mark = new KsPlot::Mark;
	_mark->_color = _color;
}

void KsGraphMark::reset()
{
	_isSet = false;
	_mark->_visible = false;
	_bin = -1;
	_pos = 0;
	_cpu = -1;
	_task = -1;
}

bool KsGraphMark::set(const KsDataStore &data,
		      const  KsTimeMap &histo,
		      size_t pos, int grCpu, int grTask)
{
	_isSet = true;
	_pos = pos;
	_cpu = grCpu;
	_task = grTask;
	size_t ts = data._rows[_pos]->ts;
	if (ts > histo._max || ts < histo._min) {
		_bin = -1;
		_mark->_visible = false;
		return false;
	}
	
	_bin = (ts - histo._min)/histo._binSize;
	return true;
}

bool KsGraphMark::update(const KsDataStore &data, const KsTimeMap &histo)
{
	if (!_isSet)
		return false;

	return set(data, histo, this->_pos, this->_cpu, this->_task);
}

bool KsGraphMark::isSet()
{
	return _isSet;
}

bool KsGraphMark::isVisible()
{
	return _mark->_visible;
}

void KsGraphMark::makeVisible(KsGLWidget *gl)
{
	_gl = gl;
	_mark->setMark(_bin, _cpu, _task, _gl, _gl->dpr());
	_mark->_visible = true;
}

void KsGraphMark::makeVisible()
{
	makeVisible(this->_gl);
}

void KsGraphMark::remove()
{
	_isSet = false;
	_mark->_visible = false;
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
  _markB(DualMarkerState::B, Qt::darkCyan),
  _scCtrlA(this),
  _scCtrlB(this)
{
	_buttonA.setFixedWidth(STRING_WIDTH(" Marker A "));
	_buttonB.setFixedWidth(STRING_WIDTH(" Marker B "));
// 	_buttonA.setShortcut(QKeySequence(Qt::CTRL + Qt::Key_A));
// 	_buttonB.setShortcut(QKeySequence(Qt::CTRL + Qt::Key_B));

	for (auto const &l: {&_labelMA, &_labelMB, &_labelDelta}) {
		l->setFrameStyle(QFrame::Panel | QFrame::Sunken);
		l->setStyleSheet("QLabel {background-color : white;}");
		l->setTextInteractionFlags(Qt::TextSelectableByMouse);
// 		l->setFixedHeight(FONT_HEIGHT*1.5);
		l->setFixedWidth(FONT_WIDTH*16);
	}

	QString styleSheetA = "background : " +
			      _markA.color().name() +
			      "; color : white";

	_stateA = new QState;
	_stateA->setObjectName("A");
	_stateA->assignProperty(&_buttonA,
				"styleSheet",
				styleSheetA);

	_stateA->assignProperty(&_buttonB,
				"styleSheet",
				"color : rgb(70, 70, 70)");

	QString styleSheetB = "background : " +
			      _markB.color().name() +
			      "; color : white";
	_stateB = new QState;
	_stateB->setObjectName("B");
	_stateB->assignProperty(&_buttonA,
				"styleSheet",
				"color : rgb(70, 70, 70)");

	_stateB->assignProperty(&_buttonB,
				"styleSheet",
				styleSheetB);

	_scCtrlA.setKey(Qt::CTRL + Qt::Key_A);
	_scCtrlB.setKey(Qt::CTRL + Qt::Key_B);
	_stateA->addTransition(&_scCtrlB, SIGNAL(activated()),_stateB);
	_stateB->addTransition(&_scCtrlA, SIGNAL(activated()),_stateA);

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

void KsDualMarkerSM::reset()
{
	_markA.reset();
	_markB.reset();
	_labelMA.setText("");
	_labelMB.setText(""); 
	_labelDelta.setText("");
}

void KsDualMarkerSM::setStateA()
{
	if (_markState !=  DualMarkerState::A) {
		_markState = DualMarkerState::A;
		emit markSwitch();
	} else if (activeMarker().isSet()) {
		emit updateView(activeMarker().row(), true);
		emit showInGraph(activeMarker().row());
	}
}

void KsDualMarkerSM::setStateB()
{
	if (_markState !=  DualMarkerState::B) {
		_markState = DualMarkerState::B;
		emit markSwitch();
	} else if (activeMarker().isSet()) {
		emit updateView(activeMarker().row(), true);
		emit showInGraph(activeMarker().row());
	}
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
	if(_markA.update(data, histo))
		_markA.makeVisible();

	if(_markB.update(data, histo))
		_markB.makeVisible();
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
