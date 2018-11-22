#pragma once

#include <Windows.h>
#include <iostream>
#include "BatteryInfo.h"
#include "ChargeStatusEnum.h"
#include "BatterySaverStatusEnum.h"
#include "BatteryLifePercent.h"

class BatteryInfoProvider {
	BatteryInfo battery;

public:
	BatteryInfo getBatteryInfo() {
		SYSTEM_POWER_STATUS batteryState;

		if ( !GetSystemPowerStatus(&batteryState)) {
			std::cout << GetLastError() << std::endl;
			exit(-1);
		}

		battery.ACStatus = (ACLineStatusEnum) batteryState.ACLineStatus;
		battery.ChargeLevel = (ChargeStatusEnum) batteryState.BatteryFlag;
		battery.BatteryLifePercent = (BatteryLifePercent) batteryState.BatteryLifePercent;
		battery.BatterySaver = (BatterySaverStatusEnum) batteryState.SystemStatusFlag;
		if (batteryState.ACLineStatus == 1) {
			battery.BatteryLifeTime = 0;
		}
		else {
			battery.BatteryLifeTime = batteryState.BatteryLifeTime;
		}
		

		return battery;
	}
};