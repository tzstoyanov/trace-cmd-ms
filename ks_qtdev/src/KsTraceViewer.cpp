/*
 * Copyright (C) 2017 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
 *
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

// Kernel Shark 2
#include "KsTraceViewer.h"

KsTraceViewer::KsTraceViewer(QWidget *parent)
: QWidget(parent),
  _view(this),
  _model(this),
  _tableHeader(_model.header()),
  _toolbar(this),
  //_label1("Page", this),
  _label2("Search: Column", this),
  _label3("Graph follows", this),
  //_pageSpinBox(this),
  _columnComboBox(this),
  _selectComboBox(this),
  _searchLineEdit(this),
  _graphFollowsCheckBox(this),
  _searchDone(false),
  _graphFollows(true)
{
	this->setMinimumHeight(SCREEN_HEIGHT/4);
	this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

	/** Make a search toolbar. */
	_toolbar.setOrientation(Qt::Horizontal);

	/** On the toolbar make a page Spin box. */
	//_toolbar.addWidget(&_label1);
	//_pageSpinBox.setMaximum(0);
	//connect(&_pageSpinBox, SIGNAL(valueChanged(int)),
		//this, SLOT(pageChanged(int)));

	//_toolbar.addWidget(&_pageSpinBox);
	//_toolbar.addSeparator();

	/** On the toolbar make two Combo boxes for the search settings. */
	_toolbar.addWidget(&_label2);
	_columnComboBox.addItems(_tableHeader);
	connect(&_columnComboBox, SIGNAL(currentIndexChanged(int)),
		this, SLOT(searchEditColumn(int)));
	_toolbar.addWidget(&_columnComboBox);

	_selectComboBox.addItem("contains");
	_selectComboBox.addItem("full match");
	_selectComboBox.addItem("does not have");
	connect(&_selectComboBox, SIGNAL(currentIndexChanged(int)),
		this, SLOT(searchEditSelect(int)));
	_toolbar.addWidget(&_selectComboBox);

	/** On the toolbar make a Line edit field the search. */
	_searchLineEdit.setMaximumWidth(FONT_WIDTH*30);

	connect(&_searchLineEdit, SIGNAL(returnPressed()),
		this, SLOT(search()));

	connect(&_searchLineEdit, SIGNAL(textEdited(const QString &)),
		this, SLOT(searchEditText(const QString &)));

	_toolbar.addWidget(&_searchLineEdit);
	_toolbar.addSeparator();

	/** On the toolbar make a Check box for connecting the search pannel
	 *  to the Graph widget. */
	_graphFollowsCheckBox.setCheckState(Qt::Checked);	
	connect(&_graphFollowsCheckBox, SIGNAL(stateChanged(int)),
		this, SLOT(graphFollowsChanged(int)));

	_toolbar.addWidget(&_graphFollowsCheckBox);
	_toolbar.addWidget(&_label3);

	/** Initialize the trace viewer. */
	_view.horizontalHeader()->setStretchLastSection(true);
	_view.verticalHeader()->setVisible(false);
	_view.setEditTriggers(QAbstractItemView::NoEditTriggers);
	_view.setSelectionBehavior(QAbstractItemView::SelectRows);
	_view.setSelectionMode(QAbstractItemView::SingleSelection);
 	//_view.setShowGrid(false);
	_view.setStyleSheet("QTableView {selection-background-color: green;}");
	_view.setGeometry(QApplication::desktop()->screenGeometry());
	_view.verticalHeader()->resizeSections(QHeaderView::ResizeToContents);
	_view.verticalHeader()->setDefaultSectionSize(FONT_HEIGHT*1.25);
	_view.setModel(&_model);

	connect(&_view, SIGNAL(clicked(const QModelIndex&)),
		this, SLOT(clicked(const QModelIndex&)));

	/** Set the layout */
	_layout.addWidget(&_toolbar);
	_layout.addWidget(&_view);
	this->setLayout(&_layout);
}

void KsTraceViewer::loadData(KsDataStore *data)
{
	_model.reset();
	_model.fill(data->_pevt, data->_rows, data->size());
	this->resizeToContents();
}

void KsTraceViewer::resizeToContents()
{
	_view.setVisible(false);
	_view.resizeColumnsToContents();
	_view.setVisible(true);
}

//void KsTraceViewer::pageChanged(int p)
//{
	//_searchDone = false;
//}

void KsTraceViewer::searchEditText(const QString &text)
{
	_searchDone = false; // The search has been modified.
}

void KsTraceViewer::searchEditColumn(int index)
{
	_searchDone = false; // The search has been modified.
}

void KsTraceViewer::graphFollowsChanged(int state)
{
	_graphFollows = (bool) state;
	if (_searchDone)
		emit select(*_it); // Send a signal to the Graph widget.
}

void KsTraceViewer::searchEditSelect(int index)
{
	_searchDone = false; // The search has been modified.
}

bool notHaveCond(QString searchText, QString itemText)
{
	return !itemText.contains(searchText, Qt::CaseInsensitive);
}

bool containsCond(QString searchText, QString itemText)
{
	return itemText.contains(searchText, Qt::CaseInsensitive);
}

bool matchCond(QString searchText, QString itemText)
{
	if (itemText.compare(searchText, Qt::CaseInsensitive) == 0 )
		return true;

	return false;
}

size_t KsTraceViewer::searchItems(int column,
				  const QString &searchText,
				  condition_func cond)
{
	int count = _model.search(column, searchText, cond, &_matchList);
	_searchDone = true;
	_view.clearSelection();
	_it = _matchList.begin();
	return count;
}

void KsTraceViewer::search()
{
	/** Disable the user input until the search is done. */
	_searchLineEdit.setReadOnly(true);
	if (!_searchDone) {
		/** The search is not done. This means that the search settings
		 *  have been modified since the last time we searched. */
		_matchList.clear();
		QString xText = _searchLineEdit.text();
		int xColumn = _columnComboBox.currentIndex();
		int xSelect = _selectComboBox.currentIndex();

		switch (xSelect) {
			case CONTAINS:
				searchItems(xColumn, xText, &containsCond);
				break;

			case MATCH:
				searchItems(xColumn, xText, &matchCond);
				break;

			case NOT_HAVE:
				searchItems(xColumn, xText, &notHaveCond);
				break;

			default:
				break;
		}
	}

	if (!_matchList.empty()) { // Items have been found.
		if (_it != _matchList.end() ) {
			// Select the row of the item and move the iterator.
			_view.selectRow(*_it);
			if (_graphFollows)
				emit select(*_it); // Send a signal to the Graph widget.
			_it++;
		} else {
			_view.selectRow(*_it);
			if (_graphFollows)
				emit select(*_it); // Send a signal to the Graph widget.

			// This is the last item. Go back to the beginning.
			_it = _matchList.begin();
		}
	}

	/** Enable the user input. */
	_searchLineEdit.setReadOnly(false);
}

void KsTraceViewer::clicked(const QModelIndex& i)
{
	if (_graphFollows)
		emit select(i.row()); // Send a signal to the Graph widget.
}

bool KsTraceViewer::event(QEvent *event)
{
    if (_graphFollows && event->type() == QEvent::KeyRelease) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Up || ke->key() == Qt::Key_Down) {
			QItemSelectionModel *sm = _view.selectionModel();
			if (sm->hasSelection()) {
				int row =  sm->selectedRows()[0].row();
				emit select(row); // Send a signal to the Graph widget.
			}
            return true;
        }
    }

    return QWidget::event(event);
}

void KsTraceViewer::showRow(int r, bool mark)
{
	if (mark) {
		int visiTot = _view.indexAt(_view.rect().topLeft()).row();
		int visiBottom = _view.indexAt(_view.rect().bottomLeft()).row() - 2;
		if (r < visiTot || r > visiBottom)
			_view.scrollTo(_model.index(r, 1), QAbstractItemView::PositionAtCenter);

		_view.selectRow(r);
	} else
		_view.scrollTo(_model.index(r, 1), QAbstractItemView::PositionAtTop);
}


