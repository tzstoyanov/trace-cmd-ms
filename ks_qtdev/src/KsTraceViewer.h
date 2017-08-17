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
#include <QTableWidget>

#include "trace-cmd.h"

typedef bool (*condition_func)(QString, QString);

class KsTraceViewer : public QWidget {

	Q_OBJECT

public:
	KsTraceViewer(QWidget *parent = 0);
	virtual ~KsTraceViewer();

	void loadData(struct tracecmd_input *handle);
	void loadData_dev(struct tracecmd_input *handle);
	void loadData(const QString& file);

private slots:
	void pageChanged(int);
	void searchEditText(const QString &);
	void searchEditColumn(int);
	void searchEditSelect(int);
	void searchEditGraph(int);
	void search();

private:
	struct tracecmd_input 	*_handle;

	QVBoxLayout 			_layout;
	QToolBar 				_toolbar;
	QStackedWidget 			_table;

	QStringList 			 _tableHeader;

	QVector<QTableWidget*>	 _pages;
	QTableWidget 			*_currentPage;

// 	QVector<QStringListModel*>	 _pages;
// 	QVector<QAbstractListModel*>	 _pages;
// 	QTableView 				*_currentPage;

	QLabel 			_label1, _label2, _label3;
	QSpinBox 		_pageSpinBox;
	QComboBox 		_columnComboBox;
	QComboBox 		_selectComboBox;
	QLineEdit 		_searchLineEdit;
	QCheckBox 		_checkBox;

	void getValue(	struct tracecmd_input	*handle,
					struct pevent			*pevent,
					struct pevent_record 	*record,
					int						 column,
					QTableWidgetItem		*item);

	QTableWidget* addNewPage(int rows);

	bool    _searchDone;

	enum {
		CONTAINS,
		MATCH,
		NOT_HAVE
	};

	QList<QTableWidgetItem*>           _matchList;
	QList<QTableWidgetItem*>::iterator _it;

	size_t select(int c, const QString &text, condition_func cond);
};

#endif



