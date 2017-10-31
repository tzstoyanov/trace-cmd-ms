/*
 * Copyright (C) 2017 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License (not later!)
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not,  see <http://www.gnu.org/licenses>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

// C
#include <sys/stat.h>
#include <getopt.h>

// C++
#include <iostream>

// Qt
#include <QApplication>

// trace-cmf
#include "trace-cmd.h"

// Kernel Shark 2
#include "KsMainWindow.h"
#include "KsDeff.h"

#define default_input_file (char*)"trace.dat"
static char *input_file;

void usage(char **argv)
{
	char *prog = basename(argv[0]);
	printf("Usage: %s\n", prog);
	printf("  -h	Display this help message\n");
	printf("  -v	Display version and exit\n");
	printf("  -i	input_file, default is %s\n", default_input_file);
}

static void add_plugin(const char *file)
{
	// TODO
// 	struct stat st;
// 	int ret;
// 
// 	ret = stat(file, &st);
// 	if (ret < 0) {
// 		warning("plugin %s not found", file);
// 		return;
// 	}
// 
// 	*plugin_next = (plugin_list*) calloc(sizeof(struct plugin_list), 1);
// 	if (!*plugin_next)
// 		die("failed to allocat memory for plugin");
// 
// 	(*plugin_next)->file = file;
// 	plugin_next = &(*plugin_next)->next;
}

int main(int argc, char **argv)
{
	int c;
	while ((c = getopt(argc, argv, "hvi:p:")) != -1) {
		switch(c) {
		case 'h':
			usage(argv);
			return 0;
		case 'v':
			printf("%s - %s\n", basename(argv[0]), KS_VERSION_STRING);
			return 0;
		case 'i':
			input_file = optarg;
			break;
		case 'p':
			add_plugin(optarg);
			break;
		default:
			break;
		}
	}

	if ((argc - optind) >= 1) {
		if (input_file)
			usage(argv);
		input_file = argv[optind];
	}

	if (!input_file)
		input_file = default_input_file;

	QApplication a(argc, argv);
	KsMainWindow ks;

	if (input_file)
		ks.loadFile(QString(input_file));

	ks.show();
	return a.exec();
}

