// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "stdafx.h"

void
ShowUsage(
	_In_    PWCHAR  ModuleName);

int __cdecl wmain(_In_ int argc, _In_reads_(argc) WCHAR *argv[])
{
	DWORD rc = 1;

	if (argc == 2)
	{
		if (_wcsicmp(argv[1], L"GetAllAttachedVirtualDiskPhysicalPaths") == 0)
		{
			rc = GetAllAttachedVirtualDiskPhysicalPaths();
		}
		else
		{
			ShowUsage(argv[0]);
		}
	}
	else if (argc == 3)
	{
		if (_wcsicmp(argv[1], L"GetVirtualDiskInformation") == 0)
		{
			LPCWSTR virtualDiskPath = argv[2];

			rc = GetVirtualDiskInformation(virtualDiskPath);
		}
	}

	else if (argc == 4)
	{
		if (_wcsicmp(argv[1], L"AttachVirtualDisk") == 0)
		{
			LPCWSTR virtualDiskPath = argv[2];
			LPCWSTR readOnly = argv[3];

			rc = AttachVirtualDisk(
				virtualDiskPath,
				(readOnly[0] == 't' || readOnly[0] == 'T'));
		}
	}
	else if (argc == 7)
	{
		if (_wcsicmp(argv[1], L"CreateFixedVirtualDisk") == 0)
		{
			LPCWSTR virtualDiskPath = argv[2];
			LPCWSTR parentPath = NULL;

			ULONGLONG fileSize = _wtoi64(argv[3]);
			DWORD blockSize = _wtoi(argv[4]);
			DWORD logicalSectorSize = _wtoi(argv[5]);
			DWORD physicalSectorSize = _wtoi(argv[6]);

			rc = CreateVirtualDisk(
				virtualDiskPath,
				parentPath,
				CREATE_VIRTUAL_DISK_FLAG_FULL_PHYSICAL_ALLOCATION,
				fileSize,
				blockSize,
				logicalSectorSize,
				physicalSectorSize);
		}
		else if (_wcsicmp(argv[1], L"CreateDynamicVirtualDisk") == 0)
		{
			LPCWSTR virtualDiskPath = argv[2];
			LPCWSTR parentPath = NULL;

			ULONGLONG fileSize = _wtoi64(argv[3]);
			DWORD blockSize = _wtoi(argv[4]);
			DWORD logicalSectorSize = _wtoi(argv[5]);
			DWORD physicalSectorSize = _wtoi(argv[6]);

			rc = CreateVirtualDisk(
				virtualDiskPath,
				parentPath,
				CREATE_VIRTUAL_DISK_FLAG_NONE,
				fileSize,
				blockSize,
				logicalSectorSize,
				physicalSectorSize);
		}
		else
		{
			ShowUsage(argv[0]);
		}
	}
	else
	{
		ShowUsage(L"");
	}

	return rc;
}

void
ShowUsage(
	_In_    PWCHAR  ModuleName)
{
	wprintf(L"\nUsage:\t%s <SampleName> <Arguments>\n", ModuleName);

	wprintf(L"Supported SampleNames and Arguments:\n");
	wprintf(L"   GetVirtualDiskInformation <path>\n");
	wprintf(L"   CreateFixedVirtualDisk <path> <file size> <block size> <logical sector size> <physical sector size>\n");
	wprintf(L"   CreateDynamicVirtualDisk <path> <file size> <block size> <logical sector size> <physical sector size>\n");
	wprintf(L"   AttachVirtualDisk <path> <readonly>\n");
	wprintf(L"   DetachVirtualDisk <path>\n");
	wprintf(L"   GetAllAttachedVirtualDiskPhysicalPaths\n");


	wprintf(L"\nExamples:\n");
	wprintf(L"   %s GetVirtualDiskInformation c:\\fixed.vhd\n", ModuleName);
	wprintf(L"   %s CreateFixedVirtualDisk c:\\fixed.vhd 1073741824 0 0 0\n", ModuleName);
	wprintf(L"   %s CreateDynamicVirtualDisk c:\\dynamic.vhdx 1073741824 0 0 0\n", ModuleName);
	wprintf(L"   %s AttachVirtualDisk c:\\fixed.vhd true\n", ModuleName);
	wprintf(L"   %s DetachVirtualDisk c:\\fixed.vhd\n", ModuleName);
	wprintf(L"   %s GetAllAttachedVirtualDiskPhysicalPaths\n", ModuleName);

	wprintf(L"\n\n");
}

