#include <stdio.h>
#include <stdlib.h>

#include "ks-view.h"

int main(int argc, char **argv)
{
	struct tracecmd_input *handle = tracecmd_open("trace.dat");

	struct pevent_record **rows = NULL;
	struct pevent *pevt;
	struct ks_entry e;
	size_t n_rows, r;

	n_rows = ks_load_data(handle, &rows);
	pevt = tracecmd_get_pevent(handle);

	printf("page size: %i \n", pevt->page_size);

	for (r = 0; r < 50; ++r)
	{
		ks_set_entry_values(pevt, rows[r], &e);
		printf("%i  cpu: %i  %s info: %s \n", (int)r, (int)e.cpu, e.task, e.info);
		ks_clear_entry(&e);
	}

	for (r = 0; r < n_rows; ++r)
		free_record( rows[r] );

	free(rows);
	tracecmd_close(handle);

	return 0;
}

