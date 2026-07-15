#include "DataFiles.hpp"
#include <cstring>

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
    if (this->doc) {
        this->cachedColumnNames = this->doc->GetColumnNames();
    }
}
rapidcsv::Document*             CSVFile::getDocument() const { return this->doc.get(); }
const std::vector<std::string>& CSVFile::getColumnNames() const { return this->cachedColumnNames; }

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

// VIDEO
VideoFile::VideoFile(std::filesystem::path filepath) : GenericFile(std::move(filepath)) {
    this->fileType = "Video";
    this->name     = this->filepath.filename().string();
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
    try {
        this->numericDate.push_back(std::stod(newDate));
    } catch (...) {
        this->numericDate.push_back(0.0);
    }
    return true;
}

void TelemetryFile::reserveData(size_t capacity) {
    for (auto& col : this->data) {
        col.reserve(capacity);
    }
    this->date.reserve(capacity);
    this->numericDate.reserve(capacity);
}

void TelemetryFile::insertDataSlice(const std::vector<const std::vector<double>*>& sourceColumns, const std::vector<double>& sourceDates, int endIdx) {
    if (endIdx < 0) {
        this->clearData();
        return;
    }
    
    size_t newSize = static_cast<size_t>(endIdx + 1);

    // Resize all vectors to newSize and copy only the new data slice
    for (size_t i = 0; i < this->data.size(); ++i) {
        size_t oldSize = this->data[i].size();
        if (newSize <= oldSize) continue;
        
        this->data[i].resize(newSize);
        const std::vector<double>* src = (i < sourceColumns.size()) ? sourceColumns[i] : nullptr;
        
        if (src && src->size() >= newSize) {
            memcpy(this->data[i].data() + oldSize, src->data() + oldSize, (newSize - oldSize) * sizeof(double));
        } else if (src && src->size() > oldSize) {
            size_t copySize = src->size() - oldSize;
            memcpy(this->data[i].data() + oldSize, src->data() + oldSize, copySize * sizeof(double));
            std::fill(this->data[i].begin() + src->size(), this->data[i].end(), 0.0);
        } else {
            std::fill(this->data[i].begin() + oldSize, this->data[i].end(), 0.0);
        }
    }
    
    // Handle dates
    size_t oldDateSize = this->date.size();
    if (newSize <= oldDateSize) return;

    this->date.resize(newSize);
    this->numericDate.resize(newSize);
    
    // Only convert NEW dates to strings to save massive CPU time
    if (sourceDates.size() >= newSize) {
        memcpy(this->numericDate.data() + oldDateSize, sourceDates.data() + oldDateSize, (newSize - oldDateSize) * sizeof(double));
        for (size_t i = oldDateSize; i < newSize; ++i) {
            this->date[i] = std::to_string(sourceDates[i]);
        }
    } else {
        size_t copySize = (sourceDates.size() > oldDateSize) ? (sourceDates.size() - oldDateSize) : 0;
        if (copySize > 0) {
            memcpy(this->numericDate.data() + oldDateSize, sourceDates.data() + oldDateSize, copySize * sizeof(double));
            for (size_t i = oldDateSize; i < oldDateSize + copySize; ++i) {
                this->date[i] = std::to_string(sourceDates[i]);
            }
        }
        // Set the missing dates
        if (newSize > oldDateSize + copySize) {
            for (size_t i = oldDateSize + copySize; i < newSize; ++i) {
                this->date[i] = std::to_string(i);
            }
        }
    }
}

void TelemetryFile::shrinkTo(size_t newSize) {
    for (auto& col : this->data) {
        if (col.size() > newSize) col.resize(newSize);
    }
    if (this->date.size() > newSize) this->date.resize(newSize);
    if (this->numericDate.size() > newSize) this->numericDate.resize(newSize);
}

void TelemetryFile::setInterpolatedRow(const std::vector<const std::vector<double>*>& sourceColumns, double ratio, int i0, int i1) {
    if (i0 < 0 || i1 < 0) return;
    
    size_t targetIdx = static_cast<size_t>(i0 + 1);
    
    for (size_t i = 0; i < this->data.size(); ++i) {
        if (this->data[i].size() <= targetIdx) {
            this->data[i].resize(targetIdx + 1, 0.0);
        }
        
        const std::vector<double>* src = (i < sourceColumns.size()) ? sourceColumns[i] : nullptr;
        if (src && src->size() > static_cast<size_t>(i1)) {
            double v0 = (*src)[i0];
            double v1 = (*src)[i1];
            this->data[i][targetIdx] = v0 + ratio * (v1 - v0);
        }
    }
    
    if (this->numericDate.size() <= targetIdx) {
        this->numericDate.resize(targetIdx + 1, 0.0);
        this->date.resize(targetIdx + 1, "");
    }
}

void TelemetryFile::setName(const std::string& newName) {
    this->name = newName;
}

const std::string&                      TelemetryFile::getPacketId() const { return this->packetId; }
const std::vector<std::string>&         TelemetryFile::getColumnNames() const { return this->columnNames; }
const std::vector<std::vector<double>>& TelemetryFile::getData() const { return this->data; }
const std::vector<std::string>& TelemetryFile::getDate() const { return this->date; }
const std::vector<double>& TelemetryFile::getNumericDate() const { return this->numericDate; }

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



void TelemetryFile::setPacketId(const std::string& newPacketId) {
    this->packetId = newPacketId;
}


void TelemetryFile::setColumnNames(const std::vector<std::string>& newCols) {
    size_t oldSize = this->columnNames.size();
    this->columnNames = newCols;
    this->data.resize(newCols.size());
    for (size_t i = oldSize; i < newCols.size(); ++i) {
        this->data[i].resize(this->date.size(), 0.0);
    }
}

void TelemetryFile::clearData() {
    this->data.clear();
    this->data.resize(this->columnNames.size());
    this->date.clear();
    this->numericDate.clear();
}

// TEXT FILE
TextFile::TextFile(std::filesystem::path filepath)
    : GenericFile(std::move(filepath)) {
    this->fileType = "Text";
    this->name     = this->filepath.filename().string();
    this->columnNames = { "Comentários" };
    this->data.resize(1);
}

TextFile::TextFile(std::filesystem::path filepath, const std::vector<std::string>& columnNames)
    : GenericFile(std::move(filepath)), columnNames(columnNames) {
    this->fileType = "Text";
    this->name     = this->filepath.filename().string();
    this->data.resize(columnNames.size());
}

TextFile::TextFile(std::filesystem::path filepath, const std::vector<std::string>& columnNames, const std::vector<std::string>& dates, const std::vector<std::vector<std::string>>& data)
    : GenericFile(std::move(filepath)), dates(dates), data(data), columnNames(columnNames) {
    this->fileType = "Text";
    this->name     = this->filepath.filename().string();
    this->numericDates.reserve(dates.size());
    for (const auto& d : dates) {
        try {
            this->numericDates.push_back(std::stod(d));
        } catch (...) {
            this->numericDates.push_back(0.0);
        }
    }
}

const std::vector<std::string>& TextFile::getDates() const { return this->dates; }
const std::vector<double>& TextFile::getNumericDates() const { return this->numericDates; }
const std::vector<std::vector<std::string>>& TextFile::getData() const { return this->data; }

const std::vector<std::string>& TextFile::getComments() const {
    static const std::vector<std::string> emptyVec{};
    if (this->data.empty()) return emptyVec;
    return this->data[0];
}

const std::vector<std::string>& TextFile::getColumnNames() const { return this->columnNames; }

void TextFile::addRow(const std::string& date, const std::vector<std::string>& rowData) {
    this->dates.push_back(date);
    try {
        this->numericDates.push_back(std::stod(date));
    } catch (...) {
        this->numericDates.push_back(0.0);
    }
    for (size_t i = 0; i < this->columnNames.size(); ++i) {
        if (i < rowData.size()) {
            this->data[i].push_back(rowData[i]);
        } else {
            this->data[i].push_back("");
        }
    }
}

void TextFile::addComment(const std::string& date, const std::string& comment) {
    this->addRow(date, { comment });
}

void TextFile::clear() {
    this->dates.clear();
    this->numericDates.clear();
    for (auto& col : this->data) {
        col.clear();
    }
}
