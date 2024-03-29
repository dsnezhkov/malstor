
#include "stdafx.h"
DWORD
AttachVirtualDisk(
	_In_    LPCWSTR     VirtualDiskPath,
	_In_    BOOLEAN     ReadOnly)
{
	OPEN_VIRTUAL_DISK_PARAMETERS openParameters;
	VIRTUAL_DISK_ACCESS_MASK accessMask;
	ATTACH_VIRTUAL_DISK_PARAMETERS attachParameters;
	PSECURITY_DESCRIPTOR sd;
	VIRTUAL_STORAGE_TYPE storageType;
	LPCTSTR extension;
	HANDLE vhdHandle;
	ATTACH_VIRTUAL_DISK_FLAG attachFlags;
	DWORD opStatus;

	vhdHandle = INVALID_HANDLE_VALUE;
	sd = NULL;

	//
	// Specify UNKNOWN for both device and vendor so the system will use the
	// file extension to determine the correct VHD format.
	//

	storageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_UNKNOWN;
	storageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_UNKNOWN;
	//storageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT;

	memset(&openParameters, 0, sizeof(openParameters));

	extension = ::PathFindExtension(VirtualDiskPath);

	if (extension != NULL && _wcsicmp(extension, L".iso") == 0)
	{
		//
		// ISO files can only be mounted read-only and using the V1 API.
		//

		if (ReadOnly != TRUE)
		{
			opStatus = ERROR_NOT_SUPPORTED;
			goto Cleanup;
		}

		openParameters.Version = OPEN_VIRTUAL_DISK_VERSION_1;
		accessMask = VIRTUAL_DISK_ACCESS_READ;
	}
	else
	{
		//
		// VIRTUAL_DISK_ACCESS_NONE is the only acceptable access mask for V2 handle opens.
		//

		openParameters.Version = OPEN_VIRTUAL_DISK_VERSION_2;
		openParameters.Version2.GetInfoOnly = FALSE;
		accessMask = VIRTUAL_DISK_ACCESS_NONE;
	}

	//
	// Open the VHD or ISO.
	//
	// OPEN_VIRTUAL_DISK_FLAG_NONE bypasses any special handling of the open.
	//

	opStatus = OpenVirtualDisk(
		&storageType,
		VirtualDiskPath,
		accessMask,
		OPEN_VIRTUAL_DISK_FLAG_NONE,
		&openParameters,
		&vhdHandle);

	if (opStatus != ERROR_SUCCESS)
	{
		goto Cleanup;
	}

	//
	// Create the world-RW SD.
	//

	if (!::ConvertStringSecurityDescriptorToSecurityDescriptor(
		L"O:BAG:BAD:(A;;GA;;;WD)",
		SDDL_REVISION_1,
		&sd,
		NULL))
	{
		opStatus = ::GetLastError();
		goto Cleanup;
	}

	//
	// Attach the VHD/VHDX or ISO.
	//

	memset(&attachParameters, 0, sizeof(attachParameters));
	attachParameters.Version = ATTACH_VIRTUAL_DISK_VERSION_1;

	//
	// A "Permanent" surface persists even when the handle is closed.
	//

	attachFlags = ATTACH_VIRTUAL_DISK_FLAG_PERMANENT_LIFETIME;

	if (ReadOnly)
	{
		// ATTACH_VIRTUAL_DISK_FLAG_READ_ONLY specifies a read-only mount.
		attachFlags |= ATTACH_VIRTUAL_DISK_FLAG_READ_ONLY;
	}

	opStatus = AttachVirtualDisk(
		vhdHandle,
		sd,
		attachFlags,
		0,
		&attachParameters,
		NULL);

	if (opStatus != ERROR_SUCCESS)
	{
		goto Cleanup;
	}

Cleanup:

	if (opStatus == ERROR_SUCCESS)
	{
		wprintf(L"success\n");
	}
	else
	{
		wprintf(L"error = %u\n", opStatus);
	}

	if (sd != NULL)
	{
		LocalFree(sd);
		sd = NULL;
	}

	if (vhdHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(vhdHandle);
	}

	return opStatus;
}

DWORD
CreateVirtualDisk(
	_In_        LPCWSTR                     VirtualDiskPath,
	_In_opt_    LPCWSTR                     ParentPath,
	_In_        CREATE_VIRTUAL_DISK_FLAG    Flags,
	_In_        ULONGLONG                   FileSize,
	_In_        DWORD                       BlockSize,
	_In_        DWORD                       LogicalSectorSize,
	_In_        DWORD                       PhysicalSectorSize)
{
	VIRTUAL_STORAGE_TYPE storageType;
	CREATE_VIRTUAL_DISK_PARAMETERS parameters;
	HANDLE vhdHandle = INVALID_HANDLE_VALUE;
	DWORD opStatus;
	GUID uniqueId;

	if (RPC_S_OK != UuidCreate((UUID*)&uniqueId))
	{
		opStatus = ERROR_NOT_ENOUGH_MEMORY;
		goto Cleanup;
	}

	//
	// Specify UNKNOWN for both device and vendor so the system will use the
	// file extension to determine the correct VHD format.
	//

	storageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_UNKNOWN;
	storageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_UNKNOWN;
	// storageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT;

	memset(&parameters, 0, sizeof(parameters));

	//
	// CREATE_VIRTUAL_DISK_VERSION_2 allows specifying a richer set a values and returns
	// a V2 handle.
	//
	// VIRTUAL_DISK_ACCESS_NONE is the only acceptable access mask for V2 handle opens.
	//
	// Valid BlockSize values are as follows (use 0 to indicate default value):
	//      Fixed VHD: 0
	//      Dynamic VHD: 512kb, 2mb (default)
	//      Differencing VHD: 512kb, 2mb (if parent is fixed, default is 2mb; if parent is dynamic or differencing, default is parent blocksize)
	//      Fixed VHDX: 0
	//      Dynamic VHDX: 1mb, 2mb, 4mb, 8mb, 16mb, 32mb (default), 64mb, 128mb, 256mb
	//      Differencing VHDX: 1mb, 2mb (default), 4mb, 8mb, 16mb, 32mb, 64mb, 128mb, 256mb
	//
	// Valid LogicalSectorSize values are as follows (use 0 to indicate default value):
	//      VHD: 512 (default)
	//      VHDX: 512 (for fixed or dynamic, default is 512; for differencing, default is parent logicalsectorsize), 4096
	//
	// Valid PhysicalSectorSize values are as follows (use 0 to indicate default value):
	//      VHD: 512 (default)
	//      VHDX: 512, 4096 (for fixed or dynamic, default is 4096; for differencing, default is parent physicalsectorsize)
	//
	parameters.Version = CREATE_VIRTUAL_DISK_VERSION_2;
	parameters.Version2.UniqueId = uniqueId;
	parameters.Version2.MaximumSize = FileSize;
	parameters.Version2.BlockSizeInBytes = BlockSize;
	parameters.Version2.SectorSizeInBytes = LogicalSectorSize;
	parameters.Version2.PhysicalSectorSizeInBytes = PhysicalSectorSize;
	parameters.Version2.ParentPath = ParentPath;

	opStatus = CreateVirtualDisk(
		&storageType,
		VirtualDiskPath,
		VIRTUAL_DISK_ACCESS_NONE,
		NULL,
		Flags,
		0,
		&parameters,
		NULL,
		&vhdHandle);

	if (opStatus != ERROR_SUCCESS)
	{
		goto Cleanup;
	}

Cleanup:

	if (opStatus == ERROR_SUCCESS)
	{
		wprintf(L"success\n");
	}
	else
	{
		wprintf(L"error = %u\n", opStatus);
	}

	if (vhdHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(vhdHandle);
	}

	return opStatus;
}


DWORD
DetachVirtualDisk(
	_In_    LPCWSTR     VirtualDiskPath)
{
	VIRTUAL_STORAGE_TYPE storageType;
	OPEN_VIRTUAL_DISK_PARAMETERS openParameters;
	VIRTUAL_DISK_ACCESS_MASK accessMask;
	LPCTSTR extension;
	HANDLE vhdHandle;
	DWORD opStatus;

	vhdHandle = INVALID_HANDLE_VALUE;

	//
	// Specify UNKNOWN for both device and vendor so the system will use the
	// file extension to determine the correct VHD format.
	//

	storageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_UNKNOWN;
	storageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_UNKNOWN;

	memset(&openParameters, 0, sizeof(openParameters));

	extension = ::PathFindExtension(VirtualDiskPath);

	if (extension != NULL && _wcsicmp(extension, L".iso") == 0)
	{
		//
		// ISO files can only be opened using the V1 API.
		//

		openParameters.Version = OPEN_VIRTUAL_DISK_VERSION_1;
		accessMask = VIRTUAL_DISK_ACCESS_READ;
	}
	else
	{
		//
		// VIRTUAL_DISK_ACCESS_NONE is the only acceptable access mask for V2 handle opens.
		// OPEN_VIRTUAL_DISK_FLAG_NONE bypasses any special handling of the open.
		//

		openParameters.Version = OPEN_VIRTUAL_DISK_VERSION_2;
		openParameters.Version2.GetInfoOnly = FALSE;
		accessMask = VIRTUAL_DISK_ACCESS_NONE;
	}

	//
	// Open the VHD/VHDX or ISO.
	//
	//

	opStatus = OpenVirtualDisk(
		&storageType,
		VirtualDiskPath,
		accessMask,
		OPEN_VIRTUAL_DISK_FLAG_NONE,
		&openParameters,
		&vhdHandle);

	if (opStatus != ERROR_SUCCESS)
	{
		goto Cleanup;
	}

	//
	// Detach the VHD/VHDX/ISO.
	//
	// DETACH_VIRTUAL_DISK_FLAG_NONE is the only flag currently supported for detach.
	//

	opStatus = DetachVirtualDisk(
		vhdHandle,
		DETACH_VIRTUAL_DISK_FLAG_NONE,
		0);

	if (opStatus != ERROR_SUCCESS)
	{
		goto Cleanup;
	}

Cleanup:

	if (opStatus == ERROR_SUCCESS)
	{
		wprintf(L"success\n");
	}
	else
	{
		wprintf(L"error = %u\n", opStatus);
	}

	if (vhdHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(vhdHandle);
	}

	return opStatus;
}

DWORD
GetAllAttachedVirtualDiskPhysicalPaths(
)
{
	LPWSTR  pathList;
	LPWSTR  pathListBuffer;
	size_t  nextPathListSize;
	DWORD   opStatus;
	ULONG   pathListSizeInBytes;
	size_t  pathListSizeRemaining;
	HRESULT stringLengthResult;

	pathListBuffer = NULL;
	pathListSizeInBytes = 0;

	do
	{
		//
		// Determine the size actually required.
		//

		opStatus = GetAllAttachedVirtualDiskPhysicalPaths(&pathListSizeInBytes,
			pathListBuffer);
		if (opStatus == ERROR_SUCCESS)
		{
			break;
		}

		if (opStatus != ERROR_INSUFFICIENT_BUFFER)
		{
			goto Cleanup;
		}

		if (pathListBuffer != NULL)
		{
			free(pathListBuffer);
		}

		//
		// Allocate a large enough buffer.
		//

		pathListBuffer = (LPWSTR)malloc(pathListSizeInBytes);
		if (pathListBuffer == NULL)
		{
			opStatus = ERROR_OUTOFMEMORY;
			goto Cleanup;
		}

	} while (opStatus == ERROR_INSUFFICIENT_BUFFER);

	if (pathListBuffer == NULL || pathListBuffer[0] == NULL)
	{
		// There are no loopback mounted virtual disks.
		wprintf(L"There are no loopback mounted virtual disks.\n");
		goto Cleanup;
	}

	//
	// The pathList is a MULTI_SZ.
	//

	pathList = pathListBuffer;
	pathListSizeRemaining = (size_t)pathListSizeInBytes;

	while ((pathListSizeRemaining >= sizeof(pathList[0])) && (*pathList != 0))
	{
		stringLengthResult = StringCbLengthW(pathList,
			pathListSizeRemaining,
			&nextPathListSize);

		if (FAILED(stringLengthResult))
		{
			goto Cleanup;
		}

		wprintf(L"Path = '%s'\n", pathList);

		nextPathListSize += sizeof(pathList[0]);
		pathList = pathList + (nextPathListSize / sizeof(pathList[0]));
		pathListSizeRemaining -= nextPathListSize;
	}

Cleanup:
	if (opStatus == ERROR_SUCCESS)
	{
		wprintf(L"success\n");
	}
	else
	{
		wprintf(L"error = %u\n", opStatus);
	}

	if (pathListBuffer != NULL)
	{
		free(pathListBuffer);
	}

	return opStatus;
}

DWORD
GetVirtualDiskInformation(
	_In_    LPCWSTR     VirtualDiskPath)
{
	OPEN_VIRTUAL_DISK_PARAMETERS openParameters;
	VIRTUAL_STORAGE_TYPE storageType;
	PGET_VIRTUAL_DISK_INFO diskInfo;
	ULONG diskInfoSize;
	DWORD opStatus;

	HANDLE vhdHandle;

	UINT32 driveType;
	UINT32 driveFormat;

	GUID identifier;

	ULONGLONG physicalSize;
	ULONGLONG virtualSize;
	ULONGLONG minInternalSize;
	ULONG blockSize;
	ULONG sectorSize;
	ULONG physicalSectorSize;
	LPCWSTR parentPath;
	size_t parentPathSize;
	size_t parentPathSizeRemaining;
	HRESULT stringLengthResult;
	GUID parentIdentifier;
	ULONGLONG fragmentationPercentage;

	vhdHandle = INVALID_HANDLE_VALUE;
	diskInfo = NULL;
	diskInfoSize = sizeof(GET_VIRTUAL_DISK_INFO);

	diskInfo = (PGET_VIRTUAL_DISK_INFO)malloc(diskInfoSize);
	if (diskInfo == NULL)
	{
		opStatus = ERROR_NOT_ENOUGH_MEMORY;
		goto Cleanup;
	}

	//
	// Specify UNKNOWN for both device and vendor so the system will use the
	// file extension to determine the correct VHD format.
	//

	storageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_UNKNOWN;
	storageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_UNKNOWN;

	//
	// Open the VHD for query access.
	//
	// A "GetInfoOnly" handle is a handle that can only be used to query properties or
	// metadata.
	//
	// VIRTUAL_DISK_ACCESS_NONE is the only acceptable access mask for V2 handle opens.
	// OPEN_VIRTUAL_DISK_FLAG_NO_PARENTS indicates the parent chain should not be opened.
	//

	memset(&openParameters, 0, sizeof(openParameters));
	openParameters.Version = OPEN_VIRTUAL_DISK_VERSION_2;
	openParameters.Version2.GetInfoOnly = TRUE;

	opStatus = OpenVirtualDisk(
		&storageType,
		VirtualDiskPath,
		VIRTUAL_DISK_ACCESS_NONE,
		OPEN_VIRTUAL_DISK_FLAG_NO_PARENTS,
		&openParameters,
		&vhdHandle);

	if (opStatus != ERROR_SUCCESS)
	{
		goto Cleanup;
	}

	//
	// Get the VHD/VHDX type.
	//

	diskInfo->Version = GET_VIRTUAL_DISK_INFO_PROVIDER_SUBTYPE;

	opStatus = GetVirtualDiskInformation(
		vhdHandle,
		&diskInfoSize,
		diskInfo,
		NULL);

	if (opStatus != ERROR_SUCCESS)
	{
		goto Cleanup;
	}

	driveType = diskInfo->ProviderSubtype;

	wprintf(L"driveType = %d", driveType);

	if (driveType == 2)
	{
		wprintf(L" (fixed)\n");
	}
	else if (driveType == 3)
	{
		wprintf(L" (dynamic)\n");
	}
	else if (driveType == 4)
	{
		wprintf(L" (differencing)\n");
	}
	else
	{
		wprintf(L"\n");
	}

	//
	// Get the VHD/VHDX format.
	//

	diskInfo->Version = GET_VIRTUAL_DISK_INFO_VIRTUAL_STORAGE_TYPE;

	opStatus = GetVirtualDiskInformation(
		vhdHandle,
		&diskInfoSize,
		diskInfo,
		NULL);

	if (opStatus != ERROR_SUCCESS)
	{
		goto Cleanup;
	}

	driveFormat = diskInfo->VirtualStorageType.DeviceId;

	wprintf(L"driveFormat = %d", driveFormat);

	if (driveFormat == VIRTUAL_STORAGE_TYPE_DEVICE_VHD)
	{
		wprintf(L" (vhd)\n");
	}
	else if (driveFormat == VIRTUAL_STORAGE_TYPE_DEVICE_VHDX)
	{
		wprintf(L" (vhdx)\n");
	}
	else
	{
		wprintf(L"\n");
	}

	//
	// Get the VHD/VHDX virtual disk size.
	//

	diskInfo->Version = GET_VIRTUAL_DISK_INFO_SIZE;

	opStatus = GetVirtualDiskInformation(
		vhdHandle,
		&diskInfoSize,
		diskInfo,
		NULL);

	if (opStatus != ERROR_SUCCESS)
	{
		goto Cleanup;
	}

	physicalSize = diskInfo->Size.PhysicalSize;
	virtualSize = diskInfo->Size.VirtualSize;
	sectorSize = diskInfo->Size.SectorSize;
	blockSize = diskInfo->Size.BlockSize;

	wprintf(L"physicalSize = %I64u\n", physicalSize);
	wprintf(L"virtualSize = %I64u\n", virtualSize);
	wprintf(L"sectorSize = %u\n", sectorSize);
	wprintf(L"blockSize = %u\n", blockSize);

	//
	// Get the VHD physical sector size.
	//

	diskInfo->Version = GET_VIRTUAL_DISK_INFO_VHD_PHYSICAL_SECTOR_SIZE;

	opStatus = GetVirtualDiskInformation(
		vhdHandle,
		&diskInfoSize,
		diskInfo,
		NULL);

	if (opStatus != ERROR_SUCCESS)
	{
		goto Cleanup;
	}

	physicalSectorSize = diskInfo->VhdPhysicalSectorSize;

	wprintf(L"physicalSectorSize = %u\n", physicalSectorSize);

	//
	// Get the virtual disk ID.
	//

	diskInfo->Version = GET_VIRTUAL_DISK_INFO_IDENTIFIER;

	opStatus = GetVirtualDiskInformation(
		vhdHandle,
		&diskInfoSize,
		diskInfo,
		NULL);

	if (opStatus != ERROR_SUCCESS)
	{
		goto Cleanup;
	}

	identifier = diskInfo->Identifier;

	wprintf(L"identifier = {%08X-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X}\n",
		identifier.Data1, identifier.Data2, identifier.Data3,
		identifier.Data4[0], identifier.Data4[1], identifier.Data4[2], identifier.Data4[3],
		identifier.Data4[4], identifier.Data4[5], identifier.Data4[6], identifier.Data4[7]);

	//
	// Get the VHD parent path.
	//

	if (driveType == 0x4)
	{
		diskInfo->Version = GET_VIRTUAL_DISK_INFO_PARENT_LOCATION;

		opStatus = GetVirtualDiskInformation(
			vhdHandle,
			&diskInfoSize,
			diskInfo,
			NULL);

		if (opStatus != ERROR_SUCCESS)
		{
			if (opStatus != ERROR_INSUFFICIENT_BUFFER)
			{
				goto Cleanup;
			}

			free(diskInfo);

			diskInfo = (PGET_VIRTUAL_DISK_INFO)malloc(diskInfoSize);
			if (diskInfo == NULL)
			{
				opStatus = ERROR_NOT_ENOUGH_MEMORY;
				goto Cleanup;
			}

			diskInfo->Version = GET_VIRTUAL_DISK_INFO_PARENT_LOCATION;

			opStatus = GetVirtualDiskInformation(
				vhdHandle,
				&diskInfoSize,
				diskInfo,
				NULL);

			if (opStatus != ERROR_SUCCESS)
			{
				goto Cleanup;
			}
		}

		parentPath = diskInfo->ParentLocation.ParentLocationBuffer;
		parentPathSizeRemaining = diskInfoSize - FIELD_OFFSET(GET_VIRTUAL_DISK_INFO,
			ParentLocation.ParentLocationBuffer);

		if (diskInfo->ParentLocation.ParentResolved)
		{
			wprintf(L"parentPath = '%s'\n", parentPath);
		}
		else
		{
			//
			// If the parent is not resolved, the buffer is a MULTI_SZ
			//

			wprintf(L"parentPath:\n");

			while ((parentPathSizeRemaining >= sizeof(parentPath[0])) && (*parentPath != 0))
			{
				stringLengthResult = StringCbLengthW(
					parentPath,
					parentPathSizeRemaining,
					&parentPathSize);

				if (FAILED(stringLengthResult))
				{
					goto Cleanup;
				}

				wprintf(L"    '%s'\n", parentPath);

				parentPathSize += sizeof(parentPath[0]);
				parentPath = parentPath + (parentPathSize / sizeof(parentPath[0]));
				parentPathSizeRemaining -= parentPathSize;
			}
		}

		//
		// Get parent ID.
		//

		diskInfo->Version = GET_VIRTUAL_DISK_INFO_PARENT_IDENTIFIER;

		opStatus = GetVirtualDiskInformation(
			vhdHandle,
			&diskInfoSize,
			diskInfo,
			NULL);

		if (opStatus != ERROR_SUCCESS)
		{
			goto Cleanup;
		}

		parentIdentifier = diskInfo->ParentIdentifier;

		wprintf(L"parentIdentifier = {%08X-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X}\n",
			parentIdentifier.Data1, parentIdentifier.Data2, parentIdentifier.Data3,
			parentIdentifier.Data4[0], parentIdentifier.Data4[1],
			parentIdentifier.Data4[2], parentIdentifier.Data4[3],
			parentIdentifier.Data4[4], parentIdentifier.Data4[5],
			parentIdentifier.Data4[6], parentIdentifier.Data4[7]);
	}

	//
	// Get the VHD minimum internal size.
	//

	diskInfo->Version = GET_VIRTUAL_DISK_INFO_SMALLEST_SAFE_VIRTUAL_SIZE;

	opStatus = GetVirtualDiskInformation(
		vhdHandle,
		&diskInfoSize,
		diskInfo,
		NULL);

	if (opStatus == ERROR_SUCCESS)
	{
		minInternalSize = diskInfo->SmallestSafeVirtualSize;

		wprintf(L"minInternalSize = %I64u\n", minInternalSize);
	}
	else
	{
		opStatus = ERROR_SUCCESS;
	}

	//
	// Get the VHD fragmentation percentage.
	//

	diskInfo->Version = GET_VIRTUAL_DISK_INFO_FRAGMENTATION;

	opStatus = GetVirtualDiskInformation(
		vhdHandle,
		&diskInfoSize,
		diskInfo,
		NULL);

	if (opStatus == ERROR_SUCCESS)
	{
		fragmentationPercentage = diskInfo->FragmentationPercentage;

		wprintf(L"fragmentationPercentage = %I64u\n", fragmentationPercentage);
	}
	else
	{
		opStatus = ERROR_SUCCESS;
	}

Cleanup:

	if (opStatus == ERROR_SUCCESS)
	{
		wprintf(L"success\n");
	}
	else
	{
		wprintf(L"error = %u\n", opStatus);
	}

	if (vhdHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(vhdHandle);
	}

	if (diskInfo != NULL)
	{
		free(diskInfo);
	}

	return opStatus;
}

