#define _CRT_SECURE_NO_WARNINGS

/*
#include <iostream>
#include<Windows.h>
#include<string>
#pragma comment(lib, "urlmon.lib")

using namespace std;
int main()
{
    string dwnld_URL = "http://www.url.com/downloadpage/filename.txt";
    string savepath = "C:\\tmp\\filename.txt";
    HRESULT hr;

    hr = URLDownloadToFile(NULL, dwnld_URL.c_str(), savepath.c_str(), 0, NULL);
    
    switch (hr)
    {
        case S_OK:
            std::cout << "Successful download" << std::endl;
            break;
        case E_OUTOFMEMORY:
            std::cout << "Out of memory error\n";
            break;
        case INET_E_DOWNLOAD_FAILURE:
            std::cout << "Cannot access server data\n";
            break;
        default:
            std::cout << "Unknown error";
            break;
    }

    return 0;
}
*/

#include <iostream>
#include <Windows.h>

#include <fstream>
#include <chrono>
#include <vector>
#include <cstdint>
#include <numeric>
#include <random>
#include <algorithm>
#include <cassert>

#include <limits>

std::vector<uint64_t> GenerateData(std::size_t bytes)
{
    assert(bytes % sizeof(uint64_t) == 0);
    std::vector<uint64_t> data(bytes / sizeof(uint64_t));
    std::iota(data.begin(), data.end(), 0);
    std::shuffle(data.begin(), data.end(), std::mt19937{ std::random_device{}() });
    return data;
}

void ReadFullWrite(std::wstring Read_Path, std::wstring Write_Path)
{
    BOOL bRet;

    DWORD FileSizeLow = 0, FileSizeHigh = 0;
    DWORD NumberOfBytesRead = 0;
    DWORD NumberOfBytesWritten = 0;
    LARGE_INTEGER FileSizeLarger = { 0 };
    ULONGLONG MaxFileSize = 0;
    ULONGLONG MaxFileSize_Current = 0;

    std::vector<BYTE> FileData;

    /*
    HANDLE CreateFileA(
      LPCSTR                lpFileName,				0019F870 -> PtrStr
      DWORD                 dwDesiredAccess,		C0000000
      DWORD                 dwShareMode,			00000003
      LPSECURITY_ATTRIBUTES lpSecurityAttributes,	0019F830 -> 0000000C
      DWORD                 dwCreationDisposition,	00000004
      DWORD                 dwFlagsAndAttributes,	00000080
      HANDLE                hTemplateFile			00000000
    );
    */
    HANDLE hFile = CreateFile(Read_Path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        std::cout << "CANNOT OPEN FILE" << std::endl;
        goto _outa;
    }

    FileSizeLow = GetFileSize(hFile, &FileSizeHigh);

    std::cout << "FileSizeLow: " << FileSizeLow << std::endl;
    std::cout << "FileSizeHigh: " << FileSizeHigh << std::endl;

    //Larger than 4GB (4 bytes size)
    if (FileSizeHigh)
    {
        bRet = GetFileSizeEx(hFile, &FileSizeLarger);

        if (!bRet)
        {
            std::cout << "COULD NOT GET FILE SIZE EX > 4 GB" << std::endl;
            
            goto _outa;
        }

        MaxFileSize = FileSizeLarger.QuadPart;

        std::cout << "FileSizeLarger: " << MaxFileSize << std::endl;
    }
    else
    {
        MaxFileSize = FileSizeLow;
    }

    FileData.resize(MaxFileSize);

    SetFilePointer(hFile, NULL, NULL, FILE_BEGIN);

    if (FileSizeHigh) FileSizeLow = (std::numeric_limits<unsigned int>::max)();
    MaxFileSize_Current = 0;

    while (MaxFileSize != MaxFileSize_Current)
    {
        bRet = ReadFile(hFile, &FileData[MaxFileSize_Current], FileSizeLow, &NumberOfBytesRead, NULL);

        if (!bRet)
        {
            std::cout << "COULD NOT READ THE FILE" << std::endl;
            break;
        }

        MaxFileSize_Current += NumberOfBytesRead;

        //Is file larger than 4GB? (32b)
        if (MaxFileSize > (std::numeric_limits<unsigned int>::max)())
        {
            //Remaining
            if (MaxFileSize - MaxFileSize_Current <= (std::numeric_limits<unsigned int>::max)())
            {
                FileSizeLow = (DWORD)(MaxFileSize - MaxFileSize_Current);
            }
        }
    }

    if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);

    hFile = CreateFile(Write_Path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        std::cout << "CANNOT OPEN/CREATE FILE" << std::endl;
        goto _outa;
    }

    if (FileSizeHigh)
    {
        FileSizeLow = (std::numeric_limits<unsigned int>::max)();
    }
    else
    {
        FileSizeLow = GetFileSize(hFile, &FileSizeHigh);
    }

    MaxFileSize_Current = 0;

    while (MaxFileSize != MaxFileSize_Current)
    {
        bRet = WriteFile(hFile, &FileData[MaxFileSize_Current], FileSizeLow, &NumberOfBytesWritten, NULL);

        if (!bRet)
        {
            std::cout << "COULD NOT WRITE THE FILE" << std::endl;
            break;
        }

        MaxFileSize_Current += NumberOfBytesWritten;

        //Is file larger than 4GB? (32b)
        if (MaxFileSize > (std::numeric_limits<unsigned int>::max)())
        {
            //Remaining
            if (MaxFileSize - MaxFileSize_Current <= (std::numeric_limits<unsigned int>::max)())
            {
                FileSizeLow = (DWORD)(MaxFileSize - MaxFileSize_Current);
            }
        }
    }

_outa:
    if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);

    FileData.clear();
    FileData.shrink_to_fit();
}

void ReadMBWrite(std::wstring Read_Path, std::wstring Write_Path, DWORD MegaBytes)
{
    BOOL bRet;

    DWORD FileSizeLow = 0, FileSizeHigh = 0;
    DWORD NumberOfBytesRead = 0;
    DWORD NumberOfBytesReadTmp = 0;
    DWORD NumberOfBytesWritten = 0;
    LARGE_INTEGER FileSizeLarger = { 0 };
    ULONGLONG MaxFileSize = 0;
    ULONGLONG MaxFileSize_Current = 0;

    std::vector<BYTE> FileData;

    HANDLE hFileRead, hFileWrite;

    /*
    HANDLE CreateFileA(
      LPCSTR                lpFileName,				0019F870 -> PtrStr
      DWORD                 dwDesiredAccess,		C0000000
      DWORD                 dwShareMode,			00000003
      LPSECURITY_ATTRIBUTES lpSecurityAttributes,	0019F830 -> 0000000C
      DWORD                 dwCreationDisposition,	00000004
      DWORD                 dwFlagsAndAttributes,	00000080
      HANDLE                hTemplateFile			00000000
    );
    */
    hFileRead = CreateFile(Read_Path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    hFileWrite = CreateFile(Write_Path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFileRead == INVALID_HANDLE_VALUE || hFileWrite == INVALID_HANDLE_VALUE)
    {
        std::cout << "CANNOT OPEN FILE" << std::endl;
        goto _outa;
    }

    FileSizeLow = GetFileSize(hFileRead, &FileSizeHigh);

    std::cout << "FileSizeLow: " << FileSizeLow << std::endl;
    std::cout << "FileSizeHigh: " << FileSizeHigh << std::endl;

    //Larger than 4GB (4 bytes size)
    if (FileSizeHigh)
    {
        bRet = GetFileSizeEx(hFileRead, &FileSizeLarger);

        if (!bRet)
        {
            std::cout << "COULD NOT GET FILE SIZE EX > 4 GB" << std::endl;

            goto _outa;
        }

        MaxFileSize = FileSizeLarger.QuadPart;

        std::cout << "FileSizeLarger: " << MaxFileSize << std::endl;
    }
    else
    {
        MaxFileSize = FileSizeLow;
    }

    MegaBytes *= 1024;

    FileData.resize(MegaBytes);

    SetFilePointer(hFileRead, NULL, NULL, FILE_BEGIN);
    SetFilePointer(hFileWrite, NULL, NULL, FILE_BEGIN);

    MaxFileSize_Current = 0;

    while (MaxFileSize != MaxFileSize_Current)
    {
        bRet = ReadFile(hFileRead, &FileData[0], (DWORD)FileData.size(), &NumberOfBytesRead, NULL);

        if (!bRet)
        {
            std::cout << "COULD NOT READ THE FILE" << std::endl;
            break;
        }
        
        MaxFileSize_Current += NumberOfBytesRead;

        NumberOfBytesReadTmp = NumberOfBytesRead;

        /*
        &FileData[NumberOfBytesReadTmp(1000) - NumberOfBytesRead(1000)] => &FileData[0]
        NumberOfBytesWritten = 50

        NumberOfBytesRead(1000) -= NumberOfBytesWritten(50);
        NumberOfBytesRead = 950

        ------------------

        &FileData[NumberOfBytesReadTmp(1000) - NumberOfBytesRead(950)] => &FileData[50]
        NumberOfBytesWritten = 800

        NumberOfBytesRead(950) -= NumberOfBytesWritten(800);
        NumberOfBytesRead = 150

        ------------------

        &FileData[NumberOfBytesReadTmp(1000) - NumberOfBytesRead(150)] => &FileData[850]
        NumberOfBytesWritten = 150

        NumberOfBytesRead(150) -= NumberOfBytesWritten(150);
        NumberOfBytesRead = 0
        */

        while (NumberOfBytesRead != 0)
        {
            bRet = WriteFile(hFileWrite, &FileData[NumberOfBytesReadTmp - NumberOfBytesRead], NumberOfBytesRead, &NumberOfBytesWritten, NULL);

            if (!bRet)
            {
                std::cout << "COULD NOT WRITE THE FILE" << std::endl;
                break;
            }

            NumberOfBytesRead -= NumberOfBytesWritten;
        }
    }

_outa:
    if (hFileRead != INVALID_HANDLE_VALUE) CloseHandle(hFileRead);
    if (hFileWrite != INVALID_HANDLE_VALUE) CloseHandle(hFileWrite);

    FileData.clear();
    FileData.shrink_to_fit();
}

int main()
{
    std::wstring FileNameSave = L"";
    std::wstring FileName = L"";

    //
    //FileName = L"E:\\[Movies]\\X-Men Apocalypse (2016) [1080p] [YTS.AG]\\X-Men.Apocalypse.2016.1080p.BluRay.x264-[YTS.AG] Apocalypse2.mp4";
    //FileNameSave = L"E:\\[Movies]\\X-Men Apocalypse (2016) [1080p] [YTS.AG]\\OutPut.mp4";

    //Larger 4GB
    FileName = L"E:\\[Movies]\\[NEW]\\Le temes a la oscuridad 1992-2000\\S1\\1x04.Latino.dcc_1.50x_1280x720_alq-10.mov";
    FileNameSave = L"E:\\[Movies]\\[NEW]\\Le temes a la oscuridad 1992-2000\\S1\\OutPut.mov";

    //ReadFullWrite(FileName, FileNameSave);
    ReadMBWrite(FileName, FileNameSave, 1024);

    std::cin.get();

    return 0;
}