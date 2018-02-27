/*
 * Copyright (C) 2017 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
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

// C
#include <sys/stat.h>
#include <unistd.h>

// C++11
#include <thread>

// Qt
#include <QMenu>
#include <QFileDialog>
#include <QMenuBar>
#include <QLabel>

// Kernel Shark 2
#include "KsMainWindow.hpp"
#include "KsCaptureDialog.hpp"
#include "libkshark.h"
#include "KsDeff.h"

KsMainWindow::KsMainWindow(QWidget *parent)
: QMainWindow(parent),
  _view(this),
  _graph(this),
  _mState(this),
  _openAction(tr("Open"), parent),
  _importFilterAction(tr("Import Filter"), parent),
  _saveFilterAction(tr("Save Filter"), parent),
  _saveFilterAsAction(tr("Save Filter As"), parent),
  _exportFilterAction(tr("Export Filter"), parent),
  _quitAction(tr("Quit"), parent),
  _graphFilterSyncAction(/*tr("Apply filters to Graph"),*/ parent),
  _listFilterSyncAction(/*tr("Apply filters to List"),*/ parent),
  _showEventsAction(tr("Show events"), parent),
  _showTasksAction(tr("Show tasks"), parent),
  _hideTasksAction(tr("Hide tasks"), parent),
  _cpuSelectAction(tr("CPUs"), parent),
  _taskSelectAction(tr("Tasks"), parent),
  _pluginsAction("Plugins", parent),
  _captureAction("Capture", parent),
  _aboutAction(tr("About"), parent),
  _contentsAction(tr("Contents"), parent)
{
	setWindowTitle("Kernel Shark");
	createActions();
	createMenus();

	QSplitter *splitter = new QSplitter(Qt::Vertical);
	splitter->addWidget(&_graph);
	splitter->addWidget(&_view);
	setCentralWidget(splitter);

	_view.setMarkerSM(&_mState);
	connect(&_mState, SIGNAL(markSwitch()), &_view, SLOT(markSwitch()));

	_graph.setMarkerSM(&_mState);
	_mState.markerA().setGLWidget(_graph.glPtr());
	_mState.markerB().setGLWidget(_graph.glPtr());

	connect(&_mState, SIGNAL(showInGraph(size_t)),
		&_graph, SLOT(markEntry(size_t)));

	connect(&_mState, SIGNAL(updateView(size_t, bool)),
		&_view, SLOT(showRow(size_t, bool)));

	connect(&_view, SIGNAL(select(size_t)),
		&_graph, SLOT(markEntry(size_t)));

	connect(_graph.glPtr(), SIGNAL(updateView(size_t, bool)),
		&_view, SLOT(showRow(size_t, bool)));

	connect(_graph.glPtr(), SIGNAL(deselect()),
		&_view, SLOT(deselect()));

	connect(&_data, SIGNAL(updateView(KsDataStore *)),
		&_view, SLOT(update(KsDataStore *)));

	connect(&_data, SIGNAL(updateGraph(KsDataStore *)),
		&_graph, SLOT(update(KsDataStore *)));
	
	connect(&_plugins, SIGNAL(dataReload()),
		&_data, SLOT(reload()));

	this->resize(SCREEN_WIDTH*.4, FONT_HEIGHT*3);
}

KsMainWindow::~KsMainWindow()
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);
	kshark_close(kshark_ctx);
	kshark_free(kshark_ctx);
}

void KsMainWindow::resizeEvent(QResizeEvent* event)
{
	QMainWindow::resizeEvent(event);
	_graph.resize(this->width(), _graph.height());
}

void KsMainWindow::createActions()
{
	/* File menu */
	_openAction.setIcon(QIcon::fromTheme("document-open"));
	_openAction.setShortcut(tr("Ctrl+O"));
	_openAction.setStatusTip(tr("Open an existing data file"));
	connect(&_openAction, SIGNAL(triggered()), this, SLOT(open()));

	_importFilterAction.setIcon(QIcon::fromTheme("document-revert"));
	_importFilterAction.setShortcut(tr("Ctrl+L"));
	_importFilterAction.setStatusTip(tr("Load a filter"));
	connect(&_importFilterAction, SIGNAL(triggered()), this, SLOT(importFilter()));

	_saveFilterAction.setIcon(QIcon::fromTheme("document-save"));
	_saveFilterAction.setShortcut(tr("Ctrl+S"));
	_saveFilterAction.setStatusTip(tr("Save a filter"));
	connect(&_saveFilterAction, SIGNAL(triggered()), this, SLOT(saveFilter()));

	_saveFilterAsAction.setIcon(QIcon::fromTheme("document-save-as"));
	_saveFilterAsAction.setShortcut(tr("Shift+Ctrl+S"));
	_saveFilterAsAction.setStatusTip(tr("Save a filter as"));
	connect(&_saveFilterAsAction, SIGNAL(triggered()), this, SLOT(saveFilter()));

	_exportFilterAction.setIcon(QIcon::fromTheme("document-send"));
	_exportFilterAction.setShortcut(tr("Ctrl+L"));
	_exportFilterAction.setStatusTip(tr("Export a filter"));
	connect(&_exportFilterAction, SIGNAL(triggered()), this, SLOT(saveFilter()));

	_quitAction.setIcon(QIcon::fromTheme("window-close"));
	_quitAction.setShortcut(tr("Ctrl+Q"));
	_quitAction.setStatusTip(tr("Exit KernelShark"));
	connect(&_quitAction, SIGNAL(triggered()), this, SLOT(close()));

	/* Filter menu */
	connect(&_listFilterSyncAction, SIGNAL(toggled(bool)), this, SLOT(listFilterSync(bool)));
	connect(&_showEventsAction, SIGNAL(triggered()), this, SLOT(showEvents()));
	connect(&_showTasksAction, SIGNAL(triggered()), this, SLOT(showTasks()));
	connect(&_hideTasksAction, SIGNAL(triggered()), this, SLOT(hideTasks()));

	/* Plot menu */
	connect(&_cpuSelectAction, SIGNAL(triggered()), this, SLOT(cpuSelect()));
	connect(&_taskSelectAction, SIGNAL(triggered()), this, SLOT(taskSelect()));

	/* Tools menu */
	_pluginsAction.setIcon(QIcon::fromTheme("insert-image"));
	_pluginsAction.setShortcut(tr("Ctrl+P"));
	_pluginsAction.setStatusTip(tr("Manage plugins"));
	connect(&_pluginsAction, SIGNAL(triggered()), this, SLOT(pluginSelect()));

	_captureAction.setIcon(QIcon::fromTheme("media-record"));
	_captureAction.setShortcut(tr("Ctrl+C"));
	_captureAction.setStatusTip(tr("Capture trace data"));
	connect(&_captureAction, SIGNAL(triggered()), this, SLOT(capture()));

	/* Help menu */
	_aboutAction.setIcon(QIcon::fromTheme("help-about"));
	connect(&_aboutAction, SIGNAL(triggered()), this, SLOT(aboutInfo()));

	_contentsAction.setIcon(QIcon::fromTheme("help-contents"));
	connect(&_contentsAction, SIGNAL(triggered()), this, SLOT(contents()));

	//connect(QApplication::desktop(), SIGNAL(resized(int)), this, SLOT(reset(int));
}

void KsMainWindow::createMenus()
{
	/* File menu */
	QMenu *file = menuBar()->addMenu("File");
	file->addAction(&_openAction);
	file->addAction(&_importFilterAction);
	file->addAction(&_saveFilterAction);
	file->addAction(&_saveFilterAsAction);
	file->addAction(&_exportFilterAction);
	file->addAction(&_quitAction);

	/* Filter menu */
	QMenu *filter = menuBar()->addMenu("Filter");

	auto make_cb_action = [&] (QWidgetAction *action, QString name)
	{
		QWidget  *containerWidget = new QWidget(filter);
		containerWidget->setLayout(new QHBoxLayout());
		containerWidget->layout()->setContentsMargins(FONT_WIDTH, FONT_HEIGHT/5,
							      FONT_WIDTH, FONT_HEIGHT/5);
		QCheckBox *checkBox = new QCheckBox(name, filter);
		checkBox->setChecked(true);
		connect(checkBox, SIGNAL(toggled(bool)), this, SLOT(graphFilterSync(bool)));
		containerWidget->layout()->addWidget(checkBox);
		action->setDefaultWidget(containerWidget);
	};

	make_cb_action(&_graphFilterSyncAction, tr("Apply filters to Graph"));
	make_cb_action(&_listFilterSyncAction, tr("Apply filters to List"));

	filter->addAction(&_graphFilterSyncAction);
	filter->addAction(&_listFilterSyncAction);
	filter->addAction(&_showEventsAction);
	filter->addAction(&_showTasksAction);
	filter->addAction(&_hideTasksAction);

	/* Plot menu */
	QMenu *plots = menuBar()->addMenu("Plots");
	plots->addAction(&_cpuSelectAction);
	plots->addAction(&_taskSelectAction);

	/* Capture menu */
	QMenu *tools = menuBar()->addMenu("Tools");
	tools->addAction(&_pluginsAction);
	tools->addAction(&_captureAction);
	

	/* Help menu */
	QMenu *help = menuBar()->addMenu("Help");
	help->addAction(&_aboutAction);
	help->addAction(&_contentsAction);
}

void KsMainWindow::open()
{
	QFileDialog* fileDlg = new QFileDialog(this,
					       "Open File",
					       KS_DIR,
					       "trace-cmd files (*.dat);;All files (*)");

	int status = fileDlg->exec();
	QApplication::processEvents();

	QString fileName = fileDlg->selectedFiles().first();

	if (status && !fileName.isEmpty())
		loadFile(fileName);
}

void KsMainWindow::importFilter()
{
	// TODO
}

void KsMainWindow::saveFilter()
{
	// TODO
}

void KsMainWindow::reload()
{
	// TODO
}

void KsMainWindow::listFilterSync(bool state)
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);
	if (state) {
		kshark_ctx->filter_mask &= ~KS_VIEW_FILTER_MASK;
	} else {
		kshark_ctx->filter_mask |= KS_VIEW_FILTER_MASK;
	}
}

void KsMainWindow::graphFilterSync(bool state)
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);
	if (state) {
		kshark_ctx->filter_mask &= ~KS_GRAPH_FILTER_MASK;
	} else {
		kshark_ctx->filter_mask |= KS_GRAPH_FILTER_MASK;
	}
}

void KsMainWindow::showEvents()
{
	KsCheckBoxDialog *events_cb = new KsEventsCheckBoxDialog(_data._pevt, true, this);
	events_cb->setDefault(true);

	connect(events_cb, SIGNAL(apply(QVector<int>)),
		&_data, SLOT(applyPosEventFilter(QVector<int>)));
}

void KsMainWindow::showTasks()
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	KsCheckBoxDialog *tasks_cbd = new KsTasksCheckBoxDialog(_data._pevt, true, this);
	if (!kshark_ctx->show_task_filter ||
	    !filter_id_count(kshark_ctx->show_task_filter)) {
		tasks_cbd->setDefault(true);
	} else {
		QVector<int> pids;
		int nPids = getPidList(&pids);
		QVector<bool> v(nPids, false);
		for (int i = 0; i < nPids; ++i) {
			if (kshark_filter_id_find_pid(kshark_ctx->show_task_filter,
						      pids[i]))
				v[i] = true;
		}

		tasks_cbd->set(v);
	}

	connect(tasks_cbd, SIGNAL(apply(QVector<int>)),
		&_data, SLOT(applyPosTaskFilter(QVector<int>)));
}

void KsMainWindow::hideTasks()
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	KsCheckBoxDialog *tasks_cbd = new KsTasksCheckBoxDialog(_data._pevt, false, this);
	if (!kshark_ctx->hide_task_filter ||
	    !filter_id_count(kshark_ctx->hide_task_filter)) {
		tasks_cbd->setDefault(false);
	} else {
		QVector<int> pids;
		int nPids = getPidList(&pids);
		QVector<bool> v(nPids, false);
		for (int i = 0; i < nPids; ++i) {
			if (kshark_filter_id_find_pid(kshark_ctx->hide_task_filter,
						      pids[i]))
				v[i] = true;
		}

		tasks_cbd->set(v);
	}

	connect(tasks_cbd, SIGNAL(apply(QVector<int>)),
		&_data, SLOT(applyNegTaskFilter(QVector<int>)));
}

void KsMainWindow::cpuSelect()
{
	KsCheckBoxDialog *cpus_cbd = new KsCpuCheckBoxDialog(_data._pevt, true, this);
	if(!_data._pevt)
		return;

	int nCpus = _data._pevt->cpus;
	if (nCpus == _graph.glPtr()->cpuGraphCount()) {
		cpus_cbd->setDefault(true);
	} else {
		QVector<bool> v(nCpus, false);
		for (auto const &cpu: _graph.glPtr()->_cpuList)
			v[cpu] = true;

		cpus_cbd->set(v);
	}

	connect(cpus_cbd, SIGNAL(apply(QVector<int>)),
		&_graph, SLOT(cpuReDraw(QVector<int>)));
}

void KsMainWindow::taskSelect()
{
	KsCheckBoxDialog *tasks_cbd = new KsTasksCheckBoxDialog(_data._pevt, true, this);

	QVector<int> pids;
	int nPids = getPidList(&pids);
	if (nPids == _graph.glPtr()->taskGraphCount()) {
		tasks_cbd->setDefault(true);
	} else {
		QVector<bool> v(nPids, false);
		for (int i = 0; i < nPids; ++i) {
			for (auto const &pid: _graph.glPtr()->_taskList) {
				if (pids[i] == pid) {
					v[i] = true;
					break;
				}
			}
		}

		tasks_cbd->set(v);
	}

	connect(tasks_cbd, SIGNAL(apply(QVector<int>)),
		&_graph, SLOT(taskReDraw(QVector<int>)));
}

void KsMainWindow::pluginSelect()
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	KsCheckBoxDialog *plugin_cbd = new KsPluginCheckBoxDialog(_data._pevt,
								  _plugins._pluginList,
								  true,
								  this);

	plugin_cbd->set(_plugins._registeredPlugins);

	connect(plugin_cbd, SIGNAL(apply(QVector<int>)),
		&_plugins, SLOT(updatePlugins(QVector<int>)));
}

void KsMainWindow::capture()
{
	qInfo() << "capture";
	new KsCaptureDialog(this);
}

void KsMainWindow::aboutInfo() {
	QString text;
	text.append(" KernelShark\n\n version: ");
	text.append(KS_VERSION_STRING);
	text.append("\n");

	KsMessageDialog *message = new KsMessageDialog(text);
	message->setWindowTitle("About");
	message->show();
}

void KsMainWindow::contents()
{
	QDesktopServices::openUrl(QUrl("https://www.google.bg/search?q=kernelshark",
				  QUrl::TolerantMode));
}

void KsMainWindow::loadFile(const QString& fileName)
{
	qInfo() << "Loading " << fileName;
	_mState.reset();
	_view.reset();
	_graph.reset();

	setWindowTitle("Kernel Shark");
	KsDataProgressBar pb;
	QApplication::processEvents();

	struct stat st;
	int ret = stat(fileName.toStdString().c_str(), &st);
	if (ret != 0) {
		this->resize(SCREEN_WIDTH*.4, FONT_HEIGHT*3);
		QString text("Unable to find file ");
		text.append(fileName);
		text.append("\n");
		KsMessageDialog *message = new KsMessageDialog(text);
		message->setWindowFlags(Qt::WindowStaysOnTopHint);
		message->setMinimumWidth(STRING_WIDTH(text) + FONT_WIDTH*3);
		message->setWindowTitle("ERROR");
		message->show();
		qCritical() << "ERROR: " << text;
		return;
	}

	bool loadDone = false;
	auto job = [&] (KsDataStore *d) {
		d->loadData(fileName);
		loadDone = true;
	};
	std::thread t1(job, &_data);

	for (int i = 0; i < 160; ++i) {
		if (loadDone)
			break;

		pb.setValue(i);
		usleep(150000);
		QApplication::processEvents();
	}

	t1.join();

	if (!_data.size()) {
		this->resize(SCREEN_WIDTH*.4, FONT_HEIGHT*3);
		QString text("File ");
		text.append(fileName);
		text.append(" contains no data.\n");
		KsMessageDialog *message = new KsMessageDialog(text);
		message->setWindowFlags(Qt::WindowStaysOnTopHint);
		message->setMinimumWidth(STRING_WIDTH(text) + FONT_WIDTH*3);
		message->setWindowTitle("ERROR");
		message->show();
		qCritical() << "ERROR: " << text;
		return;
	}

	pb.setValue(165);
	_view.loadData(&_data);

	//auto viewJob = [&] {_view.loadData(&_data);};
	//std::thread t1(viewJob);

	pb.setValue(180);
	_graph.loadData(&_data);
	pb.setValue(195);
	setWindowTitle("Kernel Shark (" + fileName + ")");

// 	t1.join();
}

// void KsMainWindow::loadFiles(const QList<QString> &files)
// {}