#include <iostream>
#include <QApplication>

#include "KsUtils.h"



int main(int argc,char ** argv)
{
	QApplication app(argc,argv);
	QMainWindow view;
	view.resize(200, 200);
	KsMarkerState b(&view);
	view.layout()->addWidget(&b);
	view.show();

	return app.exec();
}
