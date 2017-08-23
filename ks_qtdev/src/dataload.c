#include <stdio.h>
#include <stdlib.h>

#include "ks-view.h"

int main(int argc, char **argv)
{
	struct tracecmd_input *handle = tracecmd_open("trace.dat");

	struct ks_entry **rows = NULL;
	size_t n_rows, r;
	n_rows = ks_load_data(handle, &rows);

	for (r = 0; r < 50; ++r)
		printf("%i  cpu: %i  %s info: %s \n", rows[r]->pos, rows[r]->cpu, rows[r]->task, rows[r]->info);

	for (r = 0; r < n_rows; ++r)
		ks_free_entry( rows[r] );

	free(rows);
	tracecmd_close(handle);

	return 0;
}

