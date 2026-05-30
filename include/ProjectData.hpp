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
#include "ImGuiWrapper.hpp"
#include "Log.hpp"
#include "WarningRule.hpp"

// Third party
#include "rapidcsv.h"

class ProjectData {
    private:
        int telemetryStatus   = 0;
        bool processingStatus = false;

    public:
        ProjectData();
        ProjectData(const ProjectData&)            = delete;
        ProjectData& operator=(const ProjectData&) = delete;
        ProjectData(ProjectData&&)                 = default;
        ProjectData& operator=(ProjectData&&)      = default;

        std::string                currentProjectName;
        std::vector<CSVFile>       csvFiles;
        std::vector<VideoFile>     videoFiles;
        std::vector<TelemetryFile> telemetryFiles;
        std::vector<TextFile>      textFiles;
        std::vector<WarningRule>   warningRules;

        int getTelemetryStatus();
        void setTelemetryStatus(int status);

        bool getProcessingStatus();
        void setProcessingStatus(bool status);

        void loadCSV(const std::filesystem::path& filepath);   // Carrega os dados de um arquivo CSV
        void removeCSV(const std::filesystem::path& filepath); // Remove dados

        const std::vector<CSVFile>&       getCSVFiles();
        const std::vector<TelemetryFile>& getTelemetryFiles();
        const std::vector<TextFile>&      getTextFiles();

        bool loadPacket(const std::string& packetName, const std::string& packetId,
                        const std::vector<std::string>& columnNames);
        void removePacket(const std::string& packetId);
        bool updatePacket(const std::string& oldPacketId, const std::string& newPacketId,
                          const std::string& newName, const std::vector<std::string>& newCols);
        void swapPackets(size_t index1, size_t index2);
        void clearAllTelemetryData();
        void addTextFile(const std::filesystem::path& filepath, const std::vector<std::string>& columnNames, const std::vector<std::string>& dates, const std::vector<std::vector<std::string>>& data);
        void removeTextFile(const std::filesystem::path& filepath);

        void clear(); // Limpa os dados armazenados

        bool
        serialize(const std::filesystem::path& filepath) const; // Serializa os dados do projeto para um arquivo binário
        bool deserialize(
            const std::filesystem::path& filepath); // Desserializa os dados do projeto a partir de um arquivo binário
};

#endif
