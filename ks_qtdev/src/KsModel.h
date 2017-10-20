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
	size_t 			 _dataSize;
	
	std::vector<int64_t> _map;

	bool endOfDataDo(size_t row, size_t lastBin);
	void binUpdate(size_t row, size_t thisBin, size_t *lastBin);
	void resetBins(size_t first, size_t last);
	size_t setLowEdge();
	void setNextBinEdge(size_t prevBin);
	size_t setHighEdge();
	void setBinCounts();

public:
	std::vector<size_t> _binCount;
	KsTimeMap();
	KsTimeMap(size_t n, uint64_t min, uint64_t max);
	virtual ~KsTimeMap();

	void setBining(size_t n, uint64_t min, uint64_t max);
	void fill(struct pevent_record **data, size_t n);
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

	void dump();

	uint64_t _min, _max, _binSize;
	size_t _nBins;

	enum  {
		LowerOverflowBin = -2,
		UpperOverflowBin = -1
	};
};

class KsGraphModel : public QAbstractTableModel
{
	struct pevent	*_pevt;
	int 			_cpus;

public:

	KsTimeMap	_visMap;

	KsGraphModel(QObject *parent = nullptr);
	KsGraphModel(int cpus, QObject *parent = nullptr);
	virtual ~KsGraphModel();

	void setNCpus(int n) {_cpus = n;}
	int rowCount(const QModelIndex &) const override {return _visMap.size();}
	int columnCount(const QModelIndex &) const override {return _cpus+2;}
	QVariant data(const QModelIndex &index, int role) const override;

	void fill(pevent *pevt, pevent_record **entries, size_t n, bool defaultMap = true);
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

#endif
