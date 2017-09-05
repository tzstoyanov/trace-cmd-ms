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

#include <sys/stat.h>
#include <iostream>
#include <thread>

// Qt
#include <QMenu>
#include <QFileDialog>
#include <QMenuBar>
#include <QLabel>

// KS
#include "KsMainWindow.h"
#include "KsDeff.h"

using namespace std;

KsMainWindow::KsMainWindow(QWidget *parent)
: QMainWindow(parent),
  _view(this),
  _graph(this),
  _openAction(tr("&Open"), parent),
  _importFilterAction(tr("&Import Filter"), parent),
  _saveFilterAction(tr("&Save Filter"), parent),
  _saveFilterAsAction(tr("&Save Filter As"),parent),
  _exportFilterAction(tr("&Export Filter"),parent),
  _quitAction(tr("&Quit"),parent),
  _eventSelectAction(tr("&Events"),parent),
  _cpuSelectAction(tr("&CPUs"),parent),
  _taskSelectAction(tr("&Tasks"),parent),
  _aboutAction(tr("About"),parent)
{
	this->setWindowTitle("KernelShark");
	this->resize(1200, 750);

	this->createActions();
	this->createMenus();

// 	QWidget *window = new QWidget(this);
// 	QVBoxLayout *layout = new QVBoxLayout();
// 	layout->addWidget(&_graph);
// 	layout->addWidget(&_view);
// 	window->setLayout(layout);
// 	setCentralWidget(window);

	QSplitter *splitter = new QSplitter(Qt::Vertical);
	splitter->addWidget(&_graph);
	splitter->setHandleWidth(2);
	splitter->addWidget(&_view);
// 	splitter->setStretchFactor(0, 1);
// 	splitter->setStretchFactor(1, 1);
	setCentralWidget(splitter);
}

KsMainWindow::~KsMainWindow() {
	_data.clear();
}

void KsMainWindow::createActions()
{
	QString iconsPath(KS_DIR);
	iconsPath.append("/icons/");

	_openAction.setIcon(QIcon(iconsPath + "open.png"));
	_openAction.setShortcut(tr("Ctrl+O"));
	_openAction.setStatusTip(tr("Open an existing data file"));
	connect(&_openAction, SIGNAL(triggered()), this, SLOT(open()));

	_importFilterAction.setIcon(QIcon(iconsPath + "import.png"));
	_importFilterAction.setShortcut(tr("Ctrl+L"));
	_importFilterAction.setStatusTip(tr("Load a filter"));
	connect(&_importFilterAction, SIGNAL(triggered()), this, SLOT(importFilter()));

	_saveFilterAction.setIcon(QIcon(iconsPath + "save.png"));
	_saveFilterAction.setShortcut(tr("Ctrl+S"));
	_saveFilterAction.setStatusTip(tr("Save a filter"));
	connect(&_saveFilterAction, SIGNAL(triggered()), this, SLOT(importFilter()));

	_saveFilterAsAction.setIcon(QIcon(iconsPath + "save-as.png"));
	_saveFilterAsAction.setShortcut(tr("Shift+Ctrl+S"));
	_saveFilterAsAction.setStatusTip(tr("Save a filter as"));
	connect(&_saveFilterAsAction, SIGNAL(triggered()), this, SLOT(importFilter()));

	_exportFilterAction.setIcon(QIcon(iconsPath + "export.png"));
	_exportFilterAction.setShortcut(tr("Ctrl+L"));
	_exportFilterAction.setStatusTip(tr("Export a filter"));
	connect(&_exportFilterAction, SIGNAL(triggered()), this, SLOT(importFilter()));

	_quitAction.setIcon(QIcon(iconsPath + "quit.png"));
	_quitAction.setShortcut(tr("Ctrl+Q"));
	_quitAction.setStatusTip(tr("Exit KernelShark"));
	connect(&_quitAction, SIGNAL(triggered()), this, SLOT(close()));

	connect(&_eventSelectAction, SIGNAL(triggered()), this, SLOT(eventSelect()));

	connect(&_cpuSelectAction, SIGNAL(triggered()), this, SLOT(cpuSelect()));

	connect(&_taskSelectAction, SIGNAL(triggered()), this, SLOT(taskSelect()));

	connect(&_aboutAction, SIGNAL(triggered()), this, SLOT(aboutInfo()));
}

void KsMainWindow::createMenus()
{
	QMenu *file = menuBar()->addMenu("&File");
	file->addAction(&_openAction);
	file->addAction(&_importFilterAction);
	file->addAction(&_saveFilterAction);
	file->addAction(&_saveFilterAsAction);
	file->addAction(&_exportFilterAction);
	file->addAction(&_quitAction);

	QMenu *filter = menuBar()->addMenu("&Filter");
	filter->addAction(&_eventSelectAction);

	QMenu *plots = menuBar()->addMenu("&Plots");
	plots->addAction(&_cpuSelectAction);
	plots->addAction(&_taskSelectAction);

	/*QMenu *capture = */menuBar()->addMenu("&Capture");

	QMenu *help = menuBar()->addMenu("&Help");
	help->addAction(&_aboutAction);
}

void KsMainWindow::open()
{
	QString selfilter = tr("dat)");
	QString fileName =
	QFileDialog::getOpenFileName(	this,
									tr("Open File"),
									KS_DIR,
									tr("trace-cmd files (*.dat);;All files (*)"));
	if (!fileName.isEmpty())
		loadFile(fileName);
}

void KsMainWindow::importFilter()
{
	
}

void KsMainWindow::reload()
{
	cerr << "RELOAD\n";
}

void KsMainWindow::eventSelect()
{
	/*KSEventsCheckBoxDialog *_events_cb =*/ new KSEventsCheckBoxDialog(this);
}

void KsMainWindow::cpuSelect() {
    /*KsCheckBoxDialog *_cpus_cb =*/ new KSCpuCheckBoxDialog(this);
}

void KsMainWindow::taskSelect() {
    /*KSTasksCheckBoxDialog *_tasks_cb =*/ new KSTasksCheckBoxDialog(this);
}

void KsMainWindow::aboutInfo() {
	QString text;
	text.append(" KernelShark\n\n version: ");
	text.append(KS_VERSION_STRING);
	text.append("\n");

	KsMessageDialog *message = new KsMessageDialog(text);
	message->show();
}

void KsMainWindow::loadFile(const QString& fileName) {
	struct stat st;
	cerr << "Loading " << fileName.toStdString() << endl;
	int ret = stat(fileName.toStdString().c_str(), &st);
	if (ret != 0) {
		_view.reset();
		_graph.reset();
		QString text("Unable to find file \n");
		text.append(fileName);
		text.append("\n");
		KsMessageDialog *message = new KsMessageDialog(text);
		message->show();
		cerr << "ERROR Opening file " << fileName.toStdString() << endl;
		return;
	}

	_data.loadData(fileName);
	if (!_data.size())
	{
		_view.reset();
		_graph.reset();
		QString text("File \n");
		text.append(fileName);
		text.append("\ncontains no data.\n");
		KsMessageDialog *message = new KsMessageDialog(text);
		message->show();
		return;
	}

	_view.loadData(&_data);
	//auto job = [&] {_view.loadData(&_data);};
	//std::thread t1(job);

	_graph.loadData(&_data);

	//t1.join();
}

