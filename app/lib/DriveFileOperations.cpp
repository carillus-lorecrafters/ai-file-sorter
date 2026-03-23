#include "DriveFileOperations.hpp"
#include "Logger.hpp"

#include <json/json.h>
#include <spdlog/spdlog.h>

#include <stdexcept>

DriveFileOperations::DriveFileOperations(const GwsClient& gws_client,
                                          const std::string& root_folder_id)
    : gws_(gws_client)
    , root_folder_id_(root_folder_id.empty() ? "root" : root_folder_id)
{}

bool DriveFileOperations::rename_file(const std::string& drive_file_id,
                                       const std::string& new_name) const
{
    if (drive_file_id.empty() || new_name.empty()) {
        return false;
    }
    try {
        const std::string body = "{\"name\":\"" + new_name + "\"}";
        gws_.run({"drive", "files", "update",
                  "--fileId", drive_file_id,
                  "--json", body});
        return true;
    } catch (const std::exception& ex) {
        if (auto logger = Logger::get_logger("core_logger")) {
            logger->error("DriveFileOperations::rename_file failed for '{}': {}",
                          drive_file_id, ex.what());
        }
        return false;
    }
}

bool DriveFileOperations::move_to_category(const std::string& drive_file_id,
                                            const std::string& current_parent_id,
                                            const std::string& new_name,
                                            const std::string& category,
                                            const std::string& subcategory,
                                            bool use_subcategory) const
{
    if (drive_file_id.empty()) {
        return false;
    }

    // Resolve (or create) the destination folder
    const std::string cat_folder_id = get_or_create_folder(root_folder_id_, category);
    if (cat_folder_id.empty()) {
        return false;
    }

    std::string dest_folder_id = cat_folder_id;
    if (use_subcategory && !subcategory.empty() && subcategory != category) {
        dest_folder_id = get_or_create_folder(cat_folder_id, subcategory);
        if (dest_folder_id.empty()) {
            return false;
        }
    }

    try {
        // Build rename body (name field, even if unchanged — Drive ignores dupes)
        const std::string body = new_name.empty()
            ? "{}"
            : "{\"name\":\"" + new_name + "\"}";

        // addParents / removeParents as query parameters
        const std::string params_json =
            "{\"addParents\":\"" + dest_folder_id + "\","
            "\"removeParents\":\"" + (current_parent_id.empty() ? root_folder_id_ : current_parent_id) + "\"}";

        gws_.run({"drive", "files", "update",
                  "--fileId", drive_file_id,
                  "--params", params_json,
                  "--json", body});
        return true;
    } catch (const std::exception& ex) {
        if (auto logger = Logger::get_logger("core_logger")) {
            logger->error("DriveFileOperations::move_to_category failed for '{}': {}",
                          drive_file_id, ex.what());
        }
        return false;
    }
}

std::string DriveFileOperations::get_or_create_folder(
    const std::string& parent_folder_id,
    const std::string& folder_name) const
{
    // First: search for an existing folder with this name under the parent
    try {
        const std::string query =
            "\"" + parent_folder_id + "\" in parents"
            " and mimeType='application/vnd.google-apps.folder'"
            " and name='" + folder_name + "'"
            " and trashed=false";
        const std::string params = "{\"q\":\"" + query + "\","
                                    "\"fields\":\"files(id,name)\","
                                    "\"pageSize\":\"10\"}";

        const std::string result = gws_.run({
            "drive", "files", "list",
            "--params", params
        });

        const Json::Value root = GwsClient::parse_json(result);
        const Json::Value& files = root["files"];
        if (files.isArray() && !files.empty()) {
            const std::string existing_id = files[0].get("id", "").asString();
            if (!existing_id.empty()) {
                return existing_id;
            }
        }
    } catch (const std::exception& ex) {
        if (auto logger = Logger::get_logger("core_logger")) {
            logger->warn("DriveFileOperations::get_or_create_folder search failed: {}",
                         ex.what());
        }
        // Fall through to creation attempt
    }

    // Not found — create it
    try {
        const std::string body =
            "{\"name\":\"" + folder_name + "\","
            "\"mimeType\":\"application/vnd.google-apps.folder\","
            "\"parents\":[\"" + parent_folder_id + "\"]}";

        const std::string result = gws_.run({
            "drive", "files", "create",
            "--json", body,
            "--params", "{\"fields\":\"id\"}"
        });

        const Json::Value root = GwsClient::parse_json(result);
        const std::string new_id = root.get("id", "").asString();
        if (!new_id.empty()) {
            return new_id;
        }
    } catch (const std::exception& ex) {
        if (auto logger = Logger::get_logger("core_logger")) {
            logger->error("DriveFileOperations::get_or_create_folder create failed for '{}': {}",
                          folder_name, ex.what());
        }
    }

    return {};
}

std::string DriveFileOperations::preview_destination(const std::string& category,
                                                      const std::string& subcategory,
                                                      const std::string& file_name,
                                                      bool use_subcategory) const
{
    std::string path = "Google Drive / " + category;
    if (use_subcategory && !subcategory.empty() && subcategory != category) {
        path += " / " + subcategory;
    }
    path += " / " + file_name;
    return path;
}
