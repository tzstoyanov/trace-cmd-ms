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

#ifndef KS_CHEKBOXDIALOG_H
#define KS_CHEKBOXDIALOG_H 1

// C++
#include <vector>

// Qt
#include <QtWidgets>

// trace-cmd
#include "event-parse.h"

// Kernel Shark 2
#include "ks-view.h"

#define SCREEN_HEIGHT  QApplication::desktop()->screenGeometry().height()
#define SCREEN_WIDTH   QApplication::desktop()->screenGeometry().width()

auto fontHeight = []()
{
	QFont font;
	QFontMetrics fm(font);
	return fm.height();
};

auto stringWidth = [](QString s)
{
	QFont font;
	QFontMetrics fm(font);
	return fm.width(s);
};

#define FONT_HEIGHT fontHeight()
#define FONT_WIDTH  stringWidth("4")
#define STRING_WIDTH(s)  stringWidth(s)

#define GET_TIME std::chrono::high_resolution_clock::now()

#define GET_DURATION(t0) std::chrono::duration_cast<std::chrono::duration<double>>( \
std::chrono::high_resolution_clock::now()-t0).count()

typedef std::chrono::high_resolution_clock::time_point  hd_time;


class KsMessageDialog : public QDialog
{
	Q_OBJECT
public:
	KsMessageDialog(QWidget *parent) = delete;
	KsMessageDialog(QString message, QWidget *parent=0);

private:
	QVBoxLayout 	_layout;
	QLabel 		_text;
	QPushButton 	_close_button;
};

class KsCheckBoxDialog : public QDialog {

	Q_OBJECT

public:
	KsCheckBoxDialog(const QString &n="", QWidget *parent = 0);
	virtual ~KsCheckBoxDialog();

private slots:
	void applyPress();
	void chechAll(int);

signals:
	void apply(QVector<Qt::CheckState>);

protected:
	QCheckBox _all_cb;
	std::vector<QCheckBox*> _cb;

	QVBoxLayout 	*_cb_layout;
	QWidget     	*_cb_widget;
	QVBoxLayout 	 _top_layout;
	QHBoxLayout 	 _button_layout;
	QScrollArea 	 _scrollArea;

private:
	QString 	_name;

	QPushButton 	_cansel_button;
	QPushButton 	_apply_button;
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

class KsDataStore
{
public:
	KsDataStore()
	: _rows(nullptr), _pevt(nullptr),_handle(nullptr), _data_size(0) {}

	~KsDataStore();

	void loadData(struct tracecmd_input *handle);
	void loadData(const QString& file);
	void clear();
	size_t size() const {return _data_size;}

	struct pevent_record **_rows;
	pevent		      *_pevt;

private:
	struct tracecmd_input *_handle;
	size_t _data_size;
};

class KsTimeMap;
class KsChartView;

class KsGraphMark
{
	int		   _bin;
	size_t		   _pos;
	QGraphicsLineItem *_mark;
	QColor 		   _color;
	KsChartView 	  *_graph;

public:

	KsGraphMark()
	: _bin(-1), _pos(0), _mark(nullptr), _color(Qt::darkGreen), _graph(nullptr) {}

	KsGraphMark(QColor col)
	: _bin(-1), _pos(0), _mark(nullptr), _color(col), _graph(nullptr) {}

	bool set(KsDataStore *data, KsTimeMap *map, size_t pos);
	bool reset(KsDataStore *data, KsTimeMap *map);
	bool isSet();

	int bin() const {return _bin;}

	void draw(KsChartView *graph);
	void draw();
	void remove();
};

class KsMarkerState : public QWidget
{
	Q_OBJECT
public:
	KsMarkerState(QWidget *parent = 0);

	enum {
		STATE_A,
		STATE_B
	};

	int getState() const {return _markState;}

	QPushButton _buttonA;
	QPushButton _buttonB;

private slots:
	void setStateA();
	void setStateB();

private:
	QState *_stateA;
	QState *_stateB;
	QStateMachine _machine;
	int _markState;
};


#endif
