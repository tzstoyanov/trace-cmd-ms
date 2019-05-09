// SPDX-License-Identifier: GPL-2.0

/*
 * Copyright (C) 2018 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
 */

// C
#include <stdio.h>
#include <stdlib.h>

// KernelShark
#include "libkshark.h"
#include "libkshark-model.h"

const char *default_file = "ws_out.txt";

#define N_BINS 5

int main(int argc, char **argv)
{
	struct kshark_context *kshark_ctx;
	struct kshark_entry **data = NULL;
	struct kshark_trace_histo histo;
	size_t r, n_rows;
	char *entry_str;
	int sd;

	/* Create a new kshark session. */
	kshark_ctx = NULL;
	if (!kshark_instance(&kshark_ctx))
		return 1;

	/* Open a trace data file produced by trace-cmd. */
	if (argc > 1)
		sd = kshark_open(kshark_ctx, argv[1]);
	else
		sd = kshark_open(kshark_ctx, default_file);

	if (sd < 0) {
		kshark_free(kshark_ctx);
		return 1;
	}

	/* Load the content of the file into an array of entries. */
	n_rows = kshark_load_data_entries(kshark_ctx, sd, &data);
	printf("n_rows: %li\n", n_rows);

	for (r = 1050; r < 1060; ++r) {
		entry_str = kshark_dump_entry(data[r]);
		puts(entry_str);
		free(entry_str);
	}

	size_t time_last = data[0]->ts;
	for (r = 1; r < n_rows; ++r) {
		if (data[r]->ts < time_last)
			printf(" %lu", r+2);

		time_last = data[r]->ts;
	}

	/* Initialize the Visualization Model. */
	ksmodel_init(&histo);
	ksmodel_set_bining(&histo, N_BINS, data[0]->ts,
					   data[n_rows - 1]->ts);

	/* Fill the model with data and calculate its state. */
	ksmodel_fill(&histo, data, n_rows);

	/* Reset (clear) the model. */
	ksmodel_clear(&histo);

	/* Free the memory. */
// 	for (r = 0; r < n_rows; ++r)
// 		free(data[r]);
	free(*data);
	free(data);

	/* Close the file. */
	kshark_close(kshark_ctx, sd);

	/* Close the session. */
	kshark_free(kshark_ctx);

	return 0;
}
