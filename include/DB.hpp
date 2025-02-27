#ifndef DB_HPP
#define DB_HPP

// C++
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

// Project
#include "Log.hpp"
#include "SDLWrapper.hpp"

// Thirdparty
#include "rapidcsv.h"
#include "tinyfiledialogs.h"

class DB {
    private:
        explicit DB();
        void loadProject(const std::filesystem::path& filepath);
        void saveProject(const std::filesystem::path& filepath);
        void loadData(const std::filesystem::path& filepath);
        void saveFileDialog(const std::string& title, const std::string& defaultName, const char* filter,
                            void (DB::*action)(const std::filesystem::path&));
        void openFileDialog(const std::string& title, const char* filter,
                            void (DB::*action)(const std::filesystem::path&));

        std::string                           currentProject;
        std::vector<std::filesystem::path>    csvPaths;
        std::vector<rapidcsv::Document>       csvData;
        std::vector<std::vector<std::string>> csvColumns;

    public:
        DB(DB&&)            = delete;
        DB& operator=(DB&&) = delete;
        ~DB();
        static DB& getInstance();

        void                                  createProjectDialog();
        void                                  loadProjectDialog();
        void                                  saveProjectDialog();
        void                                  loadDataDialog();
        std::string                           getCurrentProject() const;
        std::vector<rapidcsv::Document>       getCsvData() const;
        std::vector<std::filesystem::path>    getCsvPaths() const;
        std::vector<std::vector<std::string>> getCsvColumns() const;
};

#endif