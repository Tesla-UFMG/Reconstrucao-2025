#ifndef DATAFILES_HPP
#define DATAFILES_HPP

// C++
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Third party
#include "rapidcsv.h"

#define FILE_TYPE_SIZE   16
#define FILE_NAME_SIZE   516
#define COLUMN_NAME_SIZE 32

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
        std::unique_ptr<rapidcsv::Document>                          doc;
        mutable std::unordered_map<std::string, std::vector<double>> columnCache;

    public:
        CSVFile(std::filesystem::path p, std::unique_ptr<rapidcsv::Document> d);
        rapidcsv::Document*        getDocument() const;
        std::vector<std::string>   getColumnNames() const;
        const std::vector<double>& getColumnData(const std::string& columnName) const;
};

class VideoFile : public GenericFile {};

class TelemetryFile : public GenericFile {
    private:
        std::string                      packetId;
        std::vector<std::string>         columnNames;
        std::vector<std::vector<double>> data;
        std::vector<std::string> date;

    public:
        TelemetryFile(const std::string& packetName, const std::string& packetId,
                      const std::vector<std::string>& columnNames);
        const std::string&                      getPacketId() const;
        const std::vector<std::string>&         getColumnNames() const;
        const std::vector<std::vector<double>>& getData() const;
        const std::vector<double>&              getColumnData(const std::string& columnName) const;
        const std::vector<std::string>&         getDate() const;
        bool                                    insertData(const std::vector<double>& newData);
        bool insertDate(const std::string& newDate);
        void                                    setName(const std::string& newName);
        void                                    setColumnNames(const std::vector<std::string>& newCols);
        void                                    clearData();
    };

class TextFile : public GenericFile {
    private:
        std::vector<std::string> dates;
        std::vector<std::vector<std::string>> data;
        std::vector<std::string> columnNames;

    public:
        TextFile(std::filesystem::path filepath);
        TextFile(std::filesystem::path filepath, const std::vector<std::string>& columnNames);
        TextFile(std::filesystem::path filepath, const std::vector<std::string>& columnNames, const std::vector<std::string>& dates, const std::vector<std::vector<std::string>>& data);
        const std::vector<std::string>& getDates() const;
        const std::vector<std::vector<std::string>>& getData() const;
        const std::vector<std::string>& getComments() const;
        const std::vector<std::string>& getColumnNames() const;
        void addRow(const std::string& date, const std::vector<std::string>& rowData);
        void addComment(const std::string& date, const std::string& comment);
        void clear();
};

struct ArchivePayload {
        char fileType[FILE_TYPE_SIZE];
        char fileName[FILE_NAME_SIZE];
};

struct ColumnPayload {
        char fileType[FILE_TYPE_SIZE];
        char fileName[FILE_NAME_SIZE];
        char columnName[COLUMN_NAME_SIZE];
};

#endif