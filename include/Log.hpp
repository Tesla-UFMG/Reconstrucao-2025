#ifndef LOG_HPP
#define LOG_HPP

// C++
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

#define LOG_OUTPUT "log.txt"

class Log {
    public:
        // Desabilita cópia e atribuição.
        Log(const Log&)                   = delete;
        Log&        operator=(const Log&) = delete;
        static Log& getInstance(const std::string& filepath = LOG_OUTPUT);
        void        message(const std::string& level, const std::string& message);

    private:
        explicit Log(const std::string& filepath);
        ~Log();
        std::string getCurrentTime();

        std::ofstream file;
};

#endif // LOG_HPP