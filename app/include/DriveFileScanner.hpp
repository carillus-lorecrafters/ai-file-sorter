#ifndef DRIVEFILESCANNER_HPP
#define DRIVEFILESCANNER_HPP

#include "GwsClient.hpp"
#include "Types.hpp"

#include <string>
#include <vector>

/**
 * @brief Scans a Google Drive folder via the `gws` CLI and returns FileEntry objects.
 *
 * file_path in each FileEntry is encoded as "drive:<Drive-file-id>" so that
 * downstream components (CategorizationService, cache, DriveFileOperations) can
 * identify Drive items unambiguously.
 */
class DriveFileScanner {
public:
    /**
     * @brief Construct a DriveFileScanner.
     * @param gws_client Configured GwsClient to use for subprocess calls.
     * @param page_delay_ms Milliseconds to pause between Drive API pages.
     */
    explicit DriveFileScanner(const GwsClient& gws_client,
                               int page_delay_ms = 100);

    /**
     * @brief List files in a Drive folder.
     * @param folder_id Drive folder ID, or "root" for My Drive root.
     * @return Vector of FileEntry objects; full_path is "drive:<fileId>".
     */
    std::vector<FileEntry> list_files(const std::string& folder_id) const;

    /**
     * @brief Encode a Drive file ID into a full_path string.
     */
    static std::string make_drive_path(const std::string& file_id);

    /**
     * @brief Extract the Drive file ID from a full_path encoded by make_drive_path.
     * Returns empty string if the path is not a Drive path.
     */
    static std::string extract_file_id(const std::string& full_path);

    /**
     * @brief Returns true when the full_path field encodes a Drive file reference.
     */
    static bool is_drive_path(const std::string& full_path);

private:
    const GwsClient& gws_;
    int page_delay_ms_;

    static FileType mime_to_file_type(const std::string& mime_type);
    static std::string derive_extension(const std::string& file_name,
                                         const std::string& mime_type);
};

#endif // DRIVEFILESCANNER_HPP
