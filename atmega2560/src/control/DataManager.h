// Author: Christoffer H. Brix

#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include "..\boundary\RPiIF.h"
#include "..\boundary\RangingModuleIF.h"
#include "..\boundary\TempSensorIF.h"
#include "..\domain\SystemData.h"

class DataManager {
public:
	DataManager(RPiIF& rpiIF, RangingModuleIF& rangeIF, TempSensorIF& tempIF, SystemData& sysData);
	~DataManager() = default;
	
	void updateSystemData();
	
private:
	DataManager(const DataManager &c) = delete;
	DataManager& operator=(const DataManager &c) = delete;
	
	RPiIF& rpiInterface_;
	RangingModuleIF& rangeInterface_;
	TempSensorIF& tempInterface_;
	SystemData& systemData_;
};

#endif //DATAMANAGER_H
