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

// Qt
#include <QLocalSocket>

// KernelShark
#include "libkshark.h"
#include "KsUtils.hpp"
#include "KsCmakeDef.hpp"
#include "KsCaptureDialog.hpp"

static tep_handle *local_events()
{
	const char *tracing = tracecmd_get_tracing_dir();
	return tracecmd_local_events(tracing);
}

static int get_plugins(char ***plugins)
{
	const char *tracing;
	char **all_plugins;
	int i = 0;

	tracing = tracecmd_get_tracing_dir();
	all_plugins = tracecmd_local_plugins(tracing);

	if (!all_plugins)
		return 0;

	while (all_plugins[i]) {
	/* TODO plugin selection here.
		printf("plugin %i %s\n", i, all_plugins[i]);
	*/
		 ++i;
	}

	*plugins = all_plugins;
	return i;
}

KsCaptureControl::KsCaptureControl(QWidget *parent)
: QWidget(parent),
  _localPevt(local_events()),
  _eventsWidget(_localPevt, this),
  _pluginsLabel("Plugin: ", this),
  _outputLabel("Output file: ", this),
  _commandLabel("Command: ", this),
  _outputLineEdit("trace.dat", this),
  _commandLineEdit("sleep 1", this),
  _settingsToolBar(this),
  _controlToolBar(this),
  _pluginsComboBox(this),
  _importSettings("Import Settings", this),
  _exportSettings("Export Settings", this),
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

	/* Get the list of available plugins. */
	char **plugins;
	int n = get_plugins(&plugins);

	if (n > 0) {
		for (int i = 0; i < n; ++i) {
			pluginList << plugins[i];
			free(plugins[i]);
		}

		free(plugins);
	} else {
		/*
		 * No plugins have been found. Most likely this is because
		 * the process has no Root privileges.
		 */
		QString message("Error: No events or plugins found.\n");
		message += "Root privileges are required.";
		QLabel *errorLabel = new QLabel(message);
		errorLabel->setStyleSheet("QLabel {color : red;}");
		_topLayout.addWidget(errorLabel);
		add_line();
	}

	_settingsToolBar.addWidget(&_importSettings);
	_settingsToolBar.addSeparator();
	_settingsToolBar.addWidget(&_exportSettings);
	_topLayout.addWidget(&_settingsToolBar);

	add_line();

	_eventsWidget.setDefault(false);
	_eventsWidget.setMinimumHeight(25 * FONT_HEIGHT);
	_topLayout.addWidget(&_eventsWidget);

	int row(0);
	_pluginsLabel.adjustSize();
	_execLayout.addWidget(&_pluginsLabel, row, 0);

	_pluginsComboBox.addItems(pluginList);
	_execLayout.addWidget(&_pluginsComboBox, row++, 1);

	_outputLabel.adjustSize();
	_execLayout.addWidget(&_outputLabel, row, 0);
	_outputLineEdit.setFixedWidth(FONT_WIDTH * 30);
	_execLayout.addWidget(&_outputLineEdit, row, 1);
	_outputBrowseButton.adjustSize();
	_execLayout.addWidget(&_outputBrowseButton, row++, 2);

	_commandLabel.adjustSize();
	_commandLabel.setFixedWidth(_outputLabel.width());
	_execLayout.addWidget(&_commandLabel, row, 0);
	_commandLineEdit.setFixedWidth(FONT_WIDTH * 30);
	_execLayout.addWidget(&_commandLineEdit, row, 1);
	_commandCheckBox.setCheckState(Qt::Unchecked);
	_commandCheckBox.adjustSize();
	_execLayout.addWidget(&_commandCheckBox, row++, 2);

	_topLayout.addLayout(&_execLayout);

	add_line();

	_captureButton.setFixedWidth(STRING_WIDTH("_Capture_") + FONT_WIDTH * 2);
	_applyButton.setFixedWidth(_captureButton.width());
	_closeButton.setFixedWidth(_captureButton.width());

	_controlToolBar.addWidget(&_captureButton);
	_controlToolBar.addWidget(&_applyButton);
	_controlToolBar.addWidget(&_closeButton);
	_topLayout.addWidget(&_controlToolBar);

	setLayout(&_topLayout);

	connect(&_importSettings,	&QPushButton::pressed,
		this,			&KsCaptureControl::importSettings);

	connect(&_exportSettings,	&QPushButton::pressed,
		this,			&KsCaptureControl::exportSettings);

	connect(&_outputBrowseButton,	&QPushButton::pressed,
		this,			&KsCaptureControl::browse);

	connect(&_applyButton,		&QPushButton::pressed,
		this,			&KsCaptureControl::apply);
}

void KsCaptureControl::importSettings()
{
	kshark_config_doc *conf = NULL, *jfilter, *temp;
	QString fileName =
		QFileDialog::getOpenFileName(this,
					     "Import from Filter",
					     KS_DIR,
					     "Kernel Shark Config files (*.json);;");

	if (fileName.isEmpty())
		return;

	conf = kshark_open_config_file(fileName.toStdString().c_str(),
				       "kshark.config.record");
	if (!conf)
		return;

	jfilter = kshark_config_alloc(KS_CONFIG_JSON);
	if (!kshark_config_doc_get(conf, "Filters", jfilter))
		return;

	tracecmd_filter_id *filterHash = tracecmd_filter_id_hash_alloc();
	kshark_import_event_filter(_localPevt, filterHash, "Events", jfilter);
	int nEvts = _localPevt->nr_events;
	QVector<bool> v(nEvts, false);
	for (int i = 0; i < nEvts; ++i) {
		if (tracecmd_filter_id_find(filterHash, _localPevt->events[i]->id))
			v[i] = true;
	}

	_eventsWidget.set(v);
	tracecmd_filter_id_hash_free(filterHash);

	temp = kshark_string_config_alloc();

	if (kshark_config_doc_get(conf, "Plugin", temp))
		_pluginsComboBox.setCurrentText(KS_C_STR_CAST(temp->conf_doc));

	if (kshark_config_doc_get(conf, "Output", temp))
		_outputLineEdit.setText(KS_C_STR_CAST(temp->conf_doc));

	if (kshark_config_doc_get(conf, "Command", temp))
		_commandLineEdit.setText(KS_C_STR_CAST(temp->conf_doc));
}

void KsCaptureControl::exportSettings()
{
	kshark_config_doc *conf, *jfilter;
	QString fileName =
		QFileDialog::getSaveFileName(this,
					     "Export to File",
					     KS_DIR,
					     "Kernel Shark Config files (*.json);;");

	if (fileName.isEmpty())
		return;

	if (!fileName.endsWith(".json"))
		fileName += ".json";

	conf = kshark_record_config_new(KS_CONFIG_JSON);
	jfilter = kshark_filter_config_new(KS_CONFIG_JSON);

	QVector<int> ids = _eventsWidget.getCheckedIds();
	tracecmd_filter_id *filterHash = tracecmd_filter_id_hash_alloc();
	for (auto const &id: ids)
		tracecmd_filter_id_add(filterHash, id);

	kshark_export_event_filter(_localPevt, filterHash, "Events", jfilter);
	kshark_config_doc_add(conf, "Filters", jfilter);

	tracecmd_filter_id_hash_free(filterHash);

	QString plugin = _pluginsComboBox.currentText();
	json_object *jplugin = json_object_new_string(plugin.toStdString().c_str());
	kshark_config_doc_add(conf, "Plugin", kshark_json_to_conf(jplugin));

	QString out = outputFileName();
	json_object *jout = json_object_new_string(out.toStdString().c_str());
	kshark_config_doc_add(conf, "Output", kshark_json_to_conf(jout));

	QString comm = _commandLineEdit.text();
	json_object *jcomm = json_object_new_string(comm.toStdString().c_str());
	kshark_config_doc_add(conf, "Command", kshark_json_to_conf(jcomm));

	kshark_save_config_file(fileName.toStdString().c_str(), conf);
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
			event = tep_find_event(_localPevt, id);
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

#define MAX_LINE_NUM 200

KsCaptureMonitor::KsCaptureMonitor(QWidget *parent)
: QWidget(parent),
  _panel(this),
  _name("Output display", this),
  _space("max size ", this),
  _readOnly("read only", this),
  _maxLinNumEdit(QString("%1").arg(MAX_LINE_NUM), this),
  _consolOutput("", this),
  _mergedChannels(false),
  _argsModified(false)
{
	_panel.setMaximumHeight(FONT_HEIGHT * 1.75);
	_panel.addWidget(&_name);

	_space.setAlignment(Qt::AlignRight);
	_panel.addWidget(&_space);

	_maxLinNumEdit.setFixedWidth(FONT_WIDTH * 7);
	_panel.addWidget(&_maxLinNumEdit);
	_panel.addSeparator();
	_readOnly.setCheckState(Qt::Checked);
	_panel.addWidget(&_readOnly);
	_layout.addWidget(&_panel);

	_consolOutput.setStyleSheet("QLabel {background-color : white;}");
	_consolOutput.setMinimumWidth(FONT_WIDTH * 60);
	_consolOutput.setMinimumHeight(FONT_HEIGHT * 10);
	_consolOutput.setMaximumBlockCount(MAX_LINE_NUM);

	_space.setMinimumWidth(FONT_WIDTH * 50 - _name.width() - _readOnly.width());
	_consolOutput.setReadOnly(true);
	_layout.addWidget(&_consolOutput);

	this->setLayout(&_layout);

	connect(&_maxLinNumEdit,	&QLineEdit::textChanged,
		this,			&KsCaptureMonitor::maxLineNumber);

	connect(&_readOnly,		&QCheckBox::stateChanged,
		this,			&KsCaptureMonitor::readOnly);

	connect(&_consolOutput,		&QPlainTextEdit::textChanged,
		this,			&KsCaptureMonitor::argsModified);

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

	connect(&_captureCtrl._commandCheckBox,	&QCheckBox::stateChanged,
		this,				&KsCaptureDialog::setChannelMode);

	connect(&_captureCtrl._captureButton,	&QPushButton::pressed,
		this,				&KsCaptureDialog::capture);

	connect(&_captureCtrl._closeButton,	&QPushButton::pressed,
		this,				&KsCaptureDialog::close);

	connect(&_captureCtrl,	&KsCaptureControl::argsReady,
		&_captureMon,	&KsCaptureMonitor::argsReady);

	QString captureExe = TRACECMD_BIN_DIR;
	captureExe += "/trace-cmd";
	_capture.setProgram(captureExe);

	connect(&_capture,	&QProcess::started,
		&_captureMon,	&KsCaptureMonitor::captureStarted);

	/* Using the old Signal-Slot syntax because QProcess::finished has overloads. */
	connect(&_capture,	SIGNAL(finished(int, QProcess::ExitStatus)),
		&_captureMon,	SLOT(captureFinished(int, QProcess::ExitStatus)));

	connect(&_capture,	&QProcess::readyReadStandardError,
		&_captureMon,	&KsCaptureMonitor::printAllStandardError);

	connect(&_capture,	&QProcess::readyReadStandardOutput,
		&_captureMon,	&KsCaptureMonitor::printAllStandardOutput);
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
