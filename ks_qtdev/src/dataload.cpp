#include <iostream>
#include <string>

#include "ks-view.h"
#include "KsDataStore.h"
#include "KsModel.h"

#include "GetBackTrace.h"

int main(int argc, char **argv)
{
	SetErrorHdlr();
	KsDataStore data;
	data.loadData("trace.dat");
	size_t n = data.size();
	
	//KsTimeMap map(1024*4, data._rows[0]->ts, data._rows[n-1]->ts);
	//map.fill(data._rows, n);
	//for (size_t i = 0; i <= map.size(); ++i)
		//std::cout << i << "  map: " << map[i] << "  " << map.binCount(i) << std::endl;

	KsGraphModel model(8);
	model.fill(data._pevt, data._rows, n);	
	for (size_t i = 0; i <= model._map->size(); ++i) {
		std::cout 	<< i << "  map: " << (*model._map)[i] << "  " << model._map->binCount(i) << std::endl;
		std::cout 	<< i << "  val: "
					<< model.getValue(0, i).toString().toStdString() << " "
					<< model.getValue(4, i).toString().toStdString()
					<< std::endl << std::endl;
	}

	//size_t i = model._map->size() -1;
	//std::cout 	<< i << "  map: " << (*model._map)[i] << "  " << model._map->binCount(i) << std::endl;

	std::cout << data._pevt->nr_events << "  event 0: " << std::string(data._pevt->events[15]->name) << std::endl;
	return 0;
}

