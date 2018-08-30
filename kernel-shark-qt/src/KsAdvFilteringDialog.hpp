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

#ifndef _KS_ADV_FILTERING_H
#define _KS_ADV_FILTERING_H

// Qt
#include <QtWidgets>

// KernelShark
#include "KsWidgetsLib.hpp"

class KsAdvFilteringDialog : public QDialog
{
	Q_OBJECT

	int 			_noHelpHeight;
	tep_handle 		*_pevent;

	QMap<int, QString>	_filters;
	KsCheckBoxTable		*_table;

	QVBoxLayout		_topLayout;
	QHBoxLayout		_buttonLayout;
	QLabel			_descrLabel, _sysEvLabel, _opsLabel, _fieldLabel;
	QComboBox		_systemComboBox, _eventComboBox;
	QComboBox		_opsComboBox, _fieldComboBox;
	QLineEdit		_filterEdit;
	QPushButton		_helpButton;
	QPushButton		_insertEvtButton, _insertOpButton, _insertFieldButton;
	QPushButton		_applyButton, _cancelButton;
	QMetaObject::Connection	_applyButtonConnection;

public:
	KsAdvFilteringDialog() = delete;
	KsAdvFilteringDialog(tep_handle *pe, QWidget *parent = nullptr);

private slots:
	void help();
	void applyPress();
	void systemChanged(const QString&);
	void eventChanged(const QString&);
	void insertEvt();
	void insertOperator();
	void insertField();

signals:
	void dataReload();
};

#endif
