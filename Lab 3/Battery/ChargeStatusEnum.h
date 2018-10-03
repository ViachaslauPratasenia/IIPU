#pragma once

enum ChargeStatusEnum {
	High = 0x01,
	Low = 0x02,
	Critical = 0x04,
	Charging = 0x08,
	NoSystemBattery = 128,
	UnknownChargeLevelStatus = 255
};