#include <iostream>
#include <iomanip>
#include <Windows.h>
#include <WinIoCtl.h>
#include <ntddscsi.h>
#include <conio.h>

#include "BatteryInfo.h"
#include "BatteryInfoProvider.h"
#include "BatteryLifePercent.h"

using namespace std;


ostream& operator<<(ostream& lhs, ACLineStatusEnum e) {
	switch (e) {
	case Online: lhs << "Online"; break;
	case Offline: lhs << "Offline"; break;
	case UnknownStatus: lhs << "Unknown"; break;
	}
	return lhs;
}

ostream& operator<<(ostream& lhs, ChargeStatusEnum e) {
	if (Critical & e) lhs << " Critical ";
	if (Low & e) lhs << " Low ";
	if (High & e) lhs << " High ";
	if (Charging & e) lhs << " Charging "; 
	if (NoSystemBattery & e)  lhs << "No System Battery";
	if (UnknownChargeLevelStatus == e) lhs << "Unknown";
	return lhs;
}

ostream& operator<<(ostream& lhs, BatterySaverStatusEnum e) {
	switch (e) {
	case Off: 
		lhs << "Off"; 
		break;
	case On: 
		lhs << "On"; 
		break;
	}
	return lhs;
}

ostream& operator<<(ostream& lhs, BatteryLifePercent e) {
	auto value = e.getValue();
	switch (value) {
	case UnknownChargeLevelStatus: 
		lhs << "No system battery";
		break;
	default: 
		lhs << value << "%"; 
		break;
	}
	return lhs;
}

void ShowInfo(BatteryInfo battery) {
	cout << "\tBattery info" << endl;
	cout << "AC line status:          " << battery.ACStatus << endl;
	cout << "Battery charge level:   " << battery.ChargeLevel << endl;
	cout << "Battery life percent:    " << battery.BatteryLifePercent << endl;
	cout << "Battery saver status:    " << battery.BatterySaver << endl;
	cout << "Battery life time :      " << battery.BatteryLifeTime / 60 << " min" << endl;
}

int main() {
	BatteryInfoProvider provider;

	while (true) {
		auto battery = provider.getBatteryInfo();
		ShowInfo(battery);
		Sleep(1000);

		system("cls");
	}

	system("pause");
	return 0;
}