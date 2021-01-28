#include <stdio.h>
#include <windows.h>
#include <string>
#include <iostream>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <tchar.h>
#include <vector>
#include <sstream>
#include <filesystem>

#pragma comment (lib, "setupapi.lib")

#define MAX_PATH 260

char getUSBLetter();
void splitString(std::string sString, std::vector<std::string>& vsOut);

std::string sAllDrives = { 0 };

int main(int argc, char** argv)
{
    char driveLetter = getUSBLetter();
    const char szCmd[] = "cmd.exe START CMD /C \"";
    std::vector<std::string> vsFullFileNames;
    
    STARTUPINFO si = { 0 };
    PROCESS_INFORMATION pi = { 0 };

    si.cb = sizeof(si);

    if (argc > 1)
    {
        for (int i = 0; i < argc; i++)
        {
            std::string sArgv = std::string(argv[i]);

            if (sArgv == "--autorun")
            {
                if (argv[i + 1])
                    splitString(std::string(argv[i + 1]), vsFullFileNames);
                else
                {
                    std::cerr << "Error: Expected more parameters" << std::endl;
                    return 0;
                }  
            }
        }
    }

    if (!vsFullFileNames.size())
        vsFullFileNames.push_back("WindowsAutoRun.bat");

    while (true)
    {
        driveLetter = getUSBLetter();
        if (driveLetter != '\0')
        {
            for (UINT i = 0; i < vsFullFileNames.size(); i++)
            {
                std::ostringstream ossBaseDir;
                std::ostringstream ossFullPath;
                bool bFileExists = false;

                ossBaseDir << driveLetter << ":\\";
                ossFullPath << ossBaseDir.str() << vsFullFileNames[i];

                for (const auto& entry : std::filesystem::directory_iterator(ossBaseDir.str()))
                    if (entry.path() == ossFullPath.str())
                        bFileExists = true;

                if (!bFileExists)
                    continue;

                std::string sTempCmd = szCmd;

                sTempCmd.append(ossFullPath.str());
                sTempCmd.append("\"");

                std::wstring wcString = std::wstring(sTempCmd.begin(), sTempCmd.end());
                wchar_t wcFinalCmd[1024] = { 0 };

                wcscat_s(wcFinalCmd, wcString.c_str());

                if (!CreateProcess(NULL, wcFinalCmd, NULL, NULL, false, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
                    std::cerr << "Could not open process for: " << vsFullFileNames[i] << "\nError: " << GetLastError() << std::endl;
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
    UINT uIsUSB[] = { 0 };
    UINT uMemberIndex = 0;
    char szLogicalDrives[MAX_PATH] = { 0 };
    SP_DEVINFO_DATA DeviceInfoData = { 0 };

    DeviceInfoData.cbSize = sizeof(DeviceInfoData);

    while (SetupDiEnumDeviceInfo(hDevInfo, uMemberIndex, &DeviceInfoData))
    {
        SetupDiGetDeviceRegistryProperty(hDevInfo, &DeviceInfoData, SPDRP_REMOVAL_POLICY, NULL, (BYTE*)uIsUSB, sizeof(uIsUSB), NULL); // Get driver type
        if (*uIsUSB == CM_REMOVAL_POLICY_EXPECT_SURPRISE_REMOVAL)
        {
            DWORD dwResult = GetLogicalDriveStringsA(MAX_PATH, szLogicalDrives);

            std::string sCurrentDrives = "";

            for (UINT i = 0; i < dwResult; i++)
            {
                if (szLogicalDrives[i] > 64 && szLogicalDrives[i] < 90)
                {
                    sCurrentDrives.append(1, szLogicalDrives[i]);

                    if (sAllDrives.find(szLogicalDrives[i]) > 100)
                        drive = szLogicalDrives[i];
                }
            }
            sAllDrives = sCurrentDrives;

            return drive;
        }
        uMemberIndex++;
    }
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return '\0';
}

void splitString(std::string sString, std::vector<std::string>& vsOut)
{
    std::istringstream issExtensions(sString);
    std::string sTemp = { 0 };

    while (std::getline(issExtensions, sTemp, ','))
        vsOut.push_back(sTemp);
}
