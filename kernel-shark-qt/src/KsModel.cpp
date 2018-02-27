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

// Kernel Shark 2
#include "KsModel.hpp"
#include "KsUtils.hpp"
#include "libkshark.h"

struct trace_seq KsViewModel::_seq;

KsFilterProxyModel::KsFilterProxyModel(QObject *parent)
: QSortFilterProxyModel(parent), _source(nullptr)
{}

bool KsFilterProxyModel::filterAcceptsRow(int sourceRow,
					  const QModelIndex &sourceParent) const
{
	if (_data[sourceRow]->visible & KS_VIEW_FILTER_MASK)
		return true;

	return false;
}

void KsFilterProxyModel::fill(kshark_entry **rows)
{
	_data = rows;
}

void KsFilterProxyModel::update(KsDataStore *data)
{
	fill(data->_rows);
}

void KsFilterProxyModel::setSource(KsViewModel *s)
{
	QSortFilterProxyModel::setSourceModel(s);
	_source = s;
}

size_t KsFilterProxyModel::search(int column,
				 const QString  &searchText,
				 condition_func  cond,
				 QList<size_t>  *matchList)
{
	int nRows = rowCount({});
	for (int r = 0; r < nRows; ++r) {
		/* Use the index of the proxy model to retrieve the value
		 * of the row number in the base model. This works because
		 *  the source row number is shown in column "0". */
		size_t row = data(index(r, 0)).toInt();
		QVariant item = _source->getValue(column, row);
		if (cond(searchText, item.toString())) {
			matchList->append(row);
		}
	}

	return matchList->count();
}

KsViewModel::KsViewModel(QObject *parent)
: QAbstractTableModel(parent),
//: QStandardItemModel(parent),
  _header({"#", "CPU", "Time Stamp", "Task", "PID", "Latency", "Event", "Info"}),
  _pevt(nullptr),
  _markA(-1),
  _markB(-1)
{}

KsViewModel::~KsViewModel()
{
	reset();
}

QVariant KsViewModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::ForegroundRole) {
		if (index.row() == _markA)
			return QVariant::fromValue(QColor(Qt::white));

		if (index.row() == _markB)
			return QVariant::fromValue(QColor(Qt::white));
	}

	if (role == Qt::BackgroundRole) {
		if (index.row() == _markA)
			return QVariant::fromValue(QColor(_colorMarkA));

		if (index.row() == _markB)
			return QVariant::fromValue(QColor(_colorMarkB));
	}
		
	if (role == Qt::DisplayRole)
		return this->getValue(index.column(), index.row());

	return {};
}

int KsViewModel::rowCount(const QModelIndex &) const
{
	return _data.count();
}

QVariant KsViewModel::getValue(const QModelIndex &index) const
{
	size_t row = index.row();
	size_t column = index.column();
	return getValue(column, row);
}

QVariant KsViewModel::getValue(int column, int row) const
{
	switch (column) {
		case TRACE_VIEW_COL_INDEX :
			return row;

		case TRACE_VIEW_COL_CPU:
			return _data[row]->cpu;

		case TRACE_VIEW_COL_TS:
			char time[32];
			uint64_t secs, usecs;
			usecs = _data[row]->ts;
			kshark_convert_nano(_data[row]->ts, &secs, &usecs);
			sprintf(time, "%llu.%06llu", (unsigned long long)secs, (unsigned long long)usecs);
			return time;

		case TRACE_VIEW_COL_COMM:
			return kshark_get_task_lazy(_data[row]);
// 			return pevent_data_comm_from_pid(_pevt, _data[row]->pid);

		case TRACE_VIEW_COL_PID:
			return kshark_get_pid_lazy(_data[row]);
// 			return _data[row]->pid;

		case TRACE_VIEW_COL_LAT: 
			return kshark_get_latency_lazy(_data[row]);

		case TRACE_VIEW_COL_EVENT:
			return kshark_get_event_name_lazy(_data[row]);

		case TRACE_VIEW_COL_INFO :
			return kshark_get_info_lazy(_data[row]);

		default:
			return {};
	}
}


//QVariant KsViewModel::getValue(int column, int row) const
//{
	//if (row >= _data.count())
		//return {};

	//switch (column) {
		//case TRACE_VIEW_COL_INDEX :
			//return row;

		//case TRACE_VIEW_COL_CPU :
			//return _data[row]->cpu;

		//case TRACE_VIEW_COL_TS :
			//char time[32];
			//uint64_t secs, usecs;
			//usecs = _data[row]->ts;
			//usecs /= 1000;
			//secs = usecs / 1000000ULL;
			//usecs -= secs * 1000000ULL;
			//sprintf(time, "%llu.%06llu", (long long)secs, (long long)usecs);
			//return time;

		//case TRACE_VIEW_COL_COMM :
		//{
			//int pid = pevent_data_pid(_pevt, _data[row]);
			//return pevent_data_comm_from_pid(_pevt, pid);
		//}

		//case TRACE_VIEW_COL_PID :
			//return pevent_data_pid(_pevt, _data[row]);

		//case TRACE_VIEW_COL_LAT :
			//trace_seq_reset(&_seq);
			//pevent_data_lat_fmt(_pevt, &_seq, _data[row]);
			//return _seq.buffer;

		//case TRACE_VIEW_COL_EVENT :
		//{
			//int etype = pevent_data_type(_pevt, _data[row]);
			//struct event_format *event = pevent_data_event_from_type(_pevt, etype);
			//if (!event) {
				//return "[UNKNOWN EVENT]";
			//} else {
				//return event->name;
			//}
		//}

		//case TRACE_VIEW_COL_INFO :
		//{
			//int etype = pevent_data_type(_pevt, _data[row]);
			//struct event_format *event = pevent_data_event_from_type(_pevt, etype);
			//trace_seq_reset(&_seq);
			//pevent_event_info(&_seq, event,_data[row] );
			//return _seq.buffer;
		//}

		//default: return {};
	//}
//}

QVariant KsViewModel::headerData(int section,
				 Qt::Orientation orientation,
				 int role) const
{
	if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
		return {};

	if (section < _header.count())
		return _header.at(section);
	
	return {};
}

//void KsViewModel::fill(pevent *pevt, pevent_record **entries, size_t n)
void KsViewModel::fill(pevent *pevt, kshark_entry **entries, size_t n)
{
	_pevt = pevt;
	beginInsertRows(QModelIndex(), _data.count(), _data.count() + n - 1);
	for(size_t r = 0; r < n; ++r)
		_data.append(entries[r]);
	endInsertRows();
}

void KsViewModel::selectRow(DualMarkerState state, int row)
{
	beginResetModel();
	if (state == DualMarkerState::A) {
		_markA = row;
		_markB = -1;
	} else {
		_markB = row;
		_markA = -1;
	}
	endResetModel();
}

void KsViewModel::reset()
{
	beginResetModel();
	_data.clear();
	endResetModel();
}

void KsViewModel::update(KsDataStore *data)
{
	reset();

	beginResetModel();
	if (data) {
		_data.clear();
		fill(data->_pevt, data->_rows, data->size());
	}
	endResetModel();
}

size_t KsViewModel::search(int column,
			   const QString  &searchText,
			   condition_func  cond,
			   QList<size_t>  *matchList)
{
	int nRows = rowCount({});
	for (int r = 0; r < nRows; ++r) {
		QModelIndex idx = index(r, column);
		QVariant item = getValue(idx);
		if (cond(searchText, item.toString())) {
			matchList->append(r);
		}
	}

	return matchList->count();
}

//////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////

KsGraphModel::KsGraphModel(QObject *parent)
: QAbstractTableModel(parent), _pevt(nullptr), _cpus(1)
{}

KsGraphModel::KsGraphModel(int cpus, QObject *parent)
: QAbstractTableModel(parent), _pevt(nullptr), _cpus(cpus)
{}

KsGraphModel::~KsGraphModel()
{}

QVariant KsGraphModel::getValue(const QModelIndex &index) const
{
	int bin = index.row();
	int cpu = index.column();
	int val = 0;

	if (_histo.notEmpty(bin, cpu))
		val = 1;

	return val;
}

QVariant KsGraphModel::data(const QModelIndex &index, int role) const
{
	if (role != Qt::DisplayRole)
		return {};

	QVariant val = this->getValue(index);
	return val;
}

//void KsGraphModel::fill(pevent *pevt, pevent_record **entries, size_t n, bool defaultMap)
void KsGraphModel::fill(pevent *pevt, kshark_entry **entries, size_t n)
{
	_pevt = pevt;

	if (n == 0)
		return;

	beginResetModel();
	
	if (_histo.size() == 0)
		_histo.setBining(KS_DEFAULT_NBUNS,
				  entries[0]->ts,
				  entries[n-1]->ts);

	_histo.fill(entries, n);

	endResetModel();
}

void KsGraphModel::shiftForward(size_t n)
{
	beginResetModel();
	_histo.shiftForward(n);
	endResetModel();
}

void KsGraphModel::shiftBackward(size_t n)
{
	beginResetModel();
	_histo.shiftBackward(n);
	endResetModel();
}

void KsGraphModel::shiftTo(size_t ts)
{
	beginResetModel();
	_histo.shiftTo(ts);
	endResetModel();
}

void KsGraphModel::zoomOut(double r, int mark)
{
	beginResetModel();
	_histo.zoomOut(r, mark);
	endResetModel();
}

void KsGraphModel::zoomIn(double r, int mark)
{
	beginResetModel();
	_histo.zoomIn(r, mark);
	endResetModel();
}

void KsGraphModel::reset()
{
	beginResetModel();
	_histo.setBining(1,0,1);
	endResetModel();
}

void KsGraphModel::update(KsDataStore *data)
{
	beginResetModel();
	if (data) {
		_histo.fill(data->_rows, data->size());
	}
	endResetModel();
}

KsTimeMap::KsTimeMap(size_t n, uint64_t min, uint64_t max)
:_data(nullptr), _dataSize(0)
{
	setBining(n, min, max);
}

KsTimeMap::KsTimeMap()
:_data(nullptr), _dataSize(0)
{
	this->clear();
}

KsTimeMap::~KsTimeMap() {}

void KsTimeMap::clear()
{
	_nBins = 0;
	_binSize = 0;
	_min = 0;
	_max = 0;
	_map.resize(0);
	_binCount.resize(0);
}

void KsTimeMap::setBining(size_t n, uint64_t min, uint64_t max)
{
	this->clear();
	uint64_t range = max - min;
	if (n == 0 || n > range)
		return;

	_nBins = n;
	if (range%(_nBins) == 0) {
		_min = min;
		_max = max;
		_binSize = range/_nBins;
	} else {
		_binSize = range/_nBins + 1;
		size_t deltaRange = _nBins*_binSize - range;
		_min = min - deltaRange/2;
		_max = _min + _nBins*_binSize;
	}

	_map.resize(_nBins + 2);
	_binCount.resize(_nBins + 2);

	resetBins(0, _nBins + 1);
	
}

void KsTimeMap::setBiningInRange(size_t n, uint64_t min, uint64_t max)
{
	this->clear();

	uint64_t range = max - min;
	if (n == 0 || n > range)
		return;

	_nBins = n;
	if (range%(_nBins) == 0) {
		_min = min;
		_max = max;
		_binSize = range/_nBins;
	} else {
		_binSize = range/_nBins + 1;
		size_t deltaRange = _nBins*_binSize - range;
		_min = min - deltaRange/2;
		_max = _min + _nBins*_binSize;
		if (_min < _data[0]->ts) {
			_min = _data[0]->ts;
			_max = _min + _nBins*_binSize;
		} else if (_max > _data[_dataSize - 1]->ts) {
			_max = _data[_dataSize - 1]->ts;
			_min = _max - _nBins*_binSize;
		}
	}

	_map.resize(_nBins + 2);
	_binCount.resize(_nBins + 2);

	resetBins(0, _nBins + 1);
}

void KsTimeMap::resetBins(size_t first, size_t last)
{
	for (size_t r = first; r <= last; ++r) {
		_map[r] = KS_EMPTY_BIN;
		_binCount[r] = 0;
	}
}

size_t KsTimeMap::setLowerEdge()
{
	size_t row = kshark_find_entry_row(_min, _data, 0, _dataSize - 1);

	if (row != 0) {
		_map[_nBins + 1] = 0;
		_binCount[_nBins + 1] = row;
	}  else {
		_map[_nBins + 1] = KS_EMPTY_BIN;
	}

	if (_data[row]->ts  < _min + _binSize)
		_map[0] = row;
	else
		_map[0] = KS_EMPTY_BIN;

	return row;
}

size_t KsTimeMap::setUpperhEdge()
{
	size_t row = kshark_find_entry_row(_max, _data, 0, _dataSize - 1);
	if (row < _dataSize - 1 || 
	    (row == _dataSize - 1 && _data[_dataSize - 1]->ts > _max)) {
		_map[_nBins] = row;
		_binCount[_nBins] = _dataSize - row;
		
	}  else {
		_map[_nBins] = KS_EMPTY_BIN;
	}

	return row;
}

void KsTimeMap::setNextBinEdge(size_t prevBin)
{
	size_t bin = prevBin + 1;
	size_t time = _min + bin*_binSize;

	size_t row = kshark_find_entry_row(time, _data, 0, _dataSize - 1);

	if (bin == _nBins - 1) 
		++time;

	if (_data[row]->ts  >= time + _binSize) {
		_map[bin] = KS_EMPTY_BIN;
		return;
	}

	_map[bin] = row;
}

void KsTimeMap::setBinCounts()
{
	size_t i = 0, prevNotEmpty;

	if (_map[_nBins + 1] != KS_EMPTY_BIN) {
		prevNotEmpty = _nBins + 1;
	} else {
		while (_map[i] < 0) {++i;}
		prevNotEmpty = i++;
	}

	while (i < _nBins) {
		if (_map[i] != KS_EMPTY_BIN) {
			_binCount[prevNotEmpty] = _map[i] - _map[prevNotEmpty];
			prevNotEmpty = i;
		}
		++i;
	}

	if (_map[_nBins] == KS_EMPTY_BIN) 
		_binCount[prevNotEmpty] = _dataSize - _map[prevNotEmpty];
	else
		_binCount[prevNotEmpty] = _map[_nBins] - _map[prevNotEmpty];
}

//void KsTimeMap::fill(struct pevent_record **data, size_t n)
void KsTimeMap::fill(struct kshark_entry **data, size_t n)
{
	_data = data;
	_dataSize = n;

	if (_nBins == 0 || _binSize == 0 || _dataSize == 0) {
		clear();
		return;
	}

	setLowerEdge();
	size_t bin = 0;
	while (bin < _nBins) {
		setNextBinEdge(bin);
		++bin;
	}

	setUpperhEdge();
	setBinCounts();
}

void KsTimeMap::shiftForward(size_t n)
{
	if (!_dataSize)
		return;

	if (binCount(OverflowBin::Upper) == 0)
		return;

	_min += n*_binSize;
	_max += n*_binSize;

	if (n >= _nBins) {
		setBining(this->_nBins, _min, _max);
		fill(this->_data, this->_dataSize);
		return;
	}

	setLowerEdge();
	size_t bin = 0;
	while(bin <  _nBins - n) {
		_map[bin] = _map[bin + n];
		_binCount[bin] = _binCount[bin + n];
		++bin;
	}

	_map[bin] = _map[bin + n];
	_binCount[bin] = 0;

	resetBins(bin + 1, _nBins);

	while (bin < _nBins) {
		setNextBinEdge(bin);
		++bin;
	}

	setUpperhEdge();
	setBinCounts();
}

void KsTimeMap::shiftBackward(size_t n)
{
	if (!_dataSize)
		return;

	if (binCount(OverflowBin::Lower) == 0)
		return;

	_min -= n*_binSize;
	_max -= n*_binSize;

	if (n >= _nBins) {
		setBining(this->_nBins, _min, _max);
		fill(this->_data, this->_dataSize);
		return;
	}

	size_t bin = _nBins - 1;
	while(1) {
		if (bin > _nBins - n && _map[bin] >= 0) {
			_map[_nBins] = _map[bin];
			_binCount[_nBins] += _binCount[bin];
		}

		_map[bin] = _map[bin-n];
		_binCount[bin] = _binCount[bin-n];
		if (bin == n)
			break;

		--bin;
	}

	resetBins(0, bin);
	resetBins(_nBins, _nBins + 1);
	setLowerEdge();

	bin = 0;
	while (bin < n) {
		setNextBinEdge(bin);
		++bin;
	}

	setUpperhEdge();
	setBinCounts();
}

void KsTimeMap::zoomOut(double r, int mark)
{
	if (!_dataSize)
		return;

	if (mark < 0)
		mark = _nBins/2;

	size_t range = _max - _min;
	double delta_tot = range*r;
	size_t delta_min = delta_tot*mark/_nBins;

	size_t min = _min - delta_min;
	size_t max = _max + (size_t) delta_tot - delta_min;

	if (min < _data[0]->ts)
		min = _data[0]->ts;

	if (max > _data[_dataSize - 1]->ts)
		max = _data[_dataSize - 1]->ts;

	setBiningInRange(this->_nBins, min, max);
	fill(this->_data, this->_dataSize);
}

void KsTimeMap::zoomIn(double r, int mark)
{
	if (!_dataSize)
		return;

	if (mark < 0)
		mark = _nBins/2;

	size_t range = _max - _min;

	/* Avoid overzooming. */
	if (range < _nBins*4)
		return;

	double delta_tot =  range*r;
	size_t delta_min;
	if (mark == (int)_nBins - 1)
		delta_min = delta_tot;
	else if (mark == 0)
		delta_min = 0;
	else
		delta_min = delta_tot*mark/_nBins;

	size_t min = _min + delta_min;
	size_t max = _max - (size_t) delta_tot + delta_min;

	setBiningInRange(this->_nBins, min, max);
	fill(this->_data, this->_dataSize);
}

void KsTimeMap::dump() const
{
	qInfo() << "\n lof < " << at(-2) << "  " << binCount(-2);

	for (size_t i=0; i<5; ++i)
		qInfo() << i << " " << at(i) << "  " << binCount(i);

	qInfo() << "....";

	for (size_t i=_nBins-5; i<_nBins; ++i)
		qInfo() << i << " " << at(i) << "  " << binCount(i);

	qInfo() << " uof > " << at(-1) << "  " << binCount(-1);
}

void KsTimeMap::shiftTo(size_t ts)
{
	if (ts < _max && ts > _min)
		return;

	size_t min = ts - _nBins*_binSize/2;
	if (min < _data[0]->ts)
		min = _data[0]->ts;

	if (min > _data[_dataSize - 1]->ts - _nBins*_binSize)
		min = _data[_dataSize - 1]->ts - _nBins*_binSize;

	size_t max = min + _nBins*_binSize;

	setBining(this->_nBins, min, max);
	fill(this->_data, this->_dataSize);
}

int64_t KsTimeMap::at(int i) const
{
	if (i >= 0 && i < (int)_nBins)
		return _map[i];

	if (i == OverflowBin::Upper)
		return _map[_nBins];

	if (i == OverflowBin::Lower)
		return _map[_nBins + 1];

	return KS_EMPTY_BIN;
}

int64_t KsTimeMap::atCpu(int bin, int cpu) const
{
	size_t found = KS_EMPTY_BIN;
	size_t nEntries = binCount(bin);
	if (!nEntries)
		return found;

	int64_t pos = _map[bin];
	for (size_t i = pos; i < pos + nEntries; ++i) {
		if (_data[i]->cpu == cpu) {
			if (_data[i]->visible & KS_GRAPH_FILTER_MASK)
				return i;
			else
				found = KS_FILTERED_BIN;
		}
	}

	return found;
}

int64_t KsTimeMap::atPid(int bin, int pid) const
{
	size_t found = KS_EMPTY_BIN;
	size_t nEntries = binCount(bin);
	if (!nEntries)
		return found;

	int64_t pos = _map[bin];
	for (size_t i = pos; i < pos + nEntries; ++i) {
		if (_data[i]->pid == pid) {
			if (_data[i]->visible & KS_GRAPH_FILTER_MASK)
				return i;
			else
				found = KS_FILTERED_BIN;
		}
	}

	return found;
}

bool KsTimeMap::isVisible(size_t row) const
{
	return _data[row]->visible & KS_GRAPH_FILTER_MASK;
}

size_t KsTimeMap::binCount(int bin) const
{
	if (bin >= 0 && bin < (int)_nBins)
		return _binCount[bin];

	if (bin == OverflowBin::Upper)
		return _binCount[_nBins];

	if (bin == OverflowBin::Lower)
		return _binCount[_nBins + 1];

	return 0;
}

size_t KsTimeMap::binCount(int bin, int cpu) const
{
	size_t nEntries = binCount(bin);
	if (!nEntries)
		return 0;

	size_t count = 0;
	int64_t pos = this->at(bin);

	for (size_t i = pos; i < pos + nEntries; ++i) {
		if (_data[i]->cpu == cpu)
			++count;
	}

	return count;
}

bool KsTimeMap::notEmpty(int bin) const
{
	if (binCount(bin) > 0)
		return true;

	return false;
}

int64_t KsTimeMap::operator[](int i) const
{
	return this->at(i);
}

bool KsTimeMap::notEmpty(int bin, int cpu, int *pid) const
{
	int val = getPidFront(bin, cpu, true);
	if (pid)
		*pid = val;

	if (val < 0)
		return false;

	return true;
}

kshark_entry *KsTimeMap::getEntryFront(int bin, int pid, bool visOnly) const
{
	size_t nEntries = binCount(bin);
	if (!nEntries)
		return nullptr;

	size_t first = this->at(bin);
	return kshark_get_entry_by_pid_front(first, nEntries, pid, visOnly,
					     KS_GRAPH_FILTER_MASK, _data);
}

kshark_entry *KsTimeMap::getEntryFront(int bin, int pid, bool visOnly,
				      matching_condition_func checkPidFunc) const
{
	size_t nEntries = binCount(bin);
	if (!nEntries)
		return nullptr;

	size_t first = this->at(bin);
	return kshark_get_entry_front(first, nEntries, checkPidFunc,
				     pid, visOnly, KS_GRAPH_FILTER_MASK, _data);
}

kshark_entry *KsTimeMap::getEntryBack(int bin, int pid, bool visOnly) const
{
	size_t nEntries = binCount(bin);
	if (!nEntries)
		return nullptr;

	size_t first = this->at(bin) + nEntries - 1;
	return kshark_get_entry_by_pid_back(first, nEntries, pid, visOnly,
					    KS_GRAPH_FILTER_MASK, _data);
}

kshark_entry *KsTimeMap::getEntryBack(int bin, int pid, bool visOnly,
				      matching_condition_func checkPidFunc) const
{
	size_t nEntries = binCount(bin);
	if (!nEntries)
		return nullptr;

	size_t first = this->at(bin) + nEntries - 1;
	return kshark_get_entry_back(first, nEntries, checkPidFunc,
				     pid, visOnly, KS_GRAPH_FILTER_MASK, _data);
}

int KsTimeMap::getPidFront(int bin, int cpu, bool visOnly) const
{
	size_t nEntries = binCount(bin);
	if (!nEntries)
		return KS_EMPTY_BIN;

	/* Set the position at the begining of the bin and go forward. */
	size_t first = this->at(bin);
	return kshark_get_pid_front(first, nEntries, cpu, visOnly, KS_GRAPH_FILTER_MASK, _data);
}

int KsTimeMap::getPidBack(int bin, int cpu, bool visOnly) const
{
	size_t nEntries = binCount(bin);
	if (!nEntries)
		return KS_EMPTY_BIN;

	/* Set the position at the end of the bin and go backwards. */
	size_t first = this->at(bin) + nEntries - 1;
	return kshark_get_pid_back(first, nEntries, cpu, visOnly, KS_GRAPH_FILTER_MASK, _data);
}

int KsTimeMap::getCpu(int bin, int pid, bool visOnly) const
{
	size_t nEntries = binCount(bin);
	if (!nEntries)
		return KS_EMPTY_BIN;

	if (bin == OverflowBin::Lower) {
		/* Set the position at the end of the Lower Overflow bin and go
		 * backwards. */
		size_t first = binCount(bin) - 1;
		return kshark_get_cpu_back(first, nEntries, pid, visOnly, KS_GRAPH_FILTER_MASK, _data);
	} else {
		/* Set the position at the begining of the bin and go forward. */
		size_t first = this->at(bin);
		return kshark_get_cpu_front(first, nEntries, pid, visOnly, KS_GRAPH_FILTER_MASK, _data);
	}
}
