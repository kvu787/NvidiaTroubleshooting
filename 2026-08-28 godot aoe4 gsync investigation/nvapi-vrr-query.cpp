// Read-only per-display VRR/DisplayPort probe. This file calls no NVAPI setters.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <nvapi.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

static std::wstring luid_string(const LUID &luid) {
	wchar_t buffer[64] = {};
	swprintf_s(buffer, L"%08lX:%08lX", static_cast<unsigned long>(luid.HighPart), luid.LowPart);
	return buffer;
}

static std::string status_string(NvAPI_Status status) {
	NvAPI_ShortString message = {};
	NvAPI_GetErrorMessage(status, message);
	return std::string(message);
}

static std::wstring target_name(const LUID &adapter_id, NvU32 target_id) {
	DISPLAYCONFIG_TARGET_DEVICE_NAME target = {};
	target.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
	target.header.size = sizeof(target);
	target.header.adapterId = adapter_id;
	target.header.id = target_id;
	const LONG status = DisplayConfigGetDeviceInfo(&target.header);
	if (status != ERROR_SUCCESS) {
		return L"<DisplayConfigGetDeviceInfo failed: " + std::to_wstring(status) + L">";
	}
	if (target.monitorFriendlyDeviceName[0] == L'\0') {
		return L"<no monitor name>";
	}
	return target.monitorFriendlyDeviceName;
}

int main() {
	NvAPI_Status status = NvAPI_Initialize();
	if (status != NVAPI_OK) {
		std::cerr << "NvAPI_Initialize failed: " << status << " " << status_string(status) << "\n";
		return 1;
	}

	NvPhysicalGpuHandle gpu_handles[NVAPI_MAX_PHYSICAL_GPUS] = {};
	NvU32 gpu_count = 0;
	status = NvAPI_EnumPhysicalGPUs(gpu_handles, &gpu_count);
	if (status != NVAPI_OK) {
		std::cerr << "NvAPI_EnumPhysicalGPUs failed: " << status << " " << status_string(status) << "\n";
		NvAPI_Unload();
		return 1;
	}

	NvU32 driver_version = 0;
	NvAPI_ShortString driver_branch = {};
	status = NvAPI_SYS_GetDriverAndBranchVersion(&driver_version, driver_branch);
	std::cout << "Driver: status=" << status;
	if (status == NVAPI_OK) {
		std::cout << " version=" << driver_version << " branch=" << driver_branch;
	}
	std::cout << "\nPhysical GPUs: " << gpu_count << "\n";

	for (NvU32 gpu_index = 0; gpu_index < gpu_count; ++gpu_index) {
		NvAPI_ShortString gpu_name = {};
		status = NvAPI_GPU_GetFullName(gpu_handles[gpu_index], gpu_name);
		std::cout << "\nGPU " << gpu_index << ": "
				  << (status == NVAPI_OK ? gpu_name : "<name unavailable>") << "\n";

		NvU32 display_count = 0;
		status = NvAPI_GPU_GetConnectedDisplayIds(
				gpu_handles[gpu_index], nullptr, &display_count, NV_GPU_CONNECTED_IDS_FLAG_UNCACHED | NV_GPU_CONNECTED_IDS_FLAG_LIDSTATE);
		if (status != NVAPI_OK) {
			std::cout << "  GetConnectedDisplayIds(count) failed: " << status << " " << status_string(status) << "\n";
			continue;
		}

		std::vector<NV_GPU_DISPLAYIDS> displays(display_count);
		for (auto &display : displays) {
			display.version = NV_GPU_DISPLAYIDS_VER;
		}
		status = NvAPI_GPU_GetConnectedDisplayIds(
				gpu_handles[gpu_index], displays.data(), &display_count, NV_GPU_CONNECTED_IDS_FLAG_UNCACHED | NV_GPU_CONNECTED_IDS_FLAG_LIDSTATE);
		if (status != NVAPI_OK) {
			std::cout << "  GetConnectedDisplayIds(data) failed: " << status << " " << status_string(status) << "\n";
			continue;
		}

		std::cout << "  Connected display IDs: " << display_count << "\n";
		for (NvU32 display_index = 0; display_index < display_count; ++display_index) {
			const auto &display = displays[display_index];
			std::cout << "\n  Display " << display_index << "\n";
			std::cout << "    NV display ID: 0x" << std::hex << std::uppercase << display.displayId << std::dec << "\n";
			std::cout << "    connector type enum: " << static_cast<int>(display.connectorType) << "\n";
			std::cout << "    active=" << display.isActive
					  << " osVisible=" << display.isOSVisible
					  << " connected=" << display.isConnected
					  << " physicallyConnected=" << display.isPhysicallyConnected
					  << " dynamicMst=" << display.isDynamic
					  << " mstRoot=" << display.isMultiStreamRootNode << "\n";

			NV_DISPLAY_ID_INFO_DATA display_info = {};
			display_info.version = NV_DISPLAY_ID_INFO_DATA_VER;
			status = NvAPI_Disp_GetDisplayIdInfo(display.displayId, &display_info);
			std::cout << "    display ID info: status=" << status;
			if (status == NVAPI_OK) {
				std::wcout << L" adapterLuid=" << luid_string(display_info.adapterId)
						   << L" targetId=" << display_info.targetId
						   << L" name=" << target_name(display_info.adapterId, display_info.targetId);
			}
			std::cout << "\n";

			NV_GET_VRR_INFO vrr = {};
			vrr.version = NV_GET_VRR_INFO_VER;
			status = NvAPI_Disp_GetVRRInfo(display.displayId, &vrr);
			std::cout << "    VRR info: status=" << status;
			if (status == NVAPI_OK) {
				std::cout << " possible=" << vrr.bIsVRRPossible
						  << " requested=" << vrr.bIsVRRRequested
						  << " enabled=" << vrr.bIsVRREnabled
						  << " displayInVrrMode=" << vrr.bIsDisplayInVRRMode
						  << " indicatorEnabled=" << vrr.bIsVRRIndicatorEnabled;
			} else {
				std::cout << " " << status_string(status);
			}
			std::cout << "\n";

			NV_GET_ADAPTIVE_SYNC_DATA adaptive = {};
			adaptive.version = NV_GET_ADAPTIVE_SYNC_DATA_VER;
			status = NvAPI_DISP_GetAdaptiveSyncData(display.displayId, &adaptive);
			std::cout << "    Adaptive-Sync data: status=" << status;
			if (status == NVAPI_OK) {
				std::cout << " disabled=" << adaptive.bDisableAdaptiveSync
						  << " frameSplittingDisabled=" << adaptive.bDisableFrameSplitting
						  << " maxFrameIntervalUs=" << adaptive.maxFrameInterval
						  << " lastFlipRefreshCount=" << adaptive.lastFlipRefreshCount;
			} else {
				std::cout << " " << status_string(status);
			}
			std::cout << "\n";

			NV_GET_VIRTUAL_REFRESH_RATE_DATA virtual_vrr = {};
			virtual_vrr.version = NV_GET_VIRTUAL_REFRESH_RATE_DATA_VER;
			status = NvAPI_DISP_GetVirtualRefreshRateData(display.displayId, &virtual_vrr);
			std::cout << "    Virtual refresh data: status=" << status;
			if (status == NVAPI_OK) {
				std::cout << " refreshX1000=" << virtual_vrr.rrx1k
						  << " gamingVrr=" << virtual_vrr.bIsGamingVrr
						  << " frameIntervalNs=" << virtual_vrr.frameIntervalNs;
			} else {
				std::cout << " " << status_string(status);
			}
			std::cout << "\n";

			NV_DISPLAY_PORT_INFO display_port = {};
			display_port.version = NV_DISPLAY_PORT_INFO_VER1;
			status = NvAPI_GetDisplayPortInfo(NVAPI_DEFAULT_HANDLE, display.displayId, &display_port);
			std::cout << "    DisplayPort info: status=" << status;
			if (status == NVAPI_OK) {
				std::cout << " isDp=" << display_port.isDp
						  << " internalDp=" << display_port.isInternalDp
						  << " dpcdVersion=0x" << std::hex << display_port.dpcd_ver << std::dec
						  << " maxLinkRateEnum=" << static_cast<int>(display_port.maxLinkRate)
						  << " currentLinkRateEnum=" << static_cast<int>(display_port.curLinkRate)
						  << " maxLanes=" << static_cast<int>(display_port.maxLaneCount)
						  << " currentLanes=" << static_cast<int>(display_port.curLaneCount)
						  << " bpcEnum=" << static_cast<int>(display_port.bpc);
			} else {
				std::cout << " " << status_string(status);
			}
			std::cout << "\n";
		}
	}

	NvAPI_Unload();
	return 0;
}
