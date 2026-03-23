#include "DriveFileScanner.hpp"
#include "Logger.hpp"

#include <QThread>
#include <json/json.h>
#include <spdlog/spdlog.h>

#include <stdexcept>
#include <string>

static constexpr const char* kDrivePrefix = "drive:";
static constexpr const char* kFolderMime = "application/vnd.google-apps.folder";

DriveFileScanner::DriveFileScanner(const GwsClient& gws_client, int page_delay_ms)
    : gws_(gws_client)
    , page_delay_ms_(page_delay_ms)
{}

std::vector<FileEntry> DriveFileScanner::list_files(const std::string& folder_id) const
{
    const std::string effective_folder = folder_id.empty() ? "root" : folder_id;

    // Build Drive query: files in this folder, not trashed
    const std::string query = "\"" + effective_folder +
                              "\" in parents and trashed=false";
    const std::string params_json =
        "{\"q\":\"" + query + "\","
        "\"fields\":\"files(id,name,mimeType,parents)\","
        "\"pageSize\":\"1000\"}";

    const std::vector<std::string> args = {
        "drive", "files", "list",
        "--params", params_json,
        "--page-all",
        "--page-delay", std::to_string(page_delay_ms_)
    };

    auto logger = Logger::get_logger("core_logger");
    if (logger) {
        logger->info("DriveFileScanner: listing folder '{}'", effective_folder);
    }

    std::vector<FileEntry> results;

    gws_.run_ndjson(args, [&](const Json::Value& page) -> bool {
        const Json::Value& files = page["files"];
        if (!files.isArray()) {
            return true; // continue, might be a wrapper page
        }
        for (const auto& file : files) {
            const std::string id = file.get("id", "").asString();
            const std::string name = file.get("name", "").asString();
            const std::string mime = file.get("mimeType", "").asString();

            if (id.empty() || name.empty()) {
                continue;
            }

            FileEntry entry;
            entry.full_path = make_drive_path(id);
            entry.file_name = name;
            entry.type = mime_to_file_type(mime);
            results.push_back(std::move(entry));
        }
        return true; // always consume all pages
    });

    if (logger) {
        logger->info("DriveFileScanner: found {} items in folder '{}'",
                     results.size(), effective_folder);
    }

    return results;
}

std::string DriveFileScanner::make_drive_path(const std::string& file_id)
{
    return std::string(kDrivePrefix) + file_id;
}

std::string DriveFileScanner::extract_file_id(const std::string& full_path)
{
    if (!is_drive_path(full_path)) {
        return {};
    }
    return full_path.substr(std::string(kDrivePrefix).size());
}

bool DriveFileScanner::is_drive_path(const std::string& full_path)
{
    return full_path.rfind(kDrivePrefix, 0) == 0;
}

FileType DriveFileScanner::mime_to_file_type(const std::string& mime_type)
{
    if (mime_type == kFolderMime) {
        return FileType::Directory;
    }
    return FileType::File;
}
