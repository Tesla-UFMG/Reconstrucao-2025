#ifndef PROJECTDATA_HPP
#define PROJECTDATA_HPP

// C++
#include <filesystem>
#include <string>
#include <vector>

// Project
#include "Log.hpp"

// Third party
#include "rapidcsv.h"

class ProjectData {
    public:
        std::string                           currentProject;
        std::vector<std::filesystem::path>    csvPaths;
        std::vector<rapidcsv::Document>       csvData;
        std::vector<std::vector<std::string>> csvColumns;

        void loadCSV(const std::filesystem::path& filepath);   // Carrega os dados de um arquivo CSV
        void removeCSV(const std::filesystem::path& filepath); // Remove dados
        void clear();                                          // Limpa os dados armazenados
        bool
        serialize(const std::filesystem::path& filepath) const; // Serializa os dados do projeto para um arquivo binário
        bool deserialize(
            const std::filesystem::path& filepath); // Desserializa os dados do projeto a partir de um arquivo binário
};

#endif
