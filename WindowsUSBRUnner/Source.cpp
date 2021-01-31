#include <stdio.h>
#include <windows.h>
#include <string>
#include <iostream>
#include <fstream>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <wchar.h>
#include <vector>
#include <sstream>
#include <filesystem>
#include <algorithm>

#pragma comment (lib, "setupapi.lib")

#define MAX_PATH 260

char getUSBLetter();
void parseFileNamesW(std::wstring sString, std::vector<std::wstring>& vsOut);
wchar_t* getCmdOptionW(wchar_t** begin, wchar_t** end, const std::wstring& option);
bool cmdOptionExists(wchar_t** begin, wchar_t** end, const std::wstring& option);

std::string sAllDrives = { 0 };

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{
    int argc = 0;
    LPWSTR* szArgv = CommandLineToArgvW(GetCommandLineW(), &argc);
    char driveLetter = getUSBLetter();
    const wchar_t wcCmd[] = L"cmd.exe START CMD /C \"";
    bool bNewConsole = false;
    std::vector<std::wstring> vsFullFileNames;
    std::vector<STARTUPINFO> vStartupInfo;
    std::vector<PROCESS_INFORMATION> vProcessInfo;

    if (argc > 1)
    {
        wchar_t* wcFiles = getCmdOptionW(szArgv, szArgv + argc, L"--autorun");
        if(wcFiles)
            parseFileNamesW(std::wstring(wcFiles), vsFullFileNames);

        bNewConsole = cmdOptionExists(szArgv, szArgv + argc, L"--new-console");
    }
    LocalFree(szArgv);

    if (!vsFullFileNames.size())
        vsFullFileNames.push_back(L"WindowsAutoRun.bat");

    while (true)
    {
        driveLetter = getUSBLetter();
        if (driveLetter != '\0')
        {
            for (UINT i = 0; i < vsFullFileNames.size(); i++)
            {
                std::wostringstream ossFullPath;
                bool bFileExists = false;

                ossFullPath << driveLetter << L":\\" << vsFullFileNames[i].c_str();

                bFileExists = std::filesystem::directory_entry(ossFullPath.str()).exists();

                if (!bFileExists)
                    continue;

                STARTUPINFO si = { 0 };
                PROCESS_INFORMATION pi = { 0 };

                vStartupInfo.push_back(si);
                vProcessInfo.push_back(pi);

                si.cb = sizeof(si);

                std::wstring wsTempCmd = wcCmd;

                wsTempCmd.append(ossFullPath.str());
                wsTempCmd.append(L"\"");

                wchar_t wcFinalCmd[1024] = { 0 };
                wcscat_s(wcFinalCmd, wsTempCmd.c_str());

                if (!CreateProcess(NULL, wcFinalCmd, NULL, NULL, false, (bNewConsole) ? CREATE_NEW_CONSOLE : CREATE_NO_WINDOW | DETACHED_PROCESS, NULL, NULL, &vStartupInfo[i], &vProcessInfo[i]))
                {
                    wchar_t msg[] = L"Could not open process for: ";
                    wcscat_s(msg, vsFullFileNames[i].c_str());

                    MessageBox(NULL, msg, L"Process error", MB_OK);
                }
            }
        }
        Sleep(1000);
    }

    for (auto& pi : vProcessInfo)
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
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
        if (uIsUSB[0] == CM_REMOVAL_POLICY_EXPECT_SURPRISE_REMOVAL)
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

void parseFileNamesW(std::wstring wcString, std::vector<std::wstring>& vsOut)
{
    std::wistringstream wcisFiles(wcString);
    std::wstring wcTemp = { 0 };

    while (std::getline(wcisFiles, wcTemp, L','))
        vsOut.push_back(wcTemp);
}

wchar_t* getCmdOptionW(wchar_t** begin, wchar_t** end, const std::wstring& option)
{
    wchar_t** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

bool cmdOptionExists(wchar_t** begin, wchar_t** end, const std::wstring& option)
{
    return std::find(begin, end, option) != end;
}
