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

#ifndef _KS_WIDGETS_LIB_H
#define _KS_WIDGETS_LIB_H

// Qt
#include <QtWidgets>

class KsProgressBar : public QWidget
{
	Q_OBJECT

	QStatusBar	_sb;
	QProgressBar	_pb;

public:
	KsProgressBar(QString message, QWidget *parent = nullptr);

public slots:
	void setValue(int i);
};

class KsMessageDialog : public QDialog
{
	QVBoxLayout	_layout;
	QLabel		_text;
	QPushButton	_closeButton;

public:
	explicit KsMessageDialog(QWidget *parent) = delete;
	KsMessageDialog(QString message, QWidget *parent = nullptr);
};

class KsCheckBoxWidget : public QWidget
{
	Q_OBJECT
public:
	KsCheckBoxWidget(const QString &name = "",
			 bool cond = true,
			 QWidget *parent = nullptr);

	QString name() const {return _name;}
	bool all() const
	{
		if(_allCb.checkState() == Qt::Checked)
			return true;
		return false;
	}

	void setDefault(bool);

	virtual void set(QVector<bool> v) = 0;
	virtual QVector<int> getCheckedIds() = 0;

private slots:
	virtual void chechAll(bool) = 0;

protected:
	virtual void adjustSize() = 0;

	bool		_positiveCond;
	QCheckBox	_allCb;
	QVector<int>	_id;

	QVBoxLayout	_cbLayout;
	QWidget		*_cbWidget;
	QVBoxLayout	_topLayout;

	QString		_name;
	QLabel		_nameLabel;
};

class KsCheckBoxDialog : public QDialog
{
	Q_OBJECT

	QVBoxLayout		_topLayout;
	QHBoxLayout		_buttonLayout;
	KsCheckBoxWidget	*_checkBoxWidget;
	QPushButton		_applyButton, _cancelButton;

	QMetaObject::Connection		_applyButtonConnection;

public:
	KsCheckBoxDialog() = delete;
	KsCheckBoxDialog(KsCheckBoxWidget *cbw, QWidget *parent = nullptr);

signals:
	void apply(QVector<int>);

private slots:
	void applyPress();
};

class KsCheckBoxTable : public QTableWidget
{
	Q_OBJECT
public:
	explicit KsCheckBoxTable(QWidget *parent = nullptr);

	void init(QStringList headers, int size);
	QVector<QCheckBox*>	_cb;

protected:
	void keyPressEvent(QKeyEvent *event);
	void mousePressEvent(QMouseEvent *event);

signals:
	void changeState(int row);

private slots:
	void doubleClicked(int row, int col);
};

class KsCheckBoxTableWidget : public KsCheckBoxWidget
{
	Q_OBJECT
public:
	KsCheckBoxTableWidget(const QString &name = "",
			      bool cond = true,
			      QWidget *parent = nullptr);

	void set(QVector<bool> v) override;
	QVector<int> getCheckedIds() override;

private slots:
	void chechAll(bool) override;
	void update(bool);
	void changeState(int row);

protected slots:
	void adjustSize() override;

protected:
	void initTable(QStringList headers, int size);

	KsCheckBoxTable		_table;
};

class KsCheckBoxTree : public QTreeWidget
{
	Q_OBJECT
public:
	explicit KsCheckBoxTree(QWidget *parent = nullptr);

signals:
	void verify();

private slots:
	void doubleClicked(QTreeWidgetItem *item, int col);

protected:
	void keyPressEvent(QKeyEvent *event);
	void mousePressEvent(QMouseEvent *event);
};

class KsCheckBoxTreeWidget : public KsCheckBoxWidget
{
	Q_OBJECT
public:
	KsCheckBoxTreeWidget() = delete;
	KsCheckBoxTreeWidget(const QString &name = "",
			     bool cond = true,
			     QWidget *parent = nullptr);

	void set(QVector<bool> v) override;
	QVector<int> getCheckedIds() override;

private slots:
	void chechAll(bool) override;
	void update(QTreeWidgetItem *item, int column);
	void verify();

protected slots:
	void adjustSize() override;

protected:
	void initTree();

	KsCheckBoxTree			_tree;
	QVector<QTreeWidgetItem*>	_cb;
};

struct KsCpuCheckBoxWidget : public KsCheckBoxTreeWidget
{
	KsCpuCheckBoxWidget() = delete;
	KsCpuCheckBoxWidget(struct pevent *pe,
			    bool cond = true,
			    QWidget *parent = nullptr);
};

struct KsEventsCheckBoxWidget : public KsCheckBoxTreeWidget
{
	KsEventsCheckBoxWidget() = delete;
	KsEventsCheckBoxWidget(struct pevent *pe,
			       bool cond = true,
			       QWidget *parent = nullptr);
};

struct KsTasksCheckBoxWidget : public KsCheckBoxTableWidget
{
	KsTasksCheckBoxWidget() = delete;
	KsTasksCheckBoxWidget(struct pevent *pe,
			      bool cond = true,
			      QWidget *parent = nullptr);
};

struct KsPluginCheckBoxWidget : public KsCheckBoxTableWidget
{
	KsPluginCheckBoxWidget() = delete;
	KsPluginCheckBoxWidget(QStringList pluginList,
			       QWidget *parent = nullptr);
};

class KsDataStore;
class KsGLWidget;

class KsQuickFilterMenu : public QMenu
{
	Q_OBJECT

	KsDataStore	*_data;
	size_t		 _row;

	QAction _hideTaskAction, _showTaskAction, _hideEventAction, _showEventAction;
	QAction _addTaskPlotAction;

public:
	KsQuickFilterMenu() = delete;
	explicit KsQuickFilterMenu(KsDataStore *data,
				   size_t row,
				   QWidget *parent = nullptr);
private slots:
	void hideTask();
	void showTask();
	void hideEvent();
	void showEvent();
	void addTaskPlot();

signals:
	void plotTask(int);
};

#endif
