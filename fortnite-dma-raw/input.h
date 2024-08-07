#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <cmath>
#include <thread>

namespace kmbox
{
    inline HANDLE serial_handle = nullptr;

    inline bool connected = false;

    inline int clamp(int i)
    {
        if (i > 127)
            i = 127;
        if (i < -128)
            i = -128;

        return i;
    }

    inline void move_x(int pixels)
    {
        if (!connected)
            return;

        int x = clamp(pixels);

        char data[100] = { '\0' };
        sprintf(data, "km.move(%d, 0)\n", x);

        DWORD bytes_written;
        WriteFile(serial_handle, data, strlen(data), &bytes_written, 0);

        const BYTE enter_key = 0x0D;
        WriteFile(serial_handle, &enter_key, sizeof(enter_key), &bytes_written, nullptr);
    }

    inline void move_y(int pixels)
    {
        if (!connected)
            return;

        int y = clamp(-pixels);

        char data[100];
        sprintf(data, "km.move(0, %d)\n", y);

        DWORD bytes_written;
        WriteFile(serial_handle, data, strlen(data), &bytes_written, 0);

        const BYTE enter_key = 0x0D;
        WriteFile(serial_handle, &enter_key, sizeof(enter_key), &bytes_written, nullptr);
    }

    inline void left_click()
    {
        if (!connected)
            return;

        char data[100] = { '\0' };
        sprintf(data, "km.left(1)\n");

        DWORD bytes_written;
        WriteFile(serial_handle, data, strlen(data), &bytes_written, 0);

        const BYTE enter_key = 0x0D;
        WriteFile(serial_handle, &enter_key, sizeof(enter_key), &bytes_written, nullptr);
    }

    inline void release_left_click()
    {
        if (!connected)
            return;

        char data[100] = { '\0' };
        sprintf(data, "km.left(0)\n");

        DWORD bytes_written;
        WriteFile(serial_handle, data, strlen(data), &bytes_written, 0);

        const BYTE enter_key = 0x0D;
        WriteFile(serial_handle, &enter_key, sizeof(enter_key), &bytes_written, nullptr);
    }

    inline void initialize(LPCSTR port)
    {
        connected = false;

        serial_handle = CreateFileA(port, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (serial_handle == INVALID_HANDLE_VALUE)
        {
            printf("serial handle is invalid\n");

            return;
        }

        DCB dcbSerialParams = { 0 };
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
        if (!GetCommState(serial_handle, &dcbSerialParams))
        {
            printf("failed to get communication state\n");

            CloseHandle(serial_handle);

            return;
        }

        dcbSerialParams.BaudRate = 115200;
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.Parity = NOPARITY;
        dcbSerialParams.StopBits = ONESTOPBIT;

        if (!SetCommState(serial_handle, &dcbSerialParams))
        {
            printf("failed to set communication state\n");

            CloseHandle(serial_handle);

            return;
        }

        connected = true;
    }
}