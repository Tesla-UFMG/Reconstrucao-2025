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

const std::vector<double>& CSVFile::getColumnData(const std::string& columnName) const {
    static const std::vector<double> emptyVec{};
    auto                             it = columnCache.find(columnName);
    if (it != columnCache.end()) {
        return it->second;
    }

    try {
        std::vector<double> data   = this->doc->GetColumn<double>(columnName);
        auto                result = columnCache.emplace(columnName, std::move(data));
        return result.first->second;
    } catch (const std::exception& e) {
        return emptyVec;
    }
}

// TELEMETRY
TelemetryFile::TelemetryFile(const std::string& packetName, const std::string& packetId,
                             const std::vector<std::string>& columnNames)
    : GenericFile(std::filesystem::path()) {
    this->name        = packetName;
    this->packetId    = packetId;
    this->columnNames = columnNames;
    this->fileType    = "Telemetry";
    this->data.resize(columnNames.size());
}

bool TelemetryFile::insertData(const std::vector<double>& newData) {
    if (this->columnNames.empty()) {
        return false;
    }

    size_t limit = std::min(newData.size(), this->columnNames.size());
    for (size_t i = 0; i < this->columnNames.size(); ++i) {
        if (i < limit) {
            this->data[i].push_back(newData[i]);
        } else {
            this->data[i].push_back(0.0);
        }
    }

    return true;
}


bool TelemetryFile::insertDate(const std::string& newDate) {
    this->date.push_back(newDate);
    return true;
}

const std::string&                      TelemetryFile::getPacketId() const { return this->packetId; }
const std::vector<std::string>&         TelemetryFile::getColumnNames() const { return this->columnNames; }
const std::vector<std::vector<double>>& TelemetryFile::getData() const { return this->data; }
const std::vector<std::string>& TelemetryFile::getDate() const { return this->date; }

const std::vector<double>& TelemetryFile::getColumnData(const std::string& columnName) const {
    static const std::vector<double> emptyVec{};
    const std::vector<std::string>&  cols = this->getColumnNames();
    for (size_t i = 0; i < cols.size(); ++i) {
        if (cols[i] == columnName) {
            return this->data[i];
        }
    }
    return emptyVec;
}

void TelemetryFile::setName(const std::string& newName) {
    this->name = newName;
}

void TelemetryFile::setColumnNames(const std::vector<std::string>& newCols) {
    size_t oldSize = this->columnNames.size();
    this->columnNames = newCols;
    this->data.resize(newCols.size());
    for (size_t i = oldSize; i < newCols.size(); ++i) {
        this->data[i].resize(this->date.size(), 0.0);
    }
}