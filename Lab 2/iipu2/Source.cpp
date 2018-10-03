#include <iostream>
#include <iomanip>
#include <Windows.h>
#include <WinIoCtl.h>
#include <ntddscsi.h>
#include <conio.h>

using namespace std;

#define bThousand 1024
#define Hundred 100
#define BYTE_SIZE 8

string busType[] = { "UNKNOWN", "SCSI", "ATAPI", "ATA", "ONE_TREE_NINE_FOUR", "SSA", "FIBRE", "USB", "RAID", "ISCSI", "SAS", "SATA", "SD", "MMC" };

void getDeviceInfo(HANDLE diskHandle, STORAGE_PROPERTY_QUERY storageProtertyQuery) {

	STORAGE_DEVICE_DESCRIPTOR* deviceDescriptor = (STORAGE_DEVICE_DESCRIPTOR*)calloc(bThousand, 1);

	deviceDescriptor->Size = bThousand;

	if (!DeviceIoControl(diskHandle,
		IOCTL_STORAGE_QUERY_PROPERTY,				//���������� ������ �� ������� ������� ����������. 
		&storageProtertyQuery,						//��������� �� ����� ������
		sizeof(storageProtertyQuery),				//������ �������� ������
		deviceDescriptor,							//��������� �� �������� �����
		bThousand,									//������ ��������� ������
		NULL,										
		0)) {
		printf("%d", GetLastError());
		CloseHandle(diskHandle);
		exit(-1);
	}

	cout << "Model : " << setw(34) << (char*)(deviceDescriptor)+deviceDescriptor->ProductIdOffset << endl;
	cout << "Version : " << setw(20) << (char*)(deviceDescriptor)+deviceDescriptor->ProductRevisionOffset << endl;
	cout << "Bus type : " << setw(15) << busType[deviceDescriptor->BusType].c_str() << endl;
	cout << "Serial number : " << (char*)(deviceDescriptor)+deviceDescriptor->SerialNumberOffset << endl;
}

void getMemoryInfo() {
	string path;
	_ULARGE_INTEGER diskSpace;
	_ULARGE_INTEGER freeSpace;

	diskSpace.QuadPart = 0;
	freeSpace.QuadPart = 0;

	_ULARGE_INTEGER totalDiskSpace;
	_ULARGE_INTEGER totalFreeSpace;

	totalDiskSpace.QuadPart = 0;
	totalFreeSpace.QuadPart = 0;

	//�������� ������� �����, �������������� ��������� � ��������� ����� �������� ����������. 
	unsigned long int logicalDrivesCount = GetLogicalDrives();

	//������ ���������� ������� �����(��� 0 - ���� �, ��� 1 - ���� B). 
	for (char var = 'A'; var < 'Z'; var++) {
		if ((logicalDrivesCount >> var - 65) & 1 && var != 'F') {
			path = var;
			path.append(":\\"); 
			GetDiskFreeSpaceEx(path.c_str(), 0, &diskSpace, &freeSpace);
			diskSpace.QuadPart = diskSpace.QuadPart / (bThousand * bThousand);
			freeSpace.QuadPart = freeSpace.QuadPart / (bThousand * bThousand);

			//���������� ��� �����(3 - ������� ����) 
			if (GetDriveType(path.c_str()) == 3) {
				totalDiskSpace.QuadPart += diskSpace.QuadPart;
				totalFreeSpace.QuadPart += freeSpace.QuadPart;
			}
		}
	}

	cout << endl << "Total space[Mb] : " << totalDiskSpace.QuadPart << endl;
	cout << "Free space[Mb] : " << totalFreeSpace.QuadPart << endl;
	cout << "Busy space[Mb] : " << totalDiskSpace.QuadPart - totalFreeSpace.QuadPart << endl;
	cout << "Busy space[%] : " << 100.0 - (double)totalFreeSpace.QuadPart / (double)totalDiskSpace.QuadPart * Hundred << endl << endl;
}

void getAtaPioDmaSupportStandarts(HANDLE diskHandle) {

	UCHAR identifyDataBuffer[512 + sizeof(ATA_PASS_THROUGH_EX)] = { 0 };

	ATA_PASS_THROUGH_EX &PTE = *(ATA_PASS_THROUGH_EX *)identifyDataBuffer;	//��������� ��� �������� ��� ������� ���������� 
	PTE.Length = sizeof(PTE);
	PTE.TimeOutValue = 10;									//������ ��������� 
	PTE.DataTransferLength = 512;							//������ ������ ��� ������ 
	PTE.DataBufferOffset = sizeof(ATA_PASS_THROUGH_EX);		//�������� � ������ �� ������ ��������� �� ������ ������ 
	PTE.AtaFlags = ATA_FLAGS_DATA_IN;						//����, ��������� � ������ ������ �� ���������� 

	IDEREGS *ideRegs = (IDEREGS *)PTE.CurrentTaskFile;
	ideRegs->bCommandReg = 0xEC;

	//���������� ������ ���������� 
	if (!DeviceIoControl(diskHandle,
		IOCTL_ATA_PASS_THROUGH,								//�������� ��������� � ��������� ���� ATA_PASS_THROUGH_EX
		&PTE, 
		sizeof(identifyDataBuffer), 
		&PTE, 
		sizeof(identifyDataBuffer), 
		NULL,
		NULL)) {
		cout << GetLastError() << std::endl;
		return;
	}
	//�������� ��������� �� ������ ���������� ������ 
	WORD *data = (WORD *)(identifyDataBuffer + sizeof(ATA_PASS_THROUGH_EX));	
	short ataSupportByte = data[80];
	int i = 2 * BYTE_SIZE;
	int bitArray[2 * BYTE_SIZE];
	//���������� ����� � ����������� � ��������� ATA � ������ ��� 
	while (i--) {
		bitArray[i] = ataSupportByte & 32768 ? 1 : 0;
		ataSupportByte = ataSupportByte << 1;
	}

	//����������� ���������� ������ ���. 
	cout << "ATA Support : ";
	for (int i = 8; i >= 4; i--) {
		if (bitArray[i] == 1) {
			cout << "ATA" << i;
			if (i != 4) {
				cout << ", ";
			}
		}
	}
	cout << endl;

	//����� �������������� ������� DMA 
	unsigned short dmaSupportedBytes = data[63];
	int i2 = 2 * BYTE_SIZE;
	//���������� ����� � ����������� � ��������� DMA � ������ ���
	while (i2--) {
		bitArray[i2] = dmaSupportedBytes & 32768 ? 1 : 0;
		dmaSupportedBytes = dmaSupportedBytes << 1;
	}

	//����������� ���������� ������ ���. 
	cout << "DMA Support: ";
	for (int i = 0; i < 8; i++) {
		if (bitArray[i] == 1) {
			cout << "DMA" << i;
			if (i != 2) cout << ", ";
		}
	}
	cout << endl;

	unsigned short pioSupportedBytes = data[64];
	int i3 = 2 * BYTE_SIZE;
	//���������� ����� � ����������� � ��������� PIO � ������ ��� 
	while (i3--) {
		bitArray[i3] = pioSupportedBytes & 32768 ? 1 : 0;
		pioSupportedBytes = pioSupportedBytes << 1;
	}

	//����������� ���������� ������ ���. 
	cout << "PIO Support: ";
	for (int i = 0; i < 2; i++) {
		if (bitArray[i] == 1) {
			cout << "PIO" << i + 3;
			if (i != 1) cout << ", ";
		}
	}
	cout << endl;
}

void getMemoryTransferMode(HANDLE diskHandle, STORAGE_PROPERTY_QUERY storageProtertyQuery) {
	STORAGE_ADAPTER_DESCRIPTOR adapterDescriptor;			//��������� �� ���������� ���������� 
	if (!DeviceIoControl(diskHandle,
		IOCTL_STORAGE_QUERY_PROPERTY,						//���������� ������ �� ������� ������� ����������. 
		&storageProtertyQuery,
		sizeof(storageProtertyQuery),
		&adapterDescriptor,
		sizeof(STORAGE_DESCRIPTOR_HEADER),
		NULL,
		NULL)) {
		cout << GetLastError();
		exit(-1);
	}
	else {
		//����� ������ ������� � ������
		cout << "Transfer mode: ";
		adapterDescriptor.AdapterUsesPio ? cout << "PIO" : cout << "DMA";
		cout << endl;
	}
}

void init(HANDLE& diskHandle) {
	//�������� ����� � ����������� � ����� 
	diskHandle = CreateFile("//./PhysicalDrive0", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (diskHandle == INVALID_HANDLE_VALUE) {
		cout << "Look at this dude";
		_getch();
		return;
	}
}

int main()
{
	STORAGE_PROPERTY_QUERY storagePropertyQuery;				//��������� � ����������� �� ������� 
	storagePropertyQuery.QueryType = PropertyStandardQuery;		//������ ��������, ����� �� ������ ���������� ����������. 
	storagePropertyQuery.PropertyId = StorageDeviceProperty;	//����, ��������� �� ����� �������� ���������� ����������. 
	HANDLE diskHandle;

	init(diskHandle);
	getDeviceInfo(diskHandle, storagePropertyQuery);
	getMemoryInfo();
	getAtaPioDmaSupportStandarts(diskHandle);
	getMemoryTransferMode(diskHandle, storagePropertyQuery);
	_getch();
	return 0;
}