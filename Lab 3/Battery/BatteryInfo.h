#pragma once

#include "ACLineStatusEnum.h"
#include "ChargeStatusEnum.h"
#include "BatterySaverStatusEnum.h"
#include "BatteryLifePercent.h"

class BatteryInfo
{
public:
	ACLineStatusEnum ACStatus;
	ChargeStatusEnum ChargeLevel;
	BatteryLifePercent BatteryLifePercent;
	BatterySaverStatusEnum  BatterySaver;
};