#ifndef DRIVEFILEOPERATIONS_HPP
#define DRIVEFILEOPERATIONS_HPP

#include "DriveFileScanner.hpp"
#include "GwsClient.hpp"

#include <string>

/**
 * @brief Performs rename and move operations on Google Drive files via `gws`.
 *
 * All operations use metadata-only Drive API calls — file contents are never
 * downloaded or uploaded.
 */
class DriveFileOperations {
public:
    /**
     * @brief Construct a DriveFileOperations helper.
     * @param gws_client Configured GwsClient to use for subprocess calls.
     * @param root_folder_id Drive folder ID that acts as the sort destination root.
     *        Category sub-folders will be created inside this folder.
     *        Use "root" for My Drive root.
     */
    explicit DriveFileOperations(const GwsClient& gws_client,
                                  const std::string& root_folder_id);

    /**
     * @brief Rename a Drive file.
     * @param drive_file_id Drive file ID (not the "drive:<id>" encoding).
     * @param new_name New filename (including extension).
     * @return True on success, false on failure.
     */
    bool rename_file(const std::string& drive_file_id,
                     const std::string& new_name) const;

    /**
     * @brief Move (and optionally rename) a Drive file into a category folder.
     *
     * Creates category and subcategory folders under root_folder_id if they do
     * not already exist, then moves the file there.
     *
     * @param drive_file_id Drive file ID.
     * @param current_parent_id Current parent folder ID of the file.
     * @param new_name Destination filename (use original name if unchanged).
     * @param category Category folder name.
     * @param subcategory Subcategory folder name.
     * @param use_subcategory When false, only the category folder level is used.
     * @return True on success, false on failure.
     */
    bool move_to_category(const std::string& drive_file_id,
                           const std::string& current_parent_id,
                           const std::string& new_name,
                           const std::string& category,
                           const std::string& subcategory,
                           bool use_subcategory) const;

    /**
     * @brief Return or create a Drive folder by name under the given parent.
     * @param parent_folder_id Parent folder ID.
     * @param folder_name Name for the child folder.
     * @return Drive folder ID of the existing or newly-created folder.
     *         Empty string on failure.
     */
    std::string get_or_create_folder(const std::string& parent_folder_id,
                                      const std::string& folder_name) const;

    /**
     * @brief Build a human-readable destination path for preview purposes.
     * @param category Category folder name.
     * @param subcategory Subcategory folder name.
     * @param file_name Destination file name.
     * @param use_subcategory Whether the subcategory level is shown.
     * @return Display string such as "Google Drive / Photos / 2024 / photo.jpg".
     */
    std::string preview_destination(const std::string& category,
                                     const std::string& subcategory,
                                     const std::string& file_name,
                                     bool use_subcategory) const;

    const std::string& root_folder_id() const { return root_folder_id_; }

private:
    const GwsClient& gws_;
    std::string root_folder_id_;
};

#endif // DRIVEFILEOPERATIONS_HPP
