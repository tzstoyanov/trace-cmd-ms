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


#include "KsModel.h"

#include "trace-cmd.h"

KsModel::~KsModel()
{
	reset();
}

size_t KsModel::select(	int				 column,
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

QVariant KsModel::data(const QModelIndex &index, int role) const
{
	if (role != Qt::DisplayRole)
		return {};

	QVariant val = this->getValue(index);
	return val;
}

int KsModel::rowCount(const QModelIndex &) const {
// 	return (_data.count() < VIEWER_PAGE_SIZE) ? _data.count() : VIEWER_PAGE_SIZE;
	return _data.count();
}

QVariant KsModel::getValue(const QModelIndex &index) const
{
	int row = index.row();
	int column = index.column();
	switch (column) {
		case TRACE_VIEW_STORE_COL_INDEX :
			return _data[row]->pos;

		case TRACE_VIEW_STORE_COL_CPU :
			return _data[row]->cpu;

		case TRACE_VIEW_STORE_COL_TS :
			char time[32];
			uint64_t secs, usecs;
			usecs = _data[row]->timestamp;
			usecs /= 1000;
			secs = usecs / 1000000ULL;
			usecs -= secs * 1000000ULL;
			sprintf(time, "%llu.%06llu", (long long)secs, (long long)usecs);
			return time;

		case TRACE_VIEW_STORE_COL_COMM :
			return _data[row]->task;

		case TRACE_VIEW_STORE_COL_PID :
			return _data[row]->pid;

		case TRACE_VIEW_STORE_COL_LAT :
			return _data[row]->latency;

		case TRACE_VIEW_STORE_COL_EVENT :
			return _data[row]->event;

		case TRACE_VIEW_STORE_COL_INFO :
			return _data[row]->info;

		default: return {};
	}
}


QVariant KsModel::headerData(	int section,
								Qt::Orientation orientation,
								int role) const
{
	if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
		return {};

	if (section < _header.count())
		return _header.at(section);
	
	return {};
}

void KsModel::append(ks_entry *e)
{
	beginInsertRows(QModelIndex(), _data.count(), _data.count());
	_data.append(e);
	endInsertRows();
}

void KsModel::fill(ks_entry **entries, size_t n)
{
	beginInsertRows(QModelIndex(), _data.count(), _data.count() + n);

	for(size_t r = 0; r < n; ++r)
		_data.append(entries[r]);

	endInsertRows();
}

void KsModel::reset()
{
	beginResetModel();
	for (auto const & r:_data)
		ks_free_entry(r);

	_data.clear();
	endResetModel();
}

