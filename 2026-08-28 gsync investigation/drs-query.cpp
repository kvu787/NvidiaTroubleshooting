#include <windows.h>

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "nvapi.h"

namespace {

using QueryInterface = void *(__cdecl *)(unsigned int);

std::string utf8(const wchar_t *text) {
    if (text == nullptr || *text == L'\0') {
        return "";
    }

    const int length = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), length, nullptr, nullptr);
    result.pop_back();
    return result;
}

template <typename T>
std::string utf8(const T &text) {
    return utf8(reinterpret_cast<const wchar_t *>(text));
}

std::wstring basename(const std::wstring &path) {
    const auto separator = path.find_last_of(L"\\/");
    return separator == std::wstring::npos ? path : path.substr(separator + 1);
}

std::string ascii_lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

std::string hex32(NvU32 value) {
    std::ostringstream output;
    output << "0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << value;
    return output.str();
}

const char *location_name(NVDRS_SETTING_LOCATION location) {
    switch (location) {
        case NVDRS_CURRENT_PROFILE_LOCATION:
            return "current profile";
        case NVDRS_GLOBAL_PROFILE_LOCATION:
            return "global profile";
        case NVDRS_BASE_PROFILE_LOCATION:
            return "base profile";
        case NVDRS_DEFAULT_PROFILE_LOCATION:
            return "driver default";
        default:
            return "unknown";
    }
}

const char *type_name(NVDRS_SETTING_TYPE type) {
    switch (type) {
        case NVDRS_DWORD_TYPE:
            return "DWORD";
        case NVDRS_BINARY_TYPE:
            return "binary";
        case NVDRS_STRING_TYPE:
            return "string";
        case NVDRS_WSTRING_TYPE:
            return "wide string";
        default:
            return "unknown";
    }
}

std::string setting_value(const NVDRS_SETTING &setting) {
    switch (setting.settingType) {
        case NVDRS_DWORD_TYPE:
            return hex32(setting.u32CurrentValue) + " (" + std::to_string(setting.u32CurrentValue) + ")";
        case NVDRS_STRING_TYPE:
        case NVDRS_WSTRING_TYPE:
            return "\"" + utf8(setting.wszCurrentValue) + "\"";
        case NVDRS_BINARY_TYPE:
            return "binary, " + std::to_string(setting.binaryCurrentValue.valueLength) + " bytes";
        default:
            return "unknown";
    }
}

std::string known_interpretation(NvU32 id, NvU32 value) {
    static const std::map<NvU32, std::map<NvU32, std::string>> interpretations = {
        {0x20C1221E, {{0, "driver default"}, {1, "enabled"}, {2, "disabled"}}},
        {0x1094F1F7, {{0, "disabled"}, {1, "fullscreen only"}, {2, "fullscreen and windowed"}}},
        {0x10A879CF, {{0, "allow"}, {1, "force off"}, {2, "disallow"}, {3, "ULMB"}, {4, "fixed refresh"}}},
        {0x10A879AC, {{0, "allow"}, {1, "force off"}, {2, "disallow"}, {3, "ULMB"}, {4, "fixed refresh"}}},
        {0x1194F158, {{0, "disabled"}, {1, "fullscreen only"}, {2, "fullscreen and windowed"}}},
        {0x10A879CE, {{0, "disabled"}, {1, "enabled"}, {0x9F95128E, "not supported"}}},
    };

    const auto setting = interpretations.find(id);
    if (setting == interpretations.end()) {
        return "";
    }
    const auto interpretation = setting->second.find(value);
    return interpretation == setting->second.end() ? "" : interpretation->second;
}

void print_setting(const NVDRS_SETTING &setting, const std::string &indent) {
    std::cout << indent << "id=" << hex32(setting.settingId)
              << ", name=\"" << utf8(setting.settingName) << "\""
              << ", type=" << type_name(setting.settingType)
              << ", location=" << location_name(setting.settingLocation)
              << ", current_predefined=" << setting.isCurrentPredefined
              << ", predefined_valid=" << setting.isPredefinedValid
              << ", value=" << setting_value(setting);

    if (setting.settingType == NVDRS_DWORD_TYPE) {
        const auto interpretation = known_interpretation(setting.settingId, setting.u32CurrentValue);
        if (!interpretation.empty()) {
            std::cout << " (" << interpretation << ")";
        }
    }
    std::cout << '\n';
}

template <typename Function>
Function load(QueryInterface query, unsigned int id, const char *name) {
    auto function = reinterpret_cast<Function>(query(id));
    if (function == nullptr) {
        std::cerr << "Missing NVAPI function: " << name << "\n";
        ExitProcess(2);
    }
    return function;
}

}  // namespace

int wmain(int argc, wchar_t **argv) {
    if (argc != 2) {
        std::cerr << "Usage: drs-query.exe <full-path-to-executable>\n";
        return 2;
    }

    const std::wstring target_path = argv[1];
    const std::wstring target_basename = basename(target_path);

    HMODULE nvapi = LoadLibraryW(L"nvapi64.dll");
    if (nvapi == nullptr) {
        std::cerr << "Could not load nvapi64.dll (Win32 error " << GetLastError() << ").\n";
        return 2;
    }

    auto query = reinterpret_cast<QueryInterface>(GetProcAddress(nvapi, "nvapi_QueryInterface"));
    if (query == nullptr) {
        std::cerr << "Could not resolve nvapi_QueryInterface.\n";
        return 2;
    }

    const auto initialize = load<decltype(&NvAPI_Initialize)>(query, 0x0150E828, "NvAPI_Initialize");
    const auto unload = load<decltype(&NvAPI_Unload)>(query, 0xD22BDD7E, "NvAPI_Unload");
    const auto create_session = load<decltype(&NvAPI_DRS_CreateSession)>(query, 0x0694D52E, "NvAPI_DRS_CreateSession");
    const auto destroy_session = load<decltype(&NvAPI_DRS_DestroySession)>(query, 0xDAD9CFF8, "NvAPI_DRS_DestroySession");
    const auto load_settings = load<decltype(&NvAPI_DRS_LoadSettings)>(query, 0x375DBD6B, "NvAPI_DRS_LoadSettings");
    const auto find_application = load<decltype(&NvAPI_DRS_FindApplicationByName)>(query, 0xEEE566B2, "NvAPI_DRS_FindApplicationByName");
    const auto find_profile = load<decltype(&NvAPI_DRS_FindProfileByName)>(query, 0x7E4A9A0B, "NvAPI_DRS_FindProfileByName");
    const auto get_num_profiles = load<decltype(&NvAPI_DRS_GetNumProfiles)>(query, 0x1DAE4FBC, "NvAPI_DRS_GetNumProfiles");
    const auto enum_profiles = load<decltype(&NvAPI_DRS_EnumProfiles)>(query, 0xBC371EE0, "NvAPI_DRS_EnumProfiles");
    const auto get_profile_info = load<decltype(&NvAPI_DRS_GetProfileInfo)>(query, 0x61CD6FD6, "NvAPI_DRS_GetProfileInfo");
    const auto enum_applications = load<decltype(&NvAPI_DRS_EnumApplications)>(query, 0x7FA2173A, "NvAPI_DRS_EnumApplications");
    const auto enum_settings = load<decltype(&NvAPI_DRS_EnumSettings)>(query, 0xAE3039DA, "NvAPI_DRS_EnumSettings");
    const auto get_setting = load<decltype(&NvAPI_DRS_GetSetting)>(query, 0x73BF8338, "NvAPI_DRS_GetSetting");

    NvAPI_Status status = initialize();
    if (status != NVAPI_OK) {
        std::cerr << "NvAPI_Initialize failed: " << status << '\n';
        return 2;
    }

    NvDRSSessionHandle session = nullptr;
    status = create_session(&session);
    if (status != NVAPI_OK) {
        std::cerr << "NvAPI_DRS_CreateSession failed: " << status << '\n';
        unload();
        return 2;
    }

    status = load_settings(session);
    if (status != NVAPI_OK) {
        std::cerr << "NvAPI_DRS_LoadSettings failed: " << status << '\n';
        destroy_session(session);
        unload();
        return 2;
    }

    std::cout << "Target full path: " << utf8(target_path.c_str()) << '\n';
    std::cout << "Target basename used by ordinary DRS matching: " << utf8(target_basename.c_str()) << '\n';

    std::map<NvDRSProfileHandle, std::set<std::string>> matched_profiles;
    const std::vector<std::pair<std::string, std::wstring>> searches = {
        {"full path", target_path},
        {"basename", target_basename},
    };

    for (const auto &[label, search] : searches) {
        NVDRS_APPLICATION application{};
        application.version = NVDRS_APPLICATION_VER;
        NvDRSProfileHandle profile = nullptr;
        status = find_application(
            session,
            reinterpret_cast<NvU16 *>(const_cast<wchar_t *>(search.c_str())),
            &profile,
            &application);

        std::cout << "FindApplicationByName(" << label << ") status: " << status;
        if (status == NVAPI_OK) {
            std::cout << ", matched appName=\"" << utf8(application.appName) << "\"";
            matched_profiles[profile].insert("application lookup by " + label);
        }
        std::cout << '\n';
    }

    const std::vector<std::wstring> candidate_profile_names = {
        target_basename,
        L"Godot Engine",
    };
    for (const auto &profile_name : candidate_profile_names) {
        NvDRSProfileHandle profile = nullptr;
        status = find_profile(
            session,
            reinterpret_cast<NvU16 *>(const_cast<wchar_t *>(profile_name.c_str())),
            &profile);
        std::cout << "FindProfileByName(\"" << utf8(profile_name.c_str()) << "\") status: " << status << '\n';
        if (status == NVAPI_OK) {
            matched_profiles[profile].insert("exact profile-name lookup");
        }
    }

    NvU32 total_profiles = 0;
    status = get_num_profiles(session, &total_profiles);
    std::cout << "GetNumProfiles status: " << status << ", count=" << total_profiles << '\n';
    if (status == NVAPI_OK) {
        for (NvU32 index = 0; index < total_profiles; ++index) {
            NvDRSProfileHandle profile = nullptr;
            if (enum_profiles(session, index, &profile) != NVAPI_OK) {
                continue;
            }
            NVDRS_PROFILE profile_info{};
            profile_info.version = NVDRS_PROFILE_VER;
            if (get_profile_info(session, profile, &profile_info) != NVAPI_OK) {
                continue;
            }
            if (ascii_lower(utf8(profile_info.profileName)).find("godot") != std::string::npos) {
                matched_profiles[profile].insert("profile-name scan containing 'godot'");
            }
        }
    }

    if (matched_profiles.empty()) {
        std::cout << "No DRS application association or Godot-named profile was found.\n";
        destroy_session(session);
        unload();
        return 0;
    }

    const std::vector<std::pair<NvU32, std::string>> relevant_settings = {
        {0x20C1221E, "OpenGL threaded optimization"},
        {0x1094F1F7, "VRR requested state"},
        {0x10A879CF, "G-SYNC application override"},
        {0x10A879AC, "G-SYNC application override request state"},
        {0x1194F158, "G-SYNC mode"},
        {0x10A879CE, "VSync/VRR control"},
    };

    int profile_number = 0;
    for (const auto &[profile, reasons] : matched_profiles) {
        ++profile_number;
        NVDRS_PROFILE profile_info{};
        profile_info.version = NVDRS_PROFILE_VER;
        status = get_profile_info(session, profile, &profile_info);
        if (status != NVAPI_OK) {
            std::cout << "Profile " << profile_number << ": GetProfileInfo failed: " << status << '\n';
            continue;
        }

        std::cout << "\nProfile " << profile_number << ":\n";
        std::cout << "  discovered_by=";
        bool first_reason = true;
        for (const auto &reason : reasons) {
            std::cout << (first_reason ? "" : "; ") << reason;
            first_reason = false;
        }
        std::cout << '\n';
        std::cout << "  name=\"" << utf8(profile_info.profileName) << "\"\n";
        std::cout << "  predefined=" << profile_info.isPredefined << '\n';
        std::cout << "  application_count=" << profile_info.numOfApps << '\n';
        std::cout << "  setting_count=" << profile_info.numOfSettings << '\n';

        if (profile_info.numOfApps != 0) {
            std::vector<NVDRS_APPLICATION> applications(profile_info.numOfApps);
            for (auto &application : applications) {
                application.version = NVDRS_APPLICATION_VER;
            }
            NvU32 application_count = profile_info.numOfApps;
            status = enum_applications(session, profile, 0, &application_count, applications.data());
            std::cout << "  applications (status=" << status << ", returned=" << application_count << "):\n";
            if (status == NVAPI_OK) {
                for (NvU32 index = 0; index < application_count; ++index) {
                    const auto &application = applications[index];
                    std::cout << "    - appName=\"" << utf8(application.appName)
                              << "\", userFriendlyName=\"" << utf8(application.userFriendlyName)
                              << "\", launcher=\"" << utf8(application.launcher)
                              << "\", fileInFolder=\"" << utf8(application.fileInFolder)
                              << "\", predefined=" << application.isPredefined
                              << ", isMetro=" << application.isMetro
                              << ", isCommandLine=" << application.isCommandLine
                              << ", commandLine=\"" << utf8(application.commandLine) << "\"\n";
                }
            }
        }

        if (profile_info.numOfSettings != 0) {
            std::vector<NVDRS_SETTING> settings(profile_info.numOfSettings);
            for (auto &setting : settings) {
                setting.version = NVDRS_SETTING_VER;
            }
            NvU32 setting_count = profile_info.numOfSettings;
            status = enum_settings(session, profile, 0, &setting_count, settings.data());
            std::cout << "  enumerated profile settings (status=" << status << ", returned=" << setting_count << "):\n";
            if (status == NVAPI_OK) {
                for (NvU32 index = 0; index < setting_count; ++index) {
                    print_setting(settings[index], "    - ");
                }
            }
        }

        std::cout << "  effective relevant settings:\n";
        for (const auto &[id, label] : relevant_settings) {
            NVDRS_SETTING setting{};
            setting.version = NVDRS_SETTING_VER;
            status = get_setting(session, profile, id, &setting);
            std::cout << "    " << label << ": ";
            if (status == NVAPI_OK) {
                print_setting(setting, "");
            } else {
                std::cout << "not returned (status=" << status << ")\n";
            }
        }
    }

    destroy_session(session);
    unload();
    return 0;
}
