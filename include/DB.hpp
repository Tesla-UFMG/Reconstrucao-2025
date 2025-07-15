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

class DB {
    private:
        explicit DB();
        DB(DB&&)            = delete;
        DB& operator=(DB&&) = delete;
        ~DB();

        char* saveFileDialog(const std::string& title, const std::string& defaultName, const char* filter);
        char* openFileDialog(const std::string& title, const char* filter);

        void saveProject(const std::filesystem::path& filepath);
        void loadProject(const std::filesystem::path& filepath);

        ProjectData projectData;

    public:
        static DB& getInstance();

        void createProjectDialog();
        void saveProjectDialog();
        void loadProjectDialog();

        void loadCSVDialog();
        void deleteCSV(const std::filesystem::path& filepath);

        const ProjectData& getProject() const;
        ProjectData&       getProject();

        const std::vector<double>& getCSVData(const std::string& filepath, const std::string& columnName) const;
        const std::vector<double>& getTelemetryData(const std::string& packetId, const std::string& columnName) const;

        bool processTelemetryPacket(const std::string& packetId, const std::vector<double>& data);
};

#endif
