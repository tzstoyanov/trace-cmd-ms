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
#include "KsAdvFilteringDialog.hpp"

KsMainWindow::KsMainWindow(QWidget *parent)
: QMainWindow(parent),
  _splitter(Qt::Vertical),
  _data(this),
  _view(this),
  _graph(this),
  _mState(this),
  _plugins(this),
  _capture(this),
  _captureLocalServer(this),
  _openAction("Open", parent),
  _restorSessionAction("Restor Last Session", parent),
  _importSessionAction("Import Session", parent),
  _exportSessionAction("Export Sassion", parent),
  _quitAction("Quit", parent),
  _importFilterAction("Import Filter", parent),
  _exportFilterAction("Export Filter", parent),
  _graphFilterSyncAction(/*"Apply filters to Graph",*/ parent),
  _listFilterSyncAction(/*"Apply filters to List",*/ parent),
  _showEventsAction("Show events", parent),
  _showTasksAction("Show tasks", parent),
  _hideTasksAction("Hide tasks", parent),
  _advanceFilterAction("Advance Filtering", this),
  _clearAllFilters("Clear all filters", this),
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

	_splitter.addWidget(&_graph);
	_splitter.addWidget(&_view);
	setCentralWidget(&_splitter);
	connect(&_splitter,	&QSplitter::splitterMoved,
		this,		&KsMainWindow::splitterMoved);

	_view.setMarkerSM(&_mState);
	connect(&_mState,	&KsDualMarkerSM::markSwitch,
		&_view,		&KsTraceViewer::markSwitch);

	_graph.setMarkerSM(&_mState);
	_mState.markerA().setGLWidget(_graph.glPtr());
	_mState.markerB().setGLWidget(_graph.glPtr());

	connect(&_mState,	&KsDualMarkerSM::showInGraph,
		&_graph,	&KsTraceGraph::markEntry);

	connect(&_mState,	&KsDualMarkerSM::updateView,
		&_view,		&KsTraceViewer::showRow);

	connect(&_view,		&KsTraceViewer::select,
		&_graph,	&KsTraceGraph::markEntry);

	connect(&_view,		&KsTraceViewer::plotTask,
		&_graph,	&KsTraceGraph::addTaskPlot);

	connect(_graph.glPtr(), &KsGLWidget::updateView,
		&_view,		&KsTraceViewer::showRow);

	connect(_graph.glPtr(), &KsGLWidget::deselect,
		&_view,		&KsTraceViewer::deselect);

	connect(&_data,		&KsDataStore::updateView,
		&_view,		&KsTraceViewer::update);

	connect(&_data,		&KsDataStore::updateGraph,
		&_graph,	&KsTraceGraph::update);
	
	connect(&_plugins,	&KsPluginManager::dataReload,
		&_data,		&KsDataStore::reload);

	this->resize(SCREEN_WIDTH*.4, FONT_HEIGHT*3);
}

KsMainWindow::~KsMainWindow()
{
	updateSession();
	QString file = KS_CONF_DIR;
	file += "/lastsession.json";
	kshark_save_json_file(file.toLocal8Bit().data(), _session.json());

	_data.clear();
	kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);
	kshark_free(kshark_ctx);
}

void KsMainWindow::resizeEvent(QResizeEvent* event)
{
	QMainWindow::resizeEvent(event);
	_graph.resize(width(), _graph.height());
	_session.setMainWindowSize(width(), height());

	QList<int> sizes = _splitter.sizes();
	_session.setSplitterSize(sizes[0], sizes[1]);
}

void KsMainWindow::createActions()
{
	/* File menu */
	_openAction.setIcon(QIcon::fromTheme("document-open"));
	_openAction.setShortcut(tr("Ctrl+O"));
	_openAction.setStatusTip("Open an existing data file");

	connect(&_openAction,	&QAction::triggered,
		this,		&KsMainWindow::open);

	_restorSessionAction.setIcon(QIcon::fromTheme("edit-redo"));
	connect(&_restorSessionAction,	&QAction::triggered,
		this,			&KsMainWindow::restorSession);

	_importSessionAction.setIcon(QIcon::fromTheme("document-send"));
	_importSessionAction.setStatusTip("Load a session");

	connect(&_importSessionAction,	&QAction::triggered,
		this,			&KsMainWindow::importSession);

	_exportSessionAction.setIcon(QIcon::fromTheme("document-revert"));
	_exportSessionAction.setStatusTip("Export this session");

	connect(&_exportSessionAction,	&QAction::triggered,
		this,			&KsMainWindow::exportSession);

	_quitAction.setIcon(QIcon::fromTheme("window-close"));
	_quitAction.setShortcut(tr("Ctrl+Q"));
	_quitAction.setStatusTip("Exit KernelShark");

	connect(&_quitAction,	&QAction::triggered,
		this,		&KsMainWindow::close);

	/* Filter menu */
	_importFilterAction.setIcon(QIcon::fromTheme("document-send"));
	_importFilterAction.setStatusTip("Load a filter");

	connect(&_importFilterAction,	&QAction::triggered,
		this,			&KsMainWindow::importFilter);

	_exportFilterAction.setIcon(QIcon::fromTheme("document-revert"));
	_exportFilterAction.setStatusTip("Export a filter");

	connect(&_exportFilterAction,	&QAction::triggered,
		this,			&KsMainWindow::exportFilter);

	connect(&_showEventsAction,	&QAction::triggered,
		this,			&KsMainWindow::showEvents);

	connect(&_showTasksAction,	&QAction::triggered,
		this,			&KsMainWindow::showTasks);

	connect(&_hideTasksAction,	&QAction::triggered,
		this,			&KsMainWindow::hideTasks);

	connect(&_advanceFilterAction,	&QAction::triggered,
		this,			&KsMainWindow::advancedFiltering);

	connect(&_clearAllFilters,	&QAction::triggered,
		this,			&KsMainWindow::clearFilters);

	/* Plot menu */
	connect(&_cpuSelectAction,	&QAction::triggered,
		this,			&KsMainWindow::cpuSelect);

	connect(&_taskSelectAction,	&QAction::triggered,
		this,			&KsMainWindow::taskSelect);

	/* Tools menu */
	_pluginsAction.setIcon(QIcon::fromTheme("insert-image"));
	_pluginsAction.setShortcut(tr("Ctrl+P"));
	_pluginsAction.setStatusTip("Manage plugins");

	connect(&_pluginsAction,	&QAction::triggered,
		this,			&KsMainWindow::pluginSelect);

	_captureAction.setIcon(QIcon::fromTheme("media-record"));
	_captureAction.setShortcut(tr("Ctrl+C"));
	_captureAction.setStatusTip("Capture trace data");

	connect(&_captureAction,	&QAction::triggered,
		this,			&KsMainWindow::capture);

	_colorPhaseSlider.setMinimum(20);
	_colorPhaseSlider.setMaximum(180);
	_colorPhaseSlider.setValue(75);
	_colorPhaseSlider.setFixedWidth(FONT_WIDTH*15);

	connect(&_colorPhaseSlider,	&QSlider::valueChanged,
		this,			&KsMainWindow::setColorPhase);

	QWidget *colSlider = new QWidget(this);
	colSlider->setLayout(new QHBoxLayout);
	QLabel *colLabel = new QLabel("Color scheme", this);
	colSlider->layout()->addWidget(colLabel);
	colSlider->layout()->addWidget(&_colorPhaseSlider);
	_colorAction.setDefaultWidget(colSlider);

	/* Help menu */
	_aboutAction.setIcon(QIcon::fromTheme("help-about"));

	connect(&_aboutAction,		&QAction::triggered,
		this,			&KsMainWindow::aboutInfo);

	_contentsAction.setIcon(QIcon::fromTheme("help-contents"));
	connect(&_contentsAction,	&QAction::triggered,
		this,			&KsMainWindow::contents);
}

void KsMainWindow::createMenus()
{
	/* File menu */
	QMenu *file = menuBar()->addMenu("File");
	file->addAction(&_openAction);

	QMenu *sessions = file->addMenu("Sessions");
	sessions->addAction(&_restorSessionAction);
	sessions->addAction(&_importSessionAction);
	sessions->addAction(&_exportSessionAction);
	file->addAction(&_quitAction);

	/* Filter menu */
	QMenu *filter = menuBar()->addMenu("Filter");
	filter->addAction(&_importFilterAction);
	filter->addAction(&_exportFilterAction);

	auto make_cb_action = [&] (QWidgetAction *action, QString name)
	{
		QWidget  *containerWidget = new QWidget(filter);
		containerWidget->setLayout(new QHBoxLayout());
		containerWidget->layout()->setContentsMargins(FONT_WIDTH, FONT_HEIGHT/5,
							      FONT_WIDTH, FONT_HEIGHT/5);
		QCheckBox *checkBox = new QCheckBox(name, filter);
		checkBox->setChecked(true);
		containerWidget->layout()->addWidget(checkBox);
		action->setDefaultWidget(containerWidget);
		return checkBox;
	};

	/* Set the default filter mask. Filter will apply to both View and Graph. */
	kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);
	kshark_ctx->filter_mask = KS_VIEW_FILTER_MASK | KS_GRAPH_FILTER_MASK;

	QCheckBox *cb;

	cb = make_cb_action(&_graphFilterSyncAction, "Apply filters to Graph");
	connect(cb,	&QCheckBox::stateChanged,
		this,	&KsMainWindow::graphFilterSync);

	cb = make_cb_action(&_listFilterSyncAction, "Apply filters to List");
	connect(cb,	&QCheckBox::stateChanged,
		this,	&KsMainWindow::listFilterSync);

	filter->addAction(&_graphFilterSyncAction);
	filter->addAction(&_listFilterSyncAction);
	filter->addAction(&_showEventsAction);
	filter->addAction(&_showTasksAction);
	filter->addAction(&_hideTasksAction);
	filter->addAction(&_advanceFilterAction);
	filter->addAction(&_clearAllFilters);

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
	QString fileName =
		QFileDialog::getOpenFileName(this,
					     "Open File",
					     KS_DIR,
					     "trace-cmd files (*.dat);;All files (*)");

	if (!fileName.isEmpty())
		loadData(fileName);
}

void KsMainWindow::restorSession()
{
	QString file = KS_CONF_DIR;
	file += "/lastsession.json";

	loadSession(file);
	_graph.updateGeom();
}

void KsMainWindow::importSession()
{
	QString fileName =
		QFileDialog::getOpenFileName(this,
					     "Import Session",
					     KS_DIR,
					     "Kernel Shark Config files (*.json);;");

	if (fileName.isEmpty())
		return;

	loadSession(fileName);
	_graph.updateGeom();
}

void KsMainWindow::updateSession()
{
	size_t nBins = _graph.glPtr()->model()->histo()->size();
	uint64_t min = _graph.glPtr()->model()->histo()->_min;
	uint64_t max = _graph.glPtr()->model()->histo()->_max;

	_session.setVisModel(nBins, min, max);
	_session.setCpuPlots(_graph.glPtr()->_cpuList);
	_session.setTaskPlots(_graph.glPtr()->_taskList);

	kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);
	_session.setFilters(kshark_ctx);

	_session.setDualMarker(&_mState);
	_session.setViewTop(_view.getTopRow());
	_session.setPlugins(_plugins._pluginList, _plugins._registeredPlugins);
}

void KsMainWindow::exportSession()
{
	QString fileName =
		QFileDialog::getSaveFileName(this,
					     "Export Filter",
					     KS_DIR,
					     "Kernel Shark Config files (*.json);;");

	if (fileName.isEmpty())
		return;

	if (!fileName.endsWith(".json"))
		fileName += ".json";

	updateSession();
	_session.exportToFile(fileName);
}

void KsMainWindow::importFilter()
{
	QString fileName =
		QFileDialog::getOpenFileName(this,
					     "Import Filter",
					     KS_DIR,
					     "Kernel Shark Config files (*.json);;");

	if (fileName.isEmpty())
		return;

	json_object *jobj =
		kshark_open_json_file(fileName.toStdString().c_str(),
				     "kshark.filter.config");

	if (!jobj)
		return;

	kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);
	kshark_all_event_filters_from_json(kshark_ctx, jobj);
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
					     "Kernel Shark Config files (*.json);;");

	if (fileName.isEmpty())
		return;

	if (!fileName.endsWith(".json"))
		fileName += ".json";

	kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	json_object *jobj = kshark_all_event_filters_to_json(kshark_ctx);
	kshark_save_json_file(fileName.toStdString().c_str(), jobj);
	json_object_put(jobj);
}

void KsMainWindow::listFilterSync(int state)
{
	KsUtils::listFilterSync(state);
	_data.update();
}

void KsMainWindow::graphFilterSync(int state)
{
	KsUtils::graphFilterSync(state);
	_data.update();
}

void KsMainWindow::showEvents()
{
	kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	KsCheckBoxWidget *events_cb =
		new KsEventsCheckBoxWidget(_data._pevt, true, this);

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

	connect(dialog, &KsCheckBoxDialog::apply,
		&_data, &KsDataStore::applyPosEventFilter);

	dialog->show();
}

void KsMainWindow::showTasks()
{
	kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	KsCheckBoxWidget *tasks_cbd = new KsTasksCheckBoxWidget(_data._pevt, true, this);
	if (!kshark_ctx->show_task_filter ||
	    !filter_id_count(kshark_ctx->show_task_filter)) {
		tasks_cbd->setDefault(true);
	} else {
		QVector<int> pids;
		int nPids = KsUtils::getPidList(&pids);
		QVector<bool> v(nPids, false);
		for (int i = 0; i < nPids; ++i) {
			if (filter_id_find(kshark_ctx->show_task_filter,
					   pids[i]))
				v[i] = true;
		}

		tasks_cbd->set(v);
	}

	KsCheckBoxDialog *dialog = new KsCheckBoxDialog(tasks_cbd, this);
	connect(dialog, &KsCheckBoxDialog::apply,
		&_data, &KsDataStore::applyPosTaskFilter);

	dialog->show();
}

void KsMainWindow::hideTasks()
{
	kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	KsCheckBoxWidget *tasks_cbd = new KsTasksCheckBoxWidget(_data._pevt, false, this);
	if (!kshark_ctx->hide_task_filter ||
	    !filter_id_count(kshark_ctx->hide_task_filter)) {
		tasks_cbd->setDefault(false);
	} else {
		QVector<int> pids;
		int nPids = KsUtils::getPidList(&pids);
		QVector<bool> v(nPids, false);
		for (int i = 0; i < nPids; ++i) {
			if (filter_id_find(kshark_ctx->hide_task_filter,
					   pids[i]))
				v[i] = true;
		}

		tasks_cbd->set(v);
	}

	KsCheckBoxDialog *dialog = new KsCheckBoxDialog(tasks_cbd, this);
	connect(dialog, &KsCheckBoxDialog::apply,
		&_data, &KsDataStore::applyNegTaskFilter);

	dialog->show();
}

void KsMainWindow::advancedFiltering()
{
	KsAdvFilteringDialog *dialog = new KsAdvFilteringDialog(_data._pevt,
								this);

	connect(dialog, &KsAdvFilteringDialog::dataReload,
		&_data,	&KsDataStore::reload);

	dialog->show();
}

void KsMainWindow::clearFilters()
{
	_data.clearAllFilters();
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
	connect(dialog,		&KsCheckBoxDialog::apply,
		&_graph,	&KsTraceGraph::cpuReDraw);

	dialog->show();
}

void KsMainWindow::taskSelect()
{
	KsCheckBoxWidget *tasks_cbd = new KsTasksCheckBoxWidget(_data._pevt, true, this);

	QVector<int> pids;
	int nPids = KsUtils::getPidList(&pids);
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
	connect(dialog,		&KsCheckBoxDialog::apply,
		&_graph,	&KsTraceGraph::taskReDraw);

	dialog->show();
}

void KsMainWindow::pluginSelect()
{
	kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	KsCheckBoxWidget *plugin_cbd
		= new KsPluginCheckBoxWidget(_plugins._pluginList, this);

	plugin_cbd->set(_plugins._registeredPlugins);

	KsCheckBoxDialog *dialog = new KsCheckBoxDialog(plugin_cbd, this);
	connect(dialog, &KsCheckBoxDialog::apply,
		&_plugins, &KsPluginManager::updatePlugins);

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

void KsMainWindow::loadData(const QString& fileName)
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
		text.append(".");
		QErrorMessage *em = new QErrorMessage(this);
		em->showMessage(text, "loadDataErr1");
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
		text.append(" contains no data.");
		QErrorMessage *em = new QErrorMessage(this);
		em->showMessage(text, "loadDataErr2");
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

	char buff[FILENAME_MAX];
	getcwd(buff, FILENAME_MAX);
	QString path(buff);
	path += "/";
	if (fileName.contains(path))
		_session.setDataFile(fileName, st.st_mtime);
	else
		_session.setDataFile(path + fileName, st.st_mtime);
// 	t1.join();
}

void KsMainWindow::loadSession(const QString &fileName)
{
	_session.importFromFile(fileName);

	_session.getPlugins(&_plugins._pluginList,
			    &_plugins._registeredPlugins);
	_plugins.load();

	int width, height;
	_session.getMainWindowSize(&width, &height);
	int graphSize, viewSize;
	_session.getSplitterSize(&graphSize, &viewSize);
	QList<int> sizes;
	sizes << graphSize << viewSize;

	_splitter.setSizes(sizes);
	resize(width, height);

	QString dataFile = _session.getDataFile();
	time_t ts = _session.getDataFileTS();

	struct stat st;
	int ret = stat(dataFile.toStdString().c_str(), &st);
	if (ret != 0) {
		this->resize(SCREEN_WIDTH*.4, FONT_HEIGHT*3);
		QString text("Unable to find file ");
		text.append(fileName);
		text.append("\n");
		QErrorMessage *em = new QErrorMessage(this);
		em->showMessage(text, "loadSessErr1");
		qCritical() << "ERROR: " << text;
		_plugins.unload();
		return;
	}

	if (ts != st.st_mtime) {
		QString text("Timestamp mismatch! File ");
		text.append(fileName);
		text.append(" has been modified.");
		QErrorMessage *em = new QErrorMessage(this);
		em->showMessage(text, "loadSessErr2");
		qCritical() << "ERROR: " << text;
		_plugins.unload();
		return;
	}

	loadData(dataFile);
	if (!_data._pevt) {
		_plugins.unload();
		return;
	}

	_graph.glPtr()->_cpuList = _session.getCpuPlots();
	_graph.glPtr()->_taskList = _session.getTaskPlots();
	_graph.updateGraphLegends();

	kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);
	_session.getFilters(kshark_ctx);
	if (kshark_ctx->adv_filter_is_set)
		_data.reload();
	else
		kshark_filter_entries(kshark_ctx, _data._rows, _data.size());

	uint64_t pos;
	_mState.reset();
	_mState.setState(DualMarkerState::A);
	if (_session.getMarker("markA", &pos)) {
		_graph.markEntry(pos);
	} else {
		_mState.markerA().remove();
	}

	_mState.setState(DualMarkerState::B);
	if (_session.getMarker("markB", &pos)) {
		_graph.markEntry(pos);
	} else {
		_mState.markerB().remove();
	}

	size_t nBins;
	uint64_t min, max;
	_session.getVisModel(&nBins, &min, &max);
	_graph.glPtr()->model()->histo()->setBining(nBins, min, max);
	_graph.glPtr()->model()->fill(_data._pevt, _data._rows, _data.size());

	_mState.setState(_session.getMarkerState());
	_mState.updateMarkers(_data,*_graph.glPtr()->model()->histo());
	_mState.updateLabels(*_graph.glPtr()->model()->histo());

	size_t topRow = _session.getViewTop();
	_view.setTopRow(topRow);
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

	connect(&_capture,	&QProcess::started,
		this,		&KsMainWindow::captureStarted);

	/* Using the old Signal-Slot syntax because QProcess::finished has overloads. */
	connect(&_capture, SIGNAL(finished(int, QProcess::ExitStatus)),
		this, SLOT(captureFinished(int, QProcess::ExitStatus)));

	connect(&_captureLocalServer,	&QLocalServer::newConnection,
		this,			&KsMainWindow::readSocket);
}

void KsMainWindow::captureStarted()
{
	if (_captureLocalServer.listen("KSCapture"))
		qInfo() << "listening on " << _captureLocalServer.serverName();
}

void KsMainWindow::captureFinished(int exit, QProcess::ExitStatus st)
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
	loadData(fileName);
}

void KsMainWindow::splitterMoved(int pos, int index)
{
	QList<int> sizes = _splitter.sizes();
	_session.setSplitterSize(sizes[0], sizes[1]);
}
