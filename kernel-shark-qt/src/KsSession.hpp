/* SPDX-License-Identifier: LGPL-2.1 */

/*
 * Copyright (C) 2017 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
 */

/**
 *  @file    KsSession.hpp
 *  @brief   KernelShark Session.
 */

#ifndef _KS_SESSION_H
#define _KS_SESSION_H

// Qt
#include <QtWidgets>

// KernelShark
#include "KsDualMarker.hpp"

class KsSession
{
	kshark_config_doc *_config;

	json_object *getMarkerJson();

public:
	KsSession();
	~KsSession();

	kshark_config_doc *getConfDocPtr() const {return _config;}

	json_object *json() {
		if (_config->format != KS_CONFIG_JSON)
			return nullptr;

		return KS_JSON_CAST(_config->conf_doc);
	}

	void importFromFile(QString jfileName);
	void exportToFile(QString jfileName);

	void setDataFile(QString fileName);
	QString getDataFile(kshark_context *kshark_ctx);

	void setVisModel(kshark_trace_histo *histo);
	void getVisModel(kshark_trace_histo *histo);

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

	void setViewTop(size_t r);
	size_t getViewTop();

	void setColorScheme();
	float getColorScheme();
};

#endif
