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

	cd.show();
	return a.exec();
}
