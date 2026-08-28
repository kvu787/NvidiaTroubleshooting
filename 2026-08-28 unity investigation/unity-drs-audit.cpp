#include <windows.h>

#include <algorithm>
#include <cctype>
#include <fstream>
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

struct SettingReference {
    NvU32 id{};
    std::string name;
    std::string group;
    std::string description;
};

std::string utf8(const wchar_t *text) {
    if (text == nullptr || *text == L'\0') return "";
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

std::wstring wide(const std::string &text) {
    if (text.empty()) return L"";
    const int length = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring result(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), length);
    return result;
}

std::wstring basename(const std::wstring &path) {
    const auto separator = path.find_last_of(L"\\/");
    return separator == std::wstring::npos ? path : path.substr(separator + 1);
}

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

bool contains_unity(const std::string &value) {
    return lower(value).find("unity") != std::string::npos;
}

std::string hex32(NvU32 value) {
    std::ostringstream output;
    output << "0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << value;
    return output.str();
}

const char *location_name(NVDRS_SETTING_LOCATION location) {
    switch (location) {
        case NVDRS_CURRENT_PROFILE_LOCATION: return "current profile";
        case NVDRS_GLOBAL_PROFILE_LOCATION: return "global profile";
        case NVDRS_BASE_PROFILE_LOCATION: return "base profile";
        case NVDRS_DEFAULT_PROFILE_LOCATION: return "driver default";
        default: return "unknown";
    }
}

const char *type_name(NVDRS_SETTING_TYPE type) {
    switch (type) {
        case NVDRS_DWORD_TYPE: return "DWORD";
        case NVDRS_BINARY_TYPE: return "binary";
        case NVDRS_STRING_TYPE: return "string";
        case NVDRS_WSTRING_TYPE: return "wide string";
        default: return "unknown";
    }
}

std::string binary_value(const NVDRS_BINARY_SETTING &binary) {
    std::ostringstream output;
    output << "hex:";
    for (NvU32 index = 0; index < binary.valueLength; ++index) {
        output << std::hex << std::setw(2) << std::setfill('0')
               << static_cast<unsigned int>(binary.valueData[index]);
    }
    return output.str();
}

std::string setting_value(const NVDRS_SETTING &setting, bool predefined) {
    switch (setting.settingType) {
        case NVDRS_DWORD_TYPE: {
            const NvU32 value = predefined ? setting.u32PredefinedValue : setting.u32CurrentValue;
            return hex32(value) + " (" + std::to_string(value) + ")";
        }
        case NVDRS_STRING_TYPE:
        case NVDRS_WSTRING_TYPE:
            return "\"" + utf8(predefined ? setting.wszPredefinedValue : setting.wszCurrentValue) + "\"";
        case NVDRS_BINARY_TYPE:
            return binary_value(predefined ? setting.binaryPredefinedValue : setting.binaryCurrentValue);
        default:
            return "unknown";
    }
}

void print_setting(const NVDRS_SETTING &setting, const SettingReference *reference, const std::string &indent) {
    std::string name = utf8(setting.settingName);
    if (name.empty() && reference != nullptr) name = reference->name;
    std::cout << indent << "id=" << hex32(setting.settingId)
              << ", name=" << std::quoted(name)
              << ", type=" << type_name(setting.settingType)
              << ", location=" << location_name(setting.settingLocation)
              << ", current_predefined=" << setting.isCurrentPredefined
              << ", predefined_valid=" << setting.isPredefinedValid
              << ", current=" << setting_value(setting, false);
    if (setting.isPredefinedValid) std::cout << ", predefined=" << setting_value(setting, true);
    if (reference != nullptr) {
        std::cout << ", reference_group=" << std::quoted(reference->group)
                  << ", reference_description=" << std::quoted(reference->description);
    }
    std::cout << '\n';
}

template <typename Function>
Function load(QueryInterface query, unsigned int id, const char *name) {
    auto function = reinterpret_cast<Function>(query(id));
    if (function == nullptr) {
        std::cerr << "Missing NVAPI function: " << name << '\n';
        ExitProcess(2);
    }
    return function;
}

std::vector<std::wstring> load_targets(const char *path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Could not open target inventory");
    std::vector<std::wstring> targets;
    std::string line;
    bool first = true;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (first) {
            first = false;
            if (line.rfind("Path\t", 0) == 0 || line == "Path") continue;
        }
        const auto tab = line.find('\t');
        const std::string path_column = line.substr(0, tab);
        if (!path_column.empty()) targets.push_back(wide(path_column));
    }
    return targets;
}

std::vector<SettingReference> load_references(const char *path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Could not open setting reference inventory");
    std::vector<SettingReference> references;
    std::string line;
    bool first = true;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (first) {
            first = false;
            if (line.rfind("Id\t", 0) == 0) continue;
        }
        std::vector<std::string> fields;
        size_t start = 0;
        while (true) {
            const auto tab = line.find('\t', start);
            fields.push_back(line.substr(start, tab == std::string::npos ? tab : tab - start));
            if (tab == std::string::npos) break;
            start = tab + 1;
        }
        if (fields.size() < 4 || fields[0].size() < 3) continue;
        SettingReference reference;
        reference.id = static_cast<NvU32>(std::stoul(fields[0].substr(2), nullptr, 16));
        reference.name = fields[1];
        reference.group = fields[2];
        reference.description = fields[3];
        references.push_back(std::move(reference));
    }
    std::sort(references.begin(), references.end(), [](const auto &left, const auto &right) {
        return left.id < right.id;
    });
    references.erase(std::unique(references.begin(), references.end(), [](const auto &left, const auto &right) {
        return left.id == right.id;
    }), references.end());
    return references;
}

}  // namespace

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Usage: unity-drs-audit.exe <unity-executables.tsv> <setting-reference.tsv>\n";
        return 2;
    }

    std::vector<std::wstring> targets;
    std::vector<SettingReference> references;
    try {
        targets = load_targets(argv[1]);
        references = load_references(argv[2]);
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 2;
    }

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
    const auto get_base_profile = load<decltype(&NvAPI_DRS_GetBaseProfile)>(query, 0xDA8466A0, "NvAPI_DRS_GetBaseProfile");
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

    std::cout << "Read-only NVAPI DRS audit\n";
    std::cout << "Target executable count: " << targets.size() << '\n';
    std::cout << "Reference setting ID count: " << references.size() << "\n\n";

    std::map<NvDRSProfileHandle, std::set<std::string>> matched_profiles;
    std::cout << "=== TARGET APPLICATION LOOKUPS ===\n";
    for (const auto &target : targets) {
        std::cout << "target=" << std::quoted(utf8(target.c_str())) << '\n';
        const std::vector<std::pair<std::string, std::wstring>> searches = {
            {"full path", target}, {"basename", basename(target)}
        };
        for (const auto &[label, search] : searches) {
            NVDRS_APPLICATION application{};
            application.version = NVDRS_APPLICATION_VER;
            NvDRSProfileHandle profile = nullptr;
            status = find_application(session,
                reinterpret_cast<NvU16 *>(const_cast<wchar_t *>(search.c_str())), &profile, &application);
            std::cout << "  " << label << ": status=" << status;
            if (status == NVAPI_OK) {
                NVDRS_PROFILE info{};
                info.version = NVDRS_PROFILE_VER;
                const auto info_status = get_profile_info(session, profile, &info);
                std::cout << ", profile_status=" << info_status;
                if (info_status == NVAPI_OK) std::cout << ", profile=" << std::quoted(utf8(info.profileName));
                std::cout << ", matched_app=" << std::quoted(utf8(application.appName));
                matched_profiles[profile].insert("target lookup: " + utf8(target.c_str()) + " (" + label + ")");
            }
            std::cout << '\n';
        }
    }

    std::cout << "\n=== TARGET-DERIVED PROFILE-NAME LOOKUPS ===\n";
    std::set<std::wstring> candidate_profile_names;
    for (const auto &target : targets) {
        const std::wstring filename = basename(target);
        candidate_profile_names.insert(filename);
        const auto dot = filename.find_last_of(L'.');
        if (dot != std::wstring::npos) candidate_profile_names.insert(filename.substr(0, dot));
    }
    for (const auto &candidate : candidate_profile_names) {
        NvDRSProfileHandle profile = nullptr;
        status = find_profile(session,
            reinterpret_cast<NvU16 *>(const_cast<wchar_t *>(candidate.c_str())), &profile);
        std::cout << "candidate=" << std::quoted(utf8(candidate.c_str())) << ", status=" << status;
        if (status == NVAPI_OK) {
            NVDRS_PROFILE info{};
            info.version = NVDRS_PROFILE_VER;
            const auto info_status = get_profile_info(session, profile, &info);
            std::cout << ", profile_status=" << info_status;
            if (info_status == NVAPI_OK) std::cout << ", profile=" << std::quoted(utf8(info.profileName));
            matched_profiles[profile].insert("target-derived exact profile-name lookup: " + utf8(candidate.c_str()));
        }
        std::cout << '\n';
    }

    NvU32 total_profiles = 0;
    status = get_num_profiles(session, &total_profiles);
    std::cout << "\n=== ALL-PROFILE UNITY STRING SCAN ===\n";
    std::cout << "GetNumProfiles status=" << status << ", count=" << total_profiles << '\n';
    if (status == NVAPI_OK) {
        for (NvU32 index = 0; index < total_profiles; ++index) {
            NvDRSProfileHandle profile = nullptr;
            if (enum_profiles(session, index, &profile) != NVAPI_OK) continue;
            NVDRS_PROFILE info{};
            info.version = NVDRS_PROFILE_VER;
            if (get_profile_info(session, profile, &info) != NVAPI_OK) continue;
            if (contains_unity(utf8(info.profileName)) || lower(utf8(info.profileName)).find("zoomtracks") != std::string::npos) {
                matched_profiles[profile].insert("profile name contains 'unity' or 'zoomtracks'");
            }
            if (info.numOfApps == 0) continue;
            std::vector<NVDRS_APPLICATION> applications(info.numOfApps);
            for (auto &application : applications) application.version = NVDRS_APPLICATION_VER;
            NvU32 count = info.numOfApps;
            if (enum_applications(session, profile, 0, &count, applications.data()) != NVAPI_OK) continue;
            for (NvU32 app_index = 0; app_index < count; ++app_index) {
                const auto &application = applications[app_index];
                if (contains_unity(utf8(application.appName)) ||
                    contains_unity(utf8(application.userFriendlyName)) ||
                    contains_unity(utf8(application.launcher)) ||
                    contains_unity(utf8(application.fileInFolder)) ||
                    contains_unity(utf8(application.commandLine))) {
                    matched_profiles[profile].insert("application metadata contains 'unity'");
                }
            }
        }
    }

    std::map<NvU32, const SettingReference *> reference_by_id;
    for (const auto &reference : references) reference_by_id[reference.id] = &reference;

    auto dump_profile = [&](NvDRSProfileHandle profile, const std::set<std::string> &reasons, bool effective) {
        NVDRS_PROFILE info{};
        info.version = NVDRS_PROFILE_VER;
        status = get_profile_info(session, profile, &info);
        if (status != NVAPI_OK) {
            std::cout << "GetProfileInfo failed: " << status << '\n';
            return;
        }
        std::cout << "profile=" << std::quoted(utf8(info.profileName))
                  << ", predefined=" << info.isPredefined
                  << ", app_count=" << info.numOfApps
                  << ", explicit_setting_count=" << info.numOfSettings << '\n';
        if (!reasons.empty()) {
            std::cout << "reasons:\n";
            for (const auto &reason : reasons) std::cout << "  - " << reason << '\n';
        }
        if (info.numOfApps != 0) {
            std::vector<NVDRS_APPLICATION> applications(info.numOfApps);
            for (auto &application : applications) application.version = NVDRS_APPLICATION_VER;
            NvU32 count = info.numOfApps;
            status = enum_applications(session, profile, 0, &count, applications.data());
            std::cout << "applications: status=" << status << ", returned=" << count << '\n';
            if (status == NVAPI_OK) {
                for (NvU32 index = 0; index < count; ++index) {
                    const auto &application = applications[index];
                    std::cout << "  - app=" << std::quoted(utf8(application.appName))
                              << ", friendly=" << std::quoted(utf8(application.userFriendlyName))
                              << ", launcher=" << std::quoted(utf8(application.launcher))
                              << ", file_in_folder=" << std::quoted(utf8(application.fileInFolder))
                              << ", command_line=" << std::quoted(utf8(application.commandLine))
                              << ", predefined=" << application.isPredefined
                              << ", metro=" << application.isMetro
                              << ", command_line_rule=" << application.isCommandLine << '\n';
                }
            }
        }
        if (info.numOfSettings != 0) {
            std::vector<NVDRS_SETTING> settings(info.numOfSettings);
            for (auto &setting : settings) setting.version = NVDRS_SETTING_VER;
            NvU32 count = info.numOfSettings;
            status = enum_settings(session, profile, 0, &count, settings.data());
            std::cout << "explicit settings: status=" << status << ", returned=" << count << '\n';
            if (status == NVAPI_OK) {
                for (NvU32 index = 0; index < count; ++index) {
                    const auto found = reference_by_id.find(settings[index].settingId);
                    print_setting(settings[index], found == reference_by_id.end() ? nullptr : found->second, "  - ");
                }
            }
        }
        if (effective) {
            std::cout << "effective known settings (all reference IDs for which NvAPI_DRS_GetSetting succeeds):\n";
            NvU32 returned = 0;
            for (const auto &reference : references) {
                NVDRS_SETTING setting{};
                setting.version = NVDRS_SETTING_VER;
                if (get_setting(session, profile, reference.id, &setting) == NVAPI_OK) {
                    print_setting(setting, &reference, "  - ");
                    ++returned;
                }
            }
            std::cout << "effective known setting count=" << returned << '\n';
        }
    };

    std::cout << "\n=== MATCHED UNITY-RELATED DRS PROFILES ===\n";
    std::cout << "matched profile count=" << matched_profiles.size() << '\n';
    for (const auto &[profile, reasons] : matched_profiles) {
        std::cout << "\n--- PROFILE ---\n";
        dump_profile(profile, reasons, true);
    }

    std::cout << "\n=== GLOBAL/BASE DRS PROFILE ===\n";
    NvDRSProfileHandle base_profile = nullptr;
    status = get_base_profile(session, &base_profile);
    std::cout << "GetBaseProfile status=" << status << '\n';
    if (status == NVAPI_OK) dump_profile(base_profile, {}, false);

    destroy_session(session);
    unload();
    return 0;
}
