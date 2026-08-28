#include <windows.h>

#include <algorithm>
#include <cctype>
#include <iostream>
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

std::string ascii_lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

bool contains(const std::string &value, const std::string &lower_token) {
    return ascii_lower(value).find(lower_token) != std::string::npos;
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
    const std::string token = ascii_lower(argc > 1 ? utf8(argv[1]) : "godot");
    if (token.empty()) {
        std::cerr << "The search token must not be empty.\n";
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
    const auto get_num_profiles = load<decltype(&NvAPI_DRS_GetNumProfiles)>(query, 0x1DAE4FBC, "NvAPI_DRS_GetNumProfiles");
    const auto enum_profiles = load<decltype(&NvAPI_DRS_EnumProfiles)>(query, 0xBC371EE0, "NvAPI_DRS_EnumProfiles");
    const auto get_profile_info = load<decltype(&NvAPI_DRS_GetProfileInfo)>(query, 0x61CD6FD6, "NvAPI_DRS_GetProfileInfo");
    const auto enum_applications = load<decltype(&NvAPI_DRS_EnumApplications)>(query, 0x7FA2173A, "NvAPI_DRS_EnumApplications");

    NvAPI_Status status = initialize();
    if (status != NVAPI_OK) {
        std::cerr << "NvAPI_Initialize failed: " << status << "\n";
        return 2;
    }

    NvDRSSessionHandle session = nullptr;
    status = create_session(&session);
    if (status != NVAPI_OK) {
        std::cerr << "NvAPI_DRS_CreateSession failed: " << status << "\n";
        unload();
        return 2;
    }

    status = load_settings(session);
    if (status != NVAPI_OK) {
        std::cerr << "NvAPI_DRS_LoadSettings failed: " << status << "\n";
        destroy_session(session);
        unload();
        return 2;
    }

    NvU32 total_profiles = 0;
    status = get_num_profiles(session, &total_profiles);
    if (status != NVAPI_OK) {
        std::cerr << "NvAPI_DRS_GetNumProfiles failed: " << status << "\n";
        destroy_session(session);
        unload();
        return 2;
    }

    NvU32 matching_profiles = 0;
    NvU32 matching_applications = 0;
    NvU32 profile_failures = 0;
    NvU32 application_failures = 0;

    std::cout << "Case-insensitive DRS substring audit\n";
    std::cout << "Search token: \"" << token << "\"\n";
    std::cout << "Total profiles reported: " << total_profiles << "\n";

    for (NvU32 profile_index = 0; profile_index < total_profiles; ++profile_index) {
        NvDRSProfileHandle profile = nullptr;
        status = enum_profiles(session, profile_index, &profile);
        if (status != NVAPI_OK) {
            ++profile_failures;
            continue;
        }

        NVDRS_PROFILE profile_info{};
        profile_info.version = NVDRS_PROFILE_VER;
        status = get_profile_info(session, profile, &profile_info);
        if (status != NVAPI_OK) {
            ++profile_failures;
            continue;
        }

        const std::string profile_name = utf8(profile_info.profileName);
        const bool profile_name_matches = contains(profile_name, token);
        bool profile_has_match = profile_name_matches;
        std::vector<std::string> application_matches;

        if (profile_info.numOfApps != 0) {
            std::vector<NVDRS_APPLICATION> applications(profile_info.numOfApps);
            for (auto &application : applications) {
                application.version = NVDRS_APPLICATION_VER;
            }

            NvU32 application_count = profile_info.numOfApps;
            status = enum_applications(session, profile, 0, &application_count, applications.data());
            if (status != NVAPI_OK) {
                ++application_failures;
            } else {
                for (NvU32 application_index = 0; application_index < application_count; ++application_index) {
                    const auto &application = applications[application_index];
                    const std::vector<std::pair<std::string, std::string>> fields = {
                        {"appName", utf8(application.appName)},
                        {"userFriendlyName", utf8(application.userFriendlyName)},
                        {"launcher", utf8(application.launcher)},
                        {"fileInFolder", utf8(application.fileInFolder)},
                        {"commandLine", utf8(application.commandLine)},
                    };

                    for (const auto &[field_name, field_value] : fields) {
                        if (!field_value.empty() && contains(field_value, token)) {
                            application_matches.push_back(field_name + "=\"" + field_value + "\"");
                            ++matching_applications;
                            profile_has_match = true;
                            break;
                        }
                    }
                }
            }
        }

        if (profile_has_match) {
            ++matching_profiles;
            std::cout << "MATCH profile_index=" << profile_index
                      << ", profileName=\"" << profile_name << "\""
                      << ", profileNameMatched=" << (profile_name_matches ? 1 : 0)
                      << ", applicationCount=" << profile_info.numOfApps << "\n";
            for (const auto &application_match : application_matches) {
                std::cout << "  " << application_match << "\n";
            }
        }
    }

    std::cout << "Profiles scanned: " << (total_profiles - profile_failures) << "\n";
    std::cout << "Matching profiles: " << matching_profiles << "\n";
    std::cout << "Matching applications: " << matching_applications << "\n";
    std::cout << "Profile enumeration/info failures: " << profile_failures << "\n";
    std::cout << "Application enumeration failures: " << application_failures << "\n";
    std::cout << "Audit complete: "
              << ((matching_profiles == 0 && profile_failures == 0 && application_failures == 0) ? "CLEAN" : "NOT CLEAN OR INCOMPLETE")
              << "\n";

    destroy_session(session);
    unload();
    return profile_failures == 0 && application_failures == 0 ? 0 : 1;
}
