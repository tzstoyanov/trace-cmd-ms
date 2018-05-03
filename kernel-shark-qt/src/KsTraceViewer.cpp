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
#include "KsTraceViewer.hpp"
#include "KsWidgetsLib.hpp"

KsTraceViewer::KsTraceViewer(QWidget *parent)
: QWidget(parent),
  _view(this),
  _model(this),
  _proxyModel(this),
  _tableHeader(_model.header()),
  _toolbar(this),
  _labelSearch("Search: Column", this),
  _labelGrFollows("Graph follows", this),
  _columnComboBox(this),
  _selectComboBox(this),
  _searchLineEdit(this),
  _prevButton("Prev", this),
  _nextButton("Next", this),
  _graphFollowsCheckBox(this),
  _searchDone(false),
  _graphFollows(true),
  _mState(nullptr),
  _data(nullptr)
{
	this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

	/* Make a search toolbar. */
	_toolbar.setOrientation(Qt::Horizontal);
	_toolbar.setMaximumHeight(FONT_HEIGHT*1.75);

	/* On the toolbar make two Combo boxes for the search settings. */
	_toolbar.addWidget(&_labelSearch);
	_columnComboBox.addItems(_tableHeader);

	/*
	 * Using the old Signal-Slot syntax because QComboBox::currentIndexChanged
	 * has overloads.
	 */
	connect(&_columnComboBox,	SIGNAL(currentIndexChanged(int)),
		this,			SLOT(searchEditColumn(int)));

	_toolbar.addWidget(&_columnComboBox);

	_selectComboBox.addItem("contains");
	_selectComboBox.addItem("full match");
	_selectComboBox.addItem("does not have");

	/*
	 * Using the old Signal-Slot syntax because QComboBox::currentIndexChanged
	 * has overloads.
	 */
	connect(&_selectComboBox,	SIGNAL(currentIndexChanged(int)),
		this,			SLOT(searchEditSelect(int)));

	_toolbar.addWidget(&_selectComboBox);

	/* On the toolbar, make a Line edit field for search. */
	_searchLineEdit.setMaximumWidth(FONT_WIDTH*30);

	connect(&_searchLineEdit,	&QLineEdit::returnPressed,
		this,			&KsTraceViewer::search);

	connect(&_searchLineEdit,	&QLineEdit::textEdited,
		this,			&KsTraceViewer::searchEditText);

	_toolbar.addWidget(&_searchLineEdit);
	_toolbar.addSeparator();

	/* On the toolbar, add Prev & Next buttons. */
	int bWidth = FONT_WIDTH*6;

	_nextButton.setFixedWidth(bWidth);
	_toolbar.addWidget(&_nextButton);
	connect(&_nextButton,	&QPushButton::pressed,
		this,		&KsTraceViewer::next);

	_prevButton.setFixedWidth(bWidth);
	_toolbar.addWidget(&_prevButton);
	connect(&_prevButton,	&QPushButton::pressed,
		this,		&KsTraceViewer::prev);

	_toolbar.addSeparator();

	/*
	 * On the toolbar, make a Check box for connecting the search pannel
	 * to the Graph widget.
	 */
	_graphFollowsCheckBox.setCheckState(Qt::Checked);
	connect(&_graphFollowsCheckBox,	&QCheckBox::stateChanged,
		this,			&KsTraceViewer::graphFollowsChanged);

	_toolbar.addWidget(&_graphFollowsCheckBox);
	_toolbar.addWidget(&_labelGrFollows);

	/* Initialize the trace viewer. */
// 	_view.horizontalHeader()->setStretchLastSection(true);
	_view.horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
	_view.verticalHeader()->setVisible(false);
	_view.setEditTriggers(QAbstractItemView::NoEditTriggers);
	_view.setSelectionBehavior(QAbstractItemView::SelectRows);
	_view.setSelectionMode(QAbstractItemView::SingleSelection);
	_view.verticalHeader()->setDefaultSectionSize(FONT_HEIGHT*1.25);

	 _proxyModel.setSource(&_model);
	_view.setModel(&_proxyModel);

	_view.setContextMenuPolicy(Qt::CustomContextMenu);
	connect(&_view,	&QTableView::customContextMenuRequested,
		this,	&KsTraceViewer::onCustomContextMenu);

	connect(&_view,	&QTableView::clicked,
		this,	&KsTraceViewer::clicked);

	/* Set the layout. */
	_layout.addWidget(&_toolbar);
	_layout.addWidget(&_view);
	this->setLayout(&_layout);
}

void KsTraceViewer::loadData(KsDataStore *data)
{
	hd_time t0 = GET_TIME;

	_data = data;
	_model.reset();
	_proxyModel.fill(data->_rows);
	_model.fill(data->_pevt, data->_rows, data->size());
	this->resizeToContents();

	this->setMinimumHeight(SCREEN_HEIGHT/5);

	double time = GET_DURATION(t0);
	qInfo() <<"View loading time: " << 1e3*time << " ms.";
}

void KsTraceViewer::setMarkerSM(KsDualMarkerSM *m)
{
	_mState = m;
	_model.setColors(_mState->markerA().color(), _mState->markerB().color());

	/*
	 * Assign a property to State A of the Dual marker state machine.
	 * When the marker is in State A the background color of the selected
	 * row will be the same as the color of Marker A.
	 */
	QString styleSheetA = "selection-background-color : " +
			      _mState->markerA().color().name() + ";";

	_mState->stateAPtr()->assignProperty(&_view, "styleSheet", styleSheetA);

	/*
	 * Assign a property to State B. When the marker is in State B
	 * the background color of the selected row will be the same as
	 * the color of Marker B.
	 */
	QString styleSheetB = "selection-background-color : " +
			      _mState->markerB().color().name() + ";";

	_mState->stateBPtr()->assignProperty(&_view, "styleSheet", styleSheetB);
}

void KsTraceViewer::reset()
{
	this->setMinimumHeight(FONT_HEIGHT*10);
	_model.reset();
	resizeToContents();
}

size_t  KsTraceViewer::getTopRow() const
{
	return _view.indexAt(_view.rect().topLeft()).row();
}

void  KsTraceViewer::setTopRow(size_t r)
{
	_view.scrollTo(_proxyModel.index(r, 0),
		       QAbstractItemView::PositionAtTop);
}

void KsTraceViewer::update(KsDataStore *data)
{
	/* The Proxy model has to be updated first! */ 
	_proxyModel.update(data);
	_model.update(data);
	_data = data;
	if (_mState->activeMarker().isSet())
		showRow(_mState->activeMarker().row(), true);
}

void KsTraceViewer::onCustomContextMenu(const QPoint &point)
{
	QModelIndex i = _view.indexAt(point);
	if (i.isValid()) {
		qInfo() << "onCustomContextMenu " << i.row();
		/*
		 * Use the index of the proxy model to retrieve the value
		 * of the row number in the base model. This works because
		 * the row number is shown in column "0".
		 */
		size_t row = _proxyModel.data(_proxyModel.index(i.row(), 0)).toInt();
		KsQuickFilterMenu menu(_data, row, this);
		connect(&menu, &KsQuickFilterMenu::plotTask, this, &KsTraceViewer::plotTask);
		menu.exec(mapToGlobal(point));
	}
}

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

	if (_graphFollows && _searchDone)
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
	if (itemText.compare(searchText, Qt::CaseInsensitive) == 0)
		return true;

	return false;
}

void KsTraceViewer::search()
{
	/* Disable the user input until the search is done. */
	_searchLineEdit.setReadOnly(true);
	if (!_searchDone) {
		/*
		 * The search is not done. This means that the search settings
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
		/*
		 * If the search is done, pressing "Enter" is equivalent
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
		showRow(*_it, true);

		if (_graphFollows)
			emit select(*_it); // Send a signal to the Graph widget.
	}
}

void KsTraceViewer::prev()
{
	if (!_searchDone) {
		search();
		return;
	}

	if (!_matchList.empty()) { // Items have been found.
		if (_it == _matchList.begin()) {
			// This is the first item of the list. Go to the last item.
			_it = _matchList.end() - 1;
		} else {
			--_it; // Move the iterator.
		}

		// Select the row of the item.
		showRow(*_it, true);

		if (_graphFollows)
			emit select(*_it); // Send a signal to the Graph widget.
	}
}

void KsTraceViewer::clicked(const QModelIndex& i)
{
	if (_graphFollows) {
		/*
		 * Use the index of the proxy model to retrieve the value
		 * of the row number in the base model. This works because 
		 *  the row number is shown in column "0".
		 */
		size_t row = _proxyModel.data(_proxyModel.index(i.row(), 0)).toInt();
		emit select(row); // Send a signal to the Graph widget.
	}
}

void KsTraceViewer::showRow(size_t r, bool mark)
{
	/*
	 * Use the index in the source model to retrieve the value of the row number
	 * in the proxy model.
	 */
	QModelIndex index = _proxyModel.mapFromSource(_model.index(r, 0));
	if (mark) { // The row will be selected (colored).
		/* Get the first and the last visible rows of the table. */
		int visiTot = _view.indexAt(_view.rect().topLeft()).row();
		int visiBottom = _view.indexAt(_view.rect().bottomLeft()).row() - 2;

		/* Scroll only if the row to be shown in not vizible. */
		if (index.row() < visiTot || index.row() > visiBottom)
			_view.scrollTo(index, QAbstractItemView::PositionAtCenter);

		_view.selectRow(index.row());
	} else {
		/*
		 * Just make sure that the row is visible. It will show up at
		 * the top of the visible part of the table.
		 */
		_view.scrollTo(index, QAbstractItemView::PositionAtTop);
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
		/*
		 * The passive marker is set. Use the model to color the row of
		 * the passive marker.
		 */
		_model.selectRow(!state, _mState->getMarker(!state).row());
	}
	else {
		/*
		 * The passive marker is not set.
		 * Make sure that the model colors nothing.
		 */
		_model.selectRow(!state, -1);
	}

	/*
	 * Now deal with the active marker. This has to be done after dealing
	 *  with the model, because changing the model clears the selection.
	 */
	if (_mState->getMarker(state).isSet()) {
		/*
		 * The active marker is set. Use QTableView to select its row.
		 * The index in the source model is used to retrieve the value
		 * of the row number in the proxy model.
		 */
		size_t row =_mState->getMarker(state).row();
		QModelIndex index = _proxyModel.mapFromSource(_model.index(row, 0));

		/*
		 * The row of the active marker will be colored according to
		 * the assigned property of the current state of the Dual marker.
		 */
		_view.selectRow(index.row());
	}
}

void KsTraceViewer::resizeEvent(QResizeEvent* event)
{
	int nColumns = _tableHeader.count();
	int tableSize(0);
	for (int c = 0; c < nColumns; ++c) {
		tableSize += _view.columnWidth(c);
	}

	int viewSize = _view.width() - qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
	int freeSpace;
	if ((freeSpace = viewSize - tableSize) > 0) {
		_view.setColumnWidth(nColumns - 1, _view.columnWidth(nColumns - 1) +
						   freeSpace -
						   2); /* Just a little bit less space.
							* This will allow the scroll bar to
							* disappear when the widget is
							* extended to maximum. */
	}
}

void KsTraceViewer::resizeToContents()
{
	_view.setVisible(false);
	_view.resizeColumnsToContents();
	_view.setVisible(true);

	/*
	 * Because of some unknown reason the first column doesn't get
	 * resized properly by the code above. We will resize this
	 * column by hand.
	 */
	int rows = _model.rowCount({});
	int columnSize = STRING_WIDTH(QString("%1").arg(rows)) + FONT_WIDTH;
	_view.setColumnWidth(0, columnSize);
}

bool KsTraceViewer::event(QEvent *event)
{
	if (_graphFollows && event->type() == QEvent::KeyRelease) {
		// Get the Key event.
		QKeyEvent *ke = static_cast<QKeyEvent*>(event);
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
	int count = _proxyModel.search(column, searchText, cond, &_matchList);
	_searchDone = true;

	if (count == 0) // No items have been found. Do nothing.
		return 0;

	QItemSelectionModel *sm = _view.selectionModel();
	if (sm->hasSelection()) {
		/* Only one row at the time can be selected. */
		size_t row = sm->selectedRows()[0].row();
		_view.clearSelection();
		_it = _matchList.begin();
		/*
		 * Move the iterator to the first element of the match list after
		 * the selected one.
		 */
		while (*_it <= row) {
			++_it;  // Move the iterator.
			if (_it == _matchList.end() ) {
				/* This is the last item of the list. Go back to the beginning. */
				_it = _matchList.begin();
				break;
			}
		}
		
	} else {
		/* Move the iterator to the beginning of the match list. */
		_view.clearSelection();
		_it = _matchList.begin();
	}

	return count;
}
