#include <QApplication>
#include <QVBoxLayout>
#include <QLabel>

#include <QOpenGLWidget>

#include "KsPlotTools.hpp"
#include "KsUtils.hpp"
#include "KsModel.hpp"
#include "KsTraceGraph.hpp"

int main(int argc, char **argv)
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

// 	struct task_list *list;

	QApplication app(argc, argv);
	QWidget window;
	QVBoxLayout layout, l1;
	KsDataStore 	data;
	data.loadData("trace.dat");

	KsGraphModel	model;
	QScrollArea 	scroll(&window);
	QWidget w1(&scroll);
	scroll.setWidget(&w1);
	w1.setLayout(&l1);

	KsGLWidget gl(&w1);
	gl.loadData(&data);
	w1.setMinimumSize(1000, 700);
	l1.addWidget(&gl);

// 	int nCpus, nRows, val, pid;

// 	QHash<int, KsPlot::Color> colors;
// 	colors[0] = {0, 0, 0};
// 	int nTasks = 0;
// 	KsPlot::Palette plt;


// 	for (int i = 0; i < TASK_HASH_SIZE; ++i) {
// 		list = kshark_ctx->tasks[i];
// 		while (list) {
// 			printf("pid: %i \n", list->pid);
// 			if (list->pid)
// 				colors[list->pid] = plt._colors[nTasks++];
// 			list = list->next;
// 		}
// 	}
// 
// 	nCpus = data._pevt->cpus;
// 	nRows = data.size();
// 
// 	model.histoPtr()->setBining(1024, data._rows[0]->ts, data._rows[nRows - 1]->ts);
// 	model.fill(data._pevt, data._rows, data.size(), false);
// 
// 	for (int i = 0; i < nCpus; ++i) {
// 		gl._graphs.append(new KsPlot::Graph(1024));
// 		gl._graphs[i]->setBase(30 + (nCpus - i - 1)*70);
// 		for (int b = 0; b < 1024; ++b) {
// 			val = 1 + model.getValue(i + 2, b, &pid)*20;
// 
// 			if (pid > 0) {
// 				gl._graphs[i]->setBin(b, val, colors[pid], false);
// 			} else if (pid == 0) {
// 				gl._graphs[i]->setBin(b, val, colors[pid], true);
// 			} else {
// 				gl._graphs[i]->deriveBin(b);
// 			}
// 		}
// 	}

	window.resize(1100,500);

// 	gl.resize(1000,100);

	layout.addWidget(&scroll);
	window.setLayout(&layout);
	gl.update();
	window.show();
	return app.exec();
}
