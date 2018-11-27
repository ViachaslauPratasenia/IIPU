#pragma comment (lib, "setupapi.lib")

#define UNICODE
#define _UNICODE
#define INITGUID

// Аннотации, указывающие, что это неприемлемый код пользовательского режима.
#include <DriverSpecs.h>
_Analysis_mode_(_Analysis_code_type_user_code_)

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <setupapi.h>
#include <dbt.h>
#include <strsafe.h>
#include "deviceDao.h"
#include <dontuse.h>
#include <conio.h>

//
// Global variables
//
HINSTANCE   hInst;
HWND        hWndList;
TCHAR       szTitle[] = TEXT("USB Devices handler application");
LIST_ENTRY  ListHead;
HDEVNOTIFY  hInterfaceNotification;
TCHAR       OutText[500];
UINT        ListBoxIndex = 0;
BOOLEAN     Verbose = FALSE;

const GUID USB_CLASSGUID =
// USB Raw Device Interface Class GUID
{
	0xa5dcbf10, 0x6530, 0x11d2,
{ 0x90, 0x1f, 0x00, 0xc0, 0x4f, 0xb9, 0x51, 0xed }
};

#define CLS_NAME "DUMMY_CLASS"
//создаем консоль
DWORD
WINAPI
WindowThreadRoutine(
	PVOID pvParam
)
{
	HANDLE window_create_event = OpenEventA(EVENT_ALL_ACCESS, TRUE, (LPCSTR)(pvParam));

	HWND hWnd = NULL;
	WNDCLASSEX wx;
	ZeroMemory(&wx, sizeof(wx));

	wx.cbSize = sizeof(WNDCLASSEX);
	wx.lpfnWndProc = (WNDPROC)(WndProc);
	wx.hInstance = (HINSTANCE)(GetModuleHandle(0));
	wx.style = CS_HREDRAW | CS_VREDRAW;
	wx.hInstance = GetModuleHandle(0);
	wx.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wx.lpszClassName = (LPCWSTR)CLS_NAME;

	GUID guid = USB_CLASSGUID;

	if (RegisterClassEx(&wx))
	{
		hWnd = CreateWindow((LPCWSTR)CLS_NAME, (LPCWSTR)("DevNotifWnd"), WS_ICONIC,
			0, 0, CW_USEDEFAULT, 0, 0,
			NULL, GetModuleHandle(0), (void*)&guid);
	}

	if (hWnd == NULL)
	{
		printf("Could not create message window!");
		return 1;
	}

	printf("\nWaiting for new devices...\n");

	//сообщаем главному потоку, что окно создано
	SetEvent(window_create_event);

	//для обработки события прихода устройств
	MSG msg;
	while (GetMessage(&msg, hWnd, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

LRESULT
FAR PASCAL
WndProc(
	HWND hWnd,
	UINT message,
	WPARAM wParam,
	LPARAM lParam
)
{
	DWORD nEventType = (DWORD)wParam;
	PDEV_BROADCAST_HDR p = (PDEV_BROADCAST_HDR)lParam;
	DEV_BROADCAST_DEVICEINTERFACE filter;

	switch (message)
	{
	case WM_CREATE:	//создаем окно 
		hWndList = CreateWindow(TEXT("listbox"),
			NULL,
			WS_CHILD | WS_VISIBLE | LBS_NOTIFY |
			WS_VSCROLL | WS_BORDER,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			hWnd,
			(HMENU)ID_EDIT,
			hInst,
			NULL);

		filter.dbcc_size = sizeof(filter);	//подписка на прибытие и отбытие девайса
		filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
		filter.dbcc_classguid = USB_CLASSGUID;
		hInterfaceNotification = RegisterDeviceNotification(hWnd, &filter, 0);

		InitializeListHead(&ListHead);
		EnumExistingDevices(hWnd);	//перечисляет устройства, которые уже есть в системе

		return 0;

	case WM_DEVICECHANGE:
		// DBT_DEVNODES_CHANGED выводится каждый раз, когда произошло какое-либо изменение
		if (DBT_DEVNODES_CHANGED == wParam) {
			printf("Received DBT_DEVNODES_CHANGED message\n");
			return 0;
		}

		//все нужные нам события приходят с lParam, указывающий на структуру с DEV_BROADCAST_HDR. 
		if ((wParam & 0xC000) == 0x8000) {

			if (!p)
				return 0;

			if (p->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {

				HandleDeviceInterfaceChange(hWnd, nEventType, (PDEV_BROADCAST_DEVICEINTERFACE)p);
			}
			else if (p->dbch_devicetype == DBT_DEVTYP_HANDLE) {

				HandleDeviceChange(hWnd, nEventType, (PDEV_BROADCAST_HANDLE)p);
			}
		}
		return 0;
	case WM_CLOSE:
		Cleanup(hWnd);
		UnregisterDeviceNotification(hInterfaceNotification);
		return  DefWindowProc(hWnd, message, wParam, lParam);

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

BOOL
HandleDeviceInterfaceChange(
	HWND hWnd,
	DWORD evtype,
	PDEV_BROADCAST_DEVICEINTERFACE dip
)
{
	DEV_BROADCAST_HANDLE    filter;
	PDEVICE_INFO            deviceInfo = NULL;
	HRESULT                 hr;

	switch (evtype)
	{
	case DBT_DEVICEARRIVAL:	
		//пришло новое устройство

		deviceInfo = (PDEVICE_INFO)calloc(1, sizeof(DEVICE_INFO));
		if (!deviceInfo)
			return FALSE;

		InitializeListHead(&deviceInfo->ListEntry);
		InsertTailList(&ListHead, &deviceInfo->ListEntry);


		if (!GetDeviceDescription(dip->dbcc_name,
			(PBYTE)deviceInfo->DeviceName,
			sizeof(deviceInfo->DeviceName),
			&deviceInfo->SerialNo)) {
			printf("GetDeviceDescription failed\n");
		}

		printf("New device Arrived (Interface Change Notification) %ws\n", deviceInfo->DeviceName);
		//обработка ошибки
		hr = StringCchCopy(deviceInfo->DevicePath, MAX_PATH, dip->dbcc_name);
		if (FAILED(hr)) {
			// DeviceInfo будет освобожден в программой очистке.
			break;
		}

		deviceInfo->hDevice = CreateFile(dip->dbcc_name,
			GENERIC_READ | GENERIC_WRITE, 0, NULL,
			OPEN_EXISTING, 0, NULL);
		if (deviceInfo->hDevice == INVALID_HANDLE_VALUE) {
			printf("Failed to open the device: %ws\n", deviceInfo->DeviceName);
			break;
		}

		printf("Opened handled to the device: %ws\n", deviceInfo->DeviceName);

		memset(&filter, 0, sizeof(filter)); //сборка структуры
		filter.dbch_size = sizeof(filter);
		filter.dbch_devicetype = DBT_DEVTYP_HANDLE;
		filter.dbch_handle = deviceInfo->hDevice;

		deviceInfo->hHandleNotification =
			RegisterDeviceNotification(hWnd, &filter, 0);	//подписываемся на события
		break;

	case DBT_DEVICEREMOVECOMPLETE:
		printf("Remove Complete (Interface Change Notification)\n");
		break;

		//удаление устройства

	default:
		printf("Unknown (Interface Change Notification)\n");
		break;
	}
	return TRUE;
}

BOOL
HandleDeviceChange(
	HWND hWnd,
	DWORD evtype,
	PDEV_BROADCAST_HANDLE dhp
)
{
	DEV_BROADCAST_HANDLE filter;
	PDEVICE_INFO deviceInfo = NULL;
	PLIST_ENTRY thisEntry;
	char result = 0;
	int check = 0;

	//ищем устройство в листе, подходящее для события, сравнивая с дескриптором
	for (thisEntry = ListHead.Flink; thisEntry != &ListHead;	
		thisEntry = thisEntry->Flink)
	{
		deviceInfo = CONTAINING_RECORD(thisEntry, DEVICE_INFO, ListEntry);
		if (dhp->dbch_hdevnotify == deviceInfo->hHandleNotification) {
			break;
		}
		deviceInfo = NULL;
	}

	if (!deviceInfo) {
		printf("Error: spurious message, Event Type: %d\n", evtype);
		return FALSE;
	}

	switch (evtype)
	{
	//запрос на извлечени
	case DBT_DEVICEQUERYREMOVE:
		printf("Query Remove (Handle Notification) %ws\n", deviceInfo->DeviceName);

		while (result != '0' && result != '1' && !check)
		{
			printf("Device removal was initiated. Grant permission (press 1) or disable ejecting (press 0)\n");
			rewind(stdin);
			check = scanf_s("%c", &result);
		}

		// Пользователь пытается отключить, удалить или выгрузить наше устройство.
		// Закроем дескриптор устройства, чтобы целевое устройство можно было удалить.

		if (result == '1')
		{
			if (deviceInfo->hDevice != INVALID_HANDLE_VALUE)
			{
				CloseHandle(deviceInfo->hDevice);
				deviceInfo->hDevice = INVALID_HANDLE_VALUE;
				printf("Closed handle to device: %ws\n", deviceInfo->DeviceName);
			}
		}
		// если установлен запрет
		else if (result == '0')
		{
			return BROADCAST_QUERY_DENY;
		}

		break;
	//устройство извлекли(тупо руками небезопасно)
	case DBT_DEVICEREMOVECOMPLETE:
		printf("Remove Complete (Handle Notification): %ws\n", deviceInfo->DeviceName);

		if (deviceInfo->hHandleNotification) {
			UnregisterDeviceNotification(deviceInfo->hHandleNotification);
			deviceInfo->hHandleNotification = NULL;
		}
		if (deviceInfo->hDevice != INVALID_HANDLE_VALUE) {
			CloseHandle(deviceInfo->hDevice);
			deviceInfo->hDevice = INVALID_HANDLE_VALUE;

			printf("Closed handle to device: %ws\n", deviceInfo->DeviceName);
		}
		// удаляем deviceInfo из списка и чистим память
		RemoveEntryList(&deviceInfo->ListEntry);
		free(deviceInfo);

		break;

	case DBT_DEVICEREMOVEPENDING:	//разрешено удаление, сработает при безопасном извлечении
		printf("Remove Pending (Handle Notification): %ws\n", deviceInfo->DeviceName);
		
		if (deviceInfo->hHandleNotification) {
			UnregisterDeviceNotification(deviceInfo->hHandleNotification);
			deviceInfo->hHandleNotification = NULL;
			deviceInfo->hDevice = INVALID_HANDLE_VALUE;
		}
		// удаляем deviceInfo из списка и чистим память
		RemoveEntryList(&deviceInfo->ListEntry);
		free(deviceInfo);

		break;

	case DBT_DEVICEQUERYREMOVEFAILED:	//если запрет на извлечение
		printf("Remove failed (Handle Notification): %ws\n", deviceInfo->DeviceName);
		//открываем заново хэндлеры
		if (deviceInfo->hDevice == INVALID_HANDLE_VALUE)
		{
			//заново открываем устройство и его регистрируем
			if (deviceInfo->hHandleNotification) {
				UnregisterDeviceNotification(deviceInfo->hHandleNotification);
				deviceInfo->hHandleNotification = NULL;
			}

			deviceInfo->hDevice = CreateFile(deviceInfo->DevicePath,
				GENERIC_READ | GENERIC_WRITE,
				0, NULL, OPEN_EXISTING, 0, NULL);
			if (deviceInfo->hDevice == INVALID_HANDLE_VALUE) {
				printf("Failed to reopen the device: %ws\n", deviceInfo->DeviceName);
				free(deviceInfo);
				break;
			}

			//регистрируем хэндл
			memset(&filter, 0, sizeof(filter));
			filter.dbch_size = sizeof(filter);
			filter.dbch_devicetype = DBT_DEVTYP_HANDLE;
			filter.dbch_handle = deviceInfo->hDevice;

			deviceInfo->hHandleNotification =
				RegisterDeviceNotification(hWnd, &filter, 0);
			printf("Reopened device %ws\n", deviceInfo->DeviceName);
		}

		break;

	default:
		printf("Unknown (Handle Notification)\n");
		break;

	}
	return TRUE;
}

BOOLEAN
EnumExistingDevices(
	HWND   hWnd
)
{
	HDEVINFO                            hardwareDeviceInfo;
	SP_DEVICE_INTERFACE_DATA            deviceInterfaceData;
	PSP_DEVICE_INTERFACE_DETAIL_DATA    deviceInterfaceDetailData = NULL;
	ULONG                               predictedLength = 0;
	ULONG                               requiredLength = 0;
	DWORD                               error;
	DEV_BROADCAST_HANDLE                filter;
	PDEVICE_INFO                        deviceInfo = NULL;
	UINT                                i = 0;
	HRESULT                             hr;

	hardwareDeviceInfo = SetupDiGetClassDevs(
		(LPGUID)&USB_CLASSGUID,
		NULL, // enum
		NULL,
		(DIGCF_PRESENT | // Только присутствующие устройства
			DIGCF_DEVICEINTERFACE)); // Устройства класса функций.
	if (INVALID_HANDLE_VALUE == hardwareDeviceInfo)
	{
		goto Error;
	}

	deviceInterfaceData.cbSize = sizeof(deviceInterfaceData);
	//перечисляем все устройства
	for (i = 0; SetupDiEnumDeviceInterfaces(hardwareDeviceInfo,
		0,
		(LPGUID)&USB_CLASSGUID,
		i,
		&deviceInterfaceData); i++) {

		//получаем инфу о конкретном устройстве
		//ищем длину буфера
		//удаляем структуру
		if (deviceInterfaceDetailData)
		{
			free(deviceInterfaceDetailData);
			deviceInterfaceDetailData = NULL;
		}
		//получаем размер структуры PSP_DEVICE_INTERFACE_DETAIL_DATA
		if (!SetupDiGetDeviceInterfaceDetail(
			hardwareDeviceInfo,
			&deviceInterfaceData,
			NULL, // буфера еще нет
			0, // длина буфера 0
			&requiredLength,
			NULL) && (error = GetLastError()) != ERROR_INSUFFICIENT_BUFFER)
		{
			goto Error;
		}
		predictedLength = requiredLength;
		//выделяем память
		deviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)calloc(predictedLength, sizeof(BYTE));
		if (deviceInterfaceDetailData == NULL) {
			goto Error;
		}
		deviceInterfaceDetailData->cbSize =
			sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

		//извлекаем структуру
		if (!SetupDiGetDeviceInterfaceDetail(
			hardwareDeviceInfo,
			&deviceInterfaceData,
			deviceInterfaceDetailData,
			predictedLength,
			&requiredLength,
			NULL)) {
			goto Error;
		}

		deviceInfo = (PDEVICE_INFO)calloc(1, sizeof(DEVICE_INFO));
		if (deviceInfo == NULL) {
			goto Error;
		}

		InitializeListHead(&deviceInfo->ListEntry);
		InsertTailList(&ListHead, &deviceInfo->ListEntry);

		//получаем данные об устройстве
		if (!GetDeviceDescription(deviceInterfaceDetailData->DevicePath,
			(PBYTE)deviceInfo->DeviceName,
			sizeof(deviceInfo->DeviceName),
			&deviceInfo->SerialNo)) {
			goto Error;
		}

		printf("%d.Found device : %ws\n", i, deviceInfo->DeviceName);

		hr = StringCchCopy(deviceInfo->DevicePath, MAX_PATH, deviceInterfaceDetailData->DevicePath);
		if (FAILED(hr)) {
			goto Error;
		}
		
		deviceInfo->hDevice = CreateFile(
			deviceInterfaceDetailData->DevicePath,
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL, // нет security структуры
			OPEN_EXISTING, //нет флагов
			0,
			NULL);

		if (INVALID_HANDLE_VALUE == deviceInfo->hDevice) {
			printf("Failed to open the device: %ws\n", deviceInfo->DeviceName);
			continue;
		}

		printf("Opened handled to the device: %ws\n", deviceInfo->DeviceName);

		//регистрируем уведомление об изменениях

		memset(&filter, 0, sizeof(filter));
		filter.dbch_size = sizeof(filter);
		filter.dbch_devicetype = DBT_DEVTYP_HANDLE;
		filter.dbch_handle = deviceInfo->hDevice;

		deviceInfo->hHandleNotification = RegisterDeviceNotification(hWnd, &filter, 0);

	}

	if (deviceInterfaceDetailData)
		free(deviceInterfaceDetailData);

	SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
	return 0;

Error:

	printf("EnumExisting Devices failed\n");
	if (deviceInterfaceDetailData)
		free(deviceInterfaceDetailData);

	SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
	Cleanup(hWnd);
	return 0;
}

//удаляем все данные, отписываемся от всех событий
BOOLEAN Cleanup(
	HWND hWnd
)
{
	PDEVICE_INFO    deviceInfo = NULL;
	PLIST_ENTRY     thisEntry;

	UNREFERENCED_PARAMETER(hWnd);

	while (!IsListEmpty(&ListHead)) {
		thisEntry = RemoveHeadList(&ListHead);
		deviceInfo = CONTAINING_RECORD(thisEntry, DEVICE_INFO, ListEntry);
		if (deviceInfo->hHandleNotification) {
			UnregisterDeviceNotification(deviceInfo->hHandleNotification);
			deviceInfo->hHandleNotification = NULL;
		}
		if (deviceInfo->hDevice != INVALID_HANDLE_VALUE &&
			deviceInfo->hDevice != NULL) {
			CloseHandle(deviceInfo->hDevice);
			deviceInfo->hDevice = INVALID_HANDLE_VALUE;
			printf("Closed handle to device %ws\n", deviceInfo->DeviceName);
		}
		free(deviceInfo);
	}
	return TRUE;
}


BOOL
GetDeviceDescription(
	_In_ LPTSTR DevPath,
	_Out_writes_bytes_(OutBufferLen) PBYTE OutBuffer,
	_In_ ULONG OutBufferLen,
	_In_ PULONG SerialNo
)
{
	HDEVINFO                            hardwareDeviceInfo;
	SP_DEVICE_INTERFACE_DATA            deviceInterfaceData;
	SP_DEVINFO_DATA                     deviceInfoData;
	DWORD                               dwRegType, error;

	hardwareDeviceInfo = SetupDiCreateDeviceInfoList(NULL, NULL);
	if (INVALID_HANDLE_VALUE == hardwareDeviceInfo)
	{
		goto Error;
	}

	deviceInterfaceData.cbSize = sizeof(deviceInterfaceData);

	SetupDiOpenDeviceInterface(hardwareDeviceInfo, DevPath,
		0,
		&deviceInterfaceData);

	deviceInfoData.cbSize = sizeof(deviceInfoData);
	if (!SetupDiGetDeviceInterfaceDetail(
		hardwareDeviceInfo,
		&deviceInterfaceData,
		NULL,
		0,
		NULL,
		&deviceInfoData) && (error = GetLastError()) != ERROR_INSUFFICIENT_BUFFER)
	{
		goto Error;
	}

	if (!SetupDiGetDeviceRegistryProperty(hardwareDeviceInfo, &deviceInfoData,
		SPDRP_FRIENDLYNAME,
		&dwRegType,
		OutBuffer,
		OutBufferLen,
		NULL))
	{
		if (!SetupDiGetDeviceRegistryProperty(hardwareDeviceInfo, &deviceInfoData,
			SPDRP_DEVICEDESC,
			&dwRegType,
			OutBuffer,
			OutBufferLen,
			NULL)) {
			goto Error;

		}


	}

	SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
	return TRUE;

Error:

	SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
	return FALSE;
}