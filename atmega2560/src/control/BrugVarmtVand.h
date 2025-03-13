// Author: Christoffer H. Brix

#ifndef BRUGVARMTVAND_H
#define BRUGVARMTVAND_H

#include <stdint-gcc.h>
#include <stddef.h>
#include "../domain/SystemData.h"

class BrugVarmtVand {
public:
	BrugVarmtVand(const SystemData& sysData);
	~BrugVarmtVand() = default;

	void checkDistance();
	
	static volatile uint8_t overflowCount;
	
private:
	enum class State {
		CheckDistance,
		ForceRefill,
		TimerRunning
	};
	
	BrugVarmtVand(const BrugVarmtVand &c) = delete;
	BrugVarmtVand& operator=(const BrugVarmtVand &c) = delete;
	
	void initTimer4();
	void startTimer();
	void stopTimer();
	bool isLevelMin(const uint16_t& currentDistance) const;
	bool isLevelMax(const uint16_t& currentDistance) const;
	bool isWithinLimits(const uint16_t& currentDistance) const;
	bool isDistanceDecreasing(const uint16_t& currentDistance, const uint16_t& lastDistance) const;
	bool isDistanceStable(const uint16_t& currentDistance, const uint16_t& lastDistance) const;
	
	const SystemData& sysData_;
	State currentState_;
	uint16_t maxDistance_;
	uint16_t minDistance_;
	static constexpr uint8_t stabilityThreshold_ = 2;
};

#endif //BRUGVARMTVAND_H
