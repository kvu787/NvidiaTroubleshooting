#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

static std::wstring luid_string(const LUID &luid) {
	wchar_t buffer[64] = {};
	swprintf_s(buffer, L"%08lX:%08lX", static_cast<unsigned long>(luid.HighPart), luid.LowPart);
	return buffer;
}

static std::wstring output_technology(DISPLAYCONFIG_VIDEO_OUTPUT_TECHNOLOGY technology) {
	switch (technology) {
		case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_HD15: return L"HD15";
		case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_SVIDEO: return L"S-Video";
		case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_COMPOSITE_VIDEO: return L"Composite";
		case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_COMPONENT_VIDEO: return L"Component";
		case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DVI: return L"DVI";
		case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_HDMI: return L"HDMI";
		case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_LVDS: return L"LVDS";
		case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_D_JPN: return L"D-JPN";
		case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_SDI: return L"SDI";
		case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EXTERNAL: return L"DisplayPort external";
		case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EMBEDDED: return L"DisplayPort embedded";
		case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_UDI_EXTERNAL: return L"UDI external";
		case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_UDI_EMBEDDED: return L"UDI embedded";
		case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_SDTVDONGLE: return L"SDTV dongle";
		case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_MIRACAST: return L"Miracast";
		case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INDIRECT_WIRED: return L"Indirect wired";
		case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INDIRECT_VIRTUAL: return L"Indirect virtual";
		case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INTERNAL: return L"Internal";
		case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_OTHER: return L"Other";
		default: return L"Unknown";
	}
}

static std::wstring adapter_device_string(const wchar_t *gdi_name) {
	for (DWORD index = 0;; ++index) {
		DISPLAY_DEVICEW device = {};
		device.cb = sizeof(device);
		if (!EnumDisplayDevicesW(nullptr, index, &device, 0)) {
			break;
		}
		if (_wcsicmp(device.DeviceName, gdi_name) == 0) {
			return std::wstring(device.DeviceString) + L" | " + device.DeviceID;
		}
	}
	return L"<matching GDI adapter not found>";
}

int wmain(int argc, wchar_t **argv) {
	UINT32 path_count = 0;
	UINT32 mode_count = 0;
	LONG status = GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &path_count, &mode_count);
	if (status != ERROR_SUCCESS) {
		std::wcerr << L"GetDisplayConfigBufferSizes failed: " << status << L"\n";
		return 1;
	}

	std::vector<DISPLAYCONFIG_PATH_INFO> paths(path_count);
	std::vector<DISPLAYCONFIG_MODE_INFO> modes(mode_count);
	status = QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &path_count, paths.data(), &mode_count, modes.data(), nullptr);
	if (status != ERROR_SUCCESS) {
		std::wcerr << L"QueryDisplayConfig failed: " << status << L"\n";
		return 1;
	}

	std::wcout << L"Active display paths: " << path_count << L"\n";
	for (UINT32 index = 0; index < path_count; ++index) {
		const auto &path = paths[index];

		DISPLAYCONFIG_SOURCE_DEVICE_NAME source_name = {};
		source_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
		source_name.header.size = sizeof(source_name);
		source_name.header.adapterId = path.sourceInfo.adapterId;
		source_name.header.id = path.sourceInfo.id;
		const LONG source_status = DisplayConfigGetDeviceInfo(&source_name.header);

		DISPLAYCONFIG_TARGET_DEVICE_NAME target_name = {};
		target_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
		target_name.header.size = sizeof(target_name);
		target_name.header.adapterId = path.targetInfo.adapterId;
		target_name.header.id = path.targetInfo.id;
		const LONG target_status = DisplayConfigGetDeviceInfo(&target_name.header);

		DISPLAYCONFIG_ADAPTER_NAME adapter_name = {};
		adapter_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_ADAPTER_NAME;
		adapter_name.header.size = sizeof(adapter_name);
		adapter_name.header.adapterId = path.sourceInfo.adapterId;
		const LONG adapter_status = DisplayConfigGetDeviceInfo(&adapter_name.header);

		std::wcout << L"\nPath " << index << L"\n";
		std::wcout << L"  source adapter LUID: " << luid_string(path.sourceInfo.adapterId) << L"\n";
		std::wcout << L"  target adapter LUID: " << luid_string(path.targetInfo.adapterId) << L"\n";
		std::wcout << L"  source id: " << path.sourceInfo.id << L"\n";
		std::wcout << L"  target id: " << path.targetInfo.id << L"\n";
		std::wcout << L"  path flags: 0x" << std::hex << path.flags << std::dec << L"\n";
		std::wcout << L"  output technology: " << output_technology(path.targetInfo.outputTechnology)
				   << L" (" << static_cast<int>(path.targetInfo.outputTechnology) << L")\n";
		std::wcout << L"  target available: " << (path.targetInfo.targetAvailable ? L"true" : L"false") << L"\n";
		std::wcout << L"  refresh: " << path.targetInfo.refreshRate.Numerator << L"/"
				   << path.targetInfo.refreshRate.Denominator << L" Hz\n";

		if (path.sourceInfo.modeInfoIdx != DISPLAYCONFIG_PATH_MODE_IDX_INVALID &&
				path.sourceInfo.modeInfoIdx < mode_count) {
			const auto &mode = modes[path.sourceInfo.modeInfoIdx];
			if (mode.infoType == DISPLAYCONFIG_MODE_INFO_TYPE_SOURCE) {
				std::wcout << L"  source mode: " << mode.sourceMode.width << L"x" << mode.sourceMode.height
						   << L" at (" << mode.sourceMode.position.x << L"," << mode.sourceMode.position.y << L")"
						   << L", pixel format enum " << static_cast<int>(mode.sourceMode.pixelFormat) << L"\n";
			}
		}

		if (path.targetInfo.modeInfoIdx != DISPLAYCONFIG_PATH_MODE_IDX_INVALID &&
				path.targetInfo.modeInfoIdx < mode_count) {
			const auto &mode = modes[path.targetInfo.modeInfoIdx];
			if (mode.infoType == DISPLAYCONFIG_MODE_INFO_TYPE_TARGET) {
				const auto &signal = mode.targetMode.targetVideoSignalInfo;
				std::wcout << L"  target signal active/total: "
						   << signal.activeSize.cx << L"x" << signal.activeSize.cy << L" / "
						   << signal.totalSize.cx << L"x" << signal.totalSize.cy
						   << L", pixel rate " << signal.pixelRate << L"\n";
			}
		}

		if (source_status == ERROR_SUCCESS) {
			std::wcout << L"  GDI source: " << source_name.viewGdiDeviceName << L"\n";
			std::wcout << L"  GDI adapter: " << adapter_device_string(source_name.viewGdiDeviceName) << L"\n";
		} else {
			std::wcout << L"  source name query failed: " << source_status << L"\n";
		}

		if (adapter_status == ERROR_SUCCESS) {
			std::wcout << L"  adapter device path: " << adapter_name.adapterDevicePath << L"\n";
		} else {
			std::wcout << L"  adapter name query failed: " << adapter_status << L"\n";
		}

		if (target_status == ERROR_SUCCESS) {
			std::wcout << L"  monitor friendly name: " << target_name.monitorFriendlyDeviceName << L"\n";
			std::wcout << L"  monitor device path: " << target_name.monitorDevicePath << L"\n";
			std::wcout << L"  connector instance: " << target_name.connectorInstance << L"\n";
			std::wcout << L"  EDID manufacturer/product: 0x" << std::hex
					   << target_name.edidManufactureId << L"/0x" << target_name.edidProductCodeId
					   << std::dec << L"\n";
		} else {
			std::wcout << L"  target name query failed: " << target_status << L"\n";
		}
	}

	if (argc > 1 && !paths.empty()) {
		std::wcout << L"\nExplicit target lookups on adapter "
				   << luid_string(paths[0].targetInfo.adapterId) << L"\n";
		for (int argument = 1; argument < argc; ++argument) {
			const UINT32 target_id = static_cast<UINT32>(wcstoul(argv[argument], nullptr, 0));
			DISPLAYCONFIG_TARGET_DEVICE_NAME target_name = {};
			target_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
			target_name.header.size = sizeof(target_name);
			target_name.header.adapterId = paths[0].targetInfo.adapterId;
			target_name.header.id = target_id;
			const LONG target_status = DisplayConfigGetDeviceInfo(&target_name.header);
			std::wcout << L"  target id " << target_id << L": status " << target_status;
			if (target_status == ERROR_SUCCESS) {
				std::wcout << L", name=" << target_name.monitorFriendlyDeviceName
						   << L", technology=" << output_technology(target_name.outputTechnology)
						   << L", connector instance=" << target_name.connectorInstance
						   << L", path=" << target_name.monitorDevicePath;
			}
			std::wcout << L"\n";
		}
	}

	return 0;
}
