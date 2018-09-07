/* SPDX-License-Identifier: LGPL-2.1 */

/*
 * Copyright (C) 2016 Red Hat Inc, Steven Rostedt <srostedt@redhat.com>
 */

 /**
  *  @file    KsWidgetsLib.hpp
  *  @brief   Defines small widgets and dialogues used by the KernelShark GUI.
  */

#ifndef _KS_WIDGETS_LIB_H
#define _KS_WIDGETS_LIB_H

// Qt
#include <QtWidgets>

/**
 * The KsProgressBar class provides a visualization of the progress of a
 * running job.
 */
class KsProgressBar : public QWidget
{
	Q_OBJECT

	QStatusBar	_sb;
	QProgressBar	_pb;

public:
	KsProgressBar(QString message, QWidget *parent = nullptr);

	void setValue(int i);
};

/**
 * The KsMessageDialog class provides a widget showing a message and having
 * a "Close" button.
 */
class KsMessageDialog : public QDialog
{
	QVBoxLayout	_layout;
	QLabel		_text;
	QPushButton	_closeButton;

public:
	explicit KsMessageDialog(QWidget *parent) = delete;
	KsMessageDialog(QString message, QWidget *parent = nullptr);
};

/**
 * The KsCheckBoxWidget class is the base class of all CheckBox widget used
 * by KernelShark.
 */
class KsCheckBoxWidget : public QWidget
{
	Q_OBJECT
public:
	KsCheckBoxWidget(const QString &name = "",
			 QWidget *parent = nullptr);

	/** Get the name of the widget. */
	QString name() const {return _name;}

	/** Get the state of the "all" checkboxe. */
	bool all() const
	{
		if(_allCb.checkState() == Qt::Checked)
			return true;
		return false;
	}

	void setDefault(bool);

	void set(QVector<bool> v);

	QVector<int> getCheckedIds();

protected:
	/** The "all" checkboxe. */
	QCheckBox	_allCb;

	/** A vector of Id numbers coupled to each checkboxe. */
	QVector<int>	_id;

	/** A nested widget used to position the checkboxes. */
	QWidget		_cbWidget;

	/** The layout of the nested widget. */
	QVBoxLayout	_cbLayout;

	/** The top level layout of this widget. */
	QVBoxLayout	_topLayout;

	/** The name of this widget. */
	QString		_name;

	/** A label to show the name of the widget. */
	QLabel		_nameLabel;

private:
	virtual void _setCheckState(int i, Qt::CheckState st) = 0;

	virtual Qt::CheckState _checkState(int i) const = 0;

	virtual void _verify() {};

	void _checkAll(bool);
};

/**
 * The KsCheckBoxDialog class is the base class of all CheckBox dialogs
 * used by KernelShark.
 */
class KsCheckBoxDialog : public QDialog
{
	Q_OBJECT
public:
	KsCheckBoxDialog() = delete;

	KsCheckBoxDialog(KsCheckBoxWidget *cbw, QWidget *parent = nullptr);

signals:
	/** Signal emitted when the "Apply" button is pressed. */
	void apply(QVector<int>);

private:
	void _applyPress();

	QVBoxLayout		_topLayout;

	QHBoxLayout		_buttonLayout;

	KsCheckBoxWidget	*_checkBoxWidget;

	QPushButton		_applyButton, _cancelButton;

	QMetaObject::Connection		_applyButtonConnection;
};

/** The KsCheckBoxTable class provides a table of checkboxes. */
class KsCheckBoxTable : public QTableWidget
{
	Q_OBJECT
public:
	explicit KsCheckBoxTable(QWidget *parent = nullptr);

	void init(QStringList headers, int size);

	/** A vector of checkboxes. */
	QVector<QCheckBox*>	_cb;

signals:
	/** Signal emitted when a checkboxes changes state. */
	void changeState(int row);

protected:
	void keyPressEvent(QKeyEvent *event) override;

	void mousePressEvent(QMouseEvent *event) override;

private:
	void _doubleClicked(int row, int col);
};

/**
 * The KsCheckBoxTableWidget class provides a widget to hold a table of
 * checkboxes.
 */
class KsCheckBoxTableWidget : public KsCheckBoxWidget
{
	Q_OBJECT
public:
	KsCheckBoxTableWidget(const QString &name = "",
			      QWidget *parent = nullptr);

protected:
	void _adjustSize();

	void _initTable(QStringList headers, int size);

	/** The KsCheckBoxTable, shown by this widget. */
	KsCheckBoxTable		_table;

private:
	void _setCheckState(int i, Qt::CheckState st) override
	{
		_table._cb[i]->setCheckState(st);
	}

	Qt::CheckState _checkState(int i) const override
	{
		return _table._cb[i]->checkState();
	}

	void _update(bool);

	void _changeState(int row);
};

/** The KsCheckBoxTree class provides a tree of checkboxes. */
class KsCheckBoxTree : public QTreeWidget
{
	Q_OBJECT
public:
	explicit KsCheckBoxTree(QWidget *parent = nullptr);

signals:
	/**
	 * Signal emitted when a checkboxes of the tree changes its state
	 * and the state of all toplevel and child checkboxes has to be
	 * reprocesed.
	 */
	void verify();

protected:
	void keyPressEvent(QKeyEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;

private:
	void _doubleClicked(QTreeWidgetItem *item, int col);
};

/**
 * The KsCheckBoxTreeWidget class provides a widget to hold a tree of
 * checkboxes.
 */
class KsCheckBoxTreeWidget : public KsCheckBoxWidget
{
	Q_OBJECT
public:
	KsCheckBoxTreeWidget() = delete;
	KsCheckBoxTreeWidget(const QString &name = "",
			     QWidget *parent = nullptr);

protected:
	void _adjustSize();
	void _initTree();

	/** The KsCheckBoxTree, shown by this widget. */
	KsCheckBoxTree			_tree;

	/** A vector of Tree items (checkboxes). */
	QVector<QTreeWidgetItem*>	_cb;

private:
	void _setCheckState(int i, Qt::CheckState st) override
	{
		_cb[i]->setCheckState(0, st);
	}

	Qt::CheckState _checkState(int i) const override
	{
		return _cb[i]->checkState(0);
	}

// 	void _checkAll(bool) override;

	void _update(QTreeWidgetItem *item, int column);

	void _verify();
};

/**
 * The KsCpuCheckBoxWidget class provides a widget for selecting CPU plots to
 * show.
 */
struct KsCpuCheckBoxWidget : public KsCheckBoxTreeWidget
{
	KsCpuCheckBoxWidget() = delete;
	KsCpuCheckBoxWidget(struct tep_handle *pe,
			    QWidget *parent = nullptr);
};

/**
 * The KsTasksCheckBoxWidget class provides a widget for selecting Tasks
 * to show or hide.
 */
struct KsTasksCheckBoxWidget : public KsCheckBoxTableWidget
{
	KsTasksCheckBoxWidget() = delete;
	KsTasksCheckBoxWidget(struct tep_handle *pe,
			      bool cond = true,
			      QWidget *parent = nullptr);

private:
	/**
	 * A positive condition means that you want to show Tasks and
	 * a negative condition means that you want to hide Tasks.
	 */
	bool		_cond;
};

/**
 * The KsEventsCheckBoxWidget class provides a widget for selecting Trace
 * events to show or hide.
 */
struct KsEventsCheckBoxWidget : public KsCheckBoxTreeWidget
{
	KsEventsCheckBoxWidget() = delete;
	KsEventsCheckBoxWidget(struct tep_handle *pe,
			       QWidget *parent = nullptr);
};

/**
 * The KsPluginCheckBoxWidget class provides a widget for selecting plugins.
 */
struct KsPluginCheckBoxWidget : public KsCheckBoxTableWidget
{
	KsPluginCheckBoxWidget() = delete;
	KsPluginCheckBoxWidget(QStringList pluginList,
			       QWidget *parent = nullptr);
};

class KsDataStore;
class KsGLWidget;

/**
 * The KsQuickFilterMenu class provides a menu for easy filtering and plotting.
 * The menu is initialized from a single kshark_entry and uses the content of
 * this entry to provides quick actions for filtering and plottin.
 */
class KsQuickEntryMenu : public QMenu
{
	Q_OBJECT
public:
	KsQuickEntryMenu() = delete;
	explicit KsQuickEntryMenu(KsDataStore *data,
				  size_t row,
				  QWidget *parent = nullptr);

signals:
	/** Signal to add a task plot. */
	void plotTask(int);

private:
	void _hideTask();
	void _showTask();
	void _hideEvent();
	void _showEvent();
	void _addTaskPlot();

	KsDataStore	*_data;
	size_t		 _row;

	QAction _hideTaskAction, _showTaskAction;
	QAction _hideEventAction, _showEventAction;
	QAction _addTaskPlotAction;
};

#endif
