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

// Qt
#include <QAbstractTableModel>

#include "ks-view.h"

typedef bool (*condition_func)(QString, QString);

class KsModel : public QAbstractTableModel {

	QList<ks_entry*>	_data;
	QStringList 		_header;
// 	size_t				_page;

public:
	KsModel(QStringList header, QObject * parent = nullptr)
	: QAbstractTableModel{parent}, _header(header)/*, _page(0) */{}
	virtual ~KsModel();

	int rowCount(const QModelIndex &) const override;
	int columnCount(const QModelIndex &) const override { return _header.count(); }

	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

	QVariant data(const QModelIndex &index, int role) const override;

	void append(ks_entry *entry);
	void fill(ks_entry **entries, size_t n);
	void reset();

	QVariant getValue(const QModelIndex &index) const;

	size_t select(	int				 column,
					const QString	&searchText,
					condition_func	 cond,
					QList<size_t>	*matchList);
};


#endif
