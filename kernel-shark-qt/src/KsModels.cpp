// SPDX-License-Identifier: LGPL-2.1

/*
 * Copyright (C) 2017 VMware Inc, Yordan Karadzhov <ykaradzhov@vmware.com>
 */

/**
 *  @file    KsModels.cpp
 *  @brief   Models for data representation.
 */

// KernelShark
#include "KsModels.hpp"
#include "KsWidgetsLib.hpp"
#include "KsUtils.hpp"

/** Create a default (empty) KsFilterProxyModel object. */
KsFilterProxyModel::KsFilterProxyModel(QObject *parent)
: QSortFilterProxyModel(parent),
  _searchStop(false),
  _source(nullptr)
{}

/**
 * Returns False if the item in the row indicated by the sourceRow and
 * sourceParentshould be filtered out. Otherwise returns True.
 */
bool
KsFilterProxyModel::filterAcceptsRow(int sourceRow,
				     const QModelIndex &sourceParent) const
{
	if (_data[sourceRow]->visible & KS_TEXT_VIEW_FILTER_MASK)
		return true;

	return false;
}

/** Provide the Proxy model with data. */
void KsFilterProxyModel::fill(KsDataStore *data)
{
	_data = data->rows();
}

/** Set the source model for this Proxy model. */
void KsFilterProxyModel::setSource(KsViewModel *s)
{
	QSortFilterProxyModel::setSourceModel(s);
	_source = s;
}

void KsFilterProxyModel::_search(int column,
				 const QString &searchText,
				 condition_func cond,
				 QList<int> *matchList,
				 int first, int last,
				 QProgressBar *pb,
				 QLabel *l,
				 bool notify)
{
	int row, nRows(last - first + 1);
	int pbCount(1);
	QVariant item;
	QString text;

	_searchProgress = 0;

	if (nRows > KS_PROGRESS_BAR_MAX)
		pbCount = nRows / KS_PROGRESS_BAR_MAX;
	else
		_searchProgress = KS_PROGRESS_BAR_MAX - nRows;

	/* Loop over the items of the proxy model. */
	for (int r = first; r <= last; ++r) {
		/*
		 * Use the index of the proxy model to retrieve the value
		 * of the row number in the base model.
		 */
		row = mapRowFromSource(r);
		item = _source->getValue(column, row);
		if (cond(searchText, item.toString())) {
			matchList->append(row);
		}

		if (_searchStop) {
			_searchStop = false;
			break;
		}

		/* Deal with the Progress bar of the seatch. */
		if ((r - first) % pbCount == 0) {
			if (notify) {
				std::lock_guard<std::mutex> lk(_mutex);
				++_searchProgress;
				_pbCond.notify_one();
			} else {
				if (pb)
					pb->setValue(pb->value() + 1);
				if (l)
					l->setText(QString(" %1").arg(matchList->count()));
				QApplication::processEvents();
			}
		}
	}
}

/** @brief Search the content of the table for a data satisfying an abstract
 *	   condition.
 *
 * @param column: The number of the column to search in.
 * @param searchText: The text to search for.
 * @param cond: Matching condition function.
 * @param matchList: Output location for a list containing the row indexes of
 *		     the cells satisfying matching condition.
 * @param pb: Input location for a Progressbar used to visualize the progress
 *	      of the search.
 * @param l: Input location for a label used to show the number of cells found.
 *
 * @returns The number of cells satisfying the matching condition.
 */
size_t KsFilterProxyModel::search(int column,
				  const QString &searchText,
				  condition_func cond,
				  QList<int> *matchList,
				  QProgressBar *pb,
				  QLabel *l)
{
	int nRows = rowCount({});

	_search(column, searchText, cond, matchList, 0, nRows - 1,
		pb, l, false);

	return matchList->count();
}

/** @brief Search the content of the table for a data satisfying an abstract
 *	   condition.
 *
 * @param column: The number of the column to search in.
 * @param searchText: The text to search for.
 * @param cond: Matching condition function.
 * @param first: Row index specifying the position inside the table from
 *		 where the search starts.
 * @param last:  Row index specifying the position inside the table from
 *		 where the search ends.
 * @param notify: Input location for flag specifying if the search has to
 *		  notify the main thread when to update the progress bar.
 *
 * @returns A list containing the row indexes of the cells satisfying matching
 *	    condition.
 */
QList<int> KsFilterProxyModel::searchMap(int column,
					 const QString &searchText,
					 condition_func cond,
					 int first,
					 int last,
					 bool notify)
{
	QList<int> matchList;
	qInfo() << "searchMap" << first << last;
	_search(column, searchText, cond, &matchList, first, last,
		nullptr, nullptr, notify);

	return matchList;
}

/** Create default (empty) KsViewModel object. */
KsViewModel::KsViewModel(QObject *parent)
: QAbstractTableModel(parent),
  _data(nullptr),
  _nRows(0),
  _header({"#", "CPU", "Time Stamp", "Task", "PID",
	   "Latency", "Event", "Info"}),
  _markA(-1),
  _markB(-1)
{}

/**
 * Get the data stored under the given role for the item referred to by
 * the index. This is an implementation of the pure virtual method of the
 * abstract model class.
 */
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

/** Get the data stored in a given cell of the table. */
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
			return kshark_get_task_easy(_data[row]);

		case TRACE_VIEW_COL_PID:
			return kshark_get_pid_easy(_data[row]);

		case TRACE_VIEW_COL_LAT:
			return kshark_get_latency_easy(_data[row]);

		case TRACE_VIEW_COL_EVENT:
			return kshark_get_event_name_easy(_data[row]);

		case TRACE_VIEW_COL_INFO :
			return kshark_get_info_easy(_data[row]);

		default:
			return {};
	}
}

/**
 * Get the header of a column. This is an implementation of the pure virtual
 * method of the abstract model class.
 */
QVariant KsViewModel::headerData(int column,
				 Qt::Orientation orientation,
				 int role) const
{
	if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
		return {};

	if (column < _header.count())
		return _header.at(column);

	return {};
}

/** Provide the model with data. */
void KsViewModel::fill(KsDataStore *data)
{
	beginInsertRows(QModelIndex(), 0, data->size() - 1);

	_data = data->rows();
	_nRows = data->size();

	endInsertRows();
}

/** @brief Select a row in the table.
 *
 * @param state: Identifier of the marker used to select the row.
 * @param row: Row index.
 */
void KsViewModel::selectRow(DualMarkerState state, int row)
{
	if (state == DualMarkerState::A) {
		_markA = row;
		_markB = -1;
	} else {
		_markB = row;
		_markA = -1;
	}
}

/** Reset the model. */
void KsViewModel::reset()
{
	beginResetModel();

	_data = nullptr;
	_nRows = 0;

	endResetModel();
}

/** Update the model. Use this function if the data has changed. */
void KsViewModel::update(KsDataStore *data)
{
	/*
	 * Do not try to skip the reset(). The row count has to be set to
	 * zero before you full.
	 */
	reset();
	fill(data);
}

/** @brief Search the content of the table for a data satisfying an abstract
 *	   condition.
 *
 * @param column: The number of the column to search in.
 * @param searchText: The text to search for.
 * @param cond: Matching condition function.
 * @param matchList: Output location for a list containing the row indexes of
 *		     the cells satisfying the matching condition.
 *
 * @returns The number of cells satisfying the matching condition.
 */
size_t KsViewModel::search(int column,
			   const QString &searchText,
			   condition_func cond,
			   QList<size_t> *matchList)
{
	int nRows = rowCount({});
	QVariant item;

	for (int r = 0; r < nRows; ++r) {
		item = getValue(r, column);
		if (cond(searchText, item.toString())) {
			matchList->append(r);
		}
	}

	return matchList->count();
}
