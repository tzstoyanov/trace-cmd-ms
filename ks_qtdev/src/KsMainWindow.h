/*
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

#ifndef KS_MAINWINDOW_H
#define KS_MAINWINDOW_H

// C++
#include <iostream>
#include <vector>

// Qt
#include <QMainWindow>
#include <QAction>

// Kernelshark
#include "KsTraceViewer.h"
#include "KsTraceGraph.h"
#include "KsUtils.h"

class KsMainWindow : public QMainWindow {

	Q_OBJECT

public:
	KsMainWindow(QWidget *parent = 0);
	virtual ~KsMainWindow();

	void loadFile(const QString& fileName);

private slots:
	void open();
	void importFilter();
	void reload();

// 	bool save();
// 	bool saveAs();

	void eventSelect();
	void cpuSelect();
	void taskSelect();
	void aboutInfo();

private:

	void resizeEvent(QResizeEvent* event);

	KsDataStore		_data;
	KsTraceViewer	_view;
	KsTraceGraph	_graph;
	
	// File menu
	QAction _openAction;
	QAction _importFilterAction;
	QAction _saveFilterAction;
	QAction _saveFilterAsAction;
	QAction _exportFilterAction;
	QAction _quitAction;

	// Filter menu
	QAction _eventSelectAction;
	KsCheckBoxDialog *_events_cb;

	// Plots menu
	QAction _cpuSelectAction;
	KsCheckBoxDialog *_cpus_cb;

	QAction _taskSelectAction;
	KsCheckBoxDialog *_tasks_cb;


	// Help menu
	QAction _aboutAction;

	void createActions();
	void createMenus();
};

#endif

