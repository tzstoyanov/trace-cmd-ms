/*
 * Copyright (C) 2018 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
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

#ifndef _KS_CAPTURE_H
#define _KS_CAPTURE_H

// Qt
#include <QtWidgets>

// Kernel Shark 2
#include "KsWidgetsLib.hpp"

class KsCaptureControl : public QWidget
{
	Q_OBJECT

	pevent			*_localPevt;
	KsEventsCheckBoxWidget	_eventsWidget;

	QVBoxLayout	_topLayout;
	QGridLayout	_execLayout;
	QLabel		_pluginsLabel, _outputLabel, _commandLabel;
	QLineEdit	_outputLineEdit, _commandLineEdit;
	QToolBar	_controlToolBar;
	QComboBox	_pluginsComboBox;
	QPushButton	_outputBrowseButton;

public:
	KsCaptureControl(QWidget *parent = 0);

	QStringList getArgs();
	QString outputFileName() const {return _outputLineEdit.text();}

	QCheckBox	_commandCheckBox;
	QPushButton	_captureButton, _applyButton, _closeButton;

private slots:
	void browse();
	void apply();

signals:
	void argsReady(QString args);
};

class KsCaptureMonitor : public QWidget
{
	Q_OBJECT

	QVBoxLayout	_layout;
	QToolBar	_panel;
	QLabel		_name, _space;
	QCheckBox	_readOnly;
	QPlainTextEdit	_consolOutput;

public:
	KsCaptureMonitor(QWidget *parent = 0);
	QString text() const {return _consolOutput.toPlainText();}
	void clear() {_consolOutput.clear();}
	void print(const QString &message);

	bool		_mergedChannels, _argsModified;

private slots:
	void readOnly(int);
	void argsReady(QString args);
	void argsModified();
	void printlStandardError();
	void printlStandardOutput();
	void captureStarted();
	void captureFinished(int, QProcess::ExitStatus);
};

class KsCaptureDialog : public QWidget
{
	Q_OBJECT

	QHBoxLayout		_layout;
	KsCaptureControl	_captureCtrl;
	KsCaptureMonitor	_captureMon;
	QProcess		_capture;

signals:
	void open(const QString &file);

private slots:
	void capture();
	void setChannelMode(int state);

public:
	KsCaptureDialog(QWidget *parent = 0);

private:
	void sendOpenReq(const QString &fileName);
};

#endif // _KS_CAPTURE_H
