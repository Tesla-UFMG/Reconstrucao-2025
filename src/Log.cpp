#include "Log.hpp"

Log& Log::getInstance(const std::filesystem::path& filepath) {
    static Log instance(filepath);
    return instance;
}

Log::Log(const std::filesystem::path& filepath) {
    std::filesystem::path parentPath = filepath.parent_path();
    if (!parentPath.empty()) {
        std::filesystem::create_directories(parentPath);
    }

    this->file.open(filepath, std::ios::app | std::ios::out);
    if (!this->file) {
        std::string error = "Erro ao abrir arquivo de log: " + filepath.string();
        std::cerr << error << std::endl;
        throw std::runtime_error(error);
    }

    this->log("TRACE", "Log iniciado com sucesso.");
}

Log::~Log() {
    if (this->file.is_open()) {
        this->log("TRACE", "Log encerrado.");
        this->file.close();
    }
}

std::string Log::getCurrentTime() {
    time_t t  = std::time(nullptr);
    tm     tm = *std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S %d/%m/%Y");
    return oss.str();
}

void Log::log(const std::string& level, const std::string& message) {
    if (!this->file.is_open())
        return;

    std::string formattedMessage = "[" + getCurrentTime() + "] " + level + " - " + message;
    this->file << formattedMessage << std::endl;
    this->file.flush();

    this->messages.push_back(formattedMessage);
}

std::vector<std::string> Log::getLog() { return this->messages; }

void Log::clearLog() { this->messages.clear(); }
