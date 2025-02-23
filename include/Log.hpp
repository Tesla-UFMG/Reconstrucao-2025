#ifndef LOG_HPP
#define LOG_HPP

/*
FATAL → Erro crítico que faz o sistema parar.
ERROR → Erro que afeta o funcionamento, mas o sistema continua.
WARN → Aviso de possível problema futuro.
INFO → Informações normais do sistema.
DEBUG → Detalhes técnicos para desenvolvedores.
TRACE → Passo a passo da execução do código.
AUDIT → Registra ações importantes (ex: login).
SECURITY → Eventos de segurança (ex: tentativa de acesso não autorizado).
METRICS → Dados de desempenho (ex: tempo de resposta de uma função).
*/

// C++
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#define LOG_OUTPUT "log.txt"

#define LOG(level, message) Log::getInstance().log(level, message);

class Log {
    private:
        explicit Log(const std::filesystem::path& filepath);

        std::string              getCurrentTime();
        std::fstream             file;
        std::vector<std::string> messages;

    public:
        Log(Log&&)            = delete;
        Log& operator=(Log&&) = delete;
        ~Log();

        static Log&              getInstance(const std::filesystem::path& filepath = LOG_OUTPUT);
        void                     log(const std::string& level, const std::string& message);
        std::vector<std::string> getLog();
        void                     clearLog();
};

#endif // LOG_HPP