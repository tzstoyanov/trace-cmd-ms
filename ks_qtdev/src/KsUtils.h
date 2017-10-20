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

#ifndef KS_CHEKBOXDIALOG_H
#define KS_CHEKBOXDIALOG_H 1

// C++
#include <vector>

// Qt
#include <QString>
#include <QDialog>
#include <QPushButton>
#include <QCheckBox>
#include <QBoxLayout>
#include <QLabel>

#include "event-parse.h"

#define SCREEN_HEIGHT  QApplication::desktop()->screenGeometry().height()
#define SCREEN_WIDTH   QApplication::desktop()->screenGeometry().width()

auto fontHeight = []() {QFont font; QFontMetrics fm(font); return fm.height();};
auto stringWidth  = [](QString s) {QFont font; QFontMetrics fm(font); return fm.width(s);};

#define FONT_HEIGHT fontHeight()
#define FONT_WIDTH  stringWidth("4")
#define STRING_WIDTH(s)  stringWidth(s)

#define GET_TIME std::chrono::high_resolution_clock::now()

#define GET_DURATION(t0) std::chrono::duration_cast<std::chrono::duration<double>>( \
std::chrono::high_resolution_clock::now()-t0).count()

typedef std::chrono::high_resolution_clock::time_point  hd_time;


class KsMessageDialog : public QDialog {

	Q_OBJECT

public:
	KsMessageDialog(QWidget *parent) = delete;
	KsMessageDialog(QString message, QWidget *parent=0);

private:
	QVBoxLayout 	_layout;
	QLabel 			_text;
	QPushButton 	_close_button;
};

class KsCheckBoxDialog : public QDialog {

	Q_OBJECT

public:
	KsCheckBoxDialog(const QString &n="", QWidget *parent = 0);
	virtual ~KsCheckBoxDialog();

private slots:
	void apply();
	void chechAll(int);

protected:
	QCheckBox _all_cb;
	std::vector<QCheckBox*> _cb;

	QVBoxLayout _top_layout;
	QVBoxLayout _cb_layout;
	QHBoxLayout _button_layout;

private:
	QString _name;

	QPushButton _cansel_button;
	QPushButton _apply_button;
};

struct KSCpuCheckBoxDialog : public KsCheckBoxDialog {
	KSCpuCheckBoxDialog(struct pevent *pe, QWidget *parent = 0);
};

struct KSTasksCheckBoxDialog : public KsCheckBoxDialog {
	KSTasksCheckBoxDialog(struct pevent *pe, QWidget *parent = 0);
};

struct KSEventsCheckBoxDialog : public KsCheckBoxDialog {
	KSEventsCheckBoxDialog(struct pevent *pe, QWidget *parent = 0);
};

#endif
