#include "GwsClient.hpp"
#include "Logger.hpp"

#include <QProcess>
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QProcessEnvironment>
#include <spdlog/spdlog.h>

#include <sstream>
#include <stdexcept>

GwsClient::GwsClient(const std::string& gws_binary,
                     const std::string& credentials_file)
    : gws_binary_(gws_binary)
    , credentials_file_(credentials_file)
{}

static QStringList to_qstringlist(const std::vector<std::string>& args)
{
    QStringList result;
    result.reserve(static_cast<int>(args.size()));
    for (const auto& a : args) {
        result << QString::fromStdString(a);
    }
    return result;
}

static QProcessEnvironment build_env(const std::string& credentials_file)
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!credentials_file.empty()) {
        env.insert("GOOGLE_WORKSPACE_CLI_CREDENTIALS_FILE",
                   QString::fromStdString(credentials_file));
    }
    return env;
}

std::string GwsClient::run(const std::vector<std::string>& args) const
{
    QProcess proc;
    proc.setProcessEnvironment(build_env(credentials_file_));
    proc.start(QString::fromStdString(gws_binary_), to_qstringlist(args));

    if (!proc.waitForStarted(5000)) {
        throw std::runtime_error("Failed to start gws process: " +
                                 proc.errorString().toStdString());
    }
    proc.waitForFinished(-1); // no timeout for large Drive operations

    const int exit_code = proc.exitCode();
    const std::string stdout_data = proc.readAllStandardOutput().toStdString();

    if (exit_code != 0) {
        const std::string stderr_data = proc.readAllStandardError().toStdString();
        if (auto logger = Logger::get_logger("core_logger")) {
            logger->error("gws exited with code {}: {}", exit_code, stderr_data);
        }
        throw std::runtime_error("gws command failed (exit " +
                                 std::to_string(exit_code) + "): " + stderr_data);
    }

    return stdout_data;
}

void GwsClient::run_ndjson(const std::vector<std::string>& args,
                            const std::function<bool(const Json::Value&)>& on_object) const
{
    QProcess proc;
    proc.setProcessEnvironment(build_env(credentials_file_));
    proc.setReadChannel(QProcess::StandardOutput);
    proc.start(QString::fromStdString(gws_binary_), to_qstringlist(args));

    if (!proc.waitForStarted(5000)) {
        throw std::runtime_error("Failed to start gws process: " +
                                 proc.errorString().toStdString());
    }

    bool keep_going = true;
    while (keep_going && (proc.state() != QProcess::NotRunning || proc.canReadLine())) {
        if (!proc.canReadLine()) {
            proc.waitForReadyRead(200);
            continue;
        }
        const QByteArray line_bytes = proc.readLine();
        const std::string line = line_bytes.trimmed().toStdString();
        if (line.empty()) {
            continue;
        }
        const Json::Value obj = parse_json(line);
        if (!obj.isNull()) {
            keep_going = on_object(obj);
        }
    }

    if (!keep_going) {
        proc.kill();
        proc.waitForFinished(3000);
    } else {
        proc.waitForFinished(-1);
    }
}

Json::Value GwsClient::parse_json(const std::string& json_str)
{
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    std::istringstream ss(json_str);
    if (!Json::parseFromStream(builder, ss, &root, &errs)) {
        if (auto logger = Logger::get_logger("core_logger")) {
            logger->warn("GwsClient: JSON parse error: {}", errs);
        }
        return Json::nullValue;
    }
    return root;
}
