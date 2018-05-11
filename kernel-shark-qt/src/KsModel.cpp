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

// KernelShark
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
		/*
		 * Use the index of the proxy model to retrieve the value
		 * of the row number in the base model. This works because
		 *  the source row number is shown in column "0".
		 */
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
			return KsUtils::Ts2String(_data[row]->ts, 6);

		case TRACE_VIEW_COL_COMM:
			return kshark_get_task_lazy(_data[row]);

		case TRACE_VIEW_COL_PID:
			return kshark_get_pid_lazy(_data[row]);

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
{
	kshark_histo_init(&_histo);
}

KsGraphModel::KsGraphModel(int cpus, QObject *parent)
: QAbstractTableModel(parent), _pevt(nullptr), _cpus(cpus)
{
	kshark_histo_init(&_histo);
}

KsGraphModel::~KsGraphModel()
{
	kshark_histo_clear(&_histo);
}

QVariant KsGraphModel::getValue(const QModelIndex &index) const
{
	return 0;
}

QVariant KsGraphModel::data(const QModelIndex &index, int role) const
{
	if (role != Qt::DisplayRole)
		return {};

	QVariant val = this->getValue(index);
	return val;
}

void KsGraphModel::fill(pevent *pevt, kshark_entry **entries, size_t n)
{
	_pevt = pevt;

	if (n == 0)
		return;

	beginResetModel();

	if (_histo.n_bins == 0)
		kshark_histo_set_bining(&_histo,
					KS_DEFAULT_NBUNS,
					entries[0]->ts,
					entries[n-1]->ts);

	kshark_histo_fill(&_histo, entries, n);

	endResetModel();
}

void KsGraphModel::shiftForward(size_t n)
{
	beginResetModel();
	kshark_histo_shift_forward(&_histo, n);
	endResetModel();
}

void KsGraphModel::shiftBackward(size_t n)
{
	beginResetModel();
	kshark_histo_shift_backward(&_histo, n);
	endResetModel();
}

void KsGraphModel::shiftTo(size_t ts)
{
	beginResetModel();
	kshark_histo_shift_to(&_histo, ts);
	endResetModel();
}

void KsGraphModel::zoomOut(double r, int mark)
{
	beginResetModel();
	kshark_histo_zoom_out(&_histo, r, mark);
	endResetModel();
}

void KsGraphModel::zoomIn(double r, int mark)
{
	beginResetModel();
	kshark_histo_zoom_in(&_histo, r, mark);
	endResetModel();
}

void KsGraphModel::reset()
{
	beginResetModel();
	kshark_histo_set_bining(&_histo, 1, 0, 1);
	endResetModel();
}

void KsGraphModel::update(KsDataStore *data)
{
	beginResetModel();
	if (data) {
		kshark_histo_fill(&_histo, data->_rows, data->size());
	}
	endResetModel();
}
