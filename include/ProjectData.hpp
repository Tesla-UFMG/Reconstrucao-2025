#ifndef PROJECTDATA_HPP
#define PROJECTDATA_HPP

/*
Para que serve esse arquivo.

Para salvar informações globais do projeto. Por exemplo, csv's
*/

// C++
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

// Project
#include "DataFiles.hpp"
#include "Log.hpp"

// Third party
#include "rapidcsv.h"

class ProjectData {
    private:
        bool telemetryStatus  = false;
        bool processingStatus = false;

    public:
        ProjectData()                              = default;
        ProjectData(const ProjectData&)            = delete;
        ProjectData& operator=(const ProjectData&) = delete;
        ProjectData(ProjectData&&)                 = default;
        ProjectData& operator=(ProjectData&&)      = default;

        std::string                currentProjectName;
        std::vector<CSVFile>       csvFiles;
        std::vector<VideoFile>     videoFiles;
        std::vector<TelemetryFile> telemetryFiles;

        bool getTelemetryStatus();
        void setTelemetryStatus(bool status);

        bool getProcessingStatus();
        void setProcessingStatus(bool status);

        void loadCSV(const std::filesystem::path& filepath);   // Carrega os dados de um arquivo CSV
        void removeCSV(const std::filesystem::path& filepath); // Remove dados

        const std::vector<TelemetryFile>& getTelemetryPackets();
        bool                              loadPacket(const std::string& packetName, const std::string& packetId,
                                                     const std::vector<std::string>& columnNames);
        void                              removePacket(const std::string& packetId);

        void clear(); // Limpa os dados armazenados

        bool
        serialize(const std::filesystem::path& filepath) const; // Serializa os dados do projeto para um arquivo binário
        bool deserialize(
            const std::filesystem::path& filepath); // Desserializa os dados do projeto a partir de um arquivo binário
};

#endif
