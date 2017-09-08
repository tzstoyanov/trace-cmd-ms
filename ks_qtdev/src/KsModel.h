/*
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

#ifndef KS_MODEL_H
#define KS_MODEL_H 1

#include <vector>

// Qt
#include <QAbstractTableModel>

#include "trace-cmd.h"

typedef bool (*condition_func)(QString, QString);


#define KS_GRAPH_N_BINS  2048

class KsViewModel : public QAbstractTableModel
{
	QList<pevent_record*>	_data;
// 	QVector<pevent_record*>	_data;
// 	struct pevent_record **_data;
	QStringList 		_header;
// 	size_t				_page;

	static struct trace_seq	 _seq;
	struct pevent		*_pevt;

public:
	KsViewModel(QObject *parent = nullptr);
	virtual ~KsViewModel();

	int rowCount(const QModelIndex &) const override;
	int columnCount(const QModelIndex &) const override { return _header.count(); }

	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

	QVariant data(const QModelIndex &index, int role) const override;

	void fill(pevent *pevt, pevent_record **entries, size_t n);
	void reset();

	QStringList header() const {return _header;}
	QVariant getValue(const QModelIndex &index) const;
	QVariant getValue(int column, int row) const;

	size_t search(	int				 column,
					const QString	&searchText,
					condition_func	 cond,
					QList<size_t>	*matchList);
};

class KsTimeMap {

	pevent_record  **_data;
	size_t 			_dataSize;
	
	std::vector<int64_t> _map;
	//int64_t 		*_map;

public:
	KsTimeMap();
	KsTimeMap(size_t n, uint64_t min, uint64_t max);
	virtual ~KsTimeMap();

	void setBining(size_t n, uint64_t min, uint64_t max);
	void fill(struct pevent_record **data, size_t n);
	size_t size() const {return _nBins;}
	void clear() {_map.clear();}

	size_t binCount(size_t bin) const;
	size_t binCount(size_t bin, int cpu) const;
	double binTime(size_t bin) const {return (_min + bin*_binSize)*1e-9;}
	uint64_t ts(size_t bin) const {return (_min + bin*_binSize);}

	int64_t& operator[](size_t i) {return _map[i];}

	uint64_t _min, _max, _binSize;
	size_t _nBins;
};

class KsGraphModel : public QAbstractTableModel
{
	QList<pevent_record*>	_data;
	struct pevent		*_pevt;
	int _cpus;

public:

	KsTimeMap	*_map;

	KsGraphModel(QObject *parent = nullptr);
	KsGraphModel(int cpus, QObject *parent = nullptr);
	virtual ~KsGraphModel();

	void setNCpus(int n) {_cpus = n;}
	int rowCount(const QModelIndex &) const override {return _map->size();}
	int columnCount(const QModelIndex &) const override {return _cpus+2;}
	QVariant data(const QModelIndex &index, int role) const override;

	void fill(pevent *pevt, pevent_record **entries, size_t n, bool defaultMap = true);
	void reset();

	QVariant getValue(const QModelIndex &index) const;
	QVariant getValue(int column, int row) const;
};

#endif
