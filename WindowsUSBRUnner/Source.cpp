#include <stdio.h>
#include <windows.h>
#include <string>
#include <iostream>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <tchar.h>

#pragma comment (lib, "setupapi.lib")

#define MAX_PATH 260

char getUSBLetter();

std::string allDrives = { 0 };

int main()
{
    char szDriveLetter = getUSBLetter();
    std::string cmd = "cmd.exe START CMD /C \"";

    STARTUPINFO si = { 0 };
    PROCESS_INFORMATION pi = { 0 };

    si.cb = sizeof(si);

    while (true)
    {
        szDriveLetter = getUSBLetter();
        if (szDriveLetter != '\0')
        {
            std::cout << szDriveLetter << std::endl;

            cmd.push_back(szDriveLetter);
            cmd.append(":\\WindowsAutoRun.bat\"");

            std::wstring wide_string = std::wstring(cmd.begin(), cmd.end());
            wchar_t result[1024] = { 0 };

            wcscat_s(result, wide_string.c_str());

            if (!CreateProcess(NULL, result, NULL, NULL, false, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
            {
                std::cout << "Could not open process " << GetLastError() << std::endl;
            }
        }
        Sleep(1000);
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 0;
}

char getUSBLetter()
{
    HDEVINFO hDevInfo = SetupDiGetClassDevs(NULL, (LPCTSTR)TEXT("USBSTOR"), NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);

    if (hDevInfo == INVALID_HANDLE_VALUE)
        return '\0';

    char drive = { 0 };
    UINT isUSB[] = { 0 };
    UINT memberIndex = 0;
    CONFIGRET status = { 0 };
    TCHAR szLocation[] = { 0 };
    char szLogicalDrives[MAX_PATH] = { 0 };
    SP_DEVINFO_DATA DeviceInfoData = { 0 };
    TCHAR szDeviceInstanceID[MAX_DEVICE_ID_LEN] = { 0 };

    DeviceInfoData.cbSize = sizeof(DeviceInfoData);

    while (SetupDiEnumDeviceInfo(hDevInfo, memberIndex, &DeviceInfoData))
    {
        SetupDiGetDeviceRegistryProperty(hDevInfo, &DeviceInfoData, SPDRP_REMOVAL_POLICY, NULL, (BYTE*)isUSB, sizeof(isUSB), NULL); // Get driver type
        if (*isUSB == CM_REMOVAL_POLICY_EXPECT_SURPRISE_REMOVAL) 
        {
            DWORD dwResult = GetLogicalDriveStringsA(MAX_PATH, szLogicalDrives);

            std::string currentDrives = "";

            for (UINT i = 0; i < dwResult; i++)
            {
                if (szLogicalDrives[i] > 64 && szLogicalDrives[i] < 90)
                {
                    currentDrives.append(1, szLogicalDrives[i]);

                    if (allDrives.find(szLogicalDrives[i]) > 100)
                    {
                        drive = szLogicalDrives[i];
                    }
                }
            }
            allDrives = currentDrives;

            return drive;
        }
        memberIndex++;
    }
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return '\0';
}
