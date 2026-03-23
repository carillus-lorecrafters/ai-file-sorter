#ifndef GWSCLIENT_HPP
#define GWSCLIENT_HPP

#include <functional>
#include <string>
#include <vector>
#include <json/json.h>

/**
 * @brief Thin wrapper around the `gws` (Google Workspace CLI) subprocess.
 *
 * Invokes gws with given arguments, captures stdout, and parses JSON/NDJSON.
 * Sets GOOGLE_WORKSPACE_CLI_CREDENTIALS_FILE in the child environment when
 * a credentials file is configured.
 */
class GwsClient {
public:
    /**
     * @brief Construct a GwsClient.
     * @param gws_binary Path to the gws binary (default: "gws", resolved via PATH).
     * @param credentials_file Path to the service-account JSON or OAuth export file.
     *        If empty, the environment is inherited as-is.
     */
    explicit GwsClient(const std::string& gws_binary = "gws",
                       const std::string& credentials_file = "");

    /**
     * @brief Run a gws command and return the full stdout as a string.
     * @param args Arguments to pass after the binary name.
     * @return stdout of the completed process.
     * @throws std::runtime_error on non-zero exit code or process launch failure.
     */
    std::string run(const std::vector<std::string>& args) const;

    /**
     * @brief Run a gws command and invoke a callback for each NDJSON line.
     *
     * Reads stdout line-by-line. Each non-empty line is parsed as JSON and
     * forwarded to the callback. If the callback returns false the remaining
     * lines are discarded and the child process is killed.
     *
     * @param args Arguments to pass after the binary name.
     * @param on_object Called with each parsed JSON object. Return false to stop.
     * @throws std::runtime_error on process launch failure.
     */
    void run_ndjson(const std::vector<std::string>& args,
                    const std::function<bool(const Json::Value&)>& on_object) const;

    /**
     * @brief Parse a JSON string into a Json::Value.
     * @param json_str Raw JSON text.
     * @return Parsed Json::Value; returns Json::nullValue on parse failure.
     */
    static Json::Value parse_json(const std::string& json_str);

    const std::string& binary() const { return gws_binary_; }
    const std::string& credentials_file() const { return credentials_file_; }

private:
    std::string gws_binary_;
    std::string credentials_file_;
};

#endif // GWSCLIENT_HPP
