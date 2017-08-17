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
	KSCpuCheckBoxDialog(QWidget *parent = 0);
};

struct KSTasksCheckBoxDialog : public KsCheckBoxDialog {
	KSTasksCheckBoxDialog(QWidget *parent = 0);
};

struct KSEventsCheckBoxDialog : public KsCheckBoxDialog {
	KSEventsCheckBoxDialog(QWidget *parent = 0);
};

#endif
