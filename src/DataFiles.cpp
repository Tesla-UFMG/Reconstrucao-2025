#include "DataFiles.hpp"

GenericFile::GenericFile(std::filesystem::path filepath) : filepath(std::move(filepath)) {}
const std::filesystem::path& GenericFile::getPath() const { return this->filepath; }
const std::string&           GenericFile::getFileType() const { return this->fileType; }
const std::string&           GenericFile::getName() const { return this->name; }

// CSV
CSVFile::CSVFile(std::filesystem::path filepath, std::unique_ptr<rapidcsv::Document> doc)
    : GenericFile(std::move(filepath)) {
    this->doc      = std::move(doc);
    this->fileType = "CSV";
    this->name     = this->filepath.filename().string();
}
rapidcsv::Document*      CSVFile::getDocument() const { return this->doc.get(); }
std::vector<std::string> CSVFile::getColumnNames() const { return this->doc->GetColumnNames(); }

// TELEMETRY
TelemetryFile::TelemetryFile(const std::string& packetName, const std::string& packetId,
                             const std::vector<std::string>& columnNames)
    : GenericFile(std::filesystem::path()) {
    this->name        = packetName;
    this->packetId    = packetId;
    this->columnNames = columnNames;
    this->fileType    = "Telemetry";
    this->data.resize(8);
}

bool TelemetryFile::insertData(const std::vector<double>& newData) {
    if (newData.size() != this->columnNames.size()) {
        return false;
    }

    for (size_t i = 0; i < newData.size(); ++i) {
        this->data[i].push_back(newData[i]);
    }

    return true;
}

const std::string&                      TelemetryFile::getPacketId() const { return this->packetId; }
const std::vector<std::string>&         TelemetryFile::getColumnNames() const { return this->columnNames; }
const std::vector<std::vector<double>>& TelemetryFile::getData() const { return this->data; }
