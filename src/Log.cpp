#include "Log.hpp"

Log& Log::getInstance(const std::string& filepath) {
    static Log instance(filepath);
    return instance;
}

Log::Log(const std::string& filepath) {
    this->file.open(filepath, std::ios::trunc | std::ios::in | std::ios::out);
    if (!this->file) {
        std::string error = "Erro ao abrir arquivo de log: " + filepath;
        std::cerr << error << std::endl;
        throw std::runtime_error(error);
    }

    std::string line;
    this->file.clear();
    this->file.seekg(0, std::ios::beg);
    while (std::getline(this->file, line)) { 
        this->messages.push_back(line);
    }

    this->message("TRACE", "Log iniciado com sucesso.");
}

Log::~Log() {
    if (this->file.is_open()) {
        this->message("TRACE", "Log encerrado.");
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

void Log::message(const std::string& level, const std::string& message) {
    std::string formattedMessage ("[" + getCurrentTime() + "] " +  level + " - " + message);  
    this->file << formattedMessage << "\n";
    this->messages.push_back(formattedMessage);
}

std::vector<std::string> Log::getMessages() {
    return this->messages;
}
