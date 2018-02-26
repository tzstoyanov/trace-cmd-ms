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

// auto graph_height = [] (int scale)
// {
// 	int h = (SCREEN_HEIGHT < SCREEN_WIDTH)? SCREEN_HEIGHT : SCREEN_WIDTH;
// 	return scale*h/35;
// };
// #define CPU_GRAPH_HEIGHT (graph_height(1))

#define CPU_GRAPH_HEIGHT (FONT_HEIGHT*2)

#define KS_VIEW_FILTER_MASK   0x1
#define KS_GRAPH_FILTER_MASK  0x2

typedef std::chrono::high_resolution_clock::time_point  hd_time;
#define GET_TIME std::chrono::high_resolution_clock::now()
#define GET_DURATION(t0) std::chrono::duration_cast<std::chrono::duration<double>>( \
std::chrono::high_resolution_clock::now() - t0).count()

int getPidList(QVector<int> *pids);
int getPluginList(QStringList *pl);

class KsDataProgressBar : public QWidget {
	Q_OBJECT

	QStatusBar	_sb;
	QProgressBar	_pb;

public:
	KsDataProgressBar(QWidget *parent = 0);

public slots:
	void setValue(int i);
};

class KsMessageDialog : public QDialog
{
	Q_OBJECT
public:
	KsMessageDialog(QWidget *parent) = delete;
	KsMessageDialog(QString message, QWidget *parent = 0);

private:
	QVBoxLayout	_layout;
	QLabel		_text;
	QPushButton	_closeButton;
};

class KsCheckBoxDialog : public QDialog
{
	Q_OBJECT
public:
	KsCheckBoxDialog(const QString &name = "", bool cond = true, QWidget *parent = 0);

	void setDefault(bool);
	virtual void set(QVector<bool> v) =0;

private slots:
	virtual void applyPress() =0;
	virtual void chechAll(bool) =0;

signals:
	void apply(QVector<int>);

protected:
	void resizeEvent(QResizeEvent* event);

	bool		_positiveCond;
	QCheckBox	_allCb;
	QVector<int>    _id;

	QVBoxLayout	*_cbLayout;
	QWidget		*_cbWidget;
	QVBoxLayout	 _topLayout;
	QHBoxLayout	 _buttonLayout;
	QScrollArea	 _scrollArea;

protected:
	QString		_name;
	QPushButton	_canselButton;
	QPushButton	_applyButton;
};

class KsCheckBoxTableDialog : public KsCheckBoxDialog
{
	Q_OBJECT
public:
	KsCheckBoxTableDialog(const QString &name = "", bool cond = true, QWidget *parent = 0);

	void set(QVector<bool> v) override;

private slots:
	void applyPress() override;
	void chechAll(bool) override;
	void update(bool);

protected:
	void initTable(QStringList headers, int size);
	void adjustSize();

	QTableWidget		_table;
	QVector<QCheckBox*>	_cb;
};

class KsCheckBoxTreeDialog : public KsCheckBoxDialog
{
	Q_OBJECT
public:
	KsCheckBoxTreeDialog(const QString &name = "", bool cond = true, QWidget *parent = 0);

	void set(QVector<bool> v) override;

private slots:
	void applyPress() override;
	void chechAll(bool) override;
	void update(QTreeWidgetItem *item, int column);

protected:
	void initTree();
	void adjustSize(int count);

	QTreeWidget			_tree;
	QVector<QTreeWidgetItem*>	_cb;
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

struct KsPluginCheckBoxDialog : public KsCheckBoxTableDialog
{
	KsPluginCheckBoxDialog(struct pevent *pe,
			       QStringList pluginList,
			       bool cond = true,
			       QWidget *parent = 0);
};

class KsDataStore : public QObject
{
	Q_OBJECT

	struct tracecmd_input *_handle;
	size_t _dataSize;

public:
	KsDataStore();
	~KsDataStore();

	void loadData(const QString &file);
// 	void loadData(const QList<QString> &file);
	void clear();
	size_t size() const {return _dataSize;}

signals:
	void updateView(KsDataStore *);
	void updateGraph(KsDataStore *);

public slots:
	void reload();

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

class KsPluginManager : public QObject
{
	Q_OBJECT
public:
	KsPluginManager();

	/** A list of available plugins. */
	QStringList 	_pluginList;

	/** A list of registered plugins. */
	QVector<bool>	_registeredPlugins;

signals:
	void dataReload();

public slots:
	void registerPlugin(QString plugin);
	void unregisterPlugin(QString plugin);
	void updatePlugins(QVector<int> pluginId);
};

class KsTimeMap;
class KsGLWidget;

namespace KsPlot {
	class Mark;
};

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

	void reset();
	const DualMarkerState _state;

	KsPlot::Mark *markPtr() {return _mark;}

	void setGLWidget(KsGLWidget *gl) {_gl = gl;}
	bool set(const KsDataStore &data,
		 const KsTimeMap &histo,
		 size_t pos,
		 int grCpu,
		 int grTask);

	bool update(const KsDataStore &data, const KsTimeMap &histo);
	bool isSet();
	bool isVisible();

	int bin()		const {return _bin;}
	size_t row()		const {return _pos;}
	const QColor &color()	const {return _color;}

	void makeVisible(KsGLWidget *gl);
	void makeVisible();
	void remove();

signals:
	void update(KsTimeMap *histo);

private:
	bool	_isSet;
	int	_bin;
	int	_cpu;
	int	_task;
	size_t	_pos;
	QColor	_color;

	KsPlot::Mark	*_mark;
	KsGLWidget	*_gl;
};

DualMarkerState operator !(const DualMarkerState &state);

class KsDualMarkerSM : public QWidget
{
	Q_OBJECT
public:
	KsDualMarkerSM(QWidget *parent = 0);
	void reset();
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
	void updateView(size_t pos, bool mark);
	void showInGraph(size_t pos);

private slots:
	void setStateA();
	void setStateB();

private:
	QPushButton	 _buttonA;
	QPushButton	 _buttonB;
	QLabel		 _labelMA, _labelMB, _labelDelta;
	QLabel		 _labelDeltaDescr;
	QState		*_stateA;
	QState		*_stateB;
	QStateMachine	 _machine;

	DualMarkerState	 _markState;
	KsGraphMark	 _markA, _markB;
	QShortcut        _scCtrlA, _scCtrlB;
};

#endif
