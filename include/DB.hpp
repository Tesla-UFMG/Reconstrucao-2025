#ifndef DB_HPP
#define DB_HPP

// C++
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

// Project
#include "Dialogs.hpp"
#include "Log.hpp"
#include "ProjectData.hpp"
#include "SDLWrapper.hpp"

// Defines
#define TELEMETRY_OUTPUT "telemetry/"

class DB {
    private:
        explicit DB();
        DB(DB&&)            = delete;
        DB& operator=(DB&&) = delete;
        ~DB();

        ProjectData projectData;

        // Dialogo de abrir ou fechar um arquivo
        char* saveFileDialog(const std::string& title, const std::string& defaultName, const char* filter);
        char* openFileDialog(const std::string& title, const char* filter);

        // Abrir ou fechar um projeto
        void saveProject(const std::filesystem::path& filepath);
        void loadProject(const std::filesystem::path& filepath);

    public:
        static DB& getInstance();
        // Retorna projeto
        const ProjectData& getProject() const;
        ProjectData&       getProject();

        // Cria um projeto
        void createProjectDialog();

        // Dialogo de salvar ou carregar um projeto
        void saveProjectDialog();
        void loadProjectDialog();

        // Dialogo carregar um CSV
        void loadCSVDialog();

        void deleteCSV(const std::filesystem::path& filepath);

        const std::vector<double>& getCSVData(const std::string& filepath, const std::string& columnName) const;
        const std::vector<double>& getTelemetryData(const std::string& packetId, const std::string& columnName) const;

        bool processTelemetryPacket(const std::string& packetId, const std::vector<double>& data);

        bool saveTelemetryPackets(const std::string& outputFolder);
};

#endif
