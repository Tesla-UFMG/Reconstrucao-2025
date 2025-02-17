#include "Log.hpp"

Log& Log::getInstance(const std::string& filepath) {
    static Log instance(filepath);
    return instance;
}

Log::Log(const std::string& filepath) {
    file.open(filepath, std::ios::app);
    if (!file) {
        throw std::runtime_error("Erro ao abrir arquivo de log: " + filepath);
    }
}

Log::~Log() {
    if (file.is_open()) {
        file.close();
    }
}

std::string Log::getCurrentTime() {
    time_t t  = std::time(nullptr);
    tm     tm = *std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%S:%M:%H %d/%m/%Y");
    std::string str = oss.str();

    return str;
}

void Log::message(const std::string& level, const std::string& message) {
    file << "[" << getCurrentTime() << "]" << " " << level << " - " << message << std::endl;
}