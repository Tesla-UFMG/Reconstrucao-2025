#include "ProjectData.hpp"

void ProjectData::clear() {
    this->currentProjectName.clear();
    this->csvFiles.clear();
    this->videoFiles.clear();
    this->telemetryFiles.clear();
}

void ProjectData::loadCSV(const std::filesystem::path& filepath) {
    for (CSVFile& csvFile : this->csvFiles) {
        if (csvFile.getPath() == filepath) {
            LOG("WARN", "Arquivo já carregado: " + filepath.string());
            return;
        }
    }

    try {
        std::ifstream file(filepath);
        auto          doc = std::make_unique<rapidcsv::Document>(file, rapidcsv::LabelParams(0, 0));
        csvFiles.emplace_back(CSVFile(filepath, std::move(doc)));
        file.close();
        LOG("INFO", "CSV carregado com sucesso " + filepath.string());
    } catch (const std::exception& e) {
        LOG("ERROR", std::string("Erro ao abrir o CSV: ") + e.what());
    }
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

const std::vector<CSVFile>& ProjectData::getCSVFiles() { return this->csvFiles; }

const std::vector<TelemetryFile>& ProjectData::getTelemetryFiles() { return this->telemetryFiles; }

bool ProjectData::getTelemetryStatus() { return this->telemetryStatus; }

void ProjectData::setTelemetryStatus(bool status) { this->telemetryStatus = status; }

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
    return true;
}
