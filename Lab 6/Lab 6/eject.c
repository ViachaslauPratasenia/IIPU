#include "eject.h"

//-------------------------------------------------
int EjectDevice(char DriveLetter)
{
	//переводим в верхний регистр
	DriveLetter &= ~0x20;

	if (DriveLetter < 'A' || DriveLetter > 'Z') {
		return 1;
	}
	//дл€ типа девайса
	char szRootPath[] = "X:\\";
	szRootPath[0] = DriveLetter;
	//открываем сам том
	char szVolumeAccessPath[] = "\\\\.\\X:";
	szVolumeAccessPath[4] = DriveLetter;

	long DeviceNumber = -1;

	// открываем хэндлер тома
	HANDLE hVolume = CreateFile(szVolumeAccessPath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
	if (hVolume == INVALID_HANDLE_VALUE) {
		return 1;
	}

	// получаем номер девайса
	STORAGE_DEVICE_NUMBER sdn;
	DWORD dwBytesReturned = 0;
	long res = DeviceIoControl(hVolume, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sdn, sizeof(sdn), &dwBytesReturned, NULL);
	if (res) {
		DeviceNumber = sdn.DeviceNumber;
	}
	CloseHandle(hVolume);
	//если значение не валидное
	if (DeviceNumber == -1) {
		return 1;
	}

	// получаем тип диска
	UINT DriveType = GetDriveType(szRootPath);

	// хэндлер девайс инстанса (экземпл€ра устройства дл€ его извлечени€)
	DEVINST DevInst = GetDrivesDevInstByDeviceNumber(DeviceNumber, DriveType);
	//устройство не найдено или сломано
	if (DevInst == 0) {
		return 1;
	}
	//если вызван запрет, скажет почему
	PNP_VETO_TYPE VetoType = PNP_VetoTypeUnknown;
	WCHAR VetoNameW[MAX_PATH];
	VetoNameW[0] = 0;
	int bSuccess = FALSE;

	//если флешка разбита, будет список из этих интерфейсов. »спользуем верхний элемент дл€ извлечени€
	DEVINST DevInstParent = 0;
	res = CM_Get_Parent(&DevInstParent, DevInst, 0);


	VetoNameW[0] = 0;
	//извлечение устройства
	res = CM_Request_Device_EjectW(DevInstParent, &VetoType, VetoNameW, MAX_PATH, 0);

	bSuccess = (res == CR_SUCCESS && VetoType == PNP_VetoTypeUnknown);

	if (bSuccess) {
		printf("Success\n");
		return 0;
	}

	printf("Failed!\n");

	printf("Result=0x%2X\n", res);

	if (VetoNameW[0]) {
		printf("VetoName=%ws", VetoNameW);
		printf("\n");
	}
	return 1;
}

// возвращает дескриптор экземпл€ра устройства объема хранилища или 0 при ошибке
DEVINST GetDrivesDevInstByDeviceNumber(long DeviceNumber, UINT DriveType)
{
	GUID* guid;
	//проверка возможности извлечени€
	switch (DriveType) {
	case DRIVE_REMOVABLE:
		guid = (GUID*)&GUID_DEVINTERFACE_DISK;
		break;
	default:
		return 0;
	}

	// получаем все девайсы, подключенные к системе
	HDEVINFO hDevInfo = SetupDiGetClassDevs(guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	if (hDevInfo == INVALID_HANDLE_VALUE) {
		return 0;
	}

	// получаем структуру контекста дл€ интерфейса устройства набора данных устройства
	DWORD dwIndex = 0;
	long res;
	//свойства, нужные дл€ обнаружени€ devinsta
	BYTE Buf[1024];
	PSP_DEVICE_INTERFACE_DETAIL_DATA pspdidd = (PSP_DEVICE_INTERFACE_DETAIL_DATA)Buf;
	SP_DEVICE_INTERFACE_DATA         spdid;
	SP_DEVINFO_DATA                  spdd;
	DWORD                            dwSize;

	spdid.cbSize = sizeof(spdid);

	while (TRUE) {
		res = SetupDiEnumDeviceInterfaces(hDevInfo, NULL, guid, dwIndex, &spdid);
		if (!res) {
			break;
		}

		dwSize = 0;
		SetupDiGetDeviceInterfaceDetail(hDevInfo, &spdid, NULL, 0, &dwSize, NULL); // провер€ем размер буфера
		//проверка
		if (dwSize != 0 && dwSize <= sizeof(Buf)) {

			pspdidd->cbSize = sizeof(*pspdidd);

			ZeroMemory(&spdd, sizeof(spdd));
			spdd.cbSize = sizeof(spdd);
			//извлечь свойства интерфейса
			long res = SetupDiGetDeviceInterfaceDetail(hDevInfo, &spdid, pspdidd, dwSize, &dwSize, &spdd);
			if (res) {
				// открываем хэндлер диска
				HANDLE hDrive = CreateFile(pspdidd->DevicePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
				if (hDrive != INVALID_HANDLE_VALUE) {
					// получаем его номер
					STORAGE_DEVICE_NUMBER sdn;
					DWORD dwBytesReturned = 0;
					res = DeviceIoControl(hDrive, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sdn, sizeof(sdn), &dwBytesReturned, NULL);
					if (res) {
						if (DeviceNumber == (long)sdn.DeviceNumber) {  //сравниваем с тем, который поступил как аргумент, и возвращаем devinst, если совпали
							CloseHandle(hDrive);
							SetupDiDestroyDeviceInfoList(hDevInfo);
							return spdd.DevInst;
						}
					}
					CloseHandle(hDrive);
				}
			}
		}
		dwIndex++;
	}
	//уничтожаем список
	SetupDiDestroyDeviceInfoList(hDevInfo);

	return 0;
}