
#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsRectItem>

int main(int argc,char ** argv)
{
	QApplication app(argc,argv);

	QGraphicsScene scene;
	QGraphicsView view(&scene);
	QGraphicsRectItem* item1 = new QGraphicsRectItem(0,0,100,100);
	QGraphicsRectItem* item2 = new QGraphicsRectItem(0,100,100,100);
	QGraphicsRectItem* item3 = new QGraphicsRectItem(100,0,100,100);
	QGraphicsRectItem* item4 = new QGraphicsRectItem(100,100,100,100);

	item1->setBrush(QBrush(Qt::red));
	item2->setBrush(QBrush(Qt::green));
	item3->setBrush(QBrush(Qt::blue));
	item4->setBrush(QBrush(Qt::yellow));

	scene.addItem(item1);
	scene.addItem(item2);
	scene.addItem(item3);
	scene.addItem(item4);

	view.setFixedSize(250,250);
	view.setWindowTitle("QGraphicsRectItem Color Example");
	view.show();

	return app.exec();
}
