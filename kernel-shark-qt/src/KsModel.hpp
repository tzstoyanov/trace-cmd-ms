/*
 * Copyright (C) 2017 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License (not later!)
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not,  see <http://www.gnu.org/licenses>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#ifndef _KS_MODEL_H
#define _KS_MODEL_H

// C++11
#include <mutex>
#include <condition_variable>

// Qt
#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QColor>
#include <QProgressBar>
#include <QLabel>

// KernelShark
#include "libkshark.h"
#include "libkshark-model.h"

typedef bool (*condition_func)(QString, QString);

enum class DualMarkerState;

class KsDataStore;

class KsViewModel : public QAbstractTableModel
{
	//QList<pevent_record*>	_data;
	QList<kshark_entry*>	_data;
	QStringList		_header;

	static struct trace_seq	 _seq;
	struct pevent		*_pevt;

	int _markA, _markB;
	QColor _colorMarkA, _colorMarkB;

// 	std::mutex _mutex;

public:
	explicit KsViewModel(QObject *parent = nullptr);
	virtual ~KsViewModel();

	void setColors(const QColor &colA, const QColor &colB) {
		_colorMarkA = colA;
		_colorMarkB = colB;
	};
	int rowCount(const QModelIndex &) const override;
	int columnCount(const QModelIndex &) const override {return _header.count();}
	QVariant headerData(int section,
			    Qt::Orientation orientation,
			    int role) const override;
	QVariant data(const QModelIndex &index, int role) const override;

	//void fill(pevent *pevt, pevent_record **entries, size_t n);
	void fill(pevent *pevt, kshark_entry **entries, size_t n);
	void selectRow(DualMarkerState state, int row);
	void reset();
	void update(KsDataStore *data = nullptr);

	QStringList header() const {return _header;}
	QVariant getValue(const QModelIndex &index) const;
	QVariant getValue(int column, int row) const;

	size_t search(int		 column,
		      const QString	&searchText,
		      condition_func	 cond,
		      QList<size_t>	*matchList);

	enum {
		TRACE_VIEW_COL_INDEX,
		TRACE_VIEW_COL_CPU,
		TRACE_VIEW_COL_TS,
		TRACE_VIEW_COL_COMM,
		TRACE_VIEW_COL_PID,
		TRACE_VIEW_COL_LAT,
		TRACE_VIEW_COL_EVENT,
		TRACE_VIEW_COL_INFO,
		TRACE_VIEW_N_COLUMNS,
	};
};

class KsFilterProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT

	kshark_entry	**_data;
	KsViewModel	 *_source;

public:
	explicit KsFilterProxyModel(QObject *parent = nullptr);

	bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

	void fill(kshark_entry **rows);
	void update(KsDataStore *data);
	void setSource(KsViewModel *s);

	size_t search(int		column,
		      const QString	&searchText,
		      condition_func	cond,
		      QList<int>	*matchList,
		      QProgressBar	*pb = nullptr,
		      QLabel		*l = nullptr);


	QList<int> searchMap(int column,
			     const QString  &searchText,
			     condition_func  cond,
			     int first,
			     int last,
				 QProgressBar	*pb);

	void searchReduce(QList<int> &resultList, const QList<int> &mapList);

	void searchStop() {_searchStop = true;}
// 	void firstMatchReady(const QList<size_t> &matchList);

public:
	std::mutex _mutex;
	std::condition_variable _pbCond;
	int _searchProgress;
	bool _searchStop;
};

#define KS_DEFAULT_NBUNS	1024

class KsGraphModel : public QAbstractTableModel
{
	pevent		*_pevt;
	int 		 _cpus;
	kshark_trace_histo	_histo;

public:
	explicit KsGraphModel(QObject *parent = nullptr);
	KsGraphModel(int cpus, QObject *parent = nullptr);
	virtual ~KsGraphModel();

	int rowCount(const QModelIndex &) const override {return _histo.n_bins;}
	int columnCount(const QModelIndex &) const override {return _cpus;}
	QVariant data(const QModelIndex &index, int role) const override;

	kshark_trace_histo *histo() {return &_histo;}
	void setNCpus(int n) {_cpus = n;}

	void fill(pevent *pevt, kshark_entry **entries, size_t n);

	void shiftForward(size_t n);
	void shiftBackward(size_t n);
	void jumpTo(size_t ts);
	void zoomOut(double r, int mark = -1);
	void zoomIn(double r, int mark = -1);

	void reset();
	void update(KsDataStore *data = nullptr);

	QVariant getValue(const QModelIndex &index) const;
};

#endif // _KS_MODEL_H
