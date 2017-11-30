#include <QApplication>
#include <QVBoxLayout>
#include <QLabel>

#include <QOpenGLWidget>

#include "KsPlotTools.h"
#include "KsUtils.h"
#include "KsModel.h"

int main(int argc, char **argv)
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	struct task_list *list;

	QApplication app(argc, argv);
	QWidget window;
	QVBoxLayout layout;
	KsDataStore 	data;
	KsGraphModel	model;
	KsGLWidget gl;
	int nCpus, val, pid;

	QHash<int, KsPlot::Color> colors;
	KsPlot::Color lastColor;

	data.loadData("trace.dat");
		for (int i = 0; i < TASK_HASH_SIZE; ++i) {
		list = kshark_ctx->tasks[i];
		while (list) {
			printf("pid: %i\n", list->pid);
			colors[list->pid] = ++lastColor;
			list = list->next;
		}
	}

	nCpus = data._pevt->cpus;
	model.fill(data._pevt, data._rows, data.size());


	for (int i = 0; i < nCpus; ++i) {
		gl._graphs.append(new KsPlot::Graph(1024));
		gl._graphs[i]->setBase(30 + i*70);
		for (int b = 0; b < 1024; ++b) {
			val = 10 + model.getValue(i + 2, b).toInt()*20;
			pid = 0;
			if (model.histo().notEmpty(b, i))
				pid = model.histo().pid(b, i);

			gl._graphs[i]->setBinValue(b, val);
			gl._graphs[i]->setBinColor(b, colors[pid]);
		}
	}
	window.resize(1100,600);

	gl.resize(100,100);
	layout.addWidget(&gl);
	window.setLayout(&layout);
	window.show();
	return app.exec();
}
