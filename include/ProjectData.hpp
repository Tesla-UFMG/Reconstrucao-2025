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
#include "Log.hpp"

// Third party
#include "rapidcsv.h"

class GenericFile {
    protected:
        std::filesystem::path filepath;

    public:
        GenericFile(std::filesystem::path p) : filepath(std::move(p)) {}
        virtual ~GenericFile() = default;

        const std::filesystem::path& getPath() const { return filepath; }
};

class CSVFile : public GenericFile {
        std::unique_ptr<rapidcsv::Document> doc;

    public:
        CSVFile(std::filesystem::path p, std::unique_ptr<rapidcsv::Document> d)
            : GenericFile(std::move(p)), doc(std::move(d)) {}

        rapidcsv::Document* getDocument() const { return doc.get(); }
};

class VideoFile : public GenericFile {};

class ProjectData {
    public:
        ProjectData()                              = default;
        ProjectData(const ProjectData&)            = delete;
        ProjectData& operator=(const ProjectData&) = delete;
        ProjectData(ProjectData&&)                 = default;
        ProjectData& operator=(ProjectData&&)      = default;

        std::string            currentProjectName;
        std::vector<CSVFile>   csvFiles;
        std::vector<VideoFile> videoFiles;

        void loadCSV(const std::filesystem::path& filepath);   // Carrega os dados de um arquivo CSV
        void removeCSV(const std::filesystem::path& filepath); // Remove dados

        void clear(); // Limpa os dados armazenados

        bool
        serialize(const std::filesystem::path& filepath) const; // Serializa os dados do projeto para um arquivo binário
        bool deserialize(
            const std::filesystem::path& filepath); // Desserializa os dados do projeto a partir de um arquivo binário
};

#endif
