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

// C++
#include <vector>

// Qt
#include <QAbstractTableModel>

// trace-cmd
#include "trace-cmd.h"

// kernel shark 2
#include "libkshark.h"


typedef bool (*condition_func)(QString, QString);

#define KS_GRAPH_N_BINS  2048

enum class DualMarkerState;

class KsViewModel : public QAbstractTableModel
{
	//QList<pevent_record*>	_data;
	QList<kshark_entry*>	_data;
	QStringList 		_header;
// 	size_t			_page;

	static struct trace_seq	 _seq;
	struct pevent		*_pevt;

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

public:
	int _markA, _markB;

	KsViewModel(QObject *parent = nullptr);
	virtual ~KsViewModel();

	int rowCount(const QModelIndex &) const override;
	int columnCount(const QModelIndex &) const override { return _header.count(); }
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	QVariant data(const QModelIndex &index, int role) const override;

	//void fill(pevent *pevt, pevent_record **entries, size_t n);
	void fill(pevent *pevt, kshark_entry **entries, size_t n);
	void selectRow(DualMarkerState state, int row);
	void reset();

	QStringList header() const {return _header;}
	QVariant getValue(const QModelIndex &index) const;
	QVariant getValue(int column, int row) const;

	size_t search(int 		 column,
		      const QString	&searchText,
		      condition_func	 cond,
		      QList<size_t>	*matchList);
};

class KsTimeMap {
	/* All functionalities of the histogram class have to be rewritten in C.
	 * The histogram code will be part of the C library (libkshark).
	*/
	//pevent_record  		**_data;
	kshark_entry  		**_data;
	size_t 		 	  _dataSize;
	std::vector<int64_t> 	  _map;
	std::vector<size_t> 	  _binCount;

	void resetBins(size_t first, size_t last);
	size_t setLowerEdge();
	void setNextBinEdge(size_t prevBin);
	size_t setUpperhEdge();
	void setBinCounts();

public:
	KsTimeMap();
	KsTimeMap(size_t n, uint64_t min, uint64_t max);
	virtual ~KsTimeMap();

	void setBining(size_t n, uint64_t min, uint64_t max);
	//void fill(struct pevent_record **data, size_t n);
	void fill(struct kshark_entry **data, size_t n);

	void shiftForward(size_t n);
	void shiftBackward(size_t n);
	void shiftTo(size_t ts);
	void zoomOut(double r);
	void zoomIn(double r, size_t mark);

	size_t size() const {return _nBins;}
	void clear();

	size_t binCount(int bin) const;
	size_t binCount(int bin, int cpu) const;
	bool isEmpty(int bin) const;
	bool isEmpty(int bin, int cpu) const;

	double binTime(size_t bin)  const {return (_min + bin*_binSize)*1e-9;}
	uint64_t ts(size_t bin)     const {return (_min + bin*_binSize);}

	int64_t operator[](int i) const;
	int64_t at(int i) const;
	int64_t at(int i, int cpu) const;
	kshark_entry *dataAt(int i) const {return _data[i];}

	void dump();

	uint64_t _min, _max, _binSize;
	size_t _nBins;

	enum OverflowBin {
		Upper = -1,
		Lower = -2
	};
};

class KsGraphModel : public QAbstractTableModel
{
	struct pevent	*_pevt;
	int 		 _cpus;
	KsTimeMap	 _histo;

public:
	KsGraphModel(QObject *parent = nullptr);
	KsGraphModel(int cpus, QObject *parent = nullptr);
	virtual ~KsGraphModel();
	
	int rowCount(const QModelIndex &) const override {return _histo.size();}
	int columnCount(const QModelIndex &) const override {return _cpus+2;}
	QVariant data(const QModelIndex &index, int role) const override;

	const KsTimeMap &histo() {return _histo;}
	KsTimeMap *histoPtr()    {return &_histo;}

	void setNCpus(int n) {_cpus = n;}

	//void fill(pevent *pevt, pevent_record **entries, size_t n, bool defaultMap = true);
	void fill(pevent *pevt, kshark_entry **entries, size_t n, bool defaultMap = true);

	void shiftForward(size_t n);
	void shiftBackward(size_t n);
	void shiftTo(size_t ts);
	void zoomOut(double r);
	void zoomIn(double r, size_t mark = KS_GRAPH_N_BINS/2);

	void reset();
	void update();

	QVariant getValue(const QModelIndex &index) const;
	QVariant getValue(int column, int row) const;
};

#endif // _KS_MODEL_H
