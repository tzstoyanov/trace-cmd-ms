#include <iostream>
#include <QApplication>

#include "KsUtils.h"

int main(int argc,char ** argv)
{
	QApplication app(argc,argv);
	QMainWindow view;
	view.resize(800, 200);
	QToolBar tbar;
	tbar.setOrientation(Qt::Horizontal);
	tbar.resize(800, 50);

	KsDualMarkerSM b(&view);
	b.placeInToolBar(&tbar);
	view.layout()->addWidget(&tbar);
	view.show();

	return app.exec();
}

	//QApplication app(argc,argv);
	//QMainWindow view;
	////view.setLayout(new QVBoxLayout);
	//view.resize(800, 200);
	//QToolBar tbar(&view);
	////tbar.setOrientation(Qt::Horizontal);
	//KsDualMarkerSM b(&view);
	//tbar.addWidget(&b._buttonA);
	////tbar.addSeparator();
	//tbar.addWidget(&b._buttonB);

	//view.layout()->addWidget(&tbar);
	//view.show();
