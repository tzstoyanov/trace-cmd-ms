/*
 * Copyright (C) 2017 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
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

// C
#include <sys/stat.h>
#include <getopt.h>

// Qt
#include <QApplication>

// trace-cmf
#include "trace-cmd.h"

// KernelShark
#include "KsMainWindow.hpp"
#include "KsDeff.h"

#define default_input_file (char*)"trace.dat"

static char *input_file;

void usage(const char *prog)
{
	printf("Usage: %s\n", prog);
	printf("  -h	Display this help message\n");
	printf("  -v	Display version and exit\n");
	printf("  -i	input_file, default is %s\n", default_input_file);
	printf("  -p	register plugin, use plugin name, absolute or relative path\n");
	printf("  -u	unregister plugin, use plugin name or absolute path\n");
	printf("  -s	import a session\n");
}

int main(int argc, char **argv)
{
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QApplication a(argc, argv);

	KsMainWindow ks;

	int c;
	bool fromSession = false;

	while ((c = getopt(argc, argv, "hvi:p:u:s:")) != -1) {
		switch(c) {
		case 'h':
			usage(argv[0]);
			return 0;

		case 'v':
			printf("%s - %s\n", basename(argv[0]), KS_VERSION_STRING);
			return 0;

		case 'i':
			input_file = optarg;
			break;

		case 'p':
			ks.registerPlugin(QString(optarg));
			break;

		case 'u':
			ks.unregisterPlugin(QString(optarg));
			break;

		case 's':
			ks.loadSession(QString(optarg));
			fromSession = true;

		default:
			break;
		}
	}

	if (!fromSession) {
		if ((argc - optind) >= 1) {
			if (input_file)
				usage(argv[0]);
			input_file = argv[optind];
		}

		if (!input_file) {
			struct stat st;
			if (stat(default_input_file, &st) == 0)
				input_file = default_input_file;
		}

		if (input_file)
			ks.loadData(QString(input_file));
	}

	ks.show();
	return a.exec();
}
