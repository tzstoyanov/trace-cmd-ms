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

// Kernel Shark 2
#include "KsTraceViewer.h"

KsTraceViewer::KsTraceViewer(QWidget *parent)
: QWidget(parent),
  _view(this),
  _model(this),
  _tableHeader(_model.header()),
  _toolbar(this),
  //_labelPage("Page", this),
  _labelSearch("Search: Column", this),
  _labelGrFollows("Graph follows", this),
  //_pageSpinBox(this),
  _columnComboBox(this),
  _selectComboBox(this),
  _searchLineEdit(this),
  _prevButton("Prev", this),
  _nextButton("Next", this),
  _graphFollowsCheckBox(this),
  _searchDone(false),
  _graphFollows(true),
  _mState(nullptr)
{
	this->setMinimumHeight(SCREEN_HEIGHT/4);
	this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

	/* Make a search toolbar. */
	_toolbar.setOrientation(Qt::Horizontal);

	/* On the toolbar make a page Spin box. This is currently disabled. */
	//_toolbar.addWidget(&_labelPage);
	//_pageSpinBox.setMaximum(0);
	//connect(&_pageSpinBox, SIGNAL(valueChanged(int)),
		//this, SLOT(pageChanged(int)));

	//_toolbar.addWidget(&_pageSpinBox);
	//_toolbar.addSeparator();

	/* On the toolbar make two Combo boxes for the search settings. */
	_toolbar.addWidget(&_labelSearch);
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

	/* On the toolbar, make a Line edit field for search. */
	_searchLineEdit.setMaximumWidth(FONT_WIDTH*30);

	connect(&_searchLineEdit, SIGNAL(returnPressed()),
		this, SLOT(search()));

	connect(&_searchLineEdit, SIGNAL(textEdited(const QString &)),
		this, SLOT(searchEditText(const QString &)));

	_toolbar.addWidget(&_searchLineEdit);
	_toolbar.addSeparator();

	/* On the toolbar, add Prev & Next buttons */
	int bWidth = STRING_WIDTH("  Next  ");

	_nextButton.setFixedWidth(bWidth);
	_toolbar.addWidget(&_nextButton);
	connect(&_nextButton, SIGNAL(pressed()), this, SLOT(next()));

	_prevButton.setFixedWidth(bWidth);
	_toolbar.addWidget(&_prevButton);
	connect(&_prevButton, SIGNAL(pressed()), this, SLOT(prev()));

	_toolbar.addSeparator();

	/* On the toolbar, make a Check box for connecting the search pannel
	 * to the Graph widget.
	 */
	_graphFollowsCheckBox.setCheckState(Qt::Checked);	
	connect(&_graphFollowsCheckBox, SIGNAL(stateChanged(int)),
		this, SLOT(graphFollowsChanged(int)));

	_toolbar.addWidget(&_graphFollowsCheckBox);
	_toolbar.addWidget(&_labelGrFollows);

	/* Initialize the trace viewer. */
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

	/* Set the layout */
	_layout.addWidget(&_toolbar);
	_layout.addWidget(&_view);
	this->setLayout(&_layout);
}

void KsTraceViewer::loadData(KsDataStore *data)
{
	hd_time t0 = GET_TIME;

	_model.reset();
	_model.fill(data->_pevt, data->_rows, data->size());
	this->resizeToContents();

	double time = GET_DURATION(t0);
	qInfo() <<"View loading time: " << 1e3*time << " ms.";
}

void KsTraceViewer::setMarkerSM(KsDualMarkerSM *m)
{
	/* Assign a property to State A of the Dual marker state machine.
	 * When the marker is in State A the background color of the selected
	 * row will be darkGreem. Actualy color is set to be "green" but
	 * it shows up as "darkGreem". This looks like a Qt bug.
	 */
	_mState = m;
	_mState->stateAPtr()->assignProperty(&_view,
					     "styleSheet",
					     "selection-background-color : green;");

	/* Assign a property to State B. When the marker is in State B
	 * the background color of the selected row will be "darkCyan".
	 */
	_mState->stateBPtr()->assignProperty(&_view,
					     "styleSheet",
					     "selection-background-color : darkCyan;");
}

void KsTraceViewer::reset()
{
	_model.reset();
}

//void KsTraceViewer::pageChanged(int p)
//{
	//_searchDone = false;
//}

void KsTraceViewer::searchEditColumn(int index)
{
	_searchDone = false; // The search has been modified.
}

void KsTraceViewer::searchEditSelect(int index)
{
	_searchDone = false; // The search has been modified.
}

void KsTraceViewer::searchEditText(const QString &text)
{
	_searchDone = false; // The search has been modified.
}

void KsTraceViewer::graphFollowsChanged(int state)
{
	_graphFollows = (bool) state;
	if (_searchDone)
		emit select(*_it); // Send a signal to the Graph widget.
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

void KsTraceViewer::search()
{
	/* Disable the user input until the search is done. */
	_searchLineEdit.setReadOnly(true);
	if (!_searchDone) {
		/* The search is not done. This means that the search settings
		 * have been modified since the last time we searched.
		 */
		_matchList.clear();
		QString xText = _searchLineEdit.text();
		int xColumn = _columnComboBox.currentIndex();
		int xSelect = _selectComboBox.currentIndex();

		switch (xSelect) {
			case Condition::Containes:
				searchItems(xColumn, xText, &containsCond);
				break;

			case Condition::Match:
				searchItems(xColumn, xText, &matchCond);
				break;

			case Condition::NotHave:
				searchItems(xColumn, xText, &notHaveCond);
				break;

			default:
				break;
		}

		if (!_matchList.empty()) {
			this->showRow(*_it, true);

			if (_graphFollows)
				emit select(*_it); // Send a signal to the Graph widget.
		}
	} else {
		/* If the search is done, pressing "Enter" is equivalent
		 * to pressing "Next" button.
		 */
		this->next();
	}

	/* Enable the user input. */
	_searchLineEdit.setReadOnly(false);
}

void KsTraceViewer::next()
{
	if (!_searchDone) {
		search();
		return;
	}

	if (!_matchList.empty()) { // Items have been found.
		++_it; // Move the iterator.
		if (_it == _matchList.end() ) {
			// This is the last item of the list. Go back to the beginning.
			_it = _matchList.begin();
		}

		// Select the row of the item.
		this->showRow(*_it, true);

		if (_graphFollows)
			emit select(*_it); // Send a signal to the Graph widget.
	}
}

void KsTraceViewer::prev()
{
	if (!_matchList.empty()) { // Items have been found.
		if (_it == _matchList.begin()) {
			// This is the first item of the list. Go to the last item.
			_it = _matchList.end() - 1;
		} else {
			--_it; // Move the iterator.
		}
		
		// Select the row of the item.
		this->showRow(*_it, true);

		if (_graphFollows)
			emit select(*_it); // Send a signal to the Graph widget.
	}
}

void KsTraceViewer::clicked(const QModelIndex& i)
{
	if (_graphFollows)
		emit select((size_t) i.row()); // Send a signal to the Graph widget.
}

void KsTraceViewer::showRow(int r, bool mark)
{
	if (mark) { // The row will be selected (colored).
		// Get the first and the last visible row of the table.
		int visiTot = _view.indexAt(_view.rect().topLeft()).row();
		int visiBottom = _view.indexAt(_view.rect().bottomLeft()).row() - 2;
		// Scroll, if the row to be shown in not vizible.
		if (r < visiTot || r > visiBottom)
			_view.scrollTo(_model.index(r, 1),
				       QAbstractItemView::PositionAtCenter);

		_view.selectRow(r);
	} else {
		/* Just make sure that the row is visible.
		 * It will show up at the top of the visible part of the table.
		 */
		_view.scrollTo(_model.index(r, 1), QAbstractItemView::PositionAtTop);
	}
}

void KsTraceViewer::deselect()
{
	_view.clearSelection();
}

void KsTraceViewer::markSwitch()
{
	/* The state of the Dual marker has changed. Get the new active marker. */
	DualMarkerState state = _mState->getState();

	/* First deal with the passive marker. */
	if (_mState->getMarker(!state).isSet()) {
		/* The passive marker is set. 
		 * Use the model to color the row of the passive marker.
		 */
		_model.selectRow(!state, _mState->getMarker(!state).row());
	}
	else {
		/* The passive marker is not set. 
		 * Make sure that the model colors nothing.
		 */
		_model.selectRow(!state, -1);
	}

	/* Now deal with the active marker. This has to be done after dealing
	 *  with the model, because changing the model clears the selection. */
	if (_mState->getMarker(state).isSet()) {
		/* The active marker is set. Use QTableView to select its row.
		 * The row of the active marker will be colored according to
		 * the assigned property of the current state of the Dual marker.
		 */
		_view.selectRow(_mState->getMarker(state).row());
	}
}

void KsTraceViewer::resizeToContents()
{
	_view.setVisible(false);
	_view.resizeColumnsToContents();
	_view.setVisible(true);
}

bool KsTraceViewer::event(QEvent *event)
{
	if (_graphFollows && event->type() == QEvent::KeyRelease) {
		// Get the Key event.
		QKeyEvent *ke = static_cast<QKeyEvent *>(event);
		if (ke->key() == Qt::Key_Up || ke->key() == Qt::Key_Down) {
			QItemSelectionModel *sm = _view.selectionModel();
			if (sm->hasSelection()) {
				/* Only one row at the time can be selected. */
				int row = sm->selectedRows()[0].row();
				emit select(row); // Send a signal to the Graph widget.
			}

			return true;
		}
	}

	return QWidget::event(event);
}

size_t KsTraceViewer::searchItems(int column,
				  const QString &searchText,
				  condition_func cond)
{
	int count = _model.search(column, searchText, cond, &_matchList);
	_searchDone = true;

	// Move the iterator to the the beginningof the match list.
	_view.clearSelection();
	_it = _matchList.begin();
	return count;
}
