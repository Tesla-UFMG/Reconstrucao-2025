#ifndef DATAFILES_HPP
#define DATAFILES_HPP

// C++
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

// Third party
#include "rapidcsv.h"

class GenericFile {
    protected:
        std::string           name;
        std::filesystem::path filepath;
        std::string           fileType;

    public:
        GenericFile(std::filesystem::path filepath);
        virtual ~GenericFile() = default;
        const std::filesystem::path& getPath() const;
        const std::string&           getFileType() const;
        const std::string&           getName() const;
};

class CSVFile : public GenericFile {
    private:
        std::unique_ptr<rapidcsv::Document> doc;

    public:
        CSVFile(std::filesystem::path p, std::unique_ptr<rapidcsv::Document> d);
        rapidcsv::Document*      getDocument() const;
        std::vector<std::string> getColumnNames() const;
};

class VideoFile : public GenericFile {};

class TelemetryFile : public GenericFile {
    private:
        std::string                      packetId;
        std::vector<std::string>         columnNames;
        std::vector<std::vector<double>> data;

    public:
        TelemetryFile(const std::string& packetName, const std::string& packetId,
                      const std::vector<std::string>& columnNames);
        const std::string&                      getPacketId() const;
        const std::vector<std::string>&         getColumnNames() const;
        const std::vector<std::vector<double>>& getData() const;
        bool                                    insertData(const std::vector<double>& newData);
};

struct ArchivePayload {
        char fileType[16];
        char fileName[256];
};

struct ColumnPayload {
        char fileType[16];
        char fileName[256];
        char columnName[256];
};

#endif