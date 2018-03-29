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

// C++
#include <iostream>

// Qt
#include <QLocalSocket>

// Kernel Shark 2
#include "KsCaptureDialog.hpp"
#include "KsDeff.h"
#include "KsUtils.hpp"

KsCaptureControl::KsCaptureControl(QWidget *parent)
: QWidget(parent),
  _localPevt(kshark_local_events()),
  _eventsWidget(_localPevt, true, this),
  _pluginsLabel("Plugin: ", this),
  _outputLabel("Output file: ", this),
  _commandLabel("Command: ", this),
  _outputLineEdit("trace.dat", this),
  _commandLineEdit("sleep 1", this),
  _controlToolBar(this),
  _pluginsComboBox(this),
  _outputBrowseButton("Browse", this),
  _commandCheckBox("Display output", this),
  _captureButton("Capture", this),
  _applyButton("Apply", this),
  _closeButton("Close", this)
{
	auto add_line = [&] {
		QFrame* line = new QFrame();
		line->setFrameShape(QFrame::HLine);
		line->setFrameShadow(QFrame::Sunken);
		_topLayout.addWidget(line);
	};


	QStringList pluginList;
	pluginList << "nop";

	char **plugins;
	int n = kshark_get_plugins(&plugins);

	if (n > 0) {
		for (int i = 0; i < n; ++i) {
			pluginList << plugins[i];
			free(plugins[i]);
		}

		free(plugins);
	} else {
		QLabel *errorMessage =
			new QLabel("Error: No events or plugins found.\nRoot privileges are required.");
		errorMessage->setStyleSheet("QLabel {color : red;}");
		_topLayout.addWidget(errorMessage);
	}

	add_line();

	_eventsWidget.setDefault(false);
	_eventsWidget.setMinimumHeight(25*FONT_HEIGHT);
	_topLayout.addWidget(&_eventsWidget);

	int row(0);

	_pluginsLabel.adjustSize();
	_execLayout.addWidget(&_pluginsLabel, row, 0);

	_pluginsComboBox.addItems(pluginList);
	_execLayout.addWidget(&_pluginsComboBox, row++, 1);

	_outputLabel.adjustSize();
	_execLayout.addWidget(&_outputLabel, row, 0);
	_outputLineEdit.setFixedWidth(FONT_WIDTH*30);
	_execLayout.addWidget(&_outputLineEdit, row, 1);
	_outputBrowseButton.adjustSize();
	_execLayout.addWidget(&_outputBrowseButton, row++, 2);

	_commandLabel.adjustSize();
	_commandLabel.setFixedWidth(_outputLabel.width());
	_execLayout.addWidget(&_commandLabel, row, 0);
	_commandLineEdit.setFixedWidth(FONT_WIDTH*30);
	_execLayout.addWidget(&_commandLineEdit, row, 1);
	_commandCheckBox.setCheckState(Qt::Unchecked);
	_commandCheckBox.adjustSize();
	_execLayout.addWidget(&_commandCheckBox, row++, 2);

	_topLayout.addLayout(&_execLayout);

	add_line();

	_captureButton.setFixedWidth(STRING_WIDTH("_Capture_") + FONT_WIDTH*2);
	_applyButton.setFixedWidth(_captureButton.width());
	_closeButton.setFixedWidth(_captureButton.width());

	_controlToolBar.addWidget(&_captureButton);
	_controlToolBar.addWidget(&_applyButton);
	_controlToolBar.addWidget(&_closeButton);
	_topLayout.addWidget(&_controlToolBar);

	setLayout(&_topLayout);

	connect(&_outputBrowseButton, SIGNAL(pressed()), this, SLOT(browse()));
	connect(&_applyButton, SIGNAL(pressed()), this, SLOT(apply()));
}

void KsCaptureControl::browse()
{
	QString fileName =
		QFileDialog::getSaveFileName(this,
					     "Save File",
					     KS_DIR,
					     "trace-cmd files (*.dat);;All files (*)");

	if (!fileName.isEmpty())
		_outputLineEdit.setText(fileName);
}

QStringList KsCaptureControl::getArgs()
{
	QStringList args;
	args << "record";
	args << "-p" << _pluginsComboBox.currentText();

	if (_eventsWidget.all()) {
		args << "-e" << "all";
	} else {
		QVector<int> evtIds = _eventsWidget.getCheckedIds();
		event_format *event;
		for (auto const &id: evtIds) {
			event = kshark_find_event(_localPevt, id);
			if (!event)
				continue;

			args << "-e" + QString(event->system) +
				":" + QString(event->name);
		}
	}

	args << "-o" << outputFileName();
	args << _commandLineEdit.text().split(" ");

	return args;
}

void KsCaptureControl::apply()
{
	emit argsReady(getArgs().join(" "));
}

KsCaptureMonitor::KsCaptureMonitor(QWidget *parent)
: QWidget(parent),
  _panel(this),
  _name("Output display", this),
  _space("max size ", this),
  _readOnly("read only", this),
  _maxLinNumEdit("200", this),
  _consolOutput("", this),
  _mergedChannels(false),
  _argsModified(false)
{
	_panel.setMaximumHeight(FONT_HEIGHT*1.75);
	_panel.addWidget(&_name);

	_space.setAlignment(Qt::AlignRight);
	_panel.addWidget(&_space);

	_maxLinNumEdit.setFixedWidth(FONT_WIDTH*7);
	_panel.addWidget(&_maxLinNumEdit);
	_panel.addSeparator();
	_readOnly.setCheckState(Qt::Checked);
	_panel.addWidget(&_readOnly);
	_layout.addWidget(&_panel);

	_consolOutput.setStyleSheet("QLabel {background-color : white;}");
	_consolOutput.setMinimumWidth(FONT_WIDTH*60);
	_consolOutput.setMinimumHeight(FONT_HEIGHT*10);
	_consolOutput.setMaximumBlockCount(200);

	_space.setMinimumWidth(FONT_WIDTH*50 - _name.width() - _readOnly.width());
	_consolOutput.setReadOnly(true);
	_layout.addWidget(&_consolOutput);

	this->setLayout(&_layout);

	connect(&_maxLinNumEdit, SIGNAL(textChanged(const QString &)),
		this, SLOT(maxLineNumber(const QString &)));

	connect(&_readOnly, SIGNAL(stateChanged(int)),
		this, SLOT(readOnly(int)));

	connect(&_consolOutput, SIGNAL(textChanged()),
		this, SLOT(argsModified()));

	this->show();
}

void KsCaptureMonitor::maxLineNumber(const QString &test)
{
	bool ok;
	int max = test.toInt(&ok);
	if (ok)
		_consolOutput.setMaximumBlockCount(max);
}

void KsCaptureMonitor::readOnly(int state)
{
	if (state == Qt::Checked)
		_consolOutput.setReadOnly(true);
	else
		_consolOutput.setReadOnly(false);
}

void KsCaptureMonitor::argsReady(const QString &args)
{
	_name.setText("Capture options:");
	_consolOutput.setPlainText(args);
	_argsModified = false;
}

void KsCaptureMonitor::argsModified()
{
	_argsModified = true;
}

void KsCaptureMonitor::captureStarted()
{
	_name.setText("Terminal output:");
// 	_consolOutput.setPlainText("Collecting trace data ... \n");
	_readOnly.setCheckState(Qt::Checked);

	QCoreApplication::processEvents();
}

void KsCaptureMonitor::printAllStandardError()
{
	QProcess *capture = (QProcess*) sender();
// 	_consolOutput.appendPlainText(capture->readAllStandardError());
	_consolOutput.moveCursor(QTextCursor::End);
	_consolOutput.insertPlainText(capture->readAllStandardError());
	_consolOutput.moveCursor(QTextCursor::End);
	QCoreApplication::processEvents();
}

void KsCaptureMonitor::printAllStandardOutput()
{
	if (!_mergedChannels)
		return;

	QProcess *capture = (QProcess*) sender();
	_consolOutput.appendPlainText(capture->readAllStandardOutput());

	QCoreApplication::processEvents();
}

void KsCaptureMonitor::captureFinished(int exit, QProcess::ExitStatus status)
{
	QProcess *capture = (QProcess *)sender();
	if (exit != 0 || status != QProcess::NormalExit) {
		QString errMessage = "Capture process failed: ";
		errMessage += capture->errorString();
		_consolOutput.appendPlainText(errMessage);

		QCoreApplication::processEvents();
	}
}

void KsCaptureMonitor::print(const QString &message)
{
	_consolOutput.appendPlainText(message);
}

KsCaptureDialog::KsCaptureDialog(QWidget *parent)
: QWidget(parent),
  _captureCtrl(this),
  _captureMon(this),
  _capture(this)
{
	this->setWindowTitle("Capture");
	_layout.addWidget(&_captureCtrl);
	_layout.addWidget(&_captureMon);
	this->setLayout(&_layout);

	connect(&_captureCtrl._commandCheckBox, SIGNAL(stateChanged(int)),
		this, SLOT(setChannelMode(int)));

	connect(&_captureCtrl._captureButton, SIGNAL(pressed()),
		this, SLOT(capture()));

	connect(&_captureCtrl._closeButton, SIGNAL(pressed()),
		this, SLOT(close()));

	connect(&_captureCtrl, SIGNAL(argsReady(QString)),
		&_captureMon, SLOT(argsReady(QString)));

	QString captureExe = TRACECMD_BIN_DIR;
	captureExe += "/trace-cmd";
	_capture.setProgram(captureExe);

	connect(&_capture, SIGNAL(started()),
		&_captureMon, SLOT(captureStarted()));

	connect(&_capture, SIGNAL(finished(int, QProcess::ExitStatus)),
		&_captureMon, SLOT(captureFinished(int, QProcess::ExitStatus)));

	connect(&_capture, SIGNAL(readyReadStandardError()),
		&_captureMon, SLOT(printAllStandardError()));

	connect(&_capture, SIGNAL(readyReadStandardOutput()),
		&_captureMon, SLOT(printAllStandardOutput()));
}

void KsCaptureDialog::capture()
{
	QStringList args;
	if(_captureMon._argsModified) {
		args = _captureMon.text().split(" ");
	} else {
		args = _captureCtrl.getArgs();
	}

// 	_captureMon.clear();
	_captureMon.print("\n");
	_captureMon.print(QString("trace-cmd " + args.join(" ") + "\n"));
	_capture.setArguments(args);
	_capture.start();
	_capture.waitForFinished();

	int argc = args.count();
	for (int i = 0; i < argc; ++i) {
		if (args[i] == "-o") {
			sendOpenReq(args[i + 1]);
			break;
		}
	}

	_captureMon._argsModified = false;
}

void KsCaptureDialog::setChannelMode(int state)
{
	if (state > 0) {
		_captureMon._mergedChannels = true;
	} else {
		_captureMon._mergedChannels = false;
	}
}

void KsCaptureDialog::sendOpenReq(const QString &fileName)
{
	QLocalSocket *socket = new QLocalSocket(this);
	socket->connectToServer("KSCapture", QIODevice::WriteOnly);
	if (socket->waitForConnected()) {
		QByteArray block;
		QDataStream out(&block, QIODevice::WriteOnly);
// 		out.setVersion(QDataStream::Qt_5_7);

		const QString message = fileName;
		out << quint32(message.size());
		out << message;

		socket->write(block);
		socket->flush();
		socket->disconnectFromServer();
	} else {
// 		_captureMon.print(socket->serverName());
		_captureMon.print(socket->errorString());
	}
}
