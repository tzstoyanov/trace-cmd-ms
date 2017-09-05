/*
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

// C++
#include <iostream>
#include <chrono>

#include "KsDataStore.h"

#include "ks-view.h"

#define GET_TIME std::chrono::high_resolution_clock::now()

#define GET_DURATION(t0) std::chrono::duration_cast<std::chrono::duration<double>>( \
std::chrono::high_resolution_clock::now()-t0).count()

typedef std::chrono::high_resolution_clock::time_point  hd_time;

KsDataStore::~KsDataStore()
{
	clear();
}

void KsDataStore::loadData(const QString& file)
{
	hd_time t0 = GET_TIME;

	clear();
	_handle = tracecmd_open( file.toStdString().c_str() );

	if(!_handle) {
		std::cerr << "ERROR Loading file " << file.toStdString() << std::endl;
		return;
	}

	/* The handle should have loaded the file by now.
	 * Try to turn off function trace indent and turn on show parent
	 * if possible.
	 */
	trace_util_add_option("ftrace:parent", "1");
	trace_util_add_option("ftrace:indent", "0");

	/* Also, show the function name in the tail for function graph */
	trace_util_add_option("fgraph:tailprint", "1");

	loadData(_handle);

	double time2 = GET_DURATION(t0);
	std::cout <<"entries: " << _data_size << "  time: " << 1e3*time2 << " ms.\n";
}


void KsDataStore::loadData(struct tracecmd_input *handle)
{
	_data_size = ks_load_data(handle, &_rows);
	_pevt = tracecmd_get_pevent(handle);
}

void KsDataStore::clear()
{
	if (_data_size) {
		for (size_t r = 0; r < _data_size; ++r)
			free_record( _rows[r] );

		free(_rows);
		_rows = nullptr;
		_data_size = 0;
	}

	_pevt = nullptr;

	if (_handle)
	{
		tracecmd_close(_handle);
		_handle = nullptr;
	}
}



