#include <gtest/gtest.h>

#include "KsModel.hpp"

TEST(KsTimeMapInitTest, Init)
{
	KsTimeMap map(10, 0, 100);
	EXPECT_EQ(map._nBins, 10);
	EXPECT_EQ(map.size(), 10);
	EXPECT_EQ(map._min, 0);
	EXPECT_EQ(map._max, 100);
	EXPECT_EQ(map._binSize, 10);
}

TEST(KsTimeMapInitTest, InitEmpty)
{
	KsTimeMap map;
	EXPECT_EQ(map._nBins, 0);
	EXPECT_EQ(map.size(), 0);
	EXPECT_EQ(map._min, 0);
	EXPECT_EQ(map._max, 0);
	EXPECT_EQ(map._binSize, 0);
}

TEST(KsTimeMapInitTest, InitWrong)
{
	KsTimeMap map(100, 0, 10);
	EXPECT_EQ(map._nBins, 0);
	EXPECT_EQ(map.size(), 0);
	EXPECT_EQ(map._min, 0);
	EXPECT_EQ(map._max, 0);
	EXPECT_EQ(map._binSize, 0);
}

struct KsTimeMapTest : public testing::Test
{
	KsTimeMap *map;
	//std::vector <struct pevent_record *> data;
	std::vector <struct kshark_entry *> data;
	
protected:

	virtual void SetUp()
	{
		map = new KsTimeMap();
		int ts = 1000;
		map = new KsTimeMap();
		data.resize(1000);
		for (auto &d: data) {
			//d = new struct pevent_record;
			d = new struct kshark_entry;
			d->ts = ts++;
		}
	}
  
	virtual void TearDown()
	{
		for (auto &d: data)
			delete d;

		delete map;
	}
};


TEST_F(KsTimeMapTest, SetBiningInit)
{
	EXPECT_EQ(data.size(), 1000);
	EXPECT_EQ(data[0]->ts, 1000);
	EXPECT_EQ(data.at(999)->ts, 1999);
	EXPECT_EQ(map->_nBins, 0);
}

TEST_F(KsTimeMapTest, SetBiningWrong)
{
	map->setBining(2048, data[0]->ts, data[999]->ts);
	EXPECT_EQ(map->_nBins, 0);
	EXPECT_EQ(map->_binSize, 0);
	EXPECT_EQ(map->_min, 0);
	EXPECT_EQ(map->_max, 0);
}

TEST_F(KsTimeMapTest, SetBiningExact)
{
	map->setBining(100, 1000, 2000);
	EXPECT_EQ(map->_nBins, 100);
	EXPECT_EQ(map->_binSize, 10);
	EXPECT_EQ(map->_min, 1000);
	EXPECT_EQ(map->_max, 2000);

	map->fill(data.data(), data.size());
	for (int i = 0; i < 100; ++i) {
		EXPECT_EQ(map->at(i), i*10);
		EXPECT_EQ(map->binCount(i), 10);
	}

	EXPECT_EQ(map->at(100), -1);
	EXPECT_EQ(map->binCount(100), 0);
	EXPECT_EQ(map->at(101), -1);
	EXPECT_EQ(map->binCount(101), 0);
}

TEST_F(KsTimeMapTest, SetBiningAprox)
{
	map->setBining(100, 1000, 1994);
	EXPECT_EQ(map->_nBins, 100);
	EXPECT_EQ(map->_binSize, 10);
	EXPECT_EQ(map->_min, 997);
	EXPECT_EQ(map->_max, 1997);

	map->fill(data.data(), data.size());
	//for (int i = 0; i < 10; ++i) {
		//EXPECT_EQ(map->at(i), i + 2);
		//EXPECT_EQ(map->binCount(i), 1);
	//}
	
	//EXPECT_EQ(map->at(995), 997);
	//EXPECT_EQ(map->binCount(995), 1);

	//EXPECT_EQ(map->at(-1), 998);
	//EXPECT_EQ(map->binCount(-1), 2);
	//EXPECT_EQ(map->at(-2), 0);
	//EXPECT_EQ(map->binCount(-2), 2);
}

TEST_F(KsTimeMapTest, TestShift)
{
	for (size_t i = 0; i < data.size(); ++i) {
		data[i]->ts = i*10;
	} 

	map->setBining(100, 200, 300);
	EXPECT_EQ(map->_nBins, 100);
	EXPECT_EQ(map->_binSize, 1);
	EXPECT_EQ(map->_min, 200);
	EXPECT_EQ(map->_max, 300);

	map->fill(data.data(), data.size());
	//map->dump();
	EXPECT_EQ(map->at(-2), 0);
	EXPECT_EQ(map->binCount(-2), 20);
	EXPECT_EQ(map->at(-1), 30);
	EXPECT_EQ(map->binCount(-1), 970);

	for (size_t i = 0; i < 100; ++i) {
		if( i%10 ) {
			EXPECT_EQ(map->at(i), -1);
			EXPECT_EQ(map->binCount(i), 0);
		} else {
			EXPECT_EQ(map->at(i), 20 + i/10);
			EXPECT_EQ(map->binCount(i), 1);
		}
	}

	map->shiftForward(13);
	//map->dump();
	EXPECT_EQ(map->_nBins, 100);
	EXPECT_EQ(map->_binSize, 1);
	EXPECT_EQ(map->_min, 200 + 13);
	EXPECT_EQ(map->_max, 300 +13);
	EXPECT_EQ(map->at(-2), 0);
	EXPECT_EQ(map->binCount(-2), 20 + 2);
	EXPECT_EQ(map->at(-1), 30 +2 );
	EXPECT_EQ(map->binCount(-1), 970 - 2);
	
	for (size_t i = 0; i < 100; ++i) {
		if( (i+3)%10 ) {
			EXPECT_EQ(map->at(i), -1);
			EXPECT_EQ(map->binCount(i), 0);
		} else {
			EXPECT_EQ(map->at(i), 21 + (i+3)/10);
			EXPECT_EQ(map->binCount(i), 1);
		}
	}

	map->shiftBackward(17);
	//map->dump();
	EXPECT_EQ(map->_nBins, 100);
	EXPECT_EQ(map->_binSize, 1);
	EXPECT_EQ(map->_min, 200 + 13 - 17);
	EXPECT_EQ(map->_max, 300 +13 -17);
	EXPECT_EQ(map->at(-2), 0);
	EXPECT_EQ(map->binCount(-2), 20);
	EXPECT_EQ(map->at(-1), 30);
	EXPECT_EQ(map->binCount(-1), 970);

	for (size_t i = 0; i < 100; ++i) {
		if( (i+6)%10 ) {
			EXPECT_EQ(map->at(i), -1);
			EXPECT_EQ(map->binCount(i), 0);
		} else {
			EXPECT_EQ(map->at(i), 19 + (i+6)/10);
			EXPECT_EQ(map->binCount(i), 1);
		}
	}
}

int main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
