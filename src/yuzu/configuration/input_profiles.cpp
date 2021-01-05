// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <fmt/format.h>

#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "common/logging/log.h"
#include "yuzu/configuration/config.h"
#include "yuzu/configuration/input_profiles.h"

namespace FS = Common::FS;

namespace {

bool ProfileExistsInFilesystem(std::string_view profile_name) {
    return FS::Exists(FS::GetYuzuPath(FS::YuzuPath::ConfigDir) / "input" /
                      fmt::format("{}.ini", profile_name));
}

bool IsINI(const std::filesystem::path& filename) {
    return filename.extension() == ".ini";
}

std::filesystem::path GetNameWithoutExtension(std::filesystem::path filename) {
    return filename.replace_extension();
}

} // namespace

InputProfiles::InputProfiles() {
    const auto input_profile_loc = FS::GetYuzuPath(FS::YuzuPath::ConfigDir) / "input";

    if (!FS::IsDir(input_profile_loc)) {
        return;
    }

    FS::IterateDirEntries(
        input_profile_loc,
        [this](const std::filesystem::path& full_path) {
            const auto filename = full_path.filename();
            const auto name_without_ext =
                Common::FS::PathToUTF8String(GetNameWithoutExtension(filename));

            if (IsINI(filename) && IsProfileNameValid(name_without_ext)) {
                map_profiles_old.insert_or_assign(
                    name_without_ext,
                    std::make_unique<Config>(name_without_ext, Config::ConfigType::InputProfile));
            }

            return true;
        },
        FS::DirEntryFilter::File);

    for (const auto& profile : map_profiles_old) {
        Settings::InputProfile new_profile;
        profile.second->ReadToProfileStruct(new_profile);

        map_profiles.insert_or_assign(profile.first, new_profile);
    }
}

InputProfiles::~InputProfiles() = default;

std::vector<std::string> InputProfiles::GetInputProfileNames() {
    std::vector<std::string> profile_names;
    profile_names.reserve(map_profiles.size());

    for (const auto& profile : map_profiles) {
        profile_names.push_back(profile.first);
    }

    return profile_names;
}

bool InputProfiles::IsProfileNameValid(std::string_view profile_name) {
    return profile_name.find_first_of("<>:;\"/\\|,.!?*") == std::string::npos;
}

void InputProfiles::ApplyConfiguration() {
    // First delete all profiles that were removed during configuration...
    for (const auto& old_profile : map_profiles_old) {
        if (!map_profiles.contains(old_profile.first)) {
            FS::RemoveFile(old_profile.second->GetConfigFilePath());
        }
    }

    // ...and then write all the remaining and new ones.
    for (const auto& profile : map_profiles) {
        const auto& config_profile =
            std::make_unique<Config>(profile.first, Config::ConfigType::InputProfile);

        config_profile->WriteFromProfileStruct(profile.second);
    }
}

bool InputProfiles::ProfileExistsInMap(const std::string& profile_name) const {
    return map_profiles.contains(profile_name);
}
