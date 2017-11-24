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
	QVBoxLayout	_layout;
	QLabel		_text;
	QPushButton	_close_button;
};

class KsCheckBoxDialog : public QDialog
{
	Q_OBJECT
public:
	KsCheckBoxDialog(const QString &name = "", bool cond = true, QWidget *parent = 0);

	void setDefault(bool);

private slots:
	virtual void applyPress() =0;
	virtual void chechAll(bool) =0;

signals:
	void apply(QVector<int>);

protected:
	void resizeEvent(QResizeEvent* event);

	bool	_positiveCond;
	QCheckBox 	_all_cb;
	QVector<int>    _id;

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

class KsCheckBoxTableDialog : public KsCheckBoxDialog
{
	Q_OBJECT
public:
	KsCheckBoxTableDialog(const QString &name = "", bool cond = true, QWidget *parent = 0);

private slots:
	void applyPress() override;
	void chechAll(bool) override;
	void update(bool);

protected:
	void initTable(QStringList headers, int size);
	void adjustSize();

	QTableWidget _table;
	QVector<QCheckBox*> _cb;
};

class KsCheckBoxTreeDialog : public KsCheckBoxDialog
{
	Q_OBJECT
public:
	KsCheckBoxTreeDialog(const QString &name = "", bool cond = true, QWidget *parent = 0);

private slots:
	void applyPress() override;
	void chechAll(bool) override;
	void update(QTreeWidgetItem *item, int column);

protected:
	void initTree();
	void adjustSize(int count);

	QTreeWidget _tree;
	QVector<QTreeWidgetItem*> _cb;
};

struct KsCpuCheckBoxDialog : public KsCheckBoxTreeDialog
{
	KsCpuCheckBoxDialog(struct pevent *pe, bool cond = true, QWidget *parent = 0);
};

struct KsEventsCheckBoxDialog : public KsCheckBoxTreeDialog
{
	KsEventsCheckBoxDialog(struct pevent *pe, bool cond = true, QWidget *parent = 0);
};

struct KsTasksCheckBoxDialog : public KsCheckBoxTableDialog
{
	KsTasksCheckBoxDialog(struct pevent *pe, bool cond = true, QWidget *parent = 0);
};

class KsDataStore : public QObject
{
	Q_OBJECT

	struct tracecmd_input *_handle;
	size_t _data_size;

public:
	KsDataStore();
	~KsDataStore();

	void loadData(struct tracecmd_input *handle);
	void loadData(const QString& file);
	void clear();
	size_t size() const {return _data_size;}

signals:
	void updateView();
	void updateGraph();

private slots:
	void applyPosTaskFilter(QVector<int>);
	void applyNegTaskFilter(QVector<int>);
	void applyPosEventFilter(QVector<int>);
	void applyNegEventFilter(QVector<int>);

public:
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
	int			 _bin;
	size_t		 	 _pos;
	QColor 			 _color;
	QGraphicsLineItem	*_mark;
	KsChartView 		*_graph;
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
	QLabel		 _labelMA, _labelMB, _labelDelta;
	QLabel		 _labelDeltaDescr;
	QState		*_stateA;
	QState		*_stateB;
	QStateMachine 	 _machine;

	DualMarkerState	 _markState;
	KsGraphMark 	 _markA, _markB;
};

#endif
