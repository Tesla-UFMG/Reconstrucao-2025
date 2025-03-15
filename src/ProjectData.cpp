#include "ProjectData.hpp"

void ProjectData::loadCSV(const std::filesystem::path& filepath) {
    auto it = std::find(csvPaths.begin(), csvPaths.end(), filepath);
    if (it != csvPaths.end()) {
        LOG("WARN", "Arquivo já carregado: " + filepath.string());
        return;
    }
    
    std::ifstream file(filepath);
    if (!file) {
        LOG("ERROR", "Não foi possível carregar o CSV em '" + filepath.string() + "'.");
        return;
    }

    try {
        // Cria o documento a partir do arquivo CSV
        rapidcsv::Document doc(file, rapidcsv::LabelParams(0, 0));
        csvPaths.push_back(filepath);
        csvData.push_back(doc);
        csvColumns.push_back(doc.GetColumnNames());
        LOG("INFO", "CSV carregado com sucesso " + filepath.string());
    } catch (const std::exception& e) {
        LOG("ERROR", std::string("Erro ao abrir o CSV: ") + e.what());
    }
}

void ProjectData::clear() {
    currentProject.clear();
    csvPaths.clear();
    csvData.clear();
    csvColumns.clear();
}

bool ProjectData::serialize(const std::filesystem::path& filepath) const {
    std::ofstream file(filepath, std::ios::binary);
    if (!file) {
        LOG("ERROR", "Não foi possível abrir o arquivo para salvar: " + filepath.string());
        return false;
    }

    // Serializa o nome do projeto
    size_t currentSize = currentProject.size();
    file.write(reinterpret_cast<const char*>(&currentSize), sizeof(currentSize));
    file.write(currentProject.data(), currentSize);

    // Serializa os caminhos dos CSVs
    size_t numPaths = csvPaths.size();
    file.write(reinterpret_cast<const char*>(&numPaths), sizeof(numPaths));
    for (const auto& path : csvPaths) {
        std::string pathStr  = path.string();
        size_t      pathSize = pathStr.size();
        file.write(reinterpret_cast<const char*>(&pathSize), sizeof(pathSize));
        file.write(pathStr.data(), pathSize);
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
    size_t currentSize = 0;
    file.read(reinterpret_cast<char*>(&currentSize), sizeof(currentSize));
    currentProject.resize(currentSize);
    file.read(&currentProject[0], currentSize);

    // Desserializa os caminhos dos CSVs e recarrega os dados
    size_t numPaths = 0;
    file.read(reinterpret_cast<char*>(&numPaths), sizeof(numPaths));
    for (size_t i = 0; i < numPaths; ++i) {
        size_t pathSize = 0;
        file.read(reinterpret_cast<char*>(&pathSize), sizeof(pathSize));
        std::string pathStr(pathSize, '\0');
        file.read(&pathStr[0], pathSize);
        this->loadCSV(pathStr);
    }
    return true;
}

void ProjectData::removeCSV(const std::filesystem::path& filepath) {
    auto it = std::find(csvPaths.begin(), csvPaths.end(), filepath);
    if (it != csvPaths.end()) {
        size_t index = std::distance(csvPaths.begin(), it);
        csvPaths.erase(it);
        csvData.erase(csvData.begin() + index);
        csvColumns.erase(csvColumns.begin() + index);
        LOG("INFO", "Arquivo removido: " + filepath.string());
    } else {
        LOG("ERROR", "Arquivo não encontrado para remoção: " + filepath.string());
    }
}