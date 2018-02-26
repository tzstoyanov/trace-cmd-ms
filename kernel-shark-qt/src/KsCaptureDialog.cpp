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

// Kernel Shark 2
#include "KsCaptureDialog.hpp"
#include "KsUtils.hpp"
#include "KsDeff.h"

KsCaptureDialog::KsCaptureDialog(QWidget *parent)
: QDialog(parent),
  _consolOutput("", this),
  _captureButton("Capture", this),
  _canselButton("Close", this)
{
	this->setWindowTitle("Capture");

	_consolOutput.setStyleSheet("QLabel {background-color : white;}");
	_consolOutput.setMinimumWidth(FONT_WIDTH*60);
	_consolOutput.setMinimumHeight(FONT_HEIGHT*20);

	_captureLayout.addWidget(&_consolOutput);

	_buttonLayout.addWidget(&_captureButton,  1, Qt::AlignBottom);
	_buttonLayout.addWidget(&_canselButton, 1, Qt::AlignBottom);


	_topLayout.addLayout(&_captureLayout, 1);
	_topLayout.addLayout(&_buttonLayout, 1);
	this->setLayout(&_topLayout);

	connect(&_captureButton, SIGNAL(pressed()), this, SLOT(capture()));
	connect(&_canselButton, SIGNAL(pressed()), this, SLOT(close()));

	this->show();
}

void KsCaptureDialog::capture()
{
// 	char *cmd = "pkexec";
// 	char *capture_argv[8];
// 	capture_argv[0] = "pkexec";
// 	asprintf(&capture_argv[1], "%s/trace-cmd", TRACECMD_BIN_DIR);
	

	QProcess capture;

	QString program = KS_DIR;
	program += "/bin/exetest";

	QString data = KS_DIR;
	data += "/test.dat";
	QStringList argv;
	argv << "-p" << "function" << "-o" << data << "ls";
	capture.start(program, argv);
	
// 	capture.start("pkexec ls -al");
	capture.waitForFinished();
	qInfo() << " ### DONE";
	QString output(capture.readAllStandardOutput());
// 	QString output(capture.readAllStandardError());

	_consolOutput.setText(output);
}
