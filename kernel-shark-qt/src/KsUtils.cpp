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

#include <json_object.h>

//Qt
#include <QFrame>

// Kernel Shark 2
#include "KsDeff.h"
#include "KsUtils.hpp"
#include "KsPlotTools.hpp"
#include "KsTraceGraph.hpp"

namespace KsUtils {

int getPidList(QVector<int> *pids)
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	struct task_list *list;

	for (int i = 0; i < TASK_HASH_SIZE; ++i) {
		list = kshark_ctx->tasks[i];
		while (list) {
			pids->append(list->pid);
			list = list->next;
		}
	}

	qSort(*pids);
	return pids->size();
}

int getPluginList(QStringList *pl)
{
	*pl = plugins.split(";");
	return pl->count();
}

void listFilterSync(int st)
{
	bool state = (bool) st;

	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	if (state) {
		kshark_ctx->filter_mask |= KS_VIEW_FILTER_MASK;
	} else {
		kshark_ctx->filter_mask &= ~KS_VIEW_FILTER_MASK;
	}
}

void graphFilterSync(int st)
{
	bool state = (bool) st;

	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	if (state) {
		kshark_ctx->filter_mask |= KS_GRAPH_FILTER_MASK;
	} else {
		kshark_ctx->filter_mask &= ~KS_GRAPH_FILTER_MASK;
	}
}

}; // KsUtils

KsDataStore::KsDataStore(QWidget *parent)
: QObject(parent),
  _handle(nullptr),
  _dataSize(0),
  _rows(nullptr),
  _pevt(nullptr)
{}

KsDataStore::~KsDataStore()
{
// 	clear();
}

void KsDataStore::loadData(const QString &file)
{
	hd_time t0 = GET_TIME;

	clear();

	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	bool status = kshark_open(kshark_ctx, file.toStdString().c_str());
	if (!status)
		return;

	_handle = kshark_ctx->handle;
	_pevt = kshark_ctx->pevt;

	if (!_handle || !_pevt) {
		qCritical() << "ERROR Loading file " << file;
		return;
	}

	if (kshark_ctx->event_handlers == NULL) {
		kshark_handle_plugins(kshark_ctx, KSHARK_PLUGIN_LOAD);
	} else {
		kshark_handle_plugins(kshark_ctx, KSHARK_PLUGIN_RELOAD);
	}

	_dataSize = kshark_load_data_entries(kshark_ctx, &_rows);

	double time = GET_DURATION(t0);
	qInfo() << "Data loading time: " << 1e3*time << " ms.   entries: " << _dataSize;
}

void KsDataStore::reload()
{
	hd_time t0 = GET_TIME;

	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	for (size_t r = 0; r < _dataSize; ++r)
		free(_rows[r]);

	free(_rows);
	_rows = nullptr;

	_dataSize = kshark_load_data_entries(kshark_ctx, &_rows);

	emit updateView(this);
	emit updateGraph(this);

	double time = GET_DURATION(t0);
	qInfo() << "Data reloading time: " << 1e3*time << " ms.   entries: " << _dataSize;
}

void KsDataStore::clear()
{
	if (_dataSize) {
		for (size_t r = 0; r < _dataSize; ++r)
			//free_record(_rows[r]);
			free(_rows[r]);

		free(_rows);
		_rows = nullptr;
		_dataSize = 0;
	}

	_pevt = nullptr;

	if (_handle) {
		struct kshark_context *kshark_ctx = NULL;
		kshark_instance(&kshark_ctx);
		kshark_close(kshark_ctx);
		_handle = nullptr;
	}
}

void KsDataStore::update()
{
	hd_time t0 = GET_TIME;

	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	if ((kshark_ctx->show_task_filter && filter_id_count(kshark_ctx->show_task_filter)) ||
	    (kshark_ctx->hide_task_filter && filter_id_count(kshark_ctx->hide_task_filter)) ||
	    (kshark_ctx->show_event_filter && filter_id_count(kshark_ctx->show_event_filter)) ||
	    (kshark_ctx->hide_event_filter && filter_id_count(kshark_ctx->hide_event_filter))) {
		kshark_filter_entries(kshark_ctx, _rows, _dataSize);

		emit updateView(this);
		emit updateGraph(this);
	}

	double time = GET_DURATION(t0);
	qInfo() << "Update time (PTF): " << 1e3*time << " ms.";
}

void KsDataStore::applyPosTaskFilter(QVector<int> vec)
{
	hd_time t0 = GET_TIME;

	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	kshark_filter_clear(kshark_ctx, SHOW_TASK_FILTER);
	kshark_filter_clear(kshark_ctx, HIDE_TASK_FILTER);
	for (auto const &pid: vec)
		kshark_filter_add_id(kshark_ctx, SHOW_TASK_FILTER, pid);

	/*
	 * If the advanced event filter is set the data has to be reloaded,
	 * because the advanced filter uses pevent_records.
	 */
	if (kshark_ctx->adv_filter_is_set)
		reload();
	else
		kshark_filter_entries(kshark_ctx, _rows, _dataSize);

	emit updateView(this);
	emit updateGraph(this);

	double time = GET_DURATION(t0);
	qInfo() << "Filtering time (PTF): " << 1e3*time << " ms.";
}

void KsDataStore::applyNegTaskFilter(QVector<int> vec)
{
	hd_time t0 = GET_TIME;

	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	kshark_filter_clear(kshark_ctx, SHOW_TASK_FILTER);
	kshark_filter_clear(kshark_ctx, HIDE_TASK_FILTER);
	for (auto const &pid: vec)
		kshark_filter_add_id(kshark_ctx, HIDE_TASK_FILTER, pid);

	/*
	 * If the advanced event filter is set the data has to be reloaded,
	 * because the advanced filter uses pevent_records.
	 */
	if (kshark_ctx->adv_filter_is_set)
		reload();
	else
		kshark_filter_entries(kshark_ctx, _rows, _dataSize);

	emit updateView(this);
	emit updateGraph(this);

	double time = GET_DURATION(t0);
	qInfo() << "Filtering time (NTF): " << 1e3*time << " ms.";
}

void KsDataStore::applyPosEventFilter(QVector<int> vec)
{
	hd_time t0 = GET_TIME;

	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	kshark_filter_clear(kshark_ctx, SHOW_EVENT_FILTER);
	kshark_filter_clear(kshark_ctx, HIDE_EVENT_FILTER);

	for (auto const &pid: vec)
		kshark_filter_add_id(kshark_ctx, SHOW_EVENT_FILTER, pid);

	/*
	 * If the advanced event filter is set the data has to be reloaded,
	 * because the advanced filter uses pevent_records.
	 */
	if (kshark_ctx->adv_filter_is_set)
		reload();
	else
		kshark_filter_entries(kshark_ctx, _rows, _dataSize);

	emit updateView(this);
	emit updateGraph(this);

	double time = GET_DURATION(t0);
	qInfo() << "Filtering time (PEF): " << 1e3*time << " ms.";
}

void KsDataStore::applyNegEventFilter(QVector<int> vec)
{
	hd_time t0 = GET_TIME;

	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	kshark_filter_clear(kshark_ctx, SHOW_EVENT_FILTER);
	kshark_filter_clear(kshark_ctx, HIDE_EVENT_FILTER);
	for (auto const &pid: vec)
		kshark_filter_add_id(kshark_ctx, HIDE_EVENT_FILTER, pid);

	/*
	 * If the advanced event filter is set the data has to be reloaded,
	 * because the advanced filter uses pevent_records.
	 */
	if (kshark_ctx->adv_filter_is_set)
		reload();
	else
		kshark_filter_entries(kshark_ctx, _rows, _dataSize);

	emit updateView(this);
	emit updateGraph(this);

	double time = GET_DURATION(t0);
	qInfo() << "Filtering time (NEF): " << 1e3*time << " ms.";
}

void KsDataStore::clearAllFilters()
{
	if (!_pevt)
		return;

	hd_time t0 = GET_TIME;

	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	kshark_filter_clear(kshark_ctx, SHOW_TASK_FILTER);
	kshark_filter_clear(kshark_ctx, HIDE_TASK_FILTER);
	kshark_filter_clear(kshark_ctx, SHOW_EVENT_FILTER);
	kshark_filter_clear(kshark_ctx, HIDE_EVENT_FILTER);

	pevent_filter_reset(kshark_ctx->advanced_event_filter);
	kshark_filter_entries(kshark_ctx, _rows, _dataSize);

	emit updateView(this);
	emit updateGraph(this);

	double time = GET_DURATION(t0);
	qInfo() << "Filtering time (clearAll): " << 1e3*time << " ms.";
}

KsPluginManager::KsPluginManager(QWidget *parent)
: QObject(parent)
{
	int nPlugins = KsUtils::getPluginList(&_pluginList);
	_registeredPlugins.resize(nPlugins);

	for (int i = 0; i < nPlugins; ++i) {
		if (_pluginList[i].contains(" default", Qt::CaseInsensitive)) {
			_pluginList[i].remove(" default", Qt::CaseInsensitive);
			registerPlugin(_pluginList[i]);
		} else {
			_registeredPlugins[i] = false;
		}
	}
}

void KsPluginManager::registerPlugin(const QString &plugin)
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);
	char *lib;

	for (int i = 0; i < _pluginList.count(); ++i) {
		if (_pluginList[i] == plugin) {
			asprintf(&lib, "%s/lib/plugin-%s.so", KS_DIR,
							      plugin.toStdString().c_str());

			kshark_register_plugin(kshark_ctx, lib);
			_registeredPlugins[i] = true;
			return;
		} else if (plugin.contains("/lib/plugin-" +
			                   _pluginList[i], Qt::CaseInsensitive)) {
			asprintf(&lib, "%s", plugin.toStdString().c_str());

			kshark_register_plugin(kshark_ctx, lib);
			_registeredPlugins[i] = true;
			return;
		}
	}

	/*
	 * No plugin with this name in the list.
	 * Try to add it anyway.
	 */
	asprintf(&lib, "%s", plugin.toStdString().c_str());
	kshark_register_plugin(kshark_ctx, lib);
	_pluginList.append(plugin);
	_registeredPlugins.append(true);
}

void KsPluginManager::unregisterPlugin(const QString &plugin)
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	for (int i = 0; i < _pluginList.count(); ++i) {
		char *lib;
		if (_pluginList[i] == plugin) {
			asprintf(&lib, "%s/lib/plugin-%s.so", KS_DIR,
							      plugin.toStdString().c_str());

			kshark_unregister_plugin(kshark_ctx, lib);
			_registeredPlugins[i] = false;
			return;
		} else if  (plugin.contains("/lib/plugin-" +
			                   _pluginList[i], Qt::CaseInsensitive)) {
			asprintf(&lib, "%s", plugin.toStdString().c_str());

			kshark_unregister_plugin(kshark_ctx, lib);
			_registeredPlugins[i] = false;
			return;
		}
	}
}

void KsPluginManager::load()
{
	size_t nPlugins = _pluginList.count();
	for (size_t i = 0; i < nPlugins; ++i)
		if (_registeredPlugins[i])
			registerPlugin(_pluginList[i]);
}

void KsPluginManager::unload()
{
	size_t nPlugins = _pluginList.count();
	for (size_t i = 0; i < nPlugins; ++i)
		if (_registeredPlugins[i]) {
			unregisterPlugin(_pluginList[i]);
			_registeredPlugins[i] = false;
		}

	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	kshark_handle_plugins(kshark_ctx, KSHARK_PLUGIN_UNLOAD);
	kshark_free_plugin_list(kshark_ctx->plugins);
	kshark_ctx->plugins = NULL;
	kshark_free_event_handler_list(kshark_ctx->event_handlers);
}

void KsPluginManager::updatePlugins(QVector<int> pluginId)
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	kshark_handle_plugins(kshark_ctx, KSHARK_PLUGIN_UNLOAD);
	kshark_free_plugin_list(kshark_ctx->plugins);
	kshark_ctx->plugins = NULL;
	kshark_free_event_handler_list(kshark_ctx->event_handlers);

	for (auto &p: _registeredPlugins)
		p = false;

	for (auto const &p: pluginId)
		registerPlugin(_pluginList[p]);

	kshark_handle_plugins(kshark_ctx, KSHARK_PLUGIN_LOAD);

	emit dataReload();
}

KsGraphMark::KsGraphMark(DualMarkerState s)
: _state(s), _isSet(false), _bin(-1), _cpu(-1), _task(-1), _pos(0), _color(Qt::darkGreen)
{
	_mark = new KsPlot::Mark;
	_mark->_color = _color;
}

KsGraphMark::KsGraphMark(DualMarkerState s, QColor col)
: _state(s), _isSet(false), _bin(-1), _cpu(-1), _task(-1), _pos(0), _color(col)
{
	_mark = new KsPlot::Mark;
	_mark->_color = _color;
}

void KsGraphMark::reset()
{
	_isSet = false;
	_mark->_visible = false;
	_bin = -1;
	_pos = 0;
	_cpu = -1;
	_task = -1;
}

bool KsGraphMark::set(const KsDataStore &data,
		      const  KsTimeMap &histo,
		      size_t pos, int grCpu, int grTask)
{
	_isSet = true;
	_pos = pos;
	_cpu = grCpu;
	_task = grTask;
	size_t ts = data._rows[_pos]->ts;
	if (ts > histo._max || ts < histo._min) {
		_bin = -1;
		_mark->_visible = false;
		return false;
	}
	
	_bin = (ts - histo._min)/histo._binSize;

	return true;
}

bool KsGraphMark::update(const KsDataStore &data, const KsTimeMap &histo)
{
	if (!_isSet)
		return false;

	return set(data, histo, this->_pos, this->_cpu, this->_task);
}

bool KsGraphMark::isSet()
{
	return _isSet;
}

bool KsGraphMark::isVisible()
{
	return _mark->_visible;
}

void KsGraphMark::makeVisible(KsGLWidget *gl)
{
	_gl = gl;
	_mark->setMark(_bin, _cpu, _task, _gl, _gl->dpr());
	_mark->_visible = true;
}

void KsGraphMark::makeVisible()
{
	makeVisible(this->_gl);
}

void KsGraphMark::remove()
{
	_isSet = false;
	_mark->_visible = false;
}

DualMarkerState operator !(const DualMarkerState &state)
{
	if (state == DualMarkerState::B)
		return DualMarkerState::A;

	return DualMarkerState::B;
}

KsDualMarkerSM::KsDualMarkerSM(QWidget *parent)
: QWidget(parent),
  _buttonA("Marker A", this),
  _buttonB("Marker B", this),
  _labelDeltaDescr("    A,B Delta: ", this),
  _markA(DualMarkerState::A, Qt::darkGreen),
  _markB(DualMarkerState::B, Qt::darkCyan),
  _scCtrlA(this),
  _scCtrlB(this)
{
	_buttonA.setFixedWidth(STRING_WIDTH(" Marker A "));
	_buttonB.setFixedWidth(STRING_WIDTH(" Marker B "));

	for (auto const &l: {&_labelMA, &_labelMB, &_labelDelta}) {
		l->setFrameStyle(QFrame::Panel | QFrame::Sunken);
		l->setStyleSheet("QLabel {background-color : white;}");
		l->setTextInteractionFlags(Qt::TextSelectableByMouse);
		l->setFixedWidth(FONT_WIDTH*16);
	}

	QString styleSheetA = "background : " +
			      _markA.color().name() +
			      "; color : white";

	_stateA = new QState;
	_stateA->setObjectName("A");
	_stateA->assignProperty(&_buttonA,
				"styleSheet",
				styleSheetA);

	_stateA->assignProperty(&_buttonB,
				"styleSheet",
				"color : rgb(70, 70, 70)");

	QString styleSheetB = "background : " +
			      _markB.color().name() +
			      "; color : white";
	_stateB = new QState;
	_stateB->setObjectName("B");
	_stateB->assignProperty(&_buttonA,
				"styleSheet",
				"color : rgb(70, 70, 70)");

	_stateB->assignProperty(&_buttonB,
				"styleSheet",
				styleSheetB);

	/* Define transitions from State A to State B. */
	_stateA->addTransition(this,	&KsDualMarkerSM::machineToB, _stateB);

	_scCtrlA.setKey(Qt::CTRL + Qt::Key_A);
	_stateA->addTransition(&_scCtrlB, &QShortcut::activated,_stateB);
	connect(&_scCtrlA,	&QShortcut::activated,
		this,		&KsDualMarkerSM::doStateA);

	_stateA->addTransition(&_buttonB, &QPushButton::clicked, _stateB);
	connect(&_buttonB,	&QPushButton::clicked,
		this,		&KsDualMarkerSM::doStateB);

	/* Define transitions from State B to State A. */
	_stateB->addTransition(this,	&KsDualMarkerSM::machineToA, _stateA);

	_scCtrlB.setKey(Qt::CTRL + Qt::Key_B);
	_stateB->addTransition(&_scCtrlA, &QShortcut::activated,_stateA);
	connect(&_scCtrlB,	&QShortcut::activated,
		this,		&KsDualMarkerSM::doStateB);

	_stateB->addTransition(&_buttonA, &QPushButton::clicked, _stateA);
	connect(&_buttonA,	&QPushButton::clicked,
		this,		&KsDualMarkerSM::doStateA);

	_machine.addState(_stateA);
	_machine.addState(_stateB);
	_machine.setInitialState(_stateA);
	_markState = DualMarkerState::A;
	_machine.start();
}

void KsDualMarkerSM::reset()
{
	_markA.reset();
	_markB.reset();
	_labelMA.setText("");
	_labelMB.setText(""); 
	_labelDelta.setText("");
}

void KsDualMarkerSM::restart()
{
	_machine.stop();
	reset();
	_markState = DualMarkerState::A;
	_machine.start();
}

void KsDualMarkerSM::doStateA()
{
	if (_markState !=  DualMarkerState::A) {
		_markState = DualMarkerState::A;
		emit markSwitch();
	} else if (activeMarker().isSet()) {
		emit updateView(activeMarker().row(), true);
		emit showInGraph(activeMarker().row());
	}
}

void KsDualMarkerSM::doStateB()
{
	if (_markState !=  DualMarkerState::B) {
		_markState = DualMarkerState::B;
		emit markSwitch();
	} else if (activeMarker().isSet()) {
		emit updateView(activeMarker().row(), true);
		emit showInGraph(activeMarker().row());
	}
}

KsGraphMark &KsDualMarkerSM::getMarker(DualMarkerState s)
{
	if (s == DualMarkerState::A)
		return _markA;

	return _markB;
}

KsGraphMark &KsDualMarkerSM::activeMarker()
{
	return this->getMarker(_markState);
}

KsGraphMark &KsDualMarkerSM::markerA()
{
	return _markA;
}

KsGraphMark &KsDualMarkerSM::markerB()
{
	return _markB;
}

void KsDualMarkerSM::placeInToolBar(QToolBar *tb)
{
	tb->addWidget(new QLabel("   "));
	tb->addWidget(&_buttonA);
	tb->addWidget(&_labelMA);
	tb->addSeparator();
	tb->addWidget(new QLabel("   "));
	tb->addWidget(&_buttonB);
	tb->addWidget(&_labelMB);
	tb->addSeparator();
	tb->addWidget(&_labelDeltaDescr);
	tb->addWidget(&_labelDelta);
}

void KsDualMarkerSM::updateMarkers(const KsDataStore &data, const KsTimeMap &histo)
{
	if(_markA.update(data, histo))
		_markA.makeVisible();

	if(_markB.update(data, histo))
		_markB.makeVisible();
}

void KsDualMarkerSM::updateLabels(const KsTimeMap &histo)
{
	// Marker A
	if (_markA.isSet()) {
		QString markA = QString::number(histo.binTime(_markA.bin()), 'f', 7);
		_labelMA.setText(QString("%1").arg(markA));
	} else {
		_labelMA.setText("");
	}

	// Marker B
	if (_markB.isSet()) {
		QString markB = QString::number(histo.binTime(_markB.bin()), 'f', 7);
		_labelMB.setText(QString("%1").arg(markB));
	} else {
		_labelMB.setText("");
	}

	// Delta
	if (_markA.isSet() && _markB.isSet()) {
		QString delta = QString::number(histo.binTime(_markB.bin()) -
						histo.binTime(_markA.bin()),
						'f', 7);
		_labelDelta.setText(QString("%1").arg(delta));
	} else {
		_labelDelta.setText("");
	}
}

KsSession::KsSession()
{
	_jsession = kshark_config_alloc("kshark.session.config");

	json_object *jwindow = json_object_new_array();
	json_object *jwidth = json_object_new_int(0);
	json_object *jheight = json_object_new_int(0);
	json_object_array_add(jwindow, jwidth);
	json_object_array_add(jwindow, jheight);
	json_object_object_add(_jsession, "MainWindow", jwindow);

	json_object *jsplitter = json_object_new_array();
	json_object *jgraphsize = json_object_new_int(0);
	json_object *jviewsize = json_object_new_int(0);
	json_object_array_add(jsplitter, jgraphsize);
	json_object_array_add(jsplitter, jviewsize);
	json_object_object_add(_jsession, "Splitter", jsplitter);

	json_object *jvisrange = json_object_new_array();
	json_object *jmin = json_object_new_int(0);
	json_object *jmax = json_object_new_int(0);
	json_object_array_add(jvisrange, jmin);
	json_object_array_add(jvisrange, jmax);
	json_object_object_add(_jsession, "VisModel", jvisrange);

	json_object *jcpuplots = json_object_new_array();
	json_object_object_add(_jsession, "CpuPlots", jcpuplots);

	json_object *jtaskplots = json_object_new_array();
	json_object_object_add(_jsession, "TaskPlots", jtaskplots);

	json_object *jd_mark = kshark_config_alloc("kshark.markers.config");
	json_object_object_add(_jsession, "Markers", jd_mark);

	json_object *jplugins = kshark_config_alloc("kshark.plugins.config");
	json_object_object_add(_jsession, "Plugins", jplugins);
}

KsSession::~KsSession()
{
	json_object_put(_jsession);
}

void KsSession::importFromFile(QString jfileName)
{
	json_object *jobj =
		kshark_open_json_file(jfileName.toStdString().c_str(),
				      "kshark.session.config");

	if (!jobj)
		return;

	json_object_put(_jsession);
	_jsession = jobj;

}

void KsSession::exportToFile(QString jfileName)
{
	kshark_save_json_file(jfileName.toStdString().c_str(), _jsession);
}

void KsSession::setDataFile(QString fileName, time_t t)
{
	json_object *jdatafile, *jdatafile_ts;
	if (json_object_object_get_ex(_jsession, "DataFile", &jdatafile))
		json_object_object_del(_jsession, "DataFile");

	jdatafile = json_object_new_string(fileName.toStdString().c_str());
	json_object_object_add(_jsession, "DataFile", jdatafile);

	if (json_object_object_get_ex(_jsession, "DataFileTS", &jdatafile))
		json_object_object_del(_jsession, "DataFileTS");

	jdatafile_ts = json_object_new_int64(t);
	json_object_object_add(_jsession, "DataFileTS", jdatafile_ts);
}

QString KsSession::getDataFile()
{
	json_object *jdatafile;
	json_object_object_get_ex(_jsession, "DataFile", &jdatafile);
	return QString(json_object_get_string(jdatafile));
}

time_t KsSession::getDataFileTS()
{
	json_object *jdatafile_ts;
	json_object_object_get_ex(_jsession, "DataFileTS", &jdatafile_ts);
	return json_object_get_int64(jdatafile_ts);
}

void KsSession::setVisModel(size_t nBins, uint64_t min, uint64_t max)
{
	json_object *jrange;
	json_object_object_get_ex(_jsession, "VisModel", &jrange);
	json_object_array_put_idx(jrange, 0, json_object_new_int64(nBins));
	json_object_array_put_idx(jrange, 1, json_object_new_int64(min));
	json_object_array_put_idx(jrange, 2, json_object_new_int64(max));
}

void KsSession::getVisModel(size_t *nBins, uint64_t *min, uint64_t *max)
{
	json_object *jrange;
	json_object_object_get_ex(_jsession, "VisModel", &jrange);
	json_object *jn = json_object_array_get_idx(jrange, 0);
	json_object *jmin = json_object_array_get_idx(jrange, 1);
	json_object *jmax = json_object_array_get_idx(jrange, 2);

	*nBins = json_object_get_int64(jn);
	*min = json_object_get_int64(jmin);
	*max = json_object_get_int64(jmax);
}

void KsSession::setCpuPlots(const QVector<int> &cpus)
{
	json_object *jcpus;
	if (json_object_object_get_ex(_jsession, "CpuPlots", &jcpus)) {
		json_object_object_del(_jsession, "CpuPlots");
// 		size_t length = json_object_array_length(jcpus);
// 		if (length)
// 			json_object_array_del_idx(jcpus, 0, length - 1)
	}

	jcpus = json_object_new_array();
	for (int i = 0; i < cpus.count(); ++i) {
		json_object *jcpu = json_object_new_int(cpus[i]);
		json_object_array_put_idx(jcpus, i, jcpu);
	}

	json_object_object_add(_jsession, "CpuPlots", jcpus);
}

QVector<int> KsSession::getCpuPlots()
{
	QVector<int> cpus;
	json_object *jcpus;
	if (json_object_object_get_ex(_jsession, "CpuPlots", &jcpus)) {
		size_t length = json_object_array_length(jcpus);
		for (size_t i = 0; i < length; ++i) {
			int cpu = json_object_get_int(json_object_array_get_idx(jcpus,
										i));
			cpus.append(cpu);
		}
	}

	return cpus;
}

void KsSession::setTaskPlots(const QVector<int> &tasks)
{
	json_object *jtasks;
	if (json_object_object_get_ex(_jsession, "TaskPlots", &jtasks)) {
		json_object_object_del(_jsession, "TaskPlots");
// 		int length = json_object_array_length(jtasks);
// 		if (length)
// 			json_object_array_del_idx(jtasks, 0, length - 1)
	}

	jtasks = json_object_new_array();
	for (int i = 0; i < tasks.count(); ++i) {
		json_object *jtask = json_object_new_int(tasks[i]);
		json_object_array_put_idx(jtasks, i, jtask);
	}

	json_object_object_add(_jsession, "TaskPlots", jtasks);
}

QVector<int> KsSession::getTaskPlots()
{
	QVector<int> tasks;
	json_object *jtasks;
	if (json_object_object_get_ex(_jsession, "TaskPlots", &jtasks)) {
		size_t length = json_object_array_length(jtasks);
		for (size_t i = 0; i < length; ++i) {
			int pid = json_object_get_int(json_object_array_get_idx(jtasks,
										i));
			tasks.append(pid);
		}
	}

	return tasks;
}

void KsSession::setFilters(kshark_context *kshark_ctx)
{
	json_object *jfilters;
	if (json_object_object_get_ex(_jsession, "Filters", &jfilters))
		json_object_object_del(_jsession, "Filters");

	jfilters = kshark_all_filters_to_json(kshark_ctx);
	json_object_object_add(_jsession, "Filters", jfilters);
}

void KsSession::getFilters(kshark_context *kshark_ctx)
{
	json_object *jfilters;
	if (json_object_object_get_ex(_jsession, "Filters", &jfilters)) {
		/* Get the type of the document. */
		struct json_object *var;
		json_object_object_get_ex(jfilters, "type", &var);
		const char *type_var = json_object_get_string(var);
		if (strcmp(type_var, "kshark.filter.config") != 0) {
			QString message = "Failed to open Json file %s\n.";
			message += "The document has a wrong type.";
			QErrorMessage *em = new QErrorMessage();
			em->showMessage(message, "getFilters");
			qCritical() << message;
			return;
		}

		kshark_all_filters_from_json(kshark_ctx, jfilters);
	}
}

void KsSession::setMainWindowSize(int width, int height)
{
	json_object *jwindow;
	json_object_object_get_ex(_jsession, "MainWindow", &jwindow);
	json_object_array_put_idx(jwindow, 0, json_object_new_int(width));
	json_object_array_put_idx(jwindow, 1, json_object_new_int(height));
}

void KsSession::getMainWindowSize(int *width, int *height)
{
	json_object *jwindow;
	json_object_object_get_ex(_jsession, "MainWindow", &jwindow);
	json_object *jwidth = json_object_array_get_idx(jwindow, 0);
	json_object *jheight = json_object_array_get_idx(jwindow, 1);

	*width = json_object_get_int(jwidth);
	*height = json_object_get_int(jheight);
}

void KsSession::setSplitterSize(int graphSize, int viewSize)
{
	json_object *jsplitter;
	json_object_object_get_ex(_jsession, "Splitter", &jsplitter);
	json_object_array_put_idx(jsplitter, 0, json_object_new_int(graphSize));
	json_object_array_put_idx(jsplitter, 1, json_object_new_int(viewSize));
}

void KsSession::getSplitterSize(int *graphSize, int *viewSize)
{
	json_object *jsplitter;
	json_object_object_get_ex(_jsession, "Splitter", &jsplitter);
	json_object *jgraphsize = json_object_array_get_idx(jsplitter, 0);
	json_object *jviewsize = json_object_array_get_idx(jsplitter, 1);

	*graphSize = json_object_get_int(jgraphsize);
	*viewSize = json_object_get_int(jviewsize);
}

void KsSession::setDualMarker(KsDualMarkerSM *dm)
{
	json_object *jd_mark;
	json_object_object_get_ex(_jsession, "Markers", &jd_mark);

	auto save_mark = [&jd_mark] (KsGraphMark *m, const char *name)
	{
		json_object *jmark = json_object_new_object();

		if (m->isSet()) {
			json_object_object_add(jmark, "isSet",
					       json_object_new_boolean(true));

			json_object_object_add(jmark, "row",
					       json_object_new_int(m->row()));
		} else {
			json_object_object_add(jmark, "isSet",
					       json_object_new_boolean(false));
		}

		json_object_object_add(jd_mark, name, jmark);
	};

	save_mark(&dm->markerA(), "markA");
	save_mark(&dm->markerB(), "markB");

	if (dm->getState() == DualMarkerState::A)
		json_object_object_add(jd_mark, "Active", json_object_new_string("A"));
	else
		json_object_object_add(jd_mark, "Active", json_object_new_string("B"));
}

bool KsSession::getMarker(const char* name, size_t *pos)
{
	json_object *jd_mark, *jmark;
	json_object_object_get_ex(_jsession, "Markers", &jd_mark);

	if (json_object_object_get_ex(jd_mark, name, &jmark)) {
		json_object *jis_set;
		json_object_object_get_ex(jmark, "isSet", &jis_set);
		if (!json_object_get_boolean(jis_set))
			return false;

		json_object *jpos;
		json_object_object_get_ex(jmark, "row", &jpos);
		*pos = json_object_get_int64(jpos);
	}

	return true;
}

DualMarkerState KsSession::getMarkerState()
{
	json_object *jd_mark, *jstate;
	json_object_object_get_ex(_jsession, "Markers", &jd_mark);
	json_object_object_get_ex(jd_mark, "Active", &jstate);
	const char* state = json_object_get_string(jstate);

	if (strcmp(state, "A") == 0)
		return DualMarkerState::A;

	return DualMarkerState::B;
}

void KsSession::setPlugins(const QStringList &pluginList,
			   const QVector<bool> &registeredPlugins)
{
	json_object *jplugins, *jlist;
	json_object_object_get_ex(_jsession, "Plugins", &jplugins);
	jlist = json_object_new_array();

	size_t nPlugins = pluginList.length();
	for (size_t i = 0; i < nPlugins; ++i) {
		json_object *jpl = json_object_new_array();
		QByteArray array = pluginList[i].toLocal8Bit();
		char* buffer = array.data();
		json_object_array_put_idx(jpl, 0, json_object_new_string(buffer));
		bool active = registeredPlugins[i];
		json_object_array_put_idx(jpl, 1, json_object_new_boolean(active));
		json_object_array_put_idx(jlist, i, jpl);
	}

	json_object_object_add(jplugins, "List", jlist);
}

void KsSession::getPlugins(QStringList *pluginList,
			   QVector<bool> *registeredPlugins)
{
	json_object *jplugins;
	if (json_object_object_get_ex(_jsession, "Plugins", &jplugins)) {
		/* Get the type of the document. */
		struct json_object *var;
		json_object_object_get_ex(jplugins, "type", &var);
		const char *type_var = json_object_get_string(var);
		if (strcmp(type_var, "kshark.plugins.config") != 0) {
			QString message = "Failed to open Json file %s\n.";
			message += "The document has a wrong type.";
			QErrorMessage *em = new QErrorMessage();
			em->showMessage(message, "getFilters");
			qCritical() << message;
			return;
		}

		struct json_object *jlist;
		json_object_object_get_ex(jplugins, "List", &jlist);

		size_t length = json_object_array_length(jlist);
		for (size_t i = 0; i < length; ++i) {
			json_object *jpl = json_object_array_get_idx(jlist, i);

			(*pluginList)[i] =
				json_object_get_string(json_object_array_get_idx(jpl, 0));

			(*registeredPlugins)[i] =
				json_object_get_boolean(json_object_array_get_idx(jpl, 1));
		}
	}
}
