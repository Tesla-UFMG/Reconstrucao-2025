#include "ProjectData.hpp"

void ProjectData::clear() {
    currentProjectName.clear();
    csvFiles.clear();
}

void ProjectData::loadCSV(const std::filesystem::path& filepath) {
    // Verifica se o arquivo já foi aberto
    for (CSVFile& csvFile : this->csvFiles) {
        if (csvFile.getPath() == filepath) {
            LOG("WARN", "Arquivo já carregado: " + filepath.string());
            return;
        }
    }

    // Cria o documento a partir do arquivo CSV
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
    return true;
}
