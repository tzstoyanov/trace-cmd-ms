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

#include "KsTraceViewer.h"

#define VIEWER_PAGE_SIZE 1000000

KsTraceViewer::KsTraceViewer(QWidget *parent)
: QWidget(parent),
  _view(this),
  _model(this),
  _tableHeader(_model.header()),
  _toolbar(this),
  _label1("Page", this),
  _label2("Search: Column", this),
  _label3("Graph follows", this),
  _pageSpinBox(this),
  _columnComboBox(this),
  _selectComboBox(this),
  _searchLineEdit(this),
  _checkBox(this)
{
	init();
	_view.setModel(&_model);

	_toolbar.setOrientation(Qt::Horizontal);

	_toolbar.addWidget(&_label1);

	_pageSpinBox.setMaximum(0);
	connect(&_pageSpinBox, SIGNAL(valueChanged(int)), this, SLOT(pageChanged(int)));
// 	connect(&_pageSpinBox, SIGNAL(valueChanged(int)), &_table, SLOT(setCurrentIndex(int)));
	_toolbar.addWidget(&_pageSpinBox);

	_toolbar.addSeparator();
	_toolbar.addWidget(&_label2);

	_columnComboBox.addItems(_tableHeader);

	connect(&_columnComboBox, SIGNAL( currentIndexChanged(int) ), this, SLOT( searchEditColumn(int) ));
	_toolbar.addWidget(&_columnComboBox);

	_selectComboBox.addItem("contains");
	_selectComboBox.addItem("full match");
	_selectComboBox.addItem("does not have");
	connect(&_selectComboBox, SIGNAL( currentIndexChanged(int) ), this, SLOT( searchEditSelect(int) ));
	_toolbar.addWidget(&_selectComboBox);

	_searchLineEdit.setMaximumWidth(200);
	_searchLineEdit.setFixedWidth(240);
	connect(&_searchLineEdit, SIGNAL(returnPressed()), this, SLOT(search()));
	connect(&_searchLineEdit, SIGNAL(textEdited(const QString &)), this, SLOT(searchEditText(const QString &)));
	_toolbar.addWidget(&_searchLineEdit);

	_toolbar.addSeparator();
	_toolbar.addWidget(&_checkBox);
	_toolbar.addWidget(&_label3);

	_layout.addWidget(&_toolbar);
	_layout.addWidget(&_view);

	this->setLayout(&_layout);
}

KsTraceViewer::~KsTraceViewer() {}

void KsTraceViewer::loadData(KsDataStore *data)
{
	_model.reset();
	_model.fill(data->_pevt, data->_rows, data->size());
	resizeToContents();
}

void KsTraceViewer::init()
{
	setMinimumHeight(300);
	setMinimumWidth(500);
	_view.horizontalHeader()->setStretchLastSection(true);
	_view.verticalHeader()->setVisible(false);
	//_view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	_view.setEditTriggers(QAbstractItemView::NoEditTriggers);
	_view.setSelectionBehavior(QAbstractItemView::SelectRows);
	_view.setSelectionMode(QAbstractItemView::SingleSelection);
// 	_view.setShowGrid(false);
	_view.setStyleSheet("QTableView {selection-background-color: orange;}");
	_view.setGeometry(QApplication::desktop()->screenGeometry());

	_view.verticalHeader()->resizeSections(QHeaderView::ResizeToContents);
	_view.verticalHeader()->setDefaultSectionSize(20);
}

void KsTraceViewer::resizeToContents()
{
	_view.setVisible(false);
// 	_view.resizeRowsToContents();
	_view.resizeColumnsToContents();
	_view.setVisible(true);
}

void KsTraceViewer::pageChanged(int p)
{
	std::cout << "page: " << p << "\n";
	
	_searchDone = false;
}

void KsTraceViewer::searchEditText(const QString &text)
{
	std::cout << "search: " << text.toStdString() << "\n";
	_searchDone = false;
}

void KsTraceViewer::searchEditColumn(int index)
{
	std::cout << "search  col: " << _tableHeader.at(index).toStdString() << "  " << index << "\n";
	_searchDone = false;
}

void KsTraceViewer::searchEditGraph(int state) {
	_searchDone = false;
}

void KsTraceViewer::searchEditSelect(int index) {
	std::cout << "search  sel: " << index << "\n";
	_searchDone = false;
}

bool notHaveCond(QString searchText, QString itemText) {
	return ! itemText.contains(searchText, Qt::CaseInsensitive);
}

bool containsCond(QString searchText, QString itemText) {
	return itemText.contains(searchText, Qt::CaseInsensitive);
}

bool matchCond(QString searchText, QString itemText) {
	if (itemText.compare(searchText, Qt::CaseInsensitive) == 0 )
		return true;

	return false;
}

size_t KsTraceViewer::select(	int column, const QString &searchText, condition_func cond) {
	int count = _model.search(column, searchText, cond, &_matchList);
	std::cout << column << "   found: " << count << "\n";
	_searchDone = true;
	_view.clearSelection();
	_it = _matchList.begin();
	return count;
}

void KsTraceViewer::search() {
	_searchLineEdit.setReadOnly(true);
	if (!_searchDone) {
		std::cout << "<_| search: " << _searchLineEdit.text().toStdString() << "\n";
		_matchList.clear();
		QString xText = _searchLineEdit.text();
		int xColumn = _columnComboBox.currentIndex();
		int xSelect = _selectComboBox.currentIndex();
		std::cerr << xColumn << " " << xSelect << std::endl;
		switch (xSelect) {
			case CONTAINS:
				select(xColumn, xText, &containsCond);
				break;

			case MATCH:
				select(xColumn, xText, &matchCond);
				break;

			case NOT_HAVE:
				select(xColumn, xText, &notHaveCond);
				break;

			default:
				break;
		}
	}

	if (!_matchList.empty()) {
		if (_it != _matchList.end() ){
			_view.selectRow(*_it);
			_it++;
		} else {
			_view.selectRow(*_it);
			_it = _matchList.begin();
		}
	}

	_searchLineEdit.setReadOnly(false);
}


