#ifndef SYSTEMDATA_H
#define SYSTEMDATA_H

#include <stdbool.h>

// No implementation file exists for SystemData. All details are in header file.
class __attribute__((packed)) SystemData {
public:
	SystemData(const uint16_t maxDistance, const uint16_t minDistance) : systemPower_(false), setPoint_(0.0f), range_(0), lastRange_(0), waterTemp_(0.0f), maxDistance_(maxDistance), minDistance_(minDistance) {};

	inline bool getSystemPower() const { return systemPower_; }
	inline float getSetPoint() const { return setPoint_; }
	inline uint16_t getCurrentRange() const { return range_; }
	inline uint16_t getLastRange() const { return lastRange_; }
	inline float getTemp() const { return waterTemp_; }
	inline uint16_t getMaxDistance() const {return maxDistance_; }
	inline uint16_t getMinDistance() const {return minDistance_; }	

	inline void setSystemPower(bool systemPower) { systemPower_ = systemPower; }
	inline void setSetPoint(float setPoint) { setPoint_ = setPoint; }
	inline void setTemp(float temp) { waterTemp_ = temp; }
	inline void setCurrentRange(uint16_t range) { range_ = range; }
	inline void setLastRange(uint16_t range) { lastRange_ = range; }	
	
private:
	bool systemPower_;
	float setPoint_;
	uint16_t range_;
	uint16_t lastRange_;
	float waterTemp_;
	uint16_t maxDistance_;
	uint16_t minDistance_;
};

#endif //SYSTEMDATA_H
