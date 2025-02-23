#include "Log.hpp"

Log& Log::getInstance(const std::filesystem::path& filepath) {
    static Log instance(filepath);
    return instance;
}

Log::Log(const std::filesystem::path& filepath) {

    std::filesystem::path parentPath = filepath.parent_path();
    if (!parentPath.empty() && std::filesystem::create_directories(parentPath)) {
        std::filesystem::create_directories(parentPath);
    }

    this->file.open(filepath, std::ios::trunc | std::ios::in | std::ios::out);
    if (!this->file) {
        std::string error = "Erro ao abrir arquivo de log: " + filepath.string();
        std::cerr << error << std::endl;
        throw std::runtime_error(error);
    }

    std::string line;
    this->file.clear();
    this->file.seekg(0, std::ios::beg);
    while (std::getline(this->file, line)) {
        this->messages.push_back(line);
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
    std::string str = oss.str();

    return str;
}

void Log::log(const std::string& level, const std::string& message) {
    std::string formattedMessage("[" + getCurrentTime() + "] " + level + " - " + message);
    this->file << formattedMessage << "\n";
    this->messages.push_back(formattedMessage);
}

std::vector<std::string> Log::getLog() { return this->messages; }
