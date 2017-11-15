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

#ifndef _KS_UTILS_H
#define _KS_UTILS_H

// C++
#include <chrono>

// Qt
#include <QtWidgets>

// trace-cmd
#include "event-parse.h"

// Kernel Shark 2
#include "libkshark.h"

#define SCREEN_HEIGHT  QApplication::desktop()->screenGeometry().height()
#define SCREEN_WIDTH   QApplication::desktop()->screenGeometry().width()

auto fontHeight = [] ()
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

typedef std::chrono::high_resolution_clock::time_point  hd_time;
#define GET_TIME std::chrono::high_resolution_clock::now()
#define GET_DURATION(t0) std::chrono::duration_cast<std::chrono::duration<double>>( \
std::chrono::high_resolution_clock::now()-t0).count()

class KsMessageDialog : public QDialog
{
	Q_OBJECT
public:
	KsMessageDialog(QWidget *parent) = delete;
	KsMessageDialog(QString message, QWidget *parent = 0);

private:
	QVBoxLayout _layout;
	QLabel 	    _text;
	QPushButton _close_button;
};

class KsCheckBoxDialog : public QDialog
{
	Q_OBJECT
public:
	KsCheckBoxDialog(const QString &n = "", QWidget *parent = 0);
	void cbResize(size_t n);

private slots:
	void applyPress();
	void chechAll(int);

signals:
	void apply(QVector<Qt::CheckState>);

protected:
	void resizeEvent(QResizeEvent* event);

	QCheckBox 	    _all_cb;
	QVector<QCheckBox*> _cb;

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

struct KSCpuCheckBoxDialog : public KsCheckBoxDialog
{
	KSCpuCheckBoxDialog(struct pevent *pe, QWidget *parent = 0);
};

struct KSTasksCheckBoxDialog : public KsCheckBoxDialog
{
	KSTasksCheckBoxDialog(struct pevent *pe, QWidget *parent = 0);
};

struct KSEventsCheckBoxDialog : public KsCheckBoxDialog
{
	KSEventsCheckBoxDialog(struct pevent *pe, QWidget *parent = 0);
};

class KsDataStore
{
	struct tracecmd_input *_handle;
	size_t _data_size;

public:
	KsDataStore();
	~KsDataStore();

	void loadData(struct tracecmd_input *handle);
	void loadData(const QString& file);
	void clear();
	size_t size() const {return _data_size;}

	struct kshark_entry  **_rows;
	//struct pevent_record **_rows;
	pevent		      *_pevt;
};

class KsTimeMap;
class KsChartView;

enum class DualMarkerState
{
	A,
	B
};

class KsGraphMark : public QObject
{
	Q_OBJECT
public:
	KsGraphMark() = delete;
	KsGraphMark(DualMarkerState s);
	KsGraphMark(DualMarkerState s, QColor col);

	const DualMarkerState _state;

	bool set(const KsDataStore &data, const KsTimeMap &histo, size_t pos);
	bool reset(const KsDataStore &data, const KsTimeMap &histo);
	bool isSet();

	int bin() const {return _bin;}
	int row() const {return _pos;}
	void draw(KsChartView *graph);
	void draw();
	void remove();

signals:
	void update(KsTimeMap *histo);

private:
	int		   _bin;
	size_t		   _pos;
	QColor 		   _color;
	QGraphicsLineItem *_mark;
	KsChartView 	  *_graph;
};

DualMarkerState operator !(const DualMarkerState &state);

class KsDualMarkerSM : public QWidget
{
	Q_OBJECT
public:
	KsDualMarkerSM(QWidget *parent = 0);
	
	void placeInToolBar(QToolBar *tb);

	DualMarkerState getState() const {return _markState;}

	KsGraphMark &getMarker(DualMarkerState s);
	KsGraphMark &activeMarker();
	KsGraphMark &markerA();
	KsGraphMark &markerB();

	QState *stateAPtr() {return _stateA;}
	QState *stateBPtr() {return _stateB;}

	void updateMarkers(const KsDataStore &data, const KsTimeMap &histo);
	void updateLabels(const KsTimeMap &histo);

signals:
	void markSwitch();

private slots:
	void setStateA();
	void setStateB();

private:
	QPushButton 	 _buttonA;
	QPushButton 	 _buttonB;
	QLabel 	    	 _labelMA, _labelMB, _labelDelta;
	QLabel 	    	 _labelDeltaDescr;
	QState		*_stateA;
	QState 		*_stateB;
	QStateMachine 	 _machine;

	DualMarkerState	 _markState;
	KsGraphMark 	 _markA, _markB;
};


#endif
