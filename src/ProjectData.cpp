#include "ProjectData.hpp"

ProjectData::ProjectData() {
    this->textFiles.emplace_back("Comentários", std::vector<std::string>{"Comentários"});
    this->textFiles.emplace_back("Avisos", std::vector<std::string>{"Igual a", "Fora da Faixa", "Dentro da Faixa", "Maior que", "Menor que"});
}

void ProjectData::clear() {
    this->currentProjectName.clear();
    this->csvFiles.clear();
    this->videoFiles.clear();
    this->telemetryFiles.clear();
    this->textFiles.clear();
    this->warningRules.clear();
    this->textFiles.emplace_back("Comentários", std::vector<std::string>{"Comentários"});
    this->textFiles.emplace_back("Avisos", std::vector<std::string>{"Igual a", "Fora da Faixa", "Dentro da Faixa", "Maior que", "Menor que"});
}

void ProjectData::loadCSV(const std::filesystem::path& filepath) {
    for (CSVFile& csvFile : this->csvFiles) {
        if (csvFile.getPath() == filepath) {
            LOG("WARN", "Arquivo já carregado: " + filepath.string());
            return;
        }
    }

    std::ifstream file(filepath);
    if (file) {
        auto doc = std::make_unique<rapidcsv::Document>(file, rapidcsv::LabelParams(0, -1));
        csvFiles.emplace_back(CSVFile(filepath, std::move(doc)));
        file.close();
        LOG("INFO", "CSV carregado com sucesso " + filepath.string());
    }

    LOG("ERROR", "Erro ao abrir o CSV: " + filepath.string());
}

void ProjectData::removeCSV(const std::filesystem::path& filepath) {
    for (size_t i = 0; i < csvFiles.size(); ++i) {
        if (csvFiles[i].getPath() == filepath) {
            csvFiles.erase(csvFiles.begin() + i);
            LOG("INFO", "Arquivo removido: " + filepath.string());
            return;
        }
    }
    LOG("ERROR", "Arquivo não encontrado para remoção: " + filepath.string());
}

bool ProjectData::loadPacket(const std::string& packetName, const std::string& packetId,
                             const std::vector<std::string>& columnNames) {
    for (const TelemetryFile& telemetryFile : telemetryFiles) {
        if (telemetryFile.getPacketId() == packetId) {
            LOG("WARN", "Pacote já carregado: " + telemetryFile.getName());
            return false;
        }
    }

    telemetryFiles.emplace_back(packetName, packetId, columnNames);
    LOG("INFO", "Pacote carregado: " + packetName);
    return true;
}

void ProjectData::removePacket(const std::string& packetId) {
    for (size_t i = 0; i < telemetryFiles.size(); ++i) {
        if (telemetryFiles[i].getPacketId() == packetId) {
            telemetryFiles.erase(telemetryFiles.begin() + i);
            LOG("INFO", "Pacote removido: " + packetId);
            return;
        }
    }
    LOG("ERROR", "Pacote não encontrado para remoção: " + packetId);
}

bool ProjectData::updatePacket(const std::string& oldPacketId, const std::string& newPacketId,
                               const std::string& newName, const std::vector<std::string>& newCols) {
    if (oldPacketId != newPacketId) {
        for (const auto& f : this->telemetryFiles) {
            if (f.getPacketId() == newPacketId) {
                LOG("WARN", "Pacote já existe com o novo ID: " + newPacketId);
                return false;
            }
        }
    }

    for (auto& f : this->telemetryFiles) {
        if (f.getPacketId() == oldPacketId) {
            f.setPacketId(newPacketId);
            f.setName(newName);
            f.setColumnNames(newCols);
            LOG("INFO", "Pacote atualizado: " + newName + " (" + newPacketId + ")");
            return true;
        }
    }
    LOG("ERROR", "Pacote não encontrado para atualização: " + oldPacketId);
    return false;
}

void ProjectData::swapPackets(size_t index1, size_t index2) {
    if (index1 < this->telemetryFiles.size() && index2 < this->telemetryFiles.size()) {
        std::swap(this->telemetryFiles[index1], this->telemetryFiles[index2]);
        LOG("INFO", "Ordem dos pacotes alterada: trocado índice " + std::to_string(index1) + " com " + std::to_string(index2));
    }
}

void ProjectData::clearAllTelemetryData() {
    for (auto& f : this->telemetryFiles) {
        f.clearData();
    }
    for (auto& f : this->textFiles) {
        f.clear();
    }
}

const std::vector<TextFile>& ProjectData::getTextFiles() { return this->textFiles; }

void ProjectData::addTextFile(const std::filesystem::path& filepath, const std::vector<std::string>& columnNames, const std::vector<std::string>& dates, const std::vector<std::vector<std::string>>& data) {
    std::string name = filepath.filename().string();
    for (auto& tf : this->textFiles) {
        if (tf.getName() == name) {
            tf = TextFile(filepath, columnNames, dates, data);
            return;
        }
    }
    this->textFiles.emplace_back(filepath, columnNames, dates, data);
}

void ProjectData::removeTextFile(const std::filesystem::path& /*filepath*/) {
    for (auto& f : this->textFiles) {
        f.clear();
    }
}

const std::vector<CSVFile>& ProjectData::getCSVFiles() { return this->csvFiles; }

const std::vector<TelemetryFile>& ProjectData::getTelemetryFiles() { return this->telemetryFiles; }

int ProjectData::getTelemetryStatus() { return this->telemetryStatus; }

void ProjectData::setTelemetryStatus(int status) { this->telemetryStatus = status; }

bool ProjectData::getProcessingStatus() { return this->processingStatus; }

void ProjectData::setProcessingStatus(bool status) { this->processingStatus = status; }

bool ProjectData::serialize(const std::filesystem::path& filepath) const {
    std::ofstream file(filepath, std::ios::binary);
    if (!file) {
        LOG("ERROR", "Não foi possível abrir o arquivo para salvar: " + filepath.string());
        return false;
    }

    // Serializa o nome do projeto
    size_t currentProjectNameStrSize = this->currentProjectName.size();
    file.write(reinterpret_cast<const char*>(&currentProjectNameStrSize), sizeof(currentProjectNameStrSize));
    file.write(this->currentProjectName.data(), currentProjectNameStrSize);

    // Serializa os caminhos dos CSVs
    size_t numCsvFiles = this->csvFiles.size();
    file.write(reinterpret_cast<const char*>(&numCsvFiles), sizeof(numCsvFiles));
    for (const auto& csvFile : this->csvFiles) {
        const std::string& pathStr     = csvFile.getPath().string();
        size_t             pathStrSize = pathStr.size();
        file.write(reinterpret_cast<const char*>(&pathStrSize), sizeof(pathStrSize));
        file.write(pathStr.data(), pathStrSize);
    }

    // Serializa os pacotes de telemetria
    size_t numTelemetryFiles = this->telemetryFiles.size();
    file.write(reinterpret_cast<const char*>(&numTelemetryFiles), sizeof(numTelemetryFiles));
    for (const auto& telemetryFile : this->telemetryFiles) {
        // Nome do pacote
        const std::string& packetName     = telemetryFile.getName();
        size_t             packetNameSize = packetName.size();
        file.write(reinterpret_cast<const char*>(&packetNameSize), sizeof(packetNameSize));
        file.write(packetName.data(), packetNameSize);

        // Id do pacote
        const std::string& packetId     = telemetryFile.getPacketId();
        size_t             packetIdSize = packetId.size();
        file.write(reinterpret_cast<const char*>(&packetIdSize), sizeof(packetIdSize));
        file.write(packetId.data(), packetIdSize);

        // Nomes das colunas
        const std::vector<std::string>& columnNames     = telemetryFile.getColumnNames();
        size_t                          columnNamesSize = columnNames.size();
        file.write(reinterpret_cast<const char*>(&columnNamesSize), sizeof(columnNamesSize));
        for (const auto& columnName : columnNames) {
            size_t columnNameSize = columnName.size();
            file.write(reinterpret_cast<const char*>(&columnNameSize), sizeof(columnNameSize));
            file.write(columnName.data(), columnNameSize);
        }
    }



    // Serializa os arquivos de texto
    size_t numTextFiles = this->textFiles.size();
    file.write(reinterpret_cast<const char*>(&numTextFiles), sizeof(numTextFiles));
    for (const auto& textFile : this->textFiles) {
        const std::string& pathStr = textFile.getPath().string();
        size_t             pathStrSize = pathStr.size();
        file.write(reinterpret_cast<const char*>(&pathStrSize), sizeof(pathStrSize));
        file.write(pathStr.data(), pathStrSize);

        // Nomes das colunas
        const auto& colNames = textFile.getColumnNames();
        size_t      colNamesSize = colNames.size();
        file.write(reinterpret_cast<const char*>(&colNamesSize), sizeof(colNamesSize));
        for (const auto& colName : colNames) {
            size_t len = colName.size();
            file.write(reinterpret_cast<const char*>(&len), sizeof(len));
            file.write(colName.data(), len);
        }

        // Dates
        const auto& dates = textFile.getDates();
        size_t      datesSize = dates.size();
        file.write(reinterpret_cast<const char*>(&datesSize), sizeof(datesSize));
        for (const auto& dateStr : dates) {
            size_t dateStrSize = dateStr.size();
            file.write(reinterpret_cast<const char*>(&dateStrSize), sizeof(dateStrSize));
            file.write(dateStr.data(), dateStrSize);
        }

        // Data (matriz de strings)
        const auto& data = textFile.getData();
        size_t      dataSize = data.size();
        file.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
        for (const auto& colData : data) {
            size_t colDataSize = colData.size();
            file.write(reinterpret_cast<const char*>(&colDataSize), sizeof(colDataSize));
            for (const auto& valStr : colData) {
                size_t len = valStr.size();
                file.write(reinterpret_cast<const char*>(&len), sizeof(len));
                file.write(valStr.data(), len);
            }
        }
    }

    // Serializa as regras de aviso (WarningRule)
    size_t numWarningRules = this->warningRules.size();
    file.write(reinterpret_cast<const char*>(&numWarningRules), sizeof(numWarningRules));
    for (const auto& rule : this->warningRules) {
        size_t lenFileType = rule.fileType.size();
        file.write(reinterpret_cast<const char*>(&lenFileType), sizeof(lenFileType));
        file.write(rule.fileType.data(), lenFileType);

        size_t lenFileName = rule.fileName.size();
        file.write(reinterpret_cast<const char*>(&lenFileName), sizeof(lenFileName));
        file.write(rule.fileName.data(), lenFileName);

        size_t lenColumnName = rule.columnName.size();
        file.write(reinterpret_cast<const char*>(&lenColumnName), sizeof(lenColumnName));
        file.write(rule.columnName.data(), lenColumnName);

        file.write(reinterpret_cast<const char*>(&rule.conditionType), sizeof(rule.conditionType));
        file.write(reinterpret_cast<const char*>(&rule.targetValue), sizeof(rule.targetValue));
        file.write(reinterpret_cast<const char*>(&rule.minVal), sizeof(rule.minVal));
        file.write(reinterpret_cast<const char*>(&rule.maxVal), sizeof(rule.maxVal));
        file.write(reinterpret_cast<const char*>(rule.alertColor), sizeof(rule.alertColor));

        size_t lenDesc = rule.description.size();
        file.write(reinterpret_cast<const char*>(&lenDesc), sizeof(lenDesc));
        file.write(rule.description.data(), lenDesc);

        file.write(reinterpret_cast<const char*>(&rule.lastProcessedIndex), sizeof(rule.lastProcessedIndex));
        file.write(reinterpret_cast<const char*>(&rule.wasTriggered), sizeof(rule.wasTriggered));
    }

    return true;
}

bool ProjectData::deserialize(const std::filesystem::path& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        LOG("ERROR", "Não foi possível abrir o arquivo para carregar: " + filepath.string());
        return false;
    }

    clear();

    // Desserializa o nome do projeto
    size_t currentProjectNameStrSize = 0;
    file.read(reinterpret_cast<char*>(&currentProjectNameStrSize), sizeof(currentProjectNameStrSize));
    currentProjectName.resize(currentProjectNameStrSize);
    file.read(&currentProjectName[0], currentProjectNameStrSize);

    // Desserializa os caminhos dos CSVs e recarrega os dados
    size_t numCsvFiles = 0;
    file.read(reinterpret_cast<char*>(&numCsvFiles), sizeof(numCsvFiles));
    for (size_t i = 0; i < numCsvFiles; ++i) {
        size_t pathStrSize = 0;
        file.read(reinterpret_cast<char*>(&pathStrSize), sizeof(pathStrSize));
        std::string pathStr(pathStrSize, '\0');
        file.read(&pathStr[0], pathStrSize);
        this->loadCSV(pathStr);
    }

    // Desserializa os pacotes de telemetria
    size_t numTelemetryFiles = 0;
    file.read(reinterpret_cast<char*>(&numTelemetryFiles), sizeof(numTelemetryFiles));
    for (size_t i = 0; i < numTelemetryFiles; ++i) {
        // Nome do pacote
        size_t packetNameSize = 0;
        file.read(reinterpret_cast<char*>(&packetNameSize), sizeof(packetNameSize));
        std::string packetName(packetNameSize, '\0');
        file.read(&packetName[0], packetNameSize);

        // Id do pacote
        size_t packetIdSize = 0;
        file.read(reinterpret_cast<char*>(&packetIdSize), sizeof(packetIdSize));
        std::string packetId(packetIdSize, '\0');
        file.read(&packetId[0], packetIdSize);

        // Nome das colunas
        size_t columnNamesSize = 0;
        file.read(reinterpret_cast<char*>(&columnNamesSize), sizeof(columnNamesSize));
        std::vector<std::string> columnNames(columnNamesSize);
        for (size_t j = 0; j < columnNamesSize; ++j) {
            size_t columnNameSize = 0;
            file.read(reinterpret_cast<char*>(&columnNameSize), sizeof(columnNameSize));
            columnNames[j].resize(columnNameSize);
            file.read(&columnNames[j][0], columnNameSize);
        }

        this->loadPacket(packetName, packetId, columnNames);
    }


    // Desserializa os arquivos de texto de forma genérica
    size_t numTextFiles = 0;
    if (file.read(reinterpret_cast<char*>(&numTextFiles), sizeof(numTextFiles))) {
        for (size_t i = 0; i < numTextFiles; ++i) {
            size_t pathStrSize = 0;
            if (!file.read(reinterpret_cast<char*>(&pathStrSize), sizeof(pathStrSize))) break;
            std::string pathStr(pathStrSize, '\0');
            if (!file.read(&pathStr[0], pathStrSize)) break;

            // Nomes das colunas
            size_t colNamesSize = 0;
            if (!file.read(reinterpret_cast<char*>(&colNamesSize), sizeof(colNamesSize))) break;
            std::vector<std::string> colNames(colNamesSize);
            for (size_t j = 0; j < colNamesSize; ++j) {
                size_t len = 0;
                if (!file.read(reinterpret_cast<char*>(&len), sizeof(len))) break;
                colNames[j].resize(len);
                if (!file.read(&colNames[j][0], len)) break;
            }

            // Dates
            size_t datesSize = 0;
            if (!file.read(reinterpret_cast<char*>(&datesSize), sizeof(datesSize))) break;
            std::vector<std::string> dates(datesSize);
            for (size_t j = 0; j < datesSize; ++j) {
                size_t len = 0;
                if (!file.read(reinterpret_cast<char*>(&len), sizeof(len))) break;
                dates[j].resize(len);
                if (!file.read(&dates[j][0], len)) break;
            }

            // Data (matriz de strings)
            size_t dataSize = 0;
            if (!file.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize))) break;
            std::vector<std::vector<std::string>> data(dataSize);
            for (size_t col = 0; col < dataSize; ++col) {
                size_t colDataSize = 0;
                if (!file.read(reinterpret_cast<char*>(&colDataSize), sizeof(colDataSize))) break;
                data[col].resize(colDataSize);
                for (size_t row = 0; row < colDataSize; ++row) {
                    size_t len = 0;
                    if (!file.read(reinterpret_cast<char*>(&len), sizeof(len))) break;
                    data[col][row].resize(len);
                    if (!file.read(&data[col][row][0], len)) break;
                }
            }

            this->addTextFile(pathStr, colNames, dates, data);
        }
    }

    // Desserializa as regras de aviso (WarningRule)
    size_t numWarningRules = 0;
    if (file.read(reinterpret_cast<char*>(&numWarningRules), sizeof(numWarningRules))) {
        this->warningRules.clear();
        for (size_t i = 0; i < numWarningRules; ++i) {
            WarningRule rule;

            size_t lenFileType = 0;
            if (!file.read(reinterpret_cast<char*>(&lenFileType), sizeof(lenFileType))) break;
            rule.fileType.resize(lenFileType);
            if (!file.read(&rule.fileType[0], lenFileType)) break;

            size_t lenFileName = 0;
            if (!file.read(reinterpret_cast<char*>(&lenFileName), sizeof(lenFileName))) break;
            rule.fileName.resize(lenFileName);
            if (!file.read(&rule.fileName[0], lenFileName)) break;

            size_t lenColumnName = 0;
            if (!file.read(reinterpret_cast<char*>(&lenColumnName), sizeof(lenColumnName))) break;
            rule.columnName.resize(lenColumnName);
            if (!file.read(&rule.columnName[0], lenColumnName)) break;

            if (!file.read(reinterpret_cast<char*>(&rule.conditionType), sizeof(rule.conditionType))) break;
            if (!file.read(reinterpret_cast<char*>(&rule.targetValue), sizeof(rule.targetValue))) break;
            if (!file.read(reinterpret_cast<char*>(&rule.minVal), sizeof(rule.minVal))) break;
            if (!file.read(reinterpret_cast<char*>(&rule.maxVal), sizeof(rule.maxVal))) break;
            if (!file.read(reinterpret_cast<char*>(rule.alertColor), sizeof(rule.alertColor))) break;

            size_t lenDesc = 0;
            if (!file.read(reinterpret_cast<char*>(&lenDesc), sizeof(lenDesc))) break;
            rule.description.resize(lenDesc);
            if (!file.read(&rule.description[0], lenDesc)) break;

            if (!file.read(reinterpret_cast<char*>(&rule.lastProcessedIndex), sizeof(rule.lastProcessedIndex))) break;
            if (!file.read(reinterpret_cast<char*>(&rule.wasTriggered), sizeof(rule.wasTriggered))) break;

            this->warningRules.push_back(rule);
        }
    }

    return true;
}
