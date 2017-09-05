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

#ifndef KS_DATASTORE_H
#define KS_DATASTORE_H 1

// Qt
#include <QString>


#include "trace-cmd.h"
#include "ks-view.h"

class KsDataStore {
public:
	KsDataStore() : _rows(nullptr), _pevt(nullptr), _data_size(0),_handle(nullptr) {}
	~KsDataStore();

	void loadData(struct tracecmd_input *handle);
	void loadData(const QString& file);
	void clear();
	size_t size() const {return _data_size;}

	struct pevent_record		**_rows;
	pevent						*_pevt;

private:
	size_t				_data_size;
	struct tracecmd_input	*_handle;
};


#endif

