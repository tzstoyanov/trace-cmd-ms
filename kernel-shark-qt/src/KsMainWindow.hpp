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

// KernelShark
#include "KsTraceViewer.hpp"
#include "KsTraceGraph.hpp"
#include "KsUtils.hpp"

class KsMainWindow : public QMainWindow
{
	Q_OBJECT
public:
	explicit KsMainWindow(QWidget *parent = nullptr);
	~KsMainWindow();

	void loadData(const QString &fileName);
	void loadSession(const QString &fileName);
// 	void loadFiles(const QList<QString> &files);

	void registerPlugin(const QString &plugin) {_plugins.registerPlugin(plugin);}
	void unregisterPlugin(const QString &plugin) {_plugins.unregisterPlugin(plugin);}

private slots:
	void open();
	void restorSession();
	void importSession();
	void exportSession();
	void importFilter();
	void exportFilter();
	void listFilterSync(int state);
	void graphFilterSync(int state);
	void showEvents();
	void showTasks();
	void hideTasks();
	void advancedFiltering();
	void clearFilters();
	void cpuSelect();
	void taskSelect();
	void pluginSelect();
	void capture();
	void setColorPhase(int);
	void fullScreenMode();
	void aboutInfo();
	void contents();
	void captureStarted();
	void captureFinished(int, QProcess::ExitStatus);
	void readSocket();
	void splitterMoved(int pos, int index);

private:
	QSplitter	_splitter;

	KsSession	_session;

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

	/** The process used to record trace data. */
	QProcess	_capture;

	/** Local Server used for comunucation with the Capture process. */
	QLocalServer	_captureLocalServer;

	// File menu.
	QAction		_openAction;
	QAction		_restorSessionAction;
	QAction		_importSessionAction;
	QAction		_exportSessionAction;
	QAction		_quitAction;

	// Filter menu.
	QAction		_importFilterAction;
	QAction		_exportFilterAction;
	QWidgetAction	_graphFilterSyncAction;
	QWidgetAction	_listFilterSyncAction;
	QAction		_showEventsAction;
	QAction		_showTasksAction;
	QAction		_hideTasksAction;
	QAction		_advanceFilterAction;
	QAction		_clearAllFilters;

	// Plots menu.
	QAction		_cpuSelectAction;
	QAction		_taskSelectAction;

	// Tools menu.
	QAction		_pluginsAction;
	QAction		_captureAction;
	QWidgetAction	_colorAction;
	QSlider		_colorPhaseSlider;
	QAction		_fullScreenModeAction;
	bool		_isFullScreen;

	// Help menu.
	QAction		_aboutAction;
	QAction		_contentsAction;

	void resizeEvent(QResizeEvent* event);
	void createActions();
	void createMenus();
	void initCapture();
	void updateSession();

	inline void resizeEmpty() { resize(SCREEN_WIDTH * .4, FONT_HEIGHT * 3);}
};

#endif // _KS_MAINWINDOW_H
