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

#ifndef _KS_TRACEVIEW_H
#define _KS_TRACEVIEW_H

// Qt
#include <QTableView>

// Kernel Shark 2
#include "KsUtils.hpp"
#include "KsModel.hpp"

typedef bool (*condition_func)(QString, QString);

class KsTraceViewer : public QWidget
{
	Q_OBJECT
public:
	explicit KsTraceViewer(QWidget *parent = nullptr);

	void loadData(KsDataStore *data);
	void setMarkerSM(KsDualMarkerSM *m);
	void reset();

signals:
	void select(size_t);

private slots:
	void searchEditColumn(int);
	void searchEditSelect(int);
	void searchEditText(const QString &);
	void graphFollowsChanged(int);
	void search();
	void next();
	void prev();
	void clicked(const QModelIndex& i);
	void showRow(size_t r, bool mark);
	void deselect();
	void markSwitch();
	void update(KsDataStore *data = nullptr);

private:
	void resizeEvent(QResizeEvent* event) override;
	void resizeToContents();
	bool event(QEvent *event);

	size_t searchItems(int column, const QString &text, condition_func cond);

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
	QPushButton	_prevButton, _nextButton;
	QCheckBox	_graphFollowsCheckBox;

	bool 		_searchDone;
	bool 		_graphFollows;

	QList<size_t> _matchList;
	QList<size_t>::iterator _it;

	KsDualMarkerSM *_mState;

	enum Condition
	{
		Containes = 0,
		Match = 1,
		NotHave = 2
	};
};

#endif // _KS_TRACEVIEW_H
