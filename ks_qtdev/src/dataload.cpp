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
	model._visMap.setBining(13, data._rows[15000]->ts, data._rows[41000]->ts);

	model.fill(data._pevt, data._rows, n, false);
	//std::cout << "fill done " <<  data._rows[40000]->ts - data._rows[15000]->ts << "  " <<  (data._rows[40000]->ts - data._rows[15000]->ts)/3 << std::endl;
	//std::cout << "map " << model._map->size() << "  " << model._map->_binSize << std::endl;
	
	int c1(0),c2(0);
	c1 = c2 = model._visMap[0];
	for (size_t i = 0; i <= model._visMap.size(); ++i) {
		std::cout 	<< i << "  map: " << model._visMap[i] << "  " << model._visMap.binCount(i) << " " << model._visMap._binCount[i] << std::endl;
		//std::cout 	<< i << "  val: "
					//<< model.getValue(0, i).toString().toStdString() << " "
					//<< model.getValue(4, i).toString().toStdString() << " "
					//<< i << "  ts: " << data._rows[ (*model._map)[i] ]->ts - data._rows[15000]->ts
					//<< std::endl << std::endl;
		if (model._visMap.binCount(i) != model._visMap._binCount[i]) std::cout << "ERROR \n";
		c1 += model._visMap.binCount(i);
		c2 += model._visMap._binCount[i];
	}

	//size_t i = model._map->size() -1;
	//std::cout 	<< i << "  map: " << (*model._map)[i] << "  " << model._map->binCount(i) << std::endl;

	std::cout << data.size() << "  " << c1 << "  " << c2 << std::endl;

	model._visMap.shiftForward(2);
	c1 = c2 = model._visMap[0];
	for (size_t i = 0; i <= model._visMap.size(); ++i) {
		std::cout 	<< i << "  map: " << model._visMap[i] << "  " << model._visMap.binCount(i) << " " << model._visMap._binCount[i] << std::endl;
		//std::cout 	<< i << "  val: "
					//<< model.getValue(0, i).toString().toStdString() << " "
					//<< model.getValue(4, i).toString().toStdString() << " "
					//<< i << "  ts: " << data._rows[ (*model._map)[i] ]->ts - data._rows[15000]->ts
					//<< std::endl << std::endl;
		if (model._visMap.binCount(i) != model._visMap._binCount[i]) std::cout << "ERROR \n";
		c1 += model._visMap.binCount(i);
		c2 += model._visMap._binCount[i];
	}

	//size_t i = model._map->size() -1;
	//std::cout 	<< i << "  map: " << (*model._map)[i] << "  " << model._map->binCount(i) << std::endl;

	std::cout << data.size() << "  " << c1 << "  " << c2 << std::endl;
	return 0;
}

