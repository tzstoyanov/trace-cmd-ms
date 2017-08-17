

#include <sys/stat.h>
#include <getopt.h>
#include <iostream>
#include <QApplication>

#include "trace-cmd.h"
// #include "event-utils.h"
// #include "trace-local.h"

#include "KsMainWindow.h"
#include "KsDeff.h"

using namespace std;

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

// static struct plugin_list {
// 	struct plugin_list		*next;
// 	const char			*file;
// } *plugins;
// static struct plugin_list **plugin_next = &plugins;

static void add_plugin(const char *file)
{
	// TODO
// 	struct stat st;
// 	int ret;
// 
// 	ret = stat(default_input_file, &st);
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
			printf("%s - %s\n",
			       basename(argv[0]),
			       KS_VERSION_STRING);
			return 0;
		case 'i':
			input_file = optarg;
			break;
		case 'p':
			add_plugin(optarg);
			break;
		default:
			/* assume the other options are for gtk */
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
	KsMainWindow w;

	if (input_file)
		w.loadFile(QString(input_file));

	w.show();
	return a.exec();
}

