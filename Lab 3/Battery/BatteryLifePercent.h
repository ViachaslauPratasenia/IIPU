#pragma once

class BatteryLifePercent {
	int percent = 0;
public:
	BatteryLifePercent() {}

	BatteryLifePercent(int input) {
		this->percent = input;
	}

	int getValue() {
		return percent;
	}
};
