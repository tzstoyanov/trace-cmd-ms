/* SPDX-License-Identifier: LGPL-2.1 */

/*
 * Copyright (C) 2017 VMware Inc, Yordan Karadzhov <ykaradzhov@vmware.com>
 */

 /**
  *  @file    KsWidgetsLib.hpp
  *  @brief   Defines small widgets and dialogues used by the KernelShark GUI.
  */

#ifndef _KS_WIDGETS_LIB_H
#define _KS_WIDGETS_LIB_H

// Qt
#include <QtWidgets>

// KernelShark
#include "libkshark.h"
#include "KsUtils.hpp"

namespace KsWidgetsLib
{

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

/** Defines the progress bar's maximum value. */
#define KS_PROGRESS_BAR_MAX 200

/** The height of the KsProgressBar widget. */
#define KS_PROGBAR_HEIGHT (FONT_HEIGHT * 5)

/** The width of the KsProgressBar widget. */
#define KS_PROGBAR_WIDTH  (FONT_WIDTH * 50)

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

/** The height of the KsMessageDialog widget. */
#define KS_MSG_DIALOG_HEIGHT (FONT_HEIGHT * 8)

/** The width of the KsMessageDialog widget. */
#define KS_MSG_DIALOG_WIDTH  (SCREEN_WIDTH / 10)

bool fileExistsDialog(QString fileName);

}; // KsWidgetsLib

/**
 * The KsCheckBoxWidget class is the base class of all CheckBox widget used
 * by KernelShark.
 */
class KsCheckBoxWidget : public QWidget
{
	Q_OBJECT
public:
	KsCheckBoxWidget(int sd, const QString &name = "",
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

	void setVisibleCbAll(bool v) {_allCbAction->setVisible(v);}

	void setDefault(bool);

	void set(QVector<bool> v);

	QVector<int> getCheckedIds();

	/**
	 * Get the identifier of the Data stream for which the selection
	 * applies.
	 */
	int sd() const {return _sd;}

// 	void setStream(QString stream)
void setStream(uint8_t sd)
{
	kshark_context *kshark_ctx(nullptr);
	kshark_data_stream *stream;

	if (!kshark_instance(&kshark_ctx))
		return;

	stream = kshark_get_data_stream(kshark_ctx, sd);
	_streamName = QString(stream->file);
	_streamName += QString(" [%1]").arg(sd);

	KsUtils::setElidedText(&_stramLabel, _streamName,
			       Qt::ElideLeft, width());
	QApplication::processEvents();
}

	/**
	 * Reimplemented event handler used to update the geometry of the widget on
	 * resize events.
	 */
	void resizeEvent(QResizeEvent* event)
	{
		KsUtils::setElidedText(&_stramLabel, _streamName,
				       Qt::ElideLeft, width());
		QApplication::processEvents();
	}

private:
	QToolBar _tb;

protected:
	/** Identifier of the Data stream for which the selection applies. */
	int		_sd;

	/** The "all" checkboxe. */
	QCheckBox	_allCb;

	QAction		*_allCbAction;

	/** A vector of Id numbers coupled to each checkboxe. */
	QVector<int>	_id;

	/** A nested widget used to position the checkboxes. */
	QWidget		_cbWidget;

	/** The layout of the nested widget. */
	QVBoxLayout	_cbLayout;

	/** The top level layout of this widget. */
	QVBoxLayout	_topLayout;

	QString		_streamName;
	/**
	 * A label to show the name of the Data stream for which the selection
	 * applies. */
	QLabel		_stramLabel;

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

	KsCheckBoxDialog(QVector<KsCheckBoxWidget *> cbws, QWidget *parent = nullptr);

signals:
	/** Signal emitted when the "Apply" button is pressed. */
	void apply(int sd, QVector<int>);

private:
	void _applyPress();

	QVBoxLayout		_topLayout;

	QHBoxLayout		_cbLayout, _buttonLayout;

	QVector<KsCheckBoxWidget *>	_checkBoxWidgets;

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
	KsCheckBoxTableWidget(int sd, const QString &name = "",
			      QWidget *parent = nullptr);

	void setSingleSelection()
	{
		_table.setSelectionMode(QAbstractItemView::SingleSelection); // !!!!!!!!!!
		setVisibleCbAll(false);
	}

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

	KsCheckBoxTreeWidget(int sd, const QString &name = "",
			     QWidget *parent = nullptr);

	void setSingleSelection()
	{
		_tree.setSelectionMode(QAbstractItemView::SingleSelection);  // !!!!!!!!!!
		setVisibleCbAll(false);
	}

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

	void _update(QTreeWidgetItem *item, int column);

	void _verify();
};

class KsComboPlotDialog : public QDialog
{
	Q_OBJECT
public:
	explicit KsComboPlotDialog(QWidget *parent = nullptr);

signals:
	/** Signal emitted when the "Apply" button is pressed. */
	void apply(int sd, QVector<int>);

private:
	QVBoxLayout			_topLayout;

	QGridLayout			_streamMenuLayout;

	QHBoxLayout			_cbLayout, _buttonLayout;

	QLabel				_hostStreamLabel, _guestStreamLabel;

	QComboBox			_hostStreamComboBox;

	QComboBox			_guestStreamComboBox;

	KsCheckBoxTreeWidget		*_vcpuCheckBoxWidget;

	KsCheckBoxTableWidget		*_hostCheckBoxWidget;

	QPushButton			_applyButton, _cancelButton;

	QMetaObject::Connection		_applyButtonConnection;

	void _applyPress();

private slots:
	void _hostStreamChanged(const QString&);

	void _guestStreamChanged(const QString&);
};

/**
 * The KsCPUCheckBoxWidget class provides a widget for selecting CPU plots to
 * show.
 */
struct KsCPUCheckBoxWidget : public KsCheckBoxTreeWidget
{
	KsCPUCheckBoxWidget() = delete;

	KsCPUCheckBoxWidget(int sd, QWidget *parent = nullptr);
};

/**
 * The KsTasksCheckBoxWidget class provides a widget for selecting Tasks
 * to show or hide.
 */
struct KsTasksCheckBoxWidget : public KsCheckBoxTableWidget
{
	KsTasksCheckBoxWidget() = delete;

	KsTasksCheckBoxWidget(int sd, bool cond, QWidget *parent = nullptr);

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

	KsEventsCheckBoxWidget(int sd, QWidget *parent = nullptr);

	KsEventsCheckBoxWidget(tep_handle *pevent, QWidget *parent = nullptr);

	void removeSystem(QString name);

private:
	void _makeItems(tep_event **events, int nEvts);
};

/**
 * The KsPluginCheckBoxWidget class provides a widget for selecting plugins.
 */
struct KsPluginCheckBoxWidget : public KsCheckBoxTableWidget
{
	KsPluginCheckBoxWidget() = delete;

	KsPluginCheckBoxWidget(int sd, QStringList pluginList,
			       QWidget *parent = nullptr);
};

#endif
