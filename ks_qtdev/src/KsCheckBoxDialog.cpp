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


// C++
#include <iostream>

//Qt
#include <QFrame>

// KS
#include "KsCheckBoxDialog.h"
#include "KsDeff.h"

KsMessageDialog::KsMessageDialog(QString message, QWidget *parent)
: QDialog(parent),
  _text(message, this),
  _close_button("Close", this)
{
	this->resize(200, 50);
	_layout.addWidget(&_text);
	_layout.addWidget(&_close_button);
	connect(&_close_button,  SIGNAL(pressed()), this, SLOT(close()));

	this->setLayout(&_layout);
}

KsCheckBoxDialog::KsCheckBoxDialog(const QString &n, QWidget *parent)
: QDialog(parent),
  _all_cb("all", this),
  _name(n),
  _cansel_button("Cansel", this),
  _apply_button("Apply", this)
{
	this->resize(200, 50);
	this->setWindowTitle(_name);

	connect(&_apply_button, SIGNAL(pressed()), this, SLOT(apply()));
	connect(&_apply_button, SIGNAL(pressed()), parentWidget(), SLOT(reload()) );

	connect(&_cansel_button,  SIGNAL(pressed()), this, SLOT(close()));

	connect(&_all_cb, SIGNAL(stateChanged(int)), this, SLOT(chechAll(int)));

	_cb_layout.addWidget(&_all_cb);

	_button_layout.addWidget(&_apply_button,  1, Qt::AlignBottom);
	_button_layout.addWidget(&_cansel_button, 1, Qt::AlignBottom);

	_top_layout.addLayout(&_cb_layout, 1);

	QFrame* line = new QFrame();
	line->setFrameShape(QFrame::HLine);
	line->setFrameShadow(QFrame::Sunken);
	_top_layout.addWidget(line);

	_top_layout.addLayout(&_button_layout, 1);

	this->setLayout(&_top_layout);
	this->show();
}

KsCheckBoxDialog::~KsCheckBoxDialog() {}

void KsCheckBoxDialog::apply()
{
	// TODO
	close();
}

void KsCheckBoxDialog::chechAll(int st)
{
	Qt::CheckState state = Qt::Unchecked;
	if (st) state = Qt::Checked;

	for (auto &c: _cb) {
		c->setCheckState(state);
	}
}

KSCpuCheckBoxDialog::KSCpuCheckBoxDialog(struct pevent *pe, QWidget *parent)
: KsCheckBoxDialog("Filter CPUs", parent)
{
	int i=0;
	_cb.resize(pe->cpus);

	for (auto &c: _cb) {
		c = new QCheckBox(QString("CPU %1").arg(i++), this);
		_cb_layout.addWidget(c);
	}
}

KSTasksCheckBoxDialog::KSTasksCheckBoxDialog(struct pevent *pe, QWidget *parent)
: KsCheckBoxDialog("Tasks", parent)
{
}

KSEventsCheckBoxDialog::KSEventsCheckBoxDialog(struct pevent *pe, QWidget *parent)
: KsCheckBoxDialog("Tasks", parent)
{
	int i=0;
	_cb.resize(pe->nr_events);

	struct event_format **events;
	
	events = pevent_list_events(pe, EVENT_SORT_NAME);
	if (!events)
		return;

	for (auto &c: _cb) {
		std::cout << pe->sort_events[i]->name << std::endl;
		c = new QCheckBox(QString(pe->events[i]->name), this);
		_cb_layout.addWidget(c);
		++i;
	}
}










