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

// C++ 11
#include <chrono>

// Qt
#include <QtWidgets>

// KernelShark
#include "libkshark.h"
#include "libkshark-model.h"
#include "libkshark-json.h"
#include "KsPlotTools.hpp"

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

#define FONT_HEIGHT		fontHeight()
#define FONT_WIDTH 		stringWidth("4")
#define STRING_WIDTH(s)		stringWidth(s)
#define CPU_GRAPH_HEIGHT	(FONT_HEIGHT*2)

typedef std::chrono::high_resolution_clock::time_point  hd_time;
#define GET_TIME std::chrono::high_resolution_clock::now()
#define GET_DURATION(t0) std::chrono::duration_cast<std::chrono::duration<double>>( \
std::chrono::high_resolution_clock::now() - t0).count()

namespace KsUtils {

int getPidList(QVector<int> *pids);

int getPluginList(QStringList *pl);

void listFilterSync(int state);

void graphFilterSync(int state);

QString Ts2String(ssize_t ts, int prec);

}; // KsUtils

class KsDataStore : public QObject
{
	Q_OBJECT

	struct tracecmd_input *_handle;
	size_t _dataSize;

public:
	explicit KsDataStore(QWidget *parent = nullptr);
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
	void update();
	void applyPosTaskFilter(QVector<int>);
	void applyNegTaskFilter(QVector<int>);
	void applyPosEventFilter(QVector<int>);
	void applyNegEventFilter(QVector<int>);
	void clearAllFilters();

public:
	struct kshark_entry  **_rows;
	pevent		      *_pevt;
};

class KsPluginManager : public QObject
{
	Q_OBJECT
public:
	explicit KsPluginManager(QWidget *parent = nullptr);

	/** A list of available plugins. */
	QStringList 	_pluginList;

	/** A list of registered plugins. */
	QVector<bool>	_registeredPlugins;

	void load();
	void unload();

signals:
	void dataReload();

public slots:
	void registerPlugin(const QString &plugin);
	void unregisterPlugin(const QString &plugin);
	void updatePlugins(QVector<int> pluginId);
};

class KsGLWidget;

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
	void setRow(size_t r) {_pos = r;}
	bool set(const KsDataStore &data,
		 kshark_trace_histo *histo,
		 size_t pos,
		 int grCpu,
		 int grTask);

	bool update(const KsDataStore &data, kshark_trace_histo *histo);
	bool isSet();
	bool isVisible();

	int bin()	const {return _bin;}
	size_t row()	const {return _pos;}
	uint64_t ts()	const {return _ts;}
	int cpu()	const {return _cpu;}
	int task()	const {return _task;}

	const QColor &color()	const {return _color;}

	void makeVisible(KsGLWidget *gl);
	void makeVisible();
	void remove();

signals:
	void update(kshark_trace_histo *histo);

private:
	bool		_isSet;
	int		_bin;
	int		_cpu;
	int		_task;
	size_t		_pos;
	uint64_t	_ts;
	QColor		_color;

	KsPlot::Mark	*_mark;
	KsGLWidget	*_gl;
};

DualMarkerState operator !(const DualMarkerState &state);

class KsDualMarkerSM : public QWidget
{
	Q_OBJECT
public:
	explicit KsDualMarkerSM(QWidget *parent = nullptr);
	void reset();
	void restart();
	void placeInToolBar(QToolBar *tb);

	DualMarkerState getState() const {return _markState;}
	void setState(const DualMarkerState &st) {
		if (st == _markState) {
			emit markSwitch();
			return;
		}

		if (st == DualMarkerState::A) {
			emit machineToA();
			doStateA();
		}

		if (st == DualMarkerState::B) {
			emit machineToB();
			doStateB();
		}
	}

	KsGraphMark &getMarker(DualMarkerState s);
	KsGraphMark &activeMarker();
	KsGraphMark &passiveMarker();
	KsGraphMark &markerA();
	KsGraphMark &markerB();

	QState *stateAPtr() {return _stateA;}
	QState *stateBPtr() {return _stateB;}

	void updateMarkers(const KsDataStore &data, kshark_trace_histo *histo);
	void updateLabels(kshark_trace_histo *histo);

signals:
	void markSwitch();
	void updateView(size_t pos, bool mark);
	void showInGraph(size_t pos);
	void machineToA();
	void machineToB();

private slots:
	void doStateA();
	void doStateB();

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

class KsSession
{
	json_object *_jsession;

public:
	KsSession();
	~KsSession();

	json_object *json() {return _jsession;}

	void importFromFile(QString jfileName);
	void exportToFile(QString jfileName);

	void setDataFile(QString fileName, time_t t);
	QString getDataFile();
	time_t getDataFileTS();

	void setVisModel(size_t nBins, uint64_t min, uint64_t max);
	void getVisModel(size_t *nBins, uint64_t *min, uint64_t *max);

	void setCpuPlots(const QVector<int> &cpus);
	QVector<int> getCpuPlots();

	void setTaskPlots(const QVector<int> &tasks);
	QVector<int> getTaskPlots();

	void setFilters(kshark_context *kshark_ctx);
	void getFilters(kshark_context *kshark_ctx);

	void setMainWindowSize(int width, int height);
	void getMainWindowSize(int *width, int *height);

	void setSplitterSize(int graphSize, int viewSize);
	void getSplitterSize(int *graphSize, int *viewSize);

	void setDualMarker(KsDualMarkerSM *dm);
	bool getMarker(const char* name, size_t *pos);
	DualMarkerState getMarkerState();

	void setPlugins(const QStringList &pluginList,
			const QVector<bool> &registeredPlugins);
	void getPlugins(QStringList *pluginList,
			QVector<bool> *registeredPlugins);

	void setViewTop(size_t topRow) {
		json_object_object_add(_jsession, "ViewTop",
				       json_object_new_int64(topRow));
	}
	size_t getViewTop() {
		json_object *jrow;
		json_object_object_get_ex(_jsession, "ViewTop", &jrow);
		return json_object_get_int64(jrow);
	}

	void setColorScheme() {
		json_object_object_add(_jsession,
				       "ColorScheme",
				       json_object_new_double(KsPlot::Color::getRainbowFrequency()));
	}

	float getColorScheme() {
		json_object *jcol;
		json_object_object_get_ex(_jsession, "ColorScheme", &jcol);
		float colSch = json_object_get_double(jcol);
		KsPlot::Color::setRainbowFrequency(colSch);
		return colSch;
	}
};

#endif
