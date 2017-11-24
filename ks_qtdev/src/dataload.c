#include <stdio.h>
#include <stdlib.h>

#include "libkshark.h"

int main(int argc, char **argv)
{
	struct tracecmd_input *handle;
	struct kshark_entry **rows = NULL;
	struct pevent_record *data;
	size_t n_rows, r;
	int cpu;
	//char *comm, *event, *lat, *info;
	struct kshark_context *ctx = NULL;
	kshark_instance(&ctx);

	handle = tracecmd_open(argv[1]);
	n_rows = kshark_load_data_entries(handle, &rows);

	//for (r = 0; r < 500; ++r) {
		//char* dump = kshark_dump_entry(rows[r], &size);
		//printf("%s\n", dump);
		//free(dump);
	//}

	for (r = 0; r < n_rows; ++r) {
		data = tracecmd_read_at(ctx->handle,
					rows[r]->offset,
					&cpu);
					       
		//comm = kshark_get_task(ctx->pevt, rows[r]);
		//event = kshark_get_event_name(ctx->pevt, rows[r]);
		//lat = kshark_get_latency(ctx->pevt, data, rows[r]);
		//info = kshark_get_info(ctx->pevt, data, rows[r]);

		free_record(data);
	}

	free(rows);
	tracecmd_close(handle);

	return 0;
}
