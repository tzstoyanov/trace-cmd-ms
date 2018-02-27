/*
 *  Copyright (C) 2017 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
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
#include "KsTraceGraph.hpp"
#include "KsUtils.hpp"

KsTraceGraph::KsTraceGraph(QWidget *parent)
: QWidget(parent),
  _pointerBar(this),
  _navigationBar(this),
  _scrollLeftButton("<", this),
  _zoomInButton("+", this),
  _zoomOutButton("-", this),
  _scrollRightButton(">", this),
  _labelP1("Pointer: ", this),
  _labelP2("", this),
  _labelI1("", this),
  _labelI2("", this),
  _labelI3("", this),
  _labelI4("", this),
  _labelI5("", this),
  _scrollArea(this),
  _drawWindow(&_scrollArea),
  _legendWindow(&_drawWindow),
  _legendAxisX(&_drawWindow),
  _labelXMin("", &_legendAxisX),
  _labelXMid("", &_legendAxisX),
  _labelXMax("", &_legendAxisX),
  _glWindow(&_drawWindow),
  _mState(nullptr),
  _data(nullptr),
  _keyPressed(false)
{
	auto makeNavButton = [&](QPushButton *b) {
		b->setMaximumWidth(FONT_WIDTH*5);

		connect(b, SIGNAL(released()), this, SLOT(stopUpdating()));
		_navigationBar.addWidget(b);
	};

	_pointerBar.setMaximumHeight(FONT_HEIGHT*1.75);
	_pointerBar.setOrientation(Qt::Horizontal);

	_navigationBar.setMaximumHeight(FONT_HEIGHT*1.75);
	_navigationBar.setMinimumWidth(FONT_WIDTH*110);
	_navigationBar.setOrientation(Qt::Horizontal);

	_pointerBar.addWidget(&_labelP1);
	_labelP2.setFrameStyle(QFrame::Panel | QFrame::Sunken);
	_labelP2.setStyleSheet("QLabel { background-color : white;}");
	_labelP2.setTextInteractionFlags(Qt::TextSelectableByMouse);
	_labelP2.setFixedWidth(FONT_WIDTH*16);
	_pointerBar.addWidget(&_labelP2);
	_pointerBar.addSeparator();

	_labelI1.setStyleSheet("QLabel {color : blue;}");
	_labelI2.setStyleSheet("QLabel {color : green;}");
	_labelI3.setStyleSheet("QLabel {color : red;}");
	_labelI4.setStyleSheet("QLabel {color : blue;}");
	_labelI5.setStyleSheet("QLabel {color : green;}");

	_pointerBar.addWidget(&_labelI1);
	_pointerBar.addSeparator();
	_pointerBar.addWidget(&_labelI2);
	_pointerBar.addSeparator();
	_pointerBar.addWidget(&_labelI3);
	_pointerBar.addSeparator();
	_pointerBar.addWidget(&_labelI4);
	_pointerBar.addSeparator();
	_pointerBar.addWidget(&_labelI5);

	_legendAxisX.setFixedHeight(FONT_HEIGHT*1.5);
	_legendAxisX.setLayout(new QHBoxLayout);
	_legendAxisX.layout()->setSpacing(0);
	_legendAxisX.layout()->setContentsMargins(0, 0, FONT_WIDTH, 0);

	_labelXMin.setAlignment(Qt::AlignLeft);
	_labelXMid.setAlignment(Qt::AlignHCenter);
	_labelXMax.setAlignment(Qt::AlignRight);

	_legendAxisX.layout()->addWidget(&_labelXMin);
	_legendAxisX.layout()->addWidget(&_labelXMid);
	_legendAxisX.layout()->addWidget(&_labelXMax);
	_drawWindow.setMinimumSize(100, 100);
	_drawWindow.setStyleSheet("QWidget {background-color : white;}");
	QGridLayout *drawLayout = new QGridLayout;
	_drawWindow.setLayout(drawLayout);
	_drawWindow.installEventFilter(this);

	drawLayout->setContentsMargins(0, 0, 0, 0);
	drawLayout->setSpacing(0);
	drawLayout->addWidget(&_legendAxisX, 0, 1);
	drawLayout->addWidget(&_legendWindow, 1, 0);
	drawLayout->addWidget(&_glWindow, 1, 1);

	connect(&_glWindow, SIGNAL(select(size_t)), this, SLOT(markEntry(size_t)));
	connect(&_glWindow, SIGNAL(found(size_t)), this, SLOT(setPointerInfo(size_t)));
	connect(&_glWindow, SIGNAL(notFound(uint64_t, int, int)),
		this, SLOT(resetPointer(uint64_t,int, int)));
	connect(&_glWindow, SIGNAL(zoomIn()), this, SLOT(zoomIn()));
	connect(&_glWindow, SIGNAL(zoomOut()), this, SLOT(zoomOut()));
	connect(&_glWindow, SIGNAL(scrollLeft()), this, SLOT(scrollLeft()));
	connect(&_glWindow, SIGNAL(scrollRight()), this, SLOT(scrollRight()));
	connect(&_glWindow, SIGNAL(stopUpdating()), this, SLOT(stopUpdating()));
	connect(_glWindow.model(), SIGNAL(modelReset()), this, SLOT(updateTimeLegends()));

	_scrollArea.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	_scrollArea.setWidget(&_drawWindow);

	connect(&_scrollLeftButton, SIGNAL(pressed()), this, SLOT(scrollLeft()));
	makeNavButton(&_scrollLeftButton);

	connect(&_zoomInButton, SIGNAL(pressed()), this, SLOT(zoomIn()));
	makeNavButton(&_zoomInButton);

	connect(&_zoomOutButton, SIGNAL(pressed()), this, SLOT(zoomOut()));
	makeNavButton(&_zoomOutButton);

	connect(&_scrollRightButton, SIGNAL(pressed()), this, SLOT(scrollRight()));
	makeNavButton(&_scrollRightButton);

	_layout.addWidget(&_pointerBar);
	_layout.addWidget(&_navigationBar);
	_layout.addWidget(&_scrollArea);
	this->setLayout(&_layout);
	updateGeom();
}

void KsTraceGraph::loadData(KsDataStore *data)
{
	_data = data;
	_glWindow.loadData(data);
	updateGraphLegends();
	updateGeom();
}

void KsTraceGraph::setMarkerSM(KsDualMarkerSM *m)
{
	_mState = m;
	_navigationBar.addSeparator();
	_mState->placeInToolBar(&_navigationBar);
	_glWindow.setMarkerSM(m);
}

void KsTraceGraph::reset()
{
	/* Clear the all graph lists and update. */
	_glWindow._cpuList = {};
	_glWindow._taskList = {};
	_labelP2.setText("");
	for (auto l1: {&_labelI1, &_labelI2, &_labelI3, &_labelI4, &_labelI5})
		l1->setText("");

	_glWindow.model()->reset();
	update();
	for (auto l2: {&_labelXMin, &_labelXMid, &_labelXMax})
		l2->setText("");
}

void KsTraceGraph::zoomIn()
{
	updateGraphs(GraphActions::ZoomIn);
}

void KsTraceGraph::zoomOut()
{
	updateGraphs(GraphActions::ZoomOut);
}

void KsTraceGraph::scrollLeft()
{
	updateGraphs(GraphActions::ScrollLeft);
}

void KsTraceGraph::scrollRight()
{
	updateGraphs(GraphActions::ScrollRight);
}

void KsTraceGraph::stopUpdating()
{
	/* The user is no longer pressing the action button. Reset the "Key Pressed"
	 * flag. This will stop the ongoing user action. */
	_keyPressed = false;
}

void KsTraceGraph::resetPointer(uint64_t ts, int cpu, int pid)
{
	uint64_t sec, usec;
	kshark_convert_nano(ts, &sec, &usec);
	QString pointer;
	pointer.sprintf("%llu.%llu", (unsigned long long)sec, (unsigned long long)usec);
	_labelP2.setText(pointer);

	if (pid >= 0) {
		struct kshark_context *kshark_ctx = NULL;
		kshark_instance(&kshark_ctx);
		QString comm(kshark_get_comm_from_pid(kshark_ctx->pevt, pid));
		comm.append("-");
		comm.append(QString("%1").arg(pid));
		_labelI1.setText(comm);
	} else {
		_labelI1.setText("");
	}

	if (cpu >= 0)
		_labelI2.setText(QString("Cpu %1").arg(cpu));
	else
		_labelI2.setText("");

	for (auto const &l: {&_labelI3, &_labelI4, &_labelI5}) {
		l->setText("");
	}
}

void KsTraceGraph::setPointerInfo(size_t i)
{
	kshark_entry *e = _data->_rows[i];

	uint64_t sec, usec;
	kshark_convert_nano(e->ts, &sec, &usec);
	QString pointer;
	pointer.sprintf("%llu.%llu", (unsigned long long)sec, (unsigned long long)usec);
	_labelP2.setText(pointer);

	QString comm(kshark_get_task_lazy(e));
	comm.append("-");
	comm.append(QString("%1").arg(kshark_get_pid_lazy(e)));
	_labelI1.setText(comm);

	_labelI2.setText(QString("Cpu %1").arg(e->cpu));

	QString lat(kshark_get_latency_lazy(e));
	_labelI3.setText(lat);

	QString event(kshark_get_event_name_lazy(e));
	_labelI4.setText(event);

	QString info(kshark_get_info_lazy(e));

	_labelI5.setText(info);
	QCoreApplication::processEvents();
	
	int labelWidth = _pointerBar.geometry().right() -
			 _labelI4.geometry().right();

	if (labelWidth > STRING_WIDTH(info) + FONT_WIDTH*5)
		return;

	/* The Info string is too long and cannot be displayed on the toolbar.
	 * Try to fit the text in the available space. */
	QFontMetrics metrix(_labelI5.font());
	int width = labelWidth - FONT_WIDTH*3;
	QString elidedText = metrix.elidedText(info, Qt::ElideRight, width);

	while(labelWidth < STRING_WIDTH(elidedText) + FONT_WIDTH*5) {
		width -= FONT_WIDTH*3;
		elidedText = metrix.elidedText(info, Qt::ElideRight, width);
	}

	_labelI5.setText(elidedText);
	_labelI5.setVisible(true);
	QCoreApplication::processEvents();
}

void KsTraceGraph::markEntry(size_t row)
{
	int graph, cpuGrId, taskGrId;
	_glWindow.findGraphIds(*_data->_rows[row], &cpuGrId, &taskGrId);

	/* If a Task graph has been found, this Task graph will be
	 * visible. If no Task graph has been found, make visible
	 * the corresponding Cpu graph. */
	if (taskGrId >= 0)
		graph = taskGrId;
	else
		graph = cpuGrId;

	_scrollArea.ensureVisible(0,
				  _legendAxisX.height() +
				  _glWindow._vMargin +
				  CPU_GRAPH_HEIGHT/2 +
				  graph*(CPU_GRAPH_HEIGHT + _glWindow._vSpacing),
				  50,
				  CPU_GRAPH_HEIGHT/2 + _glWindow._vSpacing/2);

	_glWindow.model()->shiftTo(_data->_rows[row]->ts);
	_mState->activeMarker().set(*_data,
				    *_glWindow.model()->histo(),
				    row, cpuGrId, taskGrId);

	_mState->updateLabels(*_glWindow.model()->histo());
	_mState->updateMarkers(*_data, *_glWindow.model()->histo());
}

void KsTraceGraph::markerReDraw()
{
	int cpuGrId, taskGrId;
	size_t row;

	if (_mState->markerA().isSet()) {
		row = _mState->markerA().row();
		_glWindow.findGraphIds(*_data->_rows[row], &cpuGrId, &taskGrId);
		_mState->markerA().set(*_data,
				       *_glWindow.model()->histo(),
				       row, cpuGrId, taskGrId);
	}

	if (_mState->markerB().isSet()) {
		row = _mState->markerB().row();
		_glWindow.findGraphIds(*_data->_rows[row], &cpuGrId, &taskGrId);
		_mState->markerB().set(*_data,
				       *_glWindow.model()->histo(),
				       row, cpuGrId, taskGrId);
	}
}

void KsTraceGraph::cpuReDraw(QVector<int> v)
{
	_glWindow._cpuList = v;
	markerReDraw();
	update();
}

void KsTraceGraph::taskReDraw(QVector<int> v)
{
	_glWindow._taskList = v;
	markerReDraw();
	update();
}

void KsTraceGraph::update(KsDataStore *data)
{
	_glWindow.model()->update(data);
	updateGraphLegends();
	updateTimeLegends();
	updateGeom();
}

void KsTraceGraph::updateGeom()
{
	/* Set the size of the Scroll Area. */
	int saWidth = width() - _layout.contentsMargins().left() -
				_layout.contentsMargins().right();

	int saHeight = height() - _pointerBar.height() -
				  _navigationBar.height() -
				  _layout.spacing()*2 -
				  _layout.contentsMargins().top() -
				  _layout.contentsMargins().bottom();
		
	_scrollArea.resize(saWidth, saHeight);

	/* Calculate the width of the Draw Window, taking into account the size
	 * of the scroll bar. */
	int dwWidth = _scrollArea.width();
	if (_glWindow.height() + _legendAxisX.height() > _scrollArea.height())
		dwWidth -= qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);

	/* Set the height of the Draw window according to the number of
	 * plotted graphs. */
	_drawWindow.resize(dwWidth, _glWindow.height() + _legendAxisX.height());

	/* Set the minimum height of the Graph widget. */
	int hMin = _drawWindow.height() +
		   _pointerBar.height() +
		   _navigationBar.height() +
		   _layout.contentsMargins().top() +
		   _layout.contentsMargins().bottom();

	if (hMin > CPU_GRAPH_HEIGHT*8)
		hMin = CPU_GRAPH_HEIGHT*8;

	setMinimumHeight(hMin);

	/* Now use the height of the Draw Window to fix the maximum height
	 * of the Graph widget. */
	setMaximumHeight(_drawWindow.height() +
			 _pointerBar.height() +
			 _navigationBar.height() +
			 _layout.spacing()*2 +
			 _layout.contentsMargins().top() +
			 _layout.contentsMargins().bottom() +
			 2);	/* Just a little bit of extra space. This will allow
				 * the scroll bar to disappear when the widget is
				 * extended to maximum. */
}

void KsTraceGraph::updateGraphLegends()
{
	if (_legendWindow.layout()) {
		QLayoutItem *child;
		while ((child = _legendWindow.layout()->takeAt(0)) != 0) {
			delete child->widget();
			delete child;
		}

		delete _legendWindow.layout();
	}

	QString graphLegends;
	QString graphName;
	int width = 0;

	QVBoxLayout *layout = new QVBoxLayout;
	layout->setContentsMargins(FONT_WIDTH, 0, 0, 0);
	layout->setSpacing(_glWindow._vSpacing);
	layout->setAlignment(Qt::AlignTop);
	layout->addSpacing(_glWindow._vMargin);

	auto make_name = [&] {
		if (width < STRING_WIDTH(graphName))
			width = STRING_WIDTH(graphName);

		QLabel *name = new QLabel(graphName);
		name->setAlignment(Qt::AlignBottom);
		name->setStyleSheet("QLabel {background-color : white;}");
		name->setFixedHeight(CPU_GRAPH_HEIGHT);
		layout->addWidget(name);
	};
	
	for (auto const &cpu: _glWindow._cpuList) {
		graphName = QString("CPU %1").arg(cpu);
		make_name();
	}

	for (auto const &pid: _glWindow._taskList) {
		graphName = QString(kshark_get_comm_from_pid(_data->_pevt, pid));
		graphName.append(QString("-%1").arg(pid));
		make_name();
	}

	_legendWindow.setLayout(layout);
	_legendWindow.setMaximumWidth(width + FONT_WIDTH);
}

void KsTraceGraph::updateTimeLegends()
{
	uint64_t sec, usec;
	kshark_convert_nano(_glWindow.model()->histo()->_min, &sec, &usec);
	QString tMin;
	tMin.sprintf("%llu.%llu", (unsigned long long)sec, (unsigned long long)usec);
	_labelXMin.setText(tMin);

	uint64_t tsMid = (_glWindow.model()->histo()->_min + _glWindow.model()->histo()->_max)/2;
	kshark_convert_nano(tsMid, &sec, &usec);
	QString tMid;
	tMid.sprintf("%llu.%llu", (unsigned long long)sec, (unsigned long long)usec);
	_labelXMid.setText(tMid);

	kshark_convert_nano(_glWindow.model()->histo()->_max, &sec, &usec);
	QString tMax;
	tMax.sprintf("%llu.%llu", (unsigned long long)sec, (unsigned long long)usec);
	_labelXMax.setText(tMax);
}

void KsTraceGraph::resizeEvent(QResizeEvent* event)
{
	updateGeom();
}


bool KsTraceGraph::eventFilter(QObject* obj, QEvent* evt)
{
	/* Overriding a virtual function from QObject.
	 * This function is used to detect the position of the mouse
	 * with respect to the Draw window and according to this position
	 * to grab / release the focus of the keyboard. The function has
	 * nothing to do with the filtering of the trace events. */
	if (obj == &_drawWindow && evt->type() == QEvent::Enter)
		_glWindow.setFocus();

	if (obj == &_drawWindow && evt->type() == QEvent::Leave)
		_glWindow.clearFocus();

	return QWidget::eventFilter(obj, evt);
}

void KsTraceGraph::updateGraphs(GraphActions action)
{
	/* Set the "Key Pressed" flag. The flag will stay set as long as the user
	 * keeps the corresponding action button pressed. */
	_keyPressed = true;

	/* Initialize the zooming factor with a small value. */
	double k = .01;
	while (_keyPressed) {
		switch (action) {
		case GraphActions::ZoomIn:
			if (_mState->activeMarker().isSet() &&
			    _mState->activeMarker().isVisible()) {
				/* Use the position of the active marker as a focus point
				 * of the zoom. */
				_glWindow.model()->zoomIn(k, _mState->activeMarker().bin());
			} else {
				/* The default focus point is the center of the range
				 * interval of the model. */
				_glWindow.model()->zoomIn(k);
			}

			break;

		case GraphActions::ZoomOut:
			if (_mState->activeMarker().isSet() &&
			    _mState->activeMarker().isVisible()) {
				/* Use the position of the active marker as a focus point
				 * of the zoom. */
				_glWindow.model()->zoomOut(k, _mState->activeMarker().bin());
			} else {
				/* The default focus point is the center of the range
				 * interval of the model. */
				_glWindow.model()->zoomOut(k);
			}

			break;

		case GraphActions::ScrollLeft:
			_glWindow.model()->shiftBackward(10);
			break;

		case GraphActions::ScrollRight:
			_glWindow.model()->shiftForward(10);
			break;
		}

		/* As long as the action button is pressed, the zooming factor will grow
		 * smoothly until it reaches a maximum value. This will have a visible
		 * effect of an accelerating zoom. */
		if (k < .25)
			k *= 1.02;

		_mState->updateMarkers(*_data, *_glWindow.model()->histo());
		QCoreApplication::processEvents();
	}
}