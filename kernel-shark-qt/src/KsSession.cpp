// SPDX-License-Identifier: LGPL-2.1

/*
 * Copyright (C) 2017 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
 */

/**
 *  @file    KsSession.cpp
 *  @brief   KernelShark Session.
 */

// KernelShark
#include "libkshark.h"
#include "KsSession.hpp"

KsSession::KsSession()
{
	_config = kshark_config_new("kshark.config.session",
				      KS_CONFIG_JSON);

	json_object *jcpuplots = json_object_new_array();
	json_object_object_add(json(), "CpuPlots", jcpuplots);

	json_object *jtaskplots = json_object_new_array();
	json_object_object_add(json(), "TaskPlots", jtaskplots);
}

KsSession::~KsSession()
{
	kshark_free_config_doc(_config);
}

void KsSession::importFromFile(QString jfileName)
{
	if (_config)
		kshark_free_config_doc(_config);

	_config = kshark_open_config_file(jfileName.toStdString().c_str(),
					  "kshark.config.session");
}

void KsSession::exportToFile(QString jfileName)
{
	kshark_save_config_file(jfileName.toStdString().c_str(), _config);
}

void KsSession::setVisModel(kshark_trace_histo *histo)
{
	kshark_config_doc *model =
		kshark_export_model(histo, KS_CONFIG_JSON);

	kshark_config_doc_add(_config, "Model", model);
}

void KsSession::getVisModel(kshark_trace_histo *histo)
{
	kshark_config_doc *model = kshark_config_alloc(KS_CONFIG_JSON);

	if (!kshark_config_doc_get(_config, "Model", model))
		return;

	kshark_import_model(histo, model);
}

void KsSession::setDataFile(QString fileName)
{
	kshark_config_doc *file =
		kshark_export_trace_file(fileName.toStdString().c_str(),
					 KS_CONFIG_JSON);

	kshark_config_doc_add(_config, "Data", file);
}

QString KsSession::getDataFile(kshark_context *kshark_ctx)
{
	kshark_config_doc *file = kshark_config_alloc(KS_CONFIG_JSON);
	const char *file_str;

	if (!kshark_config_doc_get(_config, "Data", file))
		return QString();

	file_str = kshark_import_trace_file(kshark_ctx, file);
	if (file_str)
		return QString(file_str);

	return QString();
}

void KsSession::setFilters(kshark_context *kshark_ctx)
{
	kshark_config_doc *filters =
		kshark_export_all_filters(kshark_ctx, KS_CONFIG_JSON);

	kshark_config_doc_add(_config, "Filters", filters);
}

void KsSession::getFilters(kshark_context *kshark_ctx)
{
	kshark_config_doc *filters = kshark_config_alloc(KS_CONFIG_JSON);

	if (!kshark_config_doc_get(_config, "Filters", filters))
		return;

	kshark_import_all_filters(kshark_ctx, filters);
}

void KsSession::setViewTop(size_t r) {
	kshark_config_doc *topRow = kshark_config_alloc(KS_CONFIG_JSON);

	topRow->conf_doc = json_object_new_int64(r);
	kshark_config_doc_add(_config, "ViewTop",topRow);
}

size_t KsSession::getViewTop() {
	kshark_config_doc *topRow = kshark_config_alloc(KS_CONFIG_JSON);
	size_t r = 0;

	if (!kshark_config_doc_get(_config, "ViewTop", topRow))
		return r;

	if (_config->format == KS_CONFIG_JSON)
		r = json_object_get_int64(KS_JSON_CAST(topRow->conf_doc));

	return r;
}

void KsSession::setMainWindowSize(int width, int height)
{
	kshark_config_doc *window = kshark_config_alloc(KS_CONFIG_JSON);
	json_object *jwindow = json_object_new_array();

	json_object_array_put_idx(jwindow, 0, json_object_new_int(width));
	json_object_array_put_idx(jwindow, 1, json_object_new_int(height));

	window->conf_doc = jwindow;
	kshark_config_doc_add(_config, "MainWindow", window);
}

void KsSession::getMainWindowSize(int *width, int *height)
{
	kshark_config_doc *window = kshark_config_alloc(KS_CONFIG_JSON);
	json_object *jwindow, *jwidth, *jheight;

	*width = 800;
	*height = 600;

	if (!kshark_config_doc_get(_config, "MainWindow", window))
		return;

	if (_config->format == KS_CONFIG_JSON) {
		jwindow = KS_JSON_CAST(window->conf_doc);
		jwidth = json_object_array_get_idx(jwindow, 0);
		jheight = json_object_array_get_idx(jwindow, 1);

		*width = json_object_get_int(jwidth);
		*height = json_object_get_int(jheight);
	}
}

void KsSession::setSplitterSize(int graphSize, int viewSize)
{
	kshark_config_doc *spl = kshark_config_alloc(KS_CONFIG_JSON);
	json_object *jspl = json_object_new_array();

	json_object_array_put_idx(jspl, 0, json_object_new_int(graphSize));
	json_object_array_put_idx(jspl, 1, json_object_new_int(viewSize));

	spl->conf_doc = jspl;
	kshark_config_doc_add(_config, "Splitter", spl);
}

void KsSession::getSplitterSize(int *graphSize, int *viewSize)
{
	kshark_config_doc *spl = kshark_config_alloc(KS_CONFIG_JSON);
	json_object *jspl, *jgraphsize, *jviewsize;

	*graphSize = *viewSize = 1;

	if (!kshark_config_doc_get(_config, "Splitter", spl))
		return;

	if (_config->format == KS_CONFIG_JSON) {
		jspl = KS_JSON_CAST(spl->conf_doc);
		jgraphsize = json_object_array_get_idx(jspl, 0);
		jviewsize = json_object_array_get_idx(jspl, 1);

		*graphSize = json_object_get_int(jgraphsize);
		*viewSize = json_object_get_int(jviewsize);
	}
}

void KsSession::setColorScheme() {
	kshark_config_doc *colSch = kshark_config_alloc(KS_CONFIG_JSON);
	double s = KsPlot::Color::getRainbowFrequency();

	colSch->conf_doc = json_object_new_double(s);
	kshark_config_doc_add(_config, "ColorScheme", colSch);
}

float KsSession::getColorScheme() {
	kshark_config_doc *colSch = kshark_config_alloc(KS_CONFIG_JSON);
	float s = 0.75;

	if (!kshark_config_doc_get(_config, "ColorScheme", colSch))
		return s;

	if (_config->format == KS_CONFIG_JSON)
		s = json_object_get_double(KS_JSON_CAST(colSch->conf_doc));

	return s;
}

void KsSession::setCpuPlots(const QVector<int> &cpus)
{
	json_object *jcpus;
	if (json_object_object_get_ex(json(), "CpuPlots", &jcpus)) {
		json_object_object_del(json(), "CpuPlots");
	}

	jcpus = json_object_new_array();
	for (int i = 0; i < cpus.count(); ++i) {
		json_object *jcpu = json_object_new_int(cpus[i]);
		json_object_array_put_idx(jcpus, i, jcpu);
	}

	json_object_object_add(json(), "CpuPlots", jcpus);
}

QVector<int> KsSession::getCpuPlots()
{
	QVector<int> cpus;
	json_object *jcpus;
	if (json_object_object_get_ex(json(), "CpuPlots", &jcpus)) {
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
	if (json_object_object_get_ex(json(), "TaskPlots", &jtasks)) {
		json_object_object_del(json(), "TaskPlots");
	}

	jtasks = json_object_new_array();
	for (int i = 0; i < tasks.count(); ++i) {
		json_object *jtask = json_object_new_int(tasks[i]);
		json_object_array_put_idx(jtasks, i, jtask);
	}

	json_object_object_add(json(), "TaskPlots", jtasks);
}

QVector<int> KsSession::getTaskPlots()
{
	QVector<int> tasks;
	json_object *jtasks;
	if (json_object_object_get_ex(json(), "TaskPlots", &jtasks)) {
		size_t length = json_object_array_length(jtasks);
		for (size_t i = 0; i < length; ++i) {
			int pid = json_object_get_int(json_object_array_get_idx(jtasks,
										i));
			tasks.append(pid);
		}
	}

	return tasks;
}

void KsSession::setDualMarker(KsDualMarkerSM *dm)
{
	struct kshark_config_doc *markers =
		kshark_config_new("kshark.config.markers", KS_CONFIG_JSON);
	json_object *jd_mark = KS_JSON_CAST(markers->conf_doc);

	auto save_mark = [&jd_mark] (KsGraphMark *m, const char *name)
	{
		json_object *jmark = json_object_new_object();

		if (m->_isSet) {
			json_object_object_add(jmark, "isSet",
					       json_object_new_boolean(true));

			json_object_object_add(jmark, "row",
					       json_object_new_int(m->_pos));
		} else {
			json_object_object_add(jmark, "isSet",
					       json_object_new_boolean(false));
		}

		json_object_object_add(jd_mark, name, jmark);
	};

	save_mark(&dm->markerA(), "markA");
	save_mark(&dm->markerB(), "markB");

	if (dm->getState() == DualMarkerState::A)
		json_object_object_add(jd_mark, "Active",
				       json_object_new_string("A"));
	else
		json_object_object_add(jd_mark, "Active",
				       json_object_new_string("B"));

	kshark_config_doc_add(_config, "Markers", markers);
}

json_object *KsSession::getMarkerJson()
{
	struct kshark_config_doc *markers =
		kshark_config_alloc(KS_CONFIG_JSON);

	if (!kshark_config_doc_get(_config, "Markers", markers) ||
	    !kshark_type_check(markers, "kshark.config.markers"))
		return nullptr;

	return KS_JSON_CAST(markers->conf_doc);
}

bool KsSession::getMarker(const char* name, size_t *pos)
{
	json_object *jd_mark, *jmark;

	*pos = 0;
	jd_mark = getMarkerJson();

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
	const char* state;

	jd_mark = getMarkerJson();
	json_object_object_get_ex(jd_mark, "Active", &jstate);
	state = json_object_get_string(jstate);

	if (strcmp(state, "A") == 0)
		return DualMarkerState::A;

	return DualMarkerState::B;
}

void KsSession::setPlugins(const QStringList &pluginList,
			   const QVector<bool> &registeredPlugins)
{
// 	json_object *jplugins, *jlist;
// 	json_object_object_get_ex(json(), "Plugins", &jplugins);
// 	jlist = json_object_new_array();
// 
// 	size_t nPlugins = pluginList.length();
// 	for (size_t i = 0; i < nPlugins; ++i) {
// 		json_object *jpl = json_object_new_array();
// 		QByteArray array = pluginList[i].toLocal8Bit();
// 		char* buffer = array.data();
// 		json_object_array_put_idx(jpl, 0, json_object_new_string(buffer));
// 		bool active = registeredPlugins[i];
// 		json_object_array_put_idx(jpl, 1, json_object_new_boolean(active));
// 		json_object_array_put_idx(jlist, i, jpl);
// 	}
// 
// 	json_object_object_add(jplugins, "List", jlist);
}

void KsSession::getPlugins(QStringList *pluginList,
			   QVector<bool> *registeredPlugins)
{
// 	json_object *jplugins;
// 	if (json_object_object_get_ex(json(), "Plugins", &jplugins)) {
// 		/* Get the type of the document. */
// 		struct json_object *var;
// 		json_object_object_get_ex(jplugins, "type", &var);
// 		const char *type_var = json_object_get_string(var);
// 		if (strcmp(type_var, "kshark.config.plugins") != 0) {
// 			QString message = "Failed to open Json file %s\n.";
// 			message += "The document has a wrong type.";
// 			QErrorMessage *em = new QErrorMessage();
// 			em->showMessage(message, "getFilters");
// 			qCritical() << message;
// 			return;
// 		}
// 
// 		struct json_object *jlist;
// 		json_object_object_get_ex(jplugins, "List", &jlist);
// 
// 		size_t length = json_object_array_length(jlist);
// 		for (size_t i = 0; i < length; ++i) {
// 			json_object *jpl = json_object_array_get_idx(jlist, i);
// 
// 			(*pluginList)[i] =
// 				json_object_get_string(json_object_array_get_idx(jpl, 0));
// 
// 			(*registeredPlugins)[i] =
// 				json_object_get_boolean(json_object_array_get_idx(jpl, 1));
// 		}
// 	}
}
