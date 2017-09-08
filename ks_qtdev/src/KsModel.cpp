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
#include <iostream>

#include "KsModel.h"

#include "ks-view.h"

struct trace_seq KsViewModel::_seq;

KsViewModel::KsViewModel(QObject *parent)
: QAbstractTableModel(parent),
//  _data(nullptr),
  _header({"#", "CPU", "Time Stamp", "Task", "PID", "Latency", "Event", "Info"}),
  _pevt(nullptr) /*,
  _page(0) */
{
	trace_seq_init(&_seq);
}

KsViewModel::~KsViewModel()
{
	reset();
	trace_seq_destroy(&_seq);
}

QVariant KsViewModel::data(const QModelIndex &index, int role) const
{
	if (role != Qt::DisplayRole)
		return {};

	QVariant val = this->getValue(index.column(), index.row());
	return val;
}

int KsViewModel::rowCount(const QModelIndex &) const {
// 	return (_data.count() < VIEWER_PAGE_SIZE) ? _data.count() : VIEWER_PAGE_SIZE;
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
	//int row = index.row();
	if (row >= _data.count())
		return {};

	//int column = index.column();

	switch (column) {
		case TRACE_VIEW_STORE_COL_INDEX :
			return row;

		case TRACE_VIEW_STORE_COL_CPU :
			return _data[row]->cpu;

		case TRACE_VIEW_STORE_COL_TS :
			char time[32];
			uint64_t secs, usecs;
			usecs = _data[row]->ts;
			usecs /= 1000;
			secs = usecs / 1000000ULL;
			usecs -= secs * 1000000ULL;
			sprintf(time, "%llu.%06llu", (long long)secs, (long long)usecs);
			return time;

		case TRACE_VIEW_STORE_COL_COMM :
		{
			int pid = pevent_data_pid(_pevt, _data[row]);
			return pevent_data_comm_from_pid(_pevt, pid);
		}

		case TRACE_VIEW_STORE_COL_PID :
			return pevent_data_pid(_pevt, _data[row]);

		case TRACE_VIEW_STORE_COL_LAT :
			trace_seq_reset(&_seq);
			pevent_data_lat_fmt(_pevt, &_seq, _data[row]);
			return _seq.buffer;

		case TRACE_VIEW_STORE_COL_EVENT :
		{
			int etype = pevent_data_type(_pevt, _data[row]);
			struct event_format *event = pevent_data_event_from_type(_pevt, etype);
			if (!event) {
				return "[UNKNOWN EVENT]";
			} else {
				return event->name;
			}
		}

		case TRACE_VIEW_STORE_COL_INFO :
		{
			int etype = pevent_data_type(_pevt, _data[row]);
			struct event_format *event = pevent_data_event_from_type(_pevt, etype);
			trace_seq_reset(&_seq);
			pevent_event_info(&_seq, event,_data[row] );
			return _seq.buffer;
		}

		default: return {};
	}
}

QVariant KsViewModel::headerData(	int section,
								Qt::Orientation orientation,
								int role) const
{
	if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
		return {};

	if (section < _header.count())
		return _header.at(section);
	
	return {};
}

void KsViewModel::fill(pevent *pevt, pevent_record **entries, size_t n)
{
	_pevt = pevt;

	beginInsertRows(QModelIndex(), _data.count(), _data.count() + n - 1);

	for(size_t r = 0; r < n; ++r)
		_data.append(entries[r]);

	endInsertRows();
}

void KsViewModel::reset()
{
	beginResetModel();
	_data.clear();
	endResetModel();
}


size_t KsViewModel::search(	int				 column,
						const QString	&searchText,
						condition_func	 cond,
						QList<size_t>	*matchList)
{
	int nRows = this->rowCount({});
	for (int r = 0; r < nRows; ++r)
	{
		QModelIndex index = this->index(r, column);
		QVariant item = this->getValue(index);
		if ( cond( searchText, item.toString() ) )
		{
			matchList->append(r);
		}
	}

	return matchList->count();
}


//////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////

KsGraphModel::KsGraphModel(QObject *parent)
: QAbstractTableModel(parent),
  _pevt(nullptr),
  _cpus(1)
{
	_map = new KsTimeMap;
}

KsGraphModel::KsGraphModel(int cpus, QObject *parent)
: QAbstractTableModel(parent),
  _pevt(nullptr),
  _cpus(cpus)
{
	_map = new KsTimeMap;
}


KsGraphModel::~KsGraphModel()
{
	//reset();
}

QVariant KsGraphModel::getValue(int column, int row) const
{
	switch (column) {
		case 0 :
		{
			return row;
		}
		case 1 :
		{
			int pid = pevent_data_pid(_pevt, _data[row]);
			return pevent_data_comm_from_pid(_pevt, pid);
		}

		default:
			break;
	}

	//double val = 2. - (int)column;
	double val = 0.;
	if (_map->binCount(row, column - 2))
		val += .9;

	return val;
}

QVariant KsGraphModel::getValue(const QModelIndex &index) const
{
	size_t row = index.row();
	size_t column = index.column();
	return getValue(column, row);
}

QVariant KsGraphModel::data(const QModelIndex &index, int role) const
{
	if (role != Qt::DisplayRole)
		return {};

	QVariant val = this->getValue(index.column(), index.row());
	return val;
}

void KsGraphModel::fill(pevent *pevt, pevent_record **entries, size_t n, bool defaultMap)
{
	_pevt = pevt;

	//beginInsertRows(QModelIndex(), 0, 1023);
	beginResetModel();
	
	if (defaultMap)
		_map->setBining(KS_GRAPH_N_BINS, entries[0]->ts, entries[n-1]->ts);

	_map->fill(entries, n);

	//for(size_t r = 0; r < n; ++r)
		//_data.append(entries[r]);

	//endInsertRows();
	endResetModel();
}

void KsGraphModel::reset()
{
	beginResetModel();
	_data.clear();
	_map->setBining(0,0,1);
	endResetModel();
}


KsTimeMap::KsTimeMap(size_t n, uint64_t min, uint64_t max)
:_min(min), _max(max), _nBins(n)
{
	setBining(n, min, max);
}

KsTimeMap::KsTimeMap()
:_min(0), _max(0), _nBins(0) 
{
	_binSize = 1;
	//_map = new int64_t[1];
	_map.resize(1);
	_map[0] = -1;
	_dataSize = 0;
}


KsTimeMap::~KsTimeMap()
{
	//delete[] _map;
}

void KsTimeMap::setBining(size_t n, uint64_t min, uint64_t max)
{
	_min = min;
	_max = max;

	_dataSize = 0;
	_nBins = n;
	_binSize = (_max - _min)/(_nBins - 1);

	//delete[] _map;
	//_map = new int64_t[_nBins + 1];

	_map.resize(_nBins + 1);

	for (size_t r = 0; r <= _nBins; ++r)
		_map[r] = -1;
}

void KsTimeMap::fill(struct pevent_record **data, size_t n)
{
	size_t r = 0;
	while (data[r]->ts < _min) {++r;}

	_map[0] = r++;
	size_t bin, time;
	while (r < n) {
		time = data[r]->ts;
		if (time > _max) {
			_map[_nBins] = r;
			break;
		}

		bin = (time - _min) / _binSize;

		if (_map[bin] < 0)
			_map[bin] = r;

		++r;
	}

	_data = data;
	_dataSize = n;
}

size_t KsTimeMap::binCount(size_t bin) const
{
	if (bin >= _map.size())
		return 0;

	if (_map[bin] < 0)
		return 0;

	if (bin == _nBins)
		return _dataSize - _map[bin];

	size_t i = 1;
	while (1) {
		if (_map[bin + i] > 0)
			return _map[bin + i] - _map[bin];

		if (bin + i >= _nBins - 1) {
			if (_map[_nBins] > 0)
				return _map[_nBins] - _map[bin];
			else
				return _dataSize - _map[bin];
		}
		++i;
	} 
}


size_t KsTimeMap::binCount(size_t bin, int cpu) const
{
	size_t n = binCount(bin);
	if (!n)
		return 0;

	size_t count = 0;
	int64_t pos = _map[bin];

	for (size_t i = pos; i < pos + n; ++i) {
		if (_data[i]->cpu == cpu)
			++count;
	}

	return count;
}














