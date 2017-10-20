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
#include "KsUtils.h"

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
  _graphFollowsCheckBox(this),
  _searchDone(false),
  _graphFollows(true)
{
	init();
	_view.setModel(&_model);
	connect(&_view, SIGNAL(clicked(const QModelIndex&)), this, SLOT(clicked(const QModelIndex&)));

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

	_searchLineEdit.setMaximumWidth(FONT_WIDTH*30);
	connect(&_searchLineEdit, SIGNAL(returnPressed()), this, SLOT(search()));
	connect(&_searchLineEdit, SIGNAL(textEdited(const QString &)), this, SLOT(searchEditText(const QString &)));
	_toolbar.addWidget(&_searchLineEdit);

	_toolbar.addSeparator();
	connect(&_graphFollowsCheckBox, SIGNAL(stateChanged(int)), this, SLOT(graphFollowsChanged(int)));
	_graphFollowsCheckBox.setCheckState(Qt::Checked);
	_toolbar.addWidget(&_graphFollowsCheckBox);
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
	setMinimumHeight(SCREEN_HEIGHT/4);

	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
	_view.horizontalHeader()->setStretchLastSection(true);
	_view.verticalHeader()->setVisible(false);
	//_view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	_view.setEditTriggers(QAbstractItemView::NoEditTriggers);
	_view.setSelectionBehavior(QAbstractItemView::SelectRows);
	_view.setSelectionMode(QAbstractItemView::SingleSelection);
// 	_view.setShowGrid(false);
	_view.setStyleSheet("QTableView {selection-background-color: green;}");
	_view.setGeometry(QApplication::desktop()->screenGeometry());

	_view.verticalHeader()->resizeSections(QHeaderView::ResizeToContents);
	_view.verticalHeader()->setDefaultSectionSize(FONT_HEIGHT*1.25);
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
	_searchDone = false;
}

void KsTraceViewer::searchEditColumn(int index)
{
	_searchDone = false;
}

void KsTraceViewer::graphFollowsChanged(int state) {
	_graphFollows = (bool) state;
	if (_searchDone)
		emit select(*_it);
}

void KsTraceViewer::searchEditSelect(int index) {
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

size_t KsTraceViewer::searchItem(	int column,
									const QString &searchText,
									condition_func cond) {
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
		_matchList.clear();
		QString xText = _searchLineEdit.text();
		int xColumn = _columnComboBox.currentIndex();
		int xSelect = _selectComboBox.currentIndex();
		switch (xSelect) {
			case CONTAINS:
				searchItem(xColumn, xText, &containsCond);
				break;

			case MATCH:
				searchItem(xColumn, xText, &matchCond);
				break;

			case NOT_HAVE:
				searchItem(xColumn, xText, &notHaveCond);
				break;

			default:
				break;
		}
	}

	if (!_matchList.empty()) {
		if (_it != _matchList.end() ){
			_view.selectRow(*_it);
			if (_graphFollows)
				emit select(*_it);
			_it++;
		} else {
			_view.selectRow(*_it);
			_it = _matchList.begin();
			if (_graphFollows)
				emit select(*_it);
		}
	}

	_searchLineEdit.setReadOnly(false);
}

void KsTraceViewer::clicked(const QModelIndex& i) {
	emit select(i.row());
}

bool KsTraceViewer::event(QEvent *event)
{
    if (event->type() == QEvent::KeyRelease) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Up || ke->key() == Qt::Key_Down) {
			QItemSelectionModel *s = _view.selectionModel();
			if (s->hasSelection()) {
				int row =  s->selectedRows()[0].row();
				emit select(row);
			}
            return true;
        }
    }

    return QWidget::event(event);
}

void KsTraceViewer::showRow(int r, bool mark) {
	if (mark) {
		int visiTot = _view.indexAt(_view.rect().topLeft()).row();
		int visiBottom = _view.indexAt(_view.rect().bottomLeft()).row() - 2;
		if (r < visiTot || r > visiBottom)
			_view.scrollTo(	_model.index(r, 1), QAbstractItemView::PositionAtCenter);

		_view.selectRow(r);
	} else
		_view.scrollTo(	_model.index(r, 1), QAbstractItemView::PositionAtTop);
}
