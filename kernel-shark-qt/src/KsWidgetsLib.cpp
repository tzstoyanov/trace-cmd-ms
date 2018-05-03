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

// Kernel Shark

// extern C
// {
// #include "event-parse.h"
// }

#include "KsWidgetsLib.hpp"
#include "libkshark.h"
#include "KsDeff.h"
#include "KsUtils.hpp"
#include "KsPlotTools.hpp"
#include "KsGLWidget.hpp"

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
	this->layout()->addWidget(new QLabel("Loading trace data ..."));
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
  _closeButton("Close", this)
{
	this->resize(SCREEN_WIDTH/10, FONT_HEIGHT*8);
	_layout.addWidget(&_text);
	_layout.addWidget(&_closeButton);
	connect(&_closeButton,	&QPushButton::pressed,
		this,		&QWidget::close);

	this->setLayout(&_layout);
}

KsCheckBoxWidget::KsCheckBoxWidget(const QString &n, bool cond, QWidget *parent)
: QWidget(parent),
  _positiveCond(cond),
  _allCb("all", this),
  _cbWidget(nullptr),
  _topLayout(this),
  _name(n),
  _nameLabel(n + ":  ")
{
	this->setWindowTitle(_name);
	this->setMinimumHeight(SCREEN_HEIGHT/2);

	connect(&_allCb,	&QCheckBox::clicked,
		this,		&KsCheckBoxWidget::chechAll);

	_cbWidget = new QWidget(this);
	_cbWidget->setLayout(&_cbLayout);

	QToolBar *tb = new QToolBar(this);
	tb->addWidget(&_nameLabel);
	tb->addWidget(&_allCb);
	_topLayout.addWidget(tb);

	_topLayout.addWidget(_cbWidget);
	_topLayout.setContentsMargins(0, 0, 0, 0);

	this->setLayout(&_topLayout);
	_allCb.setCheckState(Qt::Checked);
}

void KsCheckBoxWidget::setDefault(bool st)
{
	Qt::CheckState state = Qt::Unchecked;
	if (st)
		state = Qt::Checked;

	_allCb.setCheckState(state);
	chechAll(state);

	adjustSize();
}

KsCheckBoxDialog::KsCheckBoxDialog(KsCheckBoxWidget *cbw, QWidget *parent)
: QDialog(parent), _checkBoxWidget(cbw),
  _applyButton("Apply", this),
  _cancelButton("Cancel", this)
{
	this->setWindowTitle(cbw->name());
	_topLayout.addWidget(_checkBoxWidget);
	int buttonWidth = STRING_WIDTH("--Cancel--");
	_applyButton.setFixedWidth(buttonWidth);
	_cancelButton.setFixedWidth(buttonWidth);
	_buttonLayout.addWidget(&_applyButton);
	_applyButton.setAutoDefault(false);
	_buttonLayout.addWidget(&_cancelButton);
	_cancelButton.setAutoDefault(false);
	_buttonLayout.setAlignment(Qt::AlignLeft);
	_topLayout.addLayout(&_buttonLayout);

	_applyButtonConnection =
		connect(&_applyButton,	&QPushButton::pressed,
			this,		&KsCheckBoxDialog::applyPress);

	connect(&_applyButton,	&QPushButton::pressed,
		this,		&QWidget::close);

	connect(&_cancelButton,	&QPushButton::pressed,
		this,		&QWidget::close);

	this->setLayout(&_topLayout);
}

void KsCheckBoxDialog::applyPress()
{
	QVector<int> vec = _checkBoxWidget->getCheckedIds();
	emit apply(vec);

	/* Disconnect _applyButton. This is done in order to protect
	 * against multiple clicks. */
	disconnect(_applyButtonConnection);
}

KsCheckBoxTableWidget::KsCheckBoxTableWidget(const QString &name,
					     bool cond,
					     QWidget *parent)
: KsCheckBoxWidget(name, cond, parent),
  _table(this)
{
	connect(&_table,	&KsCheckBoxTable::changeState,
		this,		&KsCheckBoxTableWidget::changeState);
}

void KsCheckBoxTableWidget::set(QVector<bool> v)
{
	int n = (v.size() <=  _table._cb.size())? v.size() :_table. _cb.size();
	Qt::CheckState state;
	_allCb.setCheckState(Qt::Checked);
	for (int i = 0; i < n; ++i) {
		if (v[i]) {
			state = Qt::Checked;
		} else {
			state = Qt::Unchecked;
			_allCb.setCheckState(state);
		}
		_table._cb[i]->setCheckState(state);
	}
}

void KsCheckBoxTableWidget::initTable(QStringList headers, int size)
{
	_table.init(headers, size);

	for (auto const & cb: _table._cb) {
		connect(cb,	&QCheckBox::clicked,
			this,	&KsCheckBoxTableWidget::update);
	}

	_cbLayout.setContentsMargins(1, 1, 1, 1);
	_cbLayout.addWidget(&_table);
}

void KsCheckBoxTableWidget::adjustSize()
{
	_table.setVisible(false);
	_table.resizeColumnsToContents();
	_table.setVisible(true);

	int width = _table.horizontalHeader()->length() +
		    FONT_WIDTH*3 +
		    style()->pixelMetric(QStyle::PM_ScrollBarExtent);

	_cbWidget->resize(width, _cbWidget->height());

	setMinimumWidth(_cbWidget->width() +
			_cbLayout.contentsMargins().left() +
			_cbLayout.contentsMargins().right() +
			_topLayout.contentsMargins().left() +
			_topLayout.contentsMargins().right());
}

void  KsCheckBoxTableWidget::update(bool state)
{
	if (!state)
		_allCb.setCheckState(Qt::Unchecked);
}

QVector<int> KsCheckBoxTableWidget::getCheckedIds()
{
	QVector<int> vec;
	int n = _table._cb.size();
	for (int i = 0; i < n; ++i)
		if (_table._cb[i]->checkState() == Qt::Checked)
			vec.append(_id[i]);

	return vec;
}

void KsCheckBoxTableWidget::chechAll(bool st)
{
	Qt::CheckState state = Qt::Unchecked;
	if (st) state = Qt::Checked;

	for (auto &c: _table._cb) {
		c->setCheckState(state);
	}
}

void KsCheckBoxTableWidget::changeState(int row)
{
	if (_table._cb[row]->checkState() == Qt::Checked)
		_table._cb[row]->setCheckState(Qt::Unchecked);
	else
		_table._cb[row]->setCheckState(Qt::Checked);

	_allCb.setCheckState(Qt::Checked);
	for (auto &c: _table._cb) {
		if (c->checkState() == Qt::Unchecked) {
			_allCb.setCheckState(Qt::Unchecked);
			break;
		}
	}
}

static void update_r(QTreeWidgetItem *item, Qt::CheckState state)
{
	item->setCheckState(0, state);

	int n = item->childCount();
	for (int i = 0; i < n; ++i)
		update_r(item->child(i), state);
}

KsCheckBoxTree::KsCheckBoxTree(QWidget *parent)
: QTreeWidget(parent)
{
	setColumnCount(2);
	setHeaderHidden(true);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	connect(this, &KsCheckBoxTree::itemDoubleClicked,
		this, &KsCheckBoxTree::doubleClicked);
}

void KsCheckBoxTree::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Return) {
		/* Loop over all selected child items and change
		* there states. */
		for (auto &s: selectedItems()) {
			if(s->childCount()) {
				if (s->isExpanded())
					continue;
			}

			if (s->checkState(0) == Qt::Unchecked)
				s->setCheckState(0, Qt::Checked);
			else
				s->setCheckState(0, Qt::Unchecked);

			if(s->childCount()) {
				update_r(s, s->checkState(0));
			}
		}
	}

	emit verify();
	QTreeWidget::keyPressEvent(event);
}

void KsCheckBoxTree::doubleClicked(QTreeWidgetItem *item, int col)
{
	if (item->checkState(0) == Qt::Unchecked)
		item->setCheckState(0, Qt::Checked);
	else
		item->setCheckState(0, Qt::Unchecked);

	for (auto &i: selectedItems())
		i->setSelected(false);

	emit itemClicked(item, col);
}

void KsCheckBoxTree::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::RightButton) {
		for (auto &i: selectedItems())
			i->setSelected(false);

		return;
	}

	QApplication::processEvents();
	QTreeWidget::mousePressEvent(event);
}

KsCheckBoxTreeWidget::KsCheckBoxTreeWidget(const QString &name,
					   bool cond,
					   QWidget *parent)
: KsCheckBoxWidget(name, cond, parent),
  _tree(this)
{
	connect(&_tree,	&KsCheckBoxTree::verify,
		this,	&KsCheckBoxTreeWidget::verify);
}

void KsCheckBoxTreeWidget::set(QVector<bool> v)
{
	int n = (v.size() <=  _cb.size())? v.size() : _cb.size();
	Qt::CheckState state;
	_allCb.setCheckState(Qt::Checked);
	for (int i = 0; i < n; ++i) {
		if (v[i]) {
			state = Qt::Checked;
		} else {
			state = Qt::Unchecked;
			_allCb.setCheckState(state);
		}
		_cb[i]->setCheckState(0, state);
	}

	verify();
}

void KsCheckBoxTreeWidget::initTree()
{
// 	_tree.setSelectionMode(QAbstractItemView::SingleSelection);
	_tree.setSelectionMode(QAbstractItemView::MultiSelection);

	connect(&_tree, &QTreeWidget::itemClicked,
		this,	&KsCheckBoxTreeWidget::update);

	_cbLayout.setContentsMargins(1, 1, 1, 1);
	_cbLayout.addWidget(&_tree);
}

void KsCheckBoxTreeWidget::adjustSize()
{
	int width, n = _tree.topLevelItemCount();
	if (n == 0)
		return;

	for (int i = 0; i < n; ++i)
		_tree.topLevelItem(i)->setExpanded(true);

	_tree.resizeColumnToContents(0);
	if (_tree.topLevelItem(0)->child(0)) {
		width = _tree.visualItemRect(_tree.topLevelItem(0)->child(0)).width();
	} else {
		width = _tree.visualItemRect(_tree.topLevelItem(0)).width();
	}

	width += FONT_WIDTH*3 + style()->pixelMetric(QStyle::PM_ScrollBarExtent);
	_cbWidget->resize(width, _cbWidget->height());

	for (int i = 0; i < n; ++i)
		_tree.topLevelItem(i)->setExpanded(false);

	setMinimumWidth(_cbWidget->width() +
			_cbLayout.contentsMargins().left() +
			_cbLayout.contentsMargins().right() +
			_topLayout.contentsMargins().left() +
			_topLayout.contentsMargins().right());
}

void KsCheckBoxTreeWidget::update(QTreeWidgetItem *item, int column)
{
	/* Get the new state of the item. */
	Qt::CheckState state = item->checkState(0);

	/* Recursively update all items below this one. */
	update_r(item, state);

	/* Update all items above this one including the "all"
	 * check box. */
	verify();
}

void KsCheckBoxTreeWidget::chechAll(bool st)
{
	Qt::CheckState state = Qt::Unchecked;
	if (st)
		state = Qt::Checked;

	int n = _tree.topLevelItemCount();
	for (int i = 0; i < n; ++i)
		update_r(_tree.topLevelItem(i), state);
}

QVector<int> KsCheckBoxTreeWidget::getCheckedIds()
{
	QVector<int> vec;
	int n = _id.size();
	for (int i = 0; i < n; ++i)
		if (_cb[i]->checkState(0) == Qt::Checked)
			vec.append(_id[i]);

	return vec;
}

void KsCheckBoxTreeWidget::verify()
{
	/* Now set the state of the top level items according to the
	* state of the childs. */
	QTreeWidgetItem *topItem, *childItem;
	for(int t = 0; t < _tree.topLevelItemCount(); ++t) {
		topItem = _tree.topLevelItem(t);
		if (topItem->childCount() == 0)
			continue;

		topItem->setCheckState(0, Qt::Checked);
		for (int c = 0; c < topItem->childCount(); ++c) {
			childItem = topItem->child(c);
			if (childItem->checkState(0) == Qt::Unchecked)
				topItem->setCheckState(0, Qt::Unchecked);
		}
	}

	_allCb.setCheckState(Qt::Checked);
	for (auto &c: _cb) {
		if (c->checkState(0) == Qt::Unchecked) {
			_allCb.setCheckState(Qt::Unchecked);
			break;
		}
	}
}

KsCpuCheckBoxWidget::KsCpuCheckBoxWidget(struct pevent *pe, bool cond, QWidget *parent)
: KsCheckBoxTreeWidget("CPUs", cond, parent)
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
		cpuCol.setRainbowColor(i);
		QTreeWidgetItem *cpuItem = new QTreeWidgetItem;
		cpuItem->setText(0, "  ");
		cpuItem->setText(1, QString("CPU %1").arg(i));
		cpuItem->setCheckState(0, Qt::Checked);
		cpuItem->setBackgroundColor(0, QColor(cpuCol._r, cpuCol._g, cpuCol._b));
		_tree.addTopLevelItem(cpuItem);
		_id[i] = i;
		_cb[i] = cpuItem;
	}

	adjustSize();
}

KsEventsCheckBoxWidget::KsEventsCheckBoxWidget(struct pevent *pe,
					       bool cond,
					       QWidget *parent)
: KsCheckBoxTreeWidget("Events", cond, parent)
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
			evtItem->setFlags(evtItem->flags() | Qt::ItemIsUserCheckable);
			evtItem->setCheckState(0, Qt::Checked);
			sysItem->addChild(evtItem);
			_id[i] = pe->events[i]->id;
			_cb[i] = evtItem;
			if (++i == nEvts)
				break;
		}
	}

	_tree.sortItems(0, Qt::AscendingOrder);
	adjustSize();
}

KsTasksCheckBoxWidget::KsTasksCheckBoxWidget(struct pevent *pe,
					     bool cond,
					     QWidget *parent)
: KsCheckBoxTableWidget("Tasks", cond, parent)
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);
	int n = KsUtils::getPidList(&_id);

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

		pidCol.setRainbowColor(i);
	}

	adjustSize();
}

KsCheckBoxTable::KsCheckBoxTable(QWidget *parent)
: QTableWidget(parent)
{
	setShowGrid(false);
	horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
	horizontalHeader()->setStretchLastSection(true);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	verticalHeader()->setVisible(false);

	connect(this, &QTableWidget::cellDoubleClicked,
		this, &KsCheckBoxTable::doubleClicked);
}

void KsCheckBoxTable::init(QStringList headers, int size)
{
	setColumnCount(headers.count());
	setRowCount(size);
	setHorizontalHeaderLabels(headers);

	_cb.resize(size);

	for (int i = 0; i < size; ++i) {
		QWidget *cbWidget = new QWidget();
		_cb[i] = new QCheckBox(cbWidget);
		QHBoxLayout *cbLayout = new QHBoxLayout(cbWidget);
		cbLayout->addWidget(_cb[i]);
		cbLayout->setAlignment(Qt::AlignCenter);
		cbLayout->setContentsMargins(0, 0, 0, 0);

		cbWidget->setLayout(cbLayout);
		setCellWidget(i, 0, cbWidget);
	}
}

void KsCheckBoxTable::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Return) {
		for (auto &s: selectedItems()) {
			if (s->column() == 1)
				emit changeState(s->row());
		}
	}

	QApplication::processEvents();
	QTableWidget::keyPressEvent(event);
}

void KsCheckBoxTable::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::RightButton) {
		for (auto &i: selectedItems())
			i->setSelected(false);

		return;
	}

	QApplication::processEvents();
	QTableWidget::mousePressEvent(event);
}

void KsCheckBoxTable::doubleClicked(int row, int col)
{
	emit changeState(row);
	for (auto &i: selectedItems())
		i->setSelected(false);
}

KsPluginCheckBoxWidget::KsPluginCheckBoxWidget(QStringList pluginList,
					       QWidget *parent)
: KsCheckBoxTableWidget("Plugins", true, parent)
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

KsQuickFilterMenu::KsQuickFilterMenu(KsDataStore *data, size_t row, QWidget *parent)
: QMenu("Quick filter menu", parent),
  _data(data),
  _row(row),
  _hideTaskAction(this),
  _showTaskAction(this),
  _hideEventAction(this),
  _showEventAction(this),
  _addTaskPlotAction(this)
{
	addSection("Quick Filter menu");

	QString descr = "Hide task [";
	descr += kshark_get_task_lazy(_data->_rows[_row]);
	descr += "-";
	descr += QString("%1").arg(_data->_rows[_row]->pid);
	descr += "]";

	_hideTaskAction.setText(descr);
	connect(&_hideTaskAction, &QAction::triggered, this,
		&KsQuickFilterMenu::hideTask);
	addAction(&_hideTaskAction);

	descr = "Show task [";
	descr += kshark_get_task_lazy(_data->_rows[_row]);
	descr += "-";
	descr += QString("%1").arg(_data->_rows[_row]->pid);
	descr += "] only";
	_showTaskAction.setText(descr);
	connect(&_showTaskAction, &QAction::triggered, this,
		&KsQuickFilterMenu::showTask);
	addAction(&_showTaskAction);

	descr = "Hide event [";
	descr += kshark_get_event_name_lazy(_data->_rows[_row]);
	descr += "]";
	_hideEventAction.setText(descr);
	connect(&_hideEventAction, &QAction::triggered,
		this, &KsQuickFilterMenu::hideEvent);
	addAction(&_hideEventAction);

	descr = "Show event [";
	descr += kshark_get_event_name_lazy(_data->_rows[_row]);
	descr += "] only";
	_showEventAction.setText(descr);
	connect(&_showEventAction, &QAction::triggered,
		this, &KsQuickFilterMenu::showEvent);
	addAction(&_showEventAction);

	addSection("Quick Plot menu");
	descr = "Add [";
	descr += kshark_get_task_lazy(_data->_rows[_row]);
	descr += "-";
	descr += QString("%1").arg(_data->_rows[_row]->pid);
	descr += "] plot";
	_addTaskPlotAction.setText(descr);
	connect(&_addTaskPlotAction, &QAction::triggered,
		this, &KsQuickFilterMenu::addTaskPlot);
	addAction(&_addTaskPlotAction);
}

void KsQuickFilterMenu::hideTask()
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	int pid = _data->_rows[_row]->pid;
	filter_id_add(kshark_ctx->hide_task_filter, pid);
	kshark_filter_entries(kshark_ctx, _data->_rows, _data->size());

	emit _data->updateView(_data);
	emit _data->updateGraph(_data);
}

void KsQuickFilterMenu::showTask()
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	int pid = _data->_rows[_row]->pid;
	filter_id_add(kshark_ctx->hide_event_filter, pid);
	kshark_filter_entries(kshark_ctx, _data->_rows, _data->size());

	emit _data->updateView(_data);
	emit _data->updateGraph(_data);
}

void KsQuickFilterMenu::hideEvent()
{
	int eventId = _data->_rows[_row]->event_id;
	_data->applyNegEventFilter(QVector<int>(1, eventId));
}

void KsQuickFilterMenu::showEvent()
{
	int eventId = _data->_rows[_row]->event_id;
	_data->applyPosEventFilter(QVector<int>(1, eventId));
}

void KsQuickFilterMenu::addTaskPlot()
{
	int pid = _data->_rows[_row]->pid;
	emit plotTask(pid);
}
