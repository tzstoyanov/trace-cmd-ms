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

#ifndef KS_TRACEVIEWER_H
#define KS_TRACEVIEWER_H 1

// Qt
#include <QtWidgets>
#include <QTableView>

#include <iostream>

#include "KsModel.h"
#include "KsDataStore.h"

typedef bool (*condition_func)(QString, QString);

class KsTraceViewer : public QWidget {

	Q_OBJECT

public:
	KsTraceViewer(QWidget *parent = 0);
	virtual ~KsTraceViewer();

	void loadData(KsDataStore *data);
	void reset() {_model.reset();}

signals:
	void select(size_t);

private slots:
	void pageChanged(int);
	void searchEditText(const QString &);
	void searchEditColumn(int);
	void searchEditSelect(int);
	void graphFollowsChanged(int);
	void search();
	void clicked(const QModelIndex& i);
	void showRow(int r, bool mark);
	
private:
	bool event(QEvent *event);
	void init();
	size_t searchItem(int c, const QString &text, condition_func cond);
	void resizeToContents();

	QVBoxLayout 	_layout;
	QTableView 		_view;
	KsViewModel		_model;

	QStringList 	_tableHeader;
	QToolBar 		_toolbar;
	QLabel 			_label1, _label2, _label3;
	QSpinBox 		_pageSpinBox;
	QComboBox 		_columnComboBox;
	QComboBox 		_selectComboBox;
	QLineEdit 		_searchLineEdit;
	QCheckBox 		_graphFollowsCheckBox;

	bool					_searchDone;
	bool 					_graphFollows;
	QList<size_t>			_matchList;
	QList<size_t>::iterator _it;

	enum {
		CONTAINS,
		MATCH,
		NOT_HAVE
	};
};

#endif



