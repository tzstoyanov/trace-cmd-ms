#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "KsDeff.h"
#include "libkshark.h"

#include "KsCaptureDialog.hpp"

int main(int argc, char **argv)
{
	QApplication a(argc, argv);
	KsCaptureDialog cd;

	int c;
	while ((c = getopt(argc, argv, "o:")) != -1) {
		switch(c) {
		case 'o':
			cd.setOutputFileName(QString(optarg));
			break;
		}
	}

	cd.show();
	return a.exec();
}
