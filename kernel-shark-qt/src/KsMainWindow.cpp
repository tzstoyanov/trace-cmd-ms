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
#include <QLocalSocket>

// Kernel Shark 2
#include "KsMainWindow.hpp"
#include "libkshark.h"
#include "libkshark-json.h"
#include "KsDeff.h"
#include "KsCaptureDialog.hpp"

KsMainWindow::KsMainWindow(QWidget *parent)
: QMainWindow(parent),
  _data(this),
  _view(this),
  _graph(this),
  _mState(this),
  _plugins(this),
  _capture(this),
  _captureLocalServer(this),
  _openAction("Open", parent),
  _importFilterAction("Import Filter", parent),
  _exportFilterAction("Export Filter", parent),
  _quitAction("Quit", parent),
  _graphFilterSyncAction(/*"Apply filters to Graph",*/ parent),
  _listFilterSyncAction(/*"Apply filters to List",*/ parent),
  _showEventsAction("Show events", parent),
  _showTasksAction("Show tasks", parent),
  _hideTasksAction("Hide tasks", parent),
  _cpuSelectAction("CPUs", parent),
  _taskSelectAction("Tasks", parent),
  _pluginsAction("Plugins", parent),
  _captureAction("Record", parent),
  _colorAction(parent),
  _colorPhaseSlider(Qt::Horizontal, this),
  _aboutAction("About", parent),
  _contentsAction("Contents", parent)
{
	setWindowTitle("Kernel Shark");
	createActions();
	createMenus();
	initCapture();

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
	_openAction.setStatusTip("Open an existing data file");
	connect(&_openAction, SIGNAL(triggered()), this, SLOT(open()));

	_importFilterAction.setIcon(QIcon::fromTheme("document-revert"));
	_importFilterAction.setShortcut(tr("Ctrl+L"));
	_importFilterAction.setStatusTip("Load a filter");
	connect(&_importFilterAction, SIGNAL(triggered()), this, SLOT(importFilter()));

	_exportFilterAction.setIcon(QIcon::fromTheme("document-send"));
	_exportFilterAction.setShortcut(tr("Ctrl+L"));
	_exportFilterAction.setStatusTip("Export a filter");
	connect(&_exportFilterAction, SIGNAL(triggered()), this, SLOT(exportFilter()));

	_quitAction.setIcon(QIcon::fromTheme("window-close"));
	_quitAction.setShortcut(tr("Ctrl+Q"));
	_quitAction.setStatusTip("Exit KernelShark");
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
	_pluginsAction.setStatusTip("Manage plugins");
	connect(&_pluginsAction, SIGNAL(triggered()), this, SLOT(pluginSelect()));

	_captureAction.setIcon(QIcon::fromTheme("media-record"));
	_captureAction.setShortcut(tr("Ctrl+C"));
	_captureAction.setStatusTip("Capture trace data");
	connect(&_captureAction, SIGNAL(triggered()), this, SLOT(capture()));

	_colorPhaseSlider.setMinimum(20);
	_colorPhaseSlider.setMaximum(180);
	_colorPhaseSlider.setValue(75);
	_colorPhaseSlider.setFixedWidth(FONT_WIDTH*15);
	connect(&_colorPhaseSlider, SIGNAL(valueChanged(int)), this, SLOT(setColorPhase(int)));

	QWidget *colSlider = new QWidget(this);
	colSlider->setLayout(new QHBoxLayout);
	QLabel *colLabel = new QLabel("Color scheme", this);
	colSlider->layout()->addWidget(colLabel);
	colSlider->layout()->addWidget(&_colorPhaseSlider);
	_colorAction.setDefaultWidget(colSlider);

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

	make_cb_action(&_graphFilterSyncAction, "Apply filters to Graph");
	make_cb_action(&_listFilterSyncAction, "Apply filters to List");

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
	tools->addAction(&_colorAction);

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

void KsMainWindow::open(QString fileName)
{
	loadFile(fileName);
}

void KsMainWindow::importFilter()
{
	QString fileName =
		QFileDialog::getOpenFileName(this,
					     "Import Filter",
					     KS_DIR,
					     "Kernel Shark Config files (*.json)");

	if (fileName.isEmpty())
		return;

	struct json_object *jobj =
		kshark_open_json_file(fileName.toStdString().c_str(),
				     "kshark.filter.config");

	if (!jobj)
		return;

	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);
	kshark_all_filters_from_json(kshark_ctx, jobj);
	json_object_put(jobj);

	kshark_filter_entries(kshark_ctx, _data._rows, _data.size());
	emit _data.updateView(&_data);
	emit _data.updateGraph(&_data);
}

void KsMainWindow::exportFilter()
{
	QString fileName =
		QFileDialog::getSaveFileName(this,
					     "Export Filter",
					     KS_DIR,
					     "Kernel Shark Config files (*.json);");

	if (fileName.isEmpty())
		return;

	if (!fileName.endsWith(".json"))
		fileName += ".json";

	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	struct json_object *jobj = kshark_all_filters_to_json(kshark_ctx);
	kshark_save_json_file(fileName.toStdString().c_str(), jobj);
	json_object_put(jobj);
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
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	KsCheckBoxWidget *events_cb = new KsEventsCheckBoxWidget(_data._pevt, true, this);
	if (!kshark_ctx->show_event_filter ||
	    !filter_id_count(kshark_ctx->show_event_filter)) {
		events_cb->setDefault(true);
	} else {
		int nEvts = _data._pevt->nr_events;
		QVector<bool> v(nEvts, false);
		for (int i = 0; i < nEvts; ++i) {
			if (filter_id_find(kshark_ctx->show_event_filter,
					   _data._pevt->events[i]->id))
				v[i] = true;
		}

		events_cb->set(v);
	}

	KsCheckBoxDialog *dialog = new KsCheckBoxDialog(events_cb, this);
	connect(dialog, SIGNAL(apply(QVector<int>)),
		&_data, SLOT(applyPosEventFilter(QVector<int>)));

	dialog->show();
}

void KsMainWindow::showTasks()
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	KsCheckBoxWidget *tasks_cbd = new KsTasksCheckBoxWidget(_data._pevt, true, this);
	if (!kshark_ctx->show_task_filter ||
	    !filter_id_count(kshark_ctx->show_task_filter)) {
		tasks_cbd->setDefault(true);
	} else {
		QVector<int> pids;
		int nPids = getPidList(&pids);
		QVector<bool> v(nPids, false);
		for (int i = 0; i < nPids; ++i) {
			if (filter_id_find(kshark_ctx->show_task_filter,
					   pids[i]))
				v[i] = true;
		}

		tasks_cbd->set(v);
	}

	KsCheckBoxDialog *dialog = new KsCheckBoxDialog(tasks_cbd, this);
	connect(dialog, SIGNAL(apply(QVector<int>)),
		&_data, SLOT(applyPosTaskFilter(QVector<int>)));

	dialog->show();
}

void KsMainWindow::hideTasks()
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	KsCheckBoxWidget *tasks_cbd = new KsTasksCheckBoxWidget(_data._pevt, false, this);
	if (!kshark_ctx->hide_task_filter ||
	    !filter_id_count(kshark_ctx->hide_task_filter)) {
		tasks_cbd->setDefault(false);
	} else {
		QVector<int> pids;
		int nPids = getPidList(&pids);
		QVector<bool> v(nPids, false);
		for (int i = 0; i < nPids; ++i) {
			if (filter_id_find(kshark_ctx->hide_task_filter,
					   pids[i]))
				v[i] = true;
		}

		tasks_cbd->set(v);
	}

	KsCheckBoxDialog *dialog = new KsCheckBoxDialog(tasks_cbd, this);
	connect(dialog, SIGNAL(apply(QVector<int>)),
		&_data, SLOT(applyNegTaskFilter(QVector<int>)));

	dialog->show();
}

void KsMainWindow::cpuSelect()
{
	KsCheckBoxWidget *cpus_cbd = new KsCpuCheckBoxWidget(_data._pevt, true, this);
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

	KsCheckBoxDialog *dialog = new KsCheckBoxDialog(cpus_cbd, this);
	connect(dialog, SIGNAL(apply(QVector<int>)),
		&_graph, SLOT(cpuReDraw(QVector<int>)));

	dialog->show();
}

void KsMainWindow::taskSelect()
{
	KsCheckBoxWidget *tasks_cbd = new KsTasksCheckBoxWidget(_data._pevt, true, this);

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

	KsCheckBoxDialog *dialog = new KsCheckBoxDialog(tasks_cbd, this);
	connect(dialog, SIGNAL(apply(QVector<int>)),
		&_graph, SLOT(taskReDraw(QVector<int>)));

	dialog->show();
}

void KsMainWindow::pluginSelect()
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	KsCheckBoxWidget *plugin_cbd
		= new KsPluginCheckBoxWidget(_data._pevt,
					     _plugins._pluginList,
					     true,
					     this);

	plugin_cbd->set(_plugins._registeredPlugins);

	KsCheckBoxDialog *dialog = new KsCheckBoxDialog(plugin_cbd, this);
	connect(dialog, SIGNAL(apply(QVector<int>)),
		&_plugins, SLOT(updatePlugins(QVector<int>)));

	dialog->show();
}

void KsMainWindow::capture()
{
	QString distribLinux(LSB_DISTRIB);
	QString desktop(DESKTOP_SESSION);

	auto capture_error = [&] {
		QStringList message;
		message << "Record is currently not supported fot your distribution"
			<< ", identified as " << LSB_DISTRIB 
			<< "(" << DESKTOP_SESSION << ")";
		QErrorMessage *em = new QErrorMessage(this);
		em->showMessage(message.join(" "), "captureErr");
	};

#ifndef DO_AS_ROOT
	capture_error();
	return;
#endif

	if (distribLinux == "Fedora" && desktop.contains("gnome")) {
		capture_error();
		return;
	}

	_capture.start();
// 	_capture.waitForFinished();
}

void KsMainWindow::setColorPhase(int f)
{
	KsPlot::Color::setRainbowFrequency(f/100.);
	_graph.glPtr()->model()->update();
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
		QErrorMessage *em = new QErrorMessage(this);
		em->showMessage(text, "loadFileErr1");
		qCritical() << "ERROR: " << text;
		return;
	}

	bool loadDone = false;
	auto loadJob = [&] (KsDataStore *d) {
		d->loadData(fileName);
		loadDone = true;
	};

	std::thread tload(loadJob, &_data);

	for (int i = 0; i < 160; ++i) {
		if (loadDone)
			break;

		pb.setValue(i);
		usleep(150000);
		QApplication::processEvents();
	}

	tload.join();

	if (!_data.size()) {
		this->resize(SCREEN_WIDTH*.4, FONT_HEIGHT*3);
		QString text("File ");
		text.append(fileName);
		text.append(" contains no data.\n");
		QErrorMessage *em = new QErrorMessage(this);
		em->showMessage(text, "loadFileErr2");
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

void KsMainWindow::initCapture()
{
#ifndef DO_AS_ROOT
	return;
#endif

	QStringList capturArgs;
	QString doAsRoot(DO_AS_ROOT);
	_capture.setProgram(doAsRoot);

	if (doAsRoot.contains("gksu")) {
		capturArgs << "--description" << "Kernel Shark Record";
	}

	QString captureExe = KS_DIR;
	captureExe += "/bin/kshark-record";
	capturArgs << captureExe;

	_capture.setArguments(capturArgs);

	connect(&_capture, SIGNAL(started()),
		this, SLOT(captureStarted()));

	connect(&_capture, SIGNAL(finished(int, QProcess::ExitStatus)),
		this, SLOT(captureFinished(int, QProcess::ExitStatus)));

	connect(&_captureLocalServer, SIGNAL(newConnection()),
		this, SLOT(readSocket()));

	return;
}

void KsMainWindow::captureStarted()
{
	if (_captureLocalServer.listen("KSCapture"))
		qInfo() << "listening on " << _captureLocalServer.serverName();
}

void KsMainWindow::captureFinished(int, QProcess::ExitStatus)
{
	_captureLocalServer.close();
}

void KsMainWindow::readSocket()
{
	QString message;
	QLocalSocket *socket = _captureLocalServer.nextPendingConnection();
	if (!socket) {
		message = "ERROR from Local Server: Pending connectio not found!";
		QErrorMessage *em = new QErrorMessage(this);
		em->showMessage(message, "readSocketErr");
		qCritical() << message;
		return;
	}

	QDataStream in(socket);
	socket->waitForReadyRead();
	if (socket->bytesAvailable() < (int)sizeof(quint32)) {
		message = "ERROR from Local Server: message size is corrupted!";
		QErrorMessage *em = new QErrorMessage(this);
		em->showMessage(message, "readSocketErr");
		qCritical() << message;
		return;
	};

	quint32 blockSize;
        in >> blockSize;

	if (socket->bytesAvailable() < blockSize || in.atEnd()) {
		QString message = "ERROR from Local Server: message is corrupted!"; 
		QErrorMessage *em = new QErrorMessage(this);
		em->showMessage(message, "readSocketErr");
		qCritical() << message;
		return;
	}

	QString fileName;
	in >> fileName;
	loadFile(fileName);
}
