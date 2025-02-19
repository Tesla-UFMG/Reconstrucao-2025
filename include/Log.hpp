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
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#define LOG_OUTPUT "log.txt"

class Log {
    private:
        explicit Log(const std::string& filepath);

        std::string   getCurrentTime();
        std::ofstream file;

    public:
        Log(Log&&)            = delete;
        Log& operator=(Log&&) = delete;
        ~Log();

        static Log& getInstance(const std::string& filepath = LOG_OUTPUT);
        void        message(const std::string& level, const std::string& message);
};

#endif // LOG_HPP