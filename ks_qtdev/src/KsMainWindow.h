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
#include <QAction>

// Kernel Shark 2
#include "KsTraceViewer.h"
#include "KsTraceGraph.h"
#include "KsUtils.h"

class KsMainWindow : public QMainWindow
{
	Q_OBJECT
public:
	KsMainWindow(QWidget *parent = 0);

	void loadFile(const QString& fileName);

private slots:
	void open();
	void importFilter();
	void reload();
	 bool saveFilter();
// 	bool saveAs();
	void showEvents();
	void showTasks();
	void hideTasks();
	void cpuSelect();
	void taskSelect();
	void aboutInfo();

private:
	/** Data manager */
	KsDataStore	_data;

	// Widget for reading searching in the trace data */
	KsTraceViewer	_view;

	/** Widget for graphical visualization of the trace data */
	KsTraceGraph	_graph;

	/** Dual Marker State Machine */
	KsDualMarkerSM  _mState;

	// File menu
	QAction _openAction;
	QAction _importFilterAction;
	QAction _saveFilterAction;
	QAction _saveFilterAsAction;
	QAction _exportFilterAction;
	QAction _quitAction;

	// Filter menu
	QAction _taskSyncAction;
	QAction _eventSyncAction;
	QAction _showEventsAction;
	QAction _showTasksAction;
	QAction _hideTasksAction;

	// Plots menu
	QAction _cpuSelectAction;
	QAction _taskSelectAction;

	// Help menu
	QAction _aboutAction;

	void resizeEvent(QResizeEvent* event);
	void createActions();
	void createMenus();
};

#endif // _KS_MAINWINDOW_H
