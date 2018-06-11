/*
 * Copyright (C) 2017 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
 *
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

// KernelShark
#include "KsAdvFilteringDialog.hpp"
#include "libkshark.h"
#include "KsUtils.hpp"

KsAdvFilteringDialog::KsAdvFilteringDialog(pevent *pe,
					   QWidget *parent)
: QDialog(parent),
  _pevt(pe),
  _descrLabel(this),
  _sysEvLabel("System/Event: ", this),
  _opsLabel("Operator: ", this),
  _fieldLabel("Field: ", this),
  _systemComboBox(this),
  _eventComboBox(this),
  _opsComboBox(this),
  _fieldComboBox(this),
  _filterEdit(this),
  _helpButton("Show Help", this),
  _insertEvtButton("Insert", this),
  _insertOpButton("Insert", this),
  _insertFieldButton("Insert", this),
  _applyButton("Apply", this),
  _cancelButton("Cancel", this)
{
	auto add_line = [&] {
		QFrame* line = new QFrame();
		line->setFrameShape(QFrame::HLine);
		line->setFrameShadow(QFrame::Sunken);
		_topLayout.addWidget(line);
	};

	setMinimumWidth(FONT_WIDTH * 80);

	int buttonWidth = STRING_WIDTH("--Show Help--");
	_helpButton.setFixedWidth(buttonWidth);
	_topLayout.addWidget(&_helpButton);
	connect(&_helpButton,	&QPushButton::pressed,
		this,		&KsAdvFilteringDialog::help);

	QString descrText = "Usage:\n";
	descrText += " <sys/event>[,<sys/event>] : [!][(]<field><op><val>[)]";
	descrText += "[&&/|| [(]<field><op><val>[)]]\n\n";
	descrText += "Examples:\n\n";
	descrText += "   sched/sched_switch : next_prio < 100 && (prev_prio > 100";
	descrText += "&& prev_pid != 0)\n\n";
	descrText += "   irq.* : irq != 38\n\n";
	descrText += "   .* : common_pid == 1234\n";

	_descrLabel.setText(descrText);
	_topLayout.addWidget(&_descrLabel);
	_descrLabel.hide();

	add_line();

	kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);
	event_format **events = pevent_list_events(kshark_ctx->pevt, EVENT_SORT_SYSTEM);
	QStringList filters;
	QVector<event_format*> filteredEvents;
	char *str;

	for (int i = 0; events[i]; i++) {
		str = pevent_filter_make_string(kshark_ctx->advanced_event_filter, events[i]->id);
		if (!str)
			continue;

		_filters.insert(events[i]->id,
				QString("%1/%2:%3\n").arg(events[i]->system, events[i]->name, str));

		filteredEvents.append(events[i]);
		filters << QString("%1/%2:%3\n").arg(events[i]->system, events[i]->name, str);
		free(str);
	}

	if (_filters.count()) {
// 		KsCheckBoxTree *tree = new KsCheckBoxTree(this);
		_table = new KsCheckBoxTable(this);
		_table->setSelectionMode(QAbstractItemView::SingleSelection);
		QStringList headers;
		headers << "Delete" << "Event" << " Id" << "Filter";
		_table->init(headers, _filters.count());
		QMapIterator<int, QString> f(_filters);
		int count(0);
		while (f.hasNext()) {
			f.next();
			event_format *event = pevent_data_event_from_type(_pevt, f.key());
			QString evtName(event->system);
			evtName += "/";
			evtName += event->name;
			QTableWidgetItem *i1 = new QTableWidgetItem(evtName);
			_table->setItem(count, 1, i1);
			QTableWidgetItem *i2 = new QTableWidgetItem(tr("%1").arg(f.key()));
			_table->setItem(count, 2, i2);
			QTableWidgetItem *i3 = new QTableWidgetItem(f.value());
			_table->setItem(count, 3, i3);
			++count;
		}

		_table->setVisible(false);
		_table->resizeColumnsToContents();
		_table->setVisible(true);

		_topLayout.addWidget(_table);
		add_line();
	}

	QToolBar *condToolBar1 = new QToolBar(this);

	condToolBar1->addWidget(&_sysEvLabel);
	condToolBar1->addWidget(&_systemComboBox);
	condToolBar1->addWidget(&_eventComboBox);

	/*
	 * Using the old Signal-Slot syntax because QComboBox::currentIndexChanged
	 * has overloads.
	 */
	connect(&_systemComboBox,	SIGNAL(currentIndexChanged(const QString&)),
		this,			SLOT(systemChanged(const QString&)));

	connect(&_eventComboBox,	SIGNAL(currentIndexChanged(const QString&)),
		this,			SLOT(eventChanged(const QString&)));

	QStringList sysList;
	int nEvts = 0;
	if (_pevt) {
		nEvts = _pevt->nr_events;
	}

	int i(0);
	while (i < nEvts) {
		QString sysName(_pevt->events[i]->system);
		sysList << sysName;
		while (sysName == _pevt->events[i]->system) {
			if (++i == nEvts)
				break;
		}
	}

	_systemComboBox.addItems(sysList);

	condToolBar1->addSeparator();
	condToolBar1->addWidget(&_insertEvtButton);
	_topLayout.addWidget(condToolBar1);

	QStringList OpsList;
	OpsList << ":" << "," << "==" << "!=" << ">" << "<" << ">=" << "<=";
	OpsList << "=~" << "!~" << "!" << "(" << ")" << "+" << "-";
	OpsList << "*" << "/" << "<<" << ">>" << "&&" << "||" << "&";
	_opsComboBox.addItems(OpsList);

	QToolBar *condToolBar2 = new QToolBar(this);
	condToolBar2->addWidget(&_opsLabel);
	condToolBar2->addWidget(&_opsComboBox);

	condToolBar2->addSeparator();
	condToolBar2->addWidget(&_insertOpButton);
	_topLayout.addWidget(condToolBar2);

	QToolBar *condToolBar3 = new QToolBar(this);
	condToolBar3->addWidget(&_fieldLabel);
	condToolBar3->addWidget(&_fieldComboBox);

	condToolBar3->addSeparator();
	condToolBar3->addWidget(&_insertFieldButton);
	_topLayout.addWidget(condToolBar3);

	add_line();

	_filterEdit.setMinimumWidth(50 * FONT_WIDTH);
	_topLayout.addWidget(&_filterEdit);
	this->setLayout(&_topLayout);

	buttonWidth = STRING_WIDTH("--Cancel--");
	_applyButton.setFixedWidth(buttonWidth);
	_cancelButton.setFixedWidth(buttonWidth);
	_buttonLayout.addWidget(&_applyButton);
	_buttonLayout.addWidget(&_cancelButton);
	_buttonLayout.setAlignment(Qt::AlignLeft);
	_topLayout.addLayout(&_buttonLayout);

	connect(&_insertEvtButton,	&QPushButton::pressed,
		this,			&KsAdvFilteringDialog::insertEvt);

	connect(&_insertOpButton,	&QPushButton::pressed,
		this,			&KsAdvFilteringDialog::insertOperator);

	connect(&_insertFieldButton,	&QPushButton::pressed,
		this,			&KsAdvFilteringDialog::insertField);

	_applyButtonConnection =
		connect(&_applyButton,	&QPushButton::pressed,
			this,		&KsAdvFilteringDialog::applyPress);

	connect(&_applyButton,	&QPushButton::pressed,
		this,		&QWidget::close);

	connect(&_cancelButton,	&QPushButton::pressed,
		this,		&QWidget::close);
}

void KsAdvFilteringDialog::help()
{
	if (_descrLabel.isVisible()) {
		_descrLabel.hide();
		QApplication::processEvents();
		_helpButton.setText("Show Help");
		resize(width(), _noHelpHeight);
	} else {
		_helpButton.setText("Hide Help");
		_noHelpHeight = height();
		_descrLabel.show();
	}
}

void KsAdvFilteringDialog::systemChanged(const QString &sysName)
{
	_eventComboBox.clear();
	QStringList evtsList;

	int nEvts = _pevt->nr_events;
	int i(0);
	while (i < nEvts) {
		if (sysName == _pevt->events[i]->system)
			evtsList << _pevt->events[i]->name;
		++i;
	}

	_eventComboBox.addItems(evtsList);
}

void KsAdvFilteringDialog::eventChanged(const QString &evtName)
{
	_fieldComboBox.clear();
	QStringList fieldList;
	int nEvts = _pevt->nr_events;
	int i(0);
	QString sysName = _systemComboBox.currentText();
	while (i < nEvts) {
		if (evtName == _pevt->events[i]->name &&
		    sysName == _pevt->events[i]->system) {
			int n = _pevt->events[i]->format.nr_fields;
			char **fields =
				kshark_get_event_format_fields(_pevt->events[i]);

			for (int j = 0; j < n; ++j) {
				fieldList << fields[j];
				free(fields[j]);
			}

			free(fields);
			_fieldComboBox.addItems(fieldList);

			return;
		}

		++i;
	}
}

void KsAdvFilteringDialog::insertEvt()
{
	QString text = _filterEdit.text();

	auto set_evt = [&] ()
	{
		text += _systemComboBox.currentText();
		text += "/";
		text += _eventComboBox.currentText();
	};

	if (text == "") {
		set_evt();
		text += ":";
	} else {
		QString evt = text;
		text = "";
		set_evt();
		text += ",";
		text += evt;
	}
	_filterEdit.setText(text);
}

void KsAdvFilteringDialog::insertOperator()
{
	QString text = _filterEdit.text();
	text += _opsComboBox.currentText();
	_filterEdit.setText(text);
}

void KsAdvFilteringDialog::insertField()
{
	QString text = _filterEdit.text();
	text += _fieldComboBox.currentText();
	_filterEdit.setText(text);
}

void KsAdvFilteringDialog::applyPress()
{
	kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);
	kshark_ctx->adv_filter_is_set = false;

	QMapIterator<int, QString> f(_filters);
	int i(0);
	while (f.hasNext()) {
		f.next();
		if (_table->_cb[i]->checkState() == Qt::Checked) {
			pevent_filter_remove_event(kshark_ctx->advanced_event_filter,
						   f.key());
		} else {
			kshark_ctx->adv_filter_is_set = true;
		}
		++i;
	}

	auto job_done = [&]() {
		/*
		* Disconnect Apply button. This is done in order to protect
		* against multiple clicks.
		*/
		disconnect(_applyButtonConnection);
		emit dataReload();
	};

	const char *text = _filterEdit.text().toLocal8Bit().data();
	if (strlen(text) == 0) {
		job_done();
		return;
	}

	char *filter = (char *) malloc(strlen(text) + 1);
	strcpy(filter, text);

	pevent_errno ret;
	ret = pevent_filter_add_filter_str(kshark_ctx->advanced_event_filter, filter);

	if (ret < 0) {
		char error_str[200];
		pevent_strerror(kshark_ctx->pevt, ret, error_str, sizeof(error_str));
		fprintf(stderr, "filter failed due to: %s\n", error_str);
		free(filter);
		return;
	}

	free(filter);
	kshark_ctx->adv_filter_is_set = true;

	job_done();
}
