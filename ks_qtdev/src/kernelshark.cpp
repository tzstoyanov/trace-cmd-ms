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

// Kernel Shark 2
#include "KsMainWindow.hpp"
#include "KsDeff.h"

#define default_input_file (char*)"trace.dat"
static char *input_file;
// QStringList files;

void usage(const char *prog)
{
	printf("Usage: %s\n", prog);
	printf("  -h	Display this help message\n");
	printf("  -v	Display version and exit\n");
	printf("  -i	input_file, default is %s\n", default_input_file);
}

int main(int argc, char **argv)
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

	int c;
	while ((c = getopt(argc, argv, "hvi:p:")) != -1) {
		switch(c) {
		case 'h':
			usage(argv[0]);
			return 0;

		case 'v':
			printf("%s - %s\n", basename(argv[0]), KS_VERSION_STRING);
			return 0;

		case 'i':
			input_file = optarg;
// 			files.append(QString(optarg));
			break;

		case 'p':
			kshark_register_plugin(kshark_ctx, optarg);
			break;

		default:
			break;
		}
	}

	if ((argc - optind) >= 1) {
		if (input_file)
			usage(argv[0]);
		input_file = argv[optind];
	}

// 	if (!input_file)
// 		input_file = default_input_file;

	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QApplication a(argc, argv);

	KsMainWindow ks;

// 	if (files.count())
// 		ks.loadFiles(files);

	if (input_file)
		ks.loadFile(QString(input_file));

	ks.show();
	return a.exec();
}
