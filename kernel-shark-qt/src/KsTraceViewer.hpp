/* SPDX-License-Identifier: LGPL-2.1 */

/*
 * Copyright (C) 2017 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
 */

#ifndef _KS_TRACEVIEW_H
#define _KS_TRACEVIEW_H

// Qt
#include <QTableView>

// KernelShark
#include "KsUtils.hpp"
#include "KsModels.hpp"
#include "KsDualMarker.hpp"

typedef bool (*condition_func)(QString, QString);

class KsTraceViewer : public QWidget
{
	Q_OBJECT
public:
	explicit KsTraceViewer(QWidget *parent = nullptr);

	void loadData(KsDataStore *data);
	void setMarkerSM(KsDualMarkerSM *m);
	void reset();
	size_t getTopRow() const;
	void setTopRow(size_t r);

signals:
	void select(size_t);
	void plotTask(int pid);

public slots:
	void searchEditColumn(int);
	void searchEditSelect(int);
	void searchEditText(const QString &);
	void graphFollowsChanged(int);
	void search();
	void next();
	void prev();
	void searchStop();
	void clicked(const QModelIndex& i);
	void showRow(size_t r, bool mark);
	void deselect();
	void markSwitch();
	void update(KsDataStore *data);
	void onCustomContextMenu(const QPoint &);

private:
	void searchReset();
	void resizeEvent(QResizeEvent* event) override;
	void resizeToContents();
	bool event(QEvent *event);

	size_t searchItems(int column, const QString &searchText,
			   condition_func cond);

	void searchItemsMapRed(int column, const QString &searchText,
			       condition_func cond);

	QVBoxLayout	_layout;
	QTableView	_view;
	KsViewModel		_model;
	KsFilterProxyModel	_proxyModel;

	QStringList	_tableHeader;
	QToolBar	_toolbar;
	QLabel		_labelSearch, _labelGrFollows;
	QComboBox	_columnComboBox;
	QComboBox	_selectComboBox;
	QLineEdit	_searchLineEdit;
	QPushButton	_prevButton, _nextButton, _searchStopButton;
	QAction		*_pbAction, *_searchStopAction;
	QCheckBox	_graphFollowsCheckBox;
	QProgressBar	_searchProgBar;
	QLabel		_searchCountLabel;

	bool		_searchDone;
	bool		_graphFollows;

	QList<int>		_matchList;
	QList<int>::iterator	_it;

	KsDualMarkerSM		*_mState;
	KsDataStore		*_data;

	enum Condition
	{
		Containes = 0,
		Match = 1,
		NotHave = 2
	};
};

#endif // _KS_TRACEVIEW_H
