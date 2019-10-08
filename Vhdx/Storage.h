#pragma once

DWORD
GetVirtualDiskInformation(
	_In_    LPCWSTR     VirtualDiskPath);

DWORD
CreateVirtualDisk(
	_In_        LPCWSTR                     VirtualDiskPath,
	_In_opt_    LPCWSTR                     ParentPath,
	_In_        CREATE_VIRTUAL_DISK_FLAG    Flags,
	_In_        ULONGLONG                   FileSize,
	_In_        DWORD                       BlockSize,
	_In_        DWORD                       LogicalSectorSize,
	_In_        DWORD                       PhysicalSectorSize);


DWORD
GetAllAttachedVirtualDiskPhysicalPaths(
);


DWORD
AttachVirtualDisk(
	_In_    LPCWSTR     VirtualDiskPath,
	_In_    BOOLEAN     ReadOnly);

DWORD
DetachVirtualDisk(
	_In_    LPCWSTR     VirtualDiskPath);


