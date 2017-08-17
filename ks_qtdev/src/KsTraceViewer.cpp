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
#include <chrono>

// #include "event-parse.h"
// #include "trace-cmd.h"
#include "trace-view-store.h"
// #include "cpu.h"


#include "KsTraceViewer.h"

#define GET_TIME std::chrono::high_resolution_clock::now()

#define GET_DURATION(t0) std::chrono::duration_cast<std::chrono::duration<double>>( \
std::chrono::high_resolution_clock::now()-t0).count()

typedef std::chrono::high_resolution_clock::time_point  hd_time;

#define VIEWER_PAGE_SIZE 100000

KsTraceViewer::KsTraceViewer(QWidget *parent)
: QWidget(parent),
  _handle(nullptr),
  _toolbar(this),
  _table(this),
  _currentPage(nullptr),
  _label1("Page", this),
  _label2("Search: Column", this),
  _label3("Graph follows", this),
  _pageSpinBox(this),
  _columnComboBox(this),
  _selectComboBox(this),
  _searchLineEdit(this),
  _checkBox(this)
{
	_tableHeader << "#" << "CPU" << "Time Stamp" << "Task" << "PID" << "Latency" << "Event" << "Info";

// 	_currentPage = new QTableView(this);
	
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
	QTableWidget *p = addNewPage(1);
	p->setRowCount(1);
	_table.addWidget(p);

	_layout.addWidget(&_table);
	this->setLayout(&_layout);
}

KsTraceViewer::~KsTraceViewer() {}

void KsTraceViewer::pageChanged(int p)
{
	std::cout << "page: " << p << "\n";
	_currentPage = _pages[p-1];
	_table.setCurrentIndex(p-1);
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

size_t KsTraceViewer::select(int column, const QString &searchText, condition_func cond) {
	for (int r=0; r<_currentPage->rowCount(); ++r) {
		QTableWidgetItem *item = _currentPage->item(r, column);
		if ( cond( searchText, item->text() ) )
		{
// 			_it->setFlags(Qt::NoItemFlags);
			_matchList.append(item);
		}
	}

    std::cout << column << "  nh found: " << _matchList.count() << "\n";
	_searchDone = true;
	_it = _matchList.begin();
	return _matchList.count();
}

void KsTraceViewer::search() {
	_searchLineEdit.setReadOnly(true);
	if (!_searchDone) {
		std::cout << "<_| search: " << _searchLineEdit.text().toStdString() << "\n";
		_matchList.clear();
		QString xText = _searchLineEdit.text();
		QString notHave("");
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
			_currentPage->selectRow( (*_it)->row());
			_it++;
		} else {
			_it = _matchList.begin();
			_currentPage->selectRow( (*_it)->row());
		}
	}

	_searchLineEdit.setReadOnly(false);
}


QTableWidget* KsTraceViewer::addNewPage(int rows)
{
	QTableWidget *page = new QTableWidget(0, 8, this);
	page->setHorizontalHeaderLabels(_tableHeader);
	_pages.push_back(page);

// 	QStringListModel *page = new QStringListModel(_tableHeader);
// 	_currentPage.push_back(page);

	page->horizontalHeader()->setStretchLastSection(true);
	page->verticalHeader()->setVisible(false);
	page->setEditTriggers(QAbstractItemView::NoEditTriggers);
	page->setSelectionBehavior(QAbstractItemView::SelectRows);
	page->setSelectionMode(QAbstractItemView::SingleSelection);
// 	page->setShowGrid(false);
	page->setStyleSheet("QTableView {selection-background-color: orange;}");
	page->setGeometry(QApplication::desktop()->screenGeometry());

	page->verticalHeader()->resizeSections(QHeaderView::ResizeToContents);
// 	page->verticalHeader()->setDefaultSectionSize(20);
	
	page->setRowCount(rows);

	return page;
}


void KsTraceViewer::loadData(const QString& file)
{
	if(_handle)
		tracecmd_close(_handle);

	_pageSpinBox.setMinimum(1);
	_handle = tracecmd_open( file.toStdString().c_str() );

	/* The handle should have loaded the file by now.
	 * Try to turn off function trace indent and turn on show parent
	 * if possible.
	 */
	trace_util_add_option("ftrace:parent", "1");
	trace_util_add_option("ftrace:indent", "0");

	/* Also, show the function name in the tail for function graph */
	trace_util_add_option("fgraph:tailprint", "1");

	loadData(_handle);
// 	loadData_dev(_handle);
}


void KsTraceViewer::loadData(struct tracecmd_input *handle)
{
	int cpu;
	hd_time t0 = GET_TIME;

	for (auto const &t:  _pages)
		_table.removeWidget(t);

	_pages.resize(0);

	struct pevent_record *pevt_rec;
	struct pevent *pevt;
	int count, total=0;
	bool done=false;

	while(!done) {
		addNewPage(VIEWER_PAGE_SIZE);
		_currentPage = _pages.back();

		for (count = 0; count < VIEWER_PAGE_SIZE; ++count) {
			pevt_rec = tracecmd_read_next_data(handle, &cpu);
			if (!pevt_rec) {
				_currentPage->setRowCount(count);
				done = true;
				break;
			}

			pevt = tracecmd_get_pevent(handle);

			QTableWidgetItem *item = new QTableWidgetItem();
			item->setData(Qt::EditRole, total);
			_currentPage->setItem(count, 0, item);
			for (int column=1; column<8; ++column) {
				item = new QTableWidgetItem();
				getValue(handle, pevt, pevt_rec, column, item);
				_currentPage->setItem(count, column, item);
			}

			free_record(pevt_rec);
			++total;
		}


		_currentPage->setVisible(false);
// 		_currentPage->resizeRowsToContents();
		_currentPage->resizeColumnsToContents();
		_currentPage->setVisible(true);
	}

	size_t pages = _pages.size();
	_pageSpinBox.setMaximum(pages);
	_currentPage = _pages[0];

	for (size_t p=0; p<pages; ++p)
		_table.addWidget(_pages[p]);

	double time2 = GET_DURATION(t0);
	std::cout << "time: " << 1e3*time2 << " ms.\n";
}

void KsTraceViewer::loadData_dev(struct tracecmd_input *handle)
{
	for (auto const &t:  _pages)
		_table.removeWidget(t);

	_pages.resize(0);

	hd_time t0 = GET_TIME;

	TraceViewStore *store;
	store = trace_view_store_new(handle);
	int nRows = store->actual_rows;

	struct pevent_record *pevt_rec;
	struct pevent *pevt;

	unsigned int r, count=0;
	int cpu;
	unsigned int nPages = nRows/VIEWER_PAGE_SIZE + 1;
	unsigned int pageSize = VIEWER_PAGE_SIZE;
	for (unsigned int p=0; p < nPages; ++p) {
		if (p == nPages-1)
			pageSize = nRows%VIEWER_PAGE_SIZE;

// 		addNewPage(pageSize);
// 		_currentPage = _pages.back();
		
		for (r=0; r<pageSize; ++r) {
			count = p*VIEWER_PAGE_SIZE + r;
			pevt_rec = tracecmd_read_at(handle, store->rows[count]->offset, &cpu);
			if (pevt_rec) {
				pevt = tracecmd_get_pevent(handle);
// 				QTableWidgetItem *item = new QTableWidgetItem();
// 				item->setData(Qt::EditRole, count);
// 				_currentPage->setItem(r, 0, item);
// 					for (int column=1; column<8; ++column) {
// 					item = new QTableWidgetItem();
// 					getValue(handle, pevt, pevt_rec, column, item);
// 					_currentPage->setItem(r, column, item);
// 				}

				free_record(pevt_rec);
			}
		}
// 		_currentPage->setVisible(false);
// // 		_currentPage->resizeRowsToContents();
// 		_currentPage->resizeColumnsToContents();
// 		_currentPage->setVisible(true);
	}

	trace_view_store_finalize(store);

	double time2 = GET_DURATION(t0);
	std::cout << "time: " << 1e3*time2 << " ms.\n";

	size_t pages = _pages.size();
	_pageSpinBox.setMaximum(pages);
	_currentPage = _pages[0];

	for (size_t p=0; p<pages; ++p)
		_table.addWidget(_pages[p]);

}

void KsTraceViewer::getValue(	struct tracecmd_input	*handle,
								struct pevent			*pevent,
								struct pevent_record	*record,
								int						 column,
								QTableWidgetItem		*item) {

	struct trace_seq s;

	struct event_format *event;
	const char *comm;
	char time[32];
	uint64_t secs, usecs;
	int val;

	switch(column)
	{
	case TRACE_VIEW_STORE_COL_INDEX:
		item->setData(Qt::EditRole, -1);
		break;

	case TRACE_VIEW_STORE_COL_CPU:
		item->setData(Qt::EditRole, record->cpu);
		break;

	case TRACE_VIEW_STORE_COL_TS:
		usecs = record->ts;
		usecs /= 1000;
		secs = usecs / 1000000ULL;
		usecs -= secs * 1000000ULL;
		sprintf(time, "%llu.%06llu", (long long)secs, (long long)usecs);
		item->setText( QString(time) );
		break;

	case TRACE_VIEW_STORE_COL_COMM:
	case TRACE_VIEW_STORE_COL_PID:
	case TRACE_VIEW_STORE_COL_LAT:
	case TRACE_VIEW_STORE_COL_EVENT:
	case TRACE_VIEW_STORE_COL_INFO:

		switch (column) {
		case TRACE_VIEW_STORE_COL_COMM:
		case TRACE_VIEW_STORE_COL_PID:
			val = pevent_data_pid(pevent, record);
			if (column == TRACE_VIEW_STORE_COL_PID) {
				item->setData(Qt::EditRole, val);
			} else {
				comm = pevent_data_comm_from_pid(pevent, val);
				item->setText( QString(std::string(comm).c_str()) );
			}
			break;

		case TRACE_VIEW_STORE_COL_LAT:
			trace_seq_init(&s);
			pevent_data_lat_fmt(pevent, &s, record);
			item->setText( QString(s.buffer) );
			trace_seq_destroy(&s);
			break;

		case TRACE_VIEW_STORE_COL_EVENT:
		case TRACE_VIEW_STORE_COL_INFO:
			val = pevent_data_type(pevent, record);
			event = pevent_data_event_from_type(pevent, val);
			if (!event) {
				if (column == TRACE_VIEW_STORE_COL_EVENT)
					item->setText("[UNKNOWN EVENT]");
				break;
			}

			if (column == TRACE_VIEW_STORE_COL_EVENT) {
				item->setText( QString(event->name) );
				break;
			}

			trace_seq_init(&s);
			pevent_event_info(&s, event, record);
// 			cerr << val << "  " << s.buffer << endl;
			item->setText( QString(s.buffer) );
			trace_seq_destroy(&s);
			break;
		}
	}
}


