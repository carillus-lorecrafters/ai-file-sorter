# Remaining Tasks — Google Drive Integration

Branch: `feat/google-drive-integration`
Fork: `carillus-lorecrafters/ai-file-sorter`

---

## 1. UI toggle for Drive mode

**File:** `app/lib/MainAppUiBuilder.cpp` (and `app/include/MainAppUiBuilder.hpp`)

Add a "Google Drive" source option to the source selection area of the main window:
- A checkbox or radio button to toggle Drive mode (`settings.set_use_google_drive()`)
- A text field for the Drive root folder ID (`settings.set_drive_root_folder_id()`)
- A file picker for the `gws` credentials file (`settings.set_gws_credentials_file()`)
- When Drive mode is active, disable the local folder path entry and browse button
- On toggle, call `MainApp::rebuild_drive_clients()` to re-initialize Drive clients with updated settings
- Persist state via `sync_ui_to_settings()` / `sync_settings_to_ui()`

---

## 2. gws authentication setup

No code change required — this is a user setup step — but the app should surface a clear error when `gws` is not found or not authenticated.

**Suggested improvement:** In `MainApp::rebuild_drive_clients()` or `MainApp::on_analyze_clicked()`, when Drive mode is active, run a lightweight `gws drive about get` probe call. If it fails, show a dialog explaining:
- Install: `npm install -g @googleworkspace/cli`
- Auth: `gws auth setup`
- Or set `GOOGLE_WORKSPACE_CLI_CREDENTIALS_FILE` to a service account JSON path

---

## 3. Capture and propagate parent folder ID for moves

**Problem:** `DriveFileOperations::move_to_category` receives an empty `current_parent_id`, so the `removeParents` parameter sent to the Drive API is wrong (falls back to `root_folder_id_`), which may fail or silently leave the file in two folders.

**Fix:**
- In `DriveFileScanner::list_files()`, the `parents` field is already requested in the API query. Store the first parent ID in the `FileEntry`.
- `FileEntry` (in `app/include/Types.hpp`) needs a new field: `std::string parent_id`.
- `DriveFileScanner::list_files()` should populate it: `entry.parent_id = file["parents"][0].asString()`.
- `CategorizedFile` (also in `Types.hpp`) needs a matching `std::string parent_id` field.
- `CategorizationService` should copy `parent_id` from `FileEntry` → `CategorizedFile`.
- In `MainApp.cpp`, the Drive move callback should pass `entry.parent_id` as `current_parent_id` to `move_to_category`.

---

## 4. Build verification

No CMake build has been run against the Qt6/vcpkg toolchain. Before merging:

```bash
cd app
cmake -B build -DCMAKE_BUILD_TYPE=Debug   # configure (requires Qt6 + vcpkg)
cmake --build build --target aifilesorter -- -j$(nproc)
```

Known risks to check:
- `GwsClient.cpp` links against `Qt6::Core` (for `QProcess`) — ensure it is listed in `target_link_libraries` if not already inherited
- `jsoncpp` include path (`<json/json.h>`) — confirm vcpkg provides this header name on the build host
- `DriveFileScanner.cpp` includes `<QThread>` — only needed if sleep is added; remove if unused

---

## 5. Unit tests

Add to `tests/unit/`:

| Test file | What to test |
|---|---|
| `test_gws_client.cpp` | Mock subprocess via a shell script that writes fixture JSON; verify `run()` return value and `run_ndjson()` callback invocations |
| `test_drive_file_scanner.cpp` | Feed mock NDJSON pages to a `GwsClient` stub; assert correct `FileEntry` fields and `is_drive_path()` / `extract_file_id()` helpers |
| `test_drive_file_operations.cpp` | Stub `GwsClient::run()` to capture args; verify correct `--fileId`, `--params`, and `--json` arguments for rename, move, and folder creation |

Register new test files in `app/CMakeLists.txt` alongside the existing `add_executable(ai_file_sorter_tests ...)` block.
