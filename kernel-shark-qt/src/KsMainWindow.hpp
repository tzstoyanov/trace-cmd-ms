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

#ifndef _KS_MAINWINDOW_H
#define _KS_MAINWINDOW_H

// Qt
#include <QMainWindow>
#include <QLocalServer>

// Kernel Shark 2
#include "KsTraceViewer.hpp"
#include "KsTraceGraph.hpp"
#include "KsUtils.hpp"

class KsMainWindow : public QMainWindow
{
	Q_OBJECT
public:
	explicit KsMainWindow(QWidget *parent = 0);
	~KsMainWindow();

	void loadFile(const QString &fileName);
// 	void loadFiles(const QList<QString> &files);

	void registerPlugin(const QString &plugin) {_plugins.registerPlugin(plugin);}
	void unregisterPlugin(const QString &plugin) {_plugins.unregisterPlugin(plugin);}

private:
	/** Data Manager. */
	KsDataStore	_data;

	/** Widget for reading and searching in the trace data. */
	KsTraceViewer	_view;

	/** Widget for graphical visualization of the trace data. */
	KsTraceGraph	_graph;

	/** Dual Marker State Machine. */
	KsDualMarkerSM	_mState;

	/** Plugin manager. */
	KsPluginManager	_plugins;

	/** Capture process. */
	QProcess	_capture;

	/** Local Server used for comunucation with the Capture process. */
	QLocalServer	_captureLocalServer;

	// File menu.
	QAction		_openAction;
	QAction		_importFilterAction;
	QAction		_exportFilterAction;
	QAction		_quitAction;

	// Filter menu.
	QWidgetAction	_graphFilterSyncAction;
	QWidgetAction	_listFilterSyncAction;
	QAction		_showEventsAction;
	QAction		_showTasksAction;
	QAction		_hideTasksAction;

	// Plots menu.
	QAction		_cpuSelectAction;
	QAction		_taskSelectAction;

	// Tools menu.
	QAction		_pluginsAction;
	QAction		_captureAction;
	QWidgetAction	_colorAction;
	QSlider		_colorPhaseSlider;

	// Help menu.
	QAction		_aboutAction;
	QAction		_contentsAction;

	void resizeEvent(QResizeEvent* event);
	void createActions();
	void createMenus();

private slots:
	void open();
	void open(QString file);
	void importFilter();
	void exportFilter();
	void listFilterSync(bool state);
	void graphFilterSync(bool state);
	void showEvents();
	void showTasks();
	void hideTasks();
	void cpuSelect();
	void taskSelect();
	void pluginSelect();
	void capture();
	void setColorPhase(int);
	void aboutInfo();
	void contents();
	void captureStarted();
	void captureFinished(int, QProcess::ExitStatus);
	void readSocket();
};

// class KsConcentratorMainWindow : public

#endif // _KS_MAINWINDOW_H
