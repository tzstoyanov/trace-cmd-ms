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

//Qt
#include <QFrame>

// Kernel Shark 2
#include "KsDeff.h"
#include "KsUtils.hpp"
#include "KsPlotTools.hpp"
#include "KsTraceGraph.hpp"

KsDataStore::KsDataStore(QWidget *parent)
: QObject(parent),
  _handle(nullptr),
  _dataSize(0),
  _rows(nullptr),
  _pevt(nullptr)
{}

KsDataStore::~KsDataStore()
{
	clear();
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

	if (kshark_ctx->event_handlers == NULL)
		kshark_handle_plugins(kshark_ctx, KSHARK_PLUGIN_LOAD);
	else
		kshark_handle_plugins(kshark_ctx, KSHARK_PLUGIN_RELOAD);

	_dataSize = kshark_load_data_entries(kshark_ctx, &_rows);

	double time = GET_DURATION(t0);
	qInfo() <<"Data loading time: " << 1e3*time << " ms.   entries: " << _dataSize;
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
	qInfo() <<"Data reloading time: " << 1e3*time << " ms.   entries: " << _dataSize;
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

void KsDataStore::applyPosTaskFilter(QVector<int> vec)
{
	hd_time t0 = GET_TIME;

	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	kshark_filter_clear(kshark_ctx, SHOW_TASK_FILTER);
	kshark_filter_clear(kshark_ctx, HIDE_TASK_FILTER);
	for (auto const &pid: vec)
		kshark_filter_add_id(kshark_ctx, SHOW_TASK_FILTER, pid);

	kshark_filter_entries(kshark_ctx, _rows, _dataSize);

	emit updateView(this);
	emit updateGraph(this);

	double time = GET_DURATION(t0);
	qInfo() <<"Filtering time (PTF): " << 1e3*time << " ms.";
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

	kshark_filter_entries(kshark_ctx, _rows, _dataSize);

	emit updateView(this);
	emit updateGraph(this);

	double time = GET_DURATION(t0);
	qInfo() <<"Filtering time (NTF): " << 1e3*time << " ms.";
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

	kshark_filter_entries(kshark_ctx, _rows, _dataSize);

	emit updateView(this);
	emit updateGraph(this);

	double time = GET_DURATION(t0);
	qInfo() <<"Filtering time (PEF): " << 1e3*time << " ms.";
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

	kshark_filter_entries(kshark_ctx, _rows, _dataSize);

	emit updateView(this);
	emit updateGraph(this);

	double time = GET_DURATION(t0);
	qInfo() <<"Filtering time (NEF): " << 1e3*time << " ms.";
}

KsPluginManager::KsPluginManager(QWidget *parent)
: QObject(parent)
{
	int nPlugins = getPluginList(&_pluginList);
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

	/** No plugin with this name in the list.
	 * Try to add it anyway.  */
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
// 	_buttonA.setShortcut(QKeySequence(Qt::CTRL + Qt::Key_A));
// 	_buttonB.setShortcut(QKeySequence(Qt::CTRL + Qt::Key_B));

	for (auto const &l: {&_labelMA, &_labelMB, &_labelDelta}) {
		l->setFrameStyle(QFrame::Panel | QFrame::Sunken);
		l->setStyleSheet("QLabel {background-color : white;}");
		l->setTextInteractionFlags(Qt::TextSelectableByMouse);
// 		l->setFixedHeight(FONT_HEIGHT*1.5);
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

	_scCtrlA.setKey(Qt::CTRL + Qt::Key_A);
	_scCtrlB.setKey(Qt::CTRL + Qt::Key_B);
	_stateA->addTransition(&_scCtrlB, SIGNAL(activated()),_stateB);
	_stateB->addTransition(&_scCtrlA, SIGNAL(activated()),_stateA);

	_stateB->addTransition(&_buttonA, SIGNAL(clicked()), _stateA);
	connect(&_buttonA, SIGNAL(clicked()), this, SLOT(setStateA()));

	_stateA->addTransition(&_buttonB, SIGNAL(clicked()), _stateB);
	connect(&_buttonB, SIGNAL(clicked()), this, SLOT(setStateB()));

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

void KsDualMarkerSM::setStateA()
{
	if (_markState !=  DualMarkerState::A) {
		_markState = DualMarkerState::A;
		emit markSwitch();
	} else if (activeMarker().isSet()) {
		emit updateView(activeMarker().row(), true);
		emit showInGraph(activeMarker().row());
	}
}

void KsDualMarkerSM::setStateB()
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
