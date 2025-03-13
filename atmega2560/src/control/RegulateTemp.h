#ifndef REGULATETEMP_H
#define REGULATETEMP_H

#include <stdbool.h>
#include "../boundary/HeatingUnitIF.h"
#include "../domain/SystemData.h"

class RegulateTemp {
public:
	static void regulateTemp(const SystemData& sysData) {
		if (sysData.getTemp() < sysData.getSetPoint()) {
			HeatingUnitIF::start();
		} else {
			HeatingUnitIF::stop();
		}
	}
	
private:
	RegulateTemp() = delete; // Prevent instantiation
	~RegulateTemp() = delete;
	RegulateTemp(const RegulateTemp&) = delete;
	RegulateTemp& operator=(const RegulateTemp&) = delete;
	
};

#endif //REGULATETEMP_H
