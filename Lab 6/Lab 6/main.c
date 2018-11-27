#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <setupapi.h>
#include <dbt.h>
#include <strsafe.h>
#include "deviceDao.h"
#include "eject.h"
#include <dontuse.h>
#include <conio.h>

#define CREATE_WINDOW_EVENT_NAME "windowCreateEvent"

int main(int argc, char* argv[])
{
	//синхронизация для окна
	HANDLE window_create_event = CreateEvent(NULL, FALSE, FALSE, CREATE_WINDOW_EVENT_NAME);
	//хэндлер потока мониторинга(создание окна)
	HANDLE thread_handle_ = CreateThread(NULL, 256, WindowThreadRoutine, (LPVOID)CREATE_WINDOW_EVENT_NAME, NULL, NULL);
	if (thread_handle_ == NULL)
	{
		printf("Couldn't create thread\n");
	}
	//ожидаем создание окна
	WaitForSingleObject(window_create_event, INFINITE);

	printf("\nPress 'f' to eject a USB device\n");
	printf("Press 'j' to exit\n");

	while (TRUE)
	{
		rewind(stdin);

		char key = _getch();

		if (key == 'j') return 0;
		if (key != 'f') continue;

		char drive_letter = 0;

		printf("Please enter volume's letter, you want to eject\n");

		if (!scanf_s("%c", &drive_letter))
		{
			printf("Invalid drive letter value was provided!\n");
			continue;
		}
		//если есть запрет на извлечение
		if (EjectDevice(drive_letter) == 1)
		{
			printf("Couldn't eject the drive\n");
		}
	}

	return 0;
}