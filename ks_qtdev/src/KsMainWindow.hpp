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
#include "KsTraceViewer.hpp"
#include "KsTraceGraph.hpp"
#include "KsUtils.hpp"

class KsMainWindow : public QMainWindow
{
	Q_OBJECT
public:
	KsMainWindow(QWidget *parent = 0);

	void loadFile(const QString &fileName);
	void loadFiles(const QList<QString> &files);

private slots:
	void open();
	void importFilter();
	void reload();
	void saveFilter();
// 	bool saveAs();
	void listFilterSync(bool state);
	void graphFilterSync(bool state);
	void showEvents();
	void showTasks();
	void hideTasks();
	void cpuSelect();
	void taskSelect();
	void aboutInfo();

private:
	/** Data manager. */
	KsDataStore	_data;

	/** Widget for reading and searching in the trace data. */
	KsTraceViewer	_view;

	/** Widget for graphical visualization of the trace data. */
	KsTraceGraph	_graph;

	/** Dual Marker State Machine. */
	KsDualMarkerSM  _mState;

	// File menu.
	QAction _openAction;
	QAction _importFilterAction;
	QAction _saveFilterAction;
	QAction _saveFilterAsAction;
	QAction _exportFilterAction;
	QAction _quitAction;

	// Filter menu.
	QWidgetAction _graphFilterSyncAction;
	QWidgetAction _listFilterSyncAction;
	QAction _showEventsAction;
	QAction _showTasksAction;
	QAction _hideTasksAction;

	// Plots menu.
	QAction _cpuSelectAction;
	QAction _taskSelectAction;

	// Help menu.
	QAction _aboutAction;

	void resizeEvent(QResizeEvent* event);
	void createActions();
	void createMenus();
};

// class KsConcentratorMainWindow : public

#endif // _KS_MAINWINDOW_H
