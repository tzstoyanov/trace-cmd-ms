#include <stdio.h>
#include <stdlib.h>

#include "libkshark.h"

int main(int argc, char **argv)
{
	struct kshark_entry **rows = NULL;
	size_t n_rows;
	int r;

	struct kshark_context *ctx = NULL;
	kshark_instance(&ctx);

	kshark_open(ctx, argv[1]);

	kshark_register_plugin(ctx, "../lib/plugin-foo.so");
	kshark_register_plugin(ctx, "../lib/plugin-bar.so");
	kshark_handle_plugins(ctx, KSHARK_PLUGIN_LOAD);
	
	n_rows = kshark_load_data_entries(ctx, &rows);
	for (r = 0; r < n_rows; ++r) {
		// Do something here ...
// 		printf("%i ts: %lu \n", r, rows[r]->ts);
		free(rows[r]);
	}

	free(rows);

	kshark_handle_plugins(ctx, KSHARK_PLUGIN_UNLOAD);
	kshark_unregister_plugin(ctx, "../lib/plugin-bar.so");

	kshark_register_plugin(ctx, "../lib/plugin-bar_new.so");
	kshark_handle_plugins(ctx, KSHARK_PLUGIN_LOAD);

	uint64_t *ts;
	n_rows = kshark_load_data_matrix(ctx, NULL, NULL, &ts, NULL, NULL, NULL);
	for (r = 0; r < n_rows; ++r) {
		// Do something else here ...
// 		printf("%i ts: %lu \n", r, ts[r]);
	}

	free(ts);

	kshark_close(ctx);
	kshark_free(ctx);

	return 0;
}
