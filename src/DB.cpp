#include "DB.hpp"

DB& DB::getInstance() {
    static DB instance;
    return instance;
}

DB::DB() { LOG("TRACE", "DB iniciada."); }

DB::~DB() { LOG("TRACE", "DB encerrado."); }

void DB::deleteCSV(const std::filesystem::path& filepath) { this->projectData.removeCSV(filepath); }

const ProjectData& DB::getProject() const { return this->projectData; }

ProjectData& DB::getProject() { return this->projectData; }

void DB::createProjectDialog() {
    char* filepath = Dialogs::showSaveFileDialog("Criar Projeto", this->projectData.currentProjectName, "*.tesla");
    if (filepath) {
        this->projectData.clear();
        DB::saveProject(filepath);
    }
}

void DB::saveProjectDialog() {
    char* filepath = Dialogs::showSaveFileDialog("Salvar Projeto", this->projectData.currentProjectName, "*.tesla");
    if (filepath) {
        DB::saveProject(filepath);
    }
}

void DB::loadProjectDialog() {
    char* filepath = Dialogs::showOpenFileDialog("Carregar Projeto", "*.tesla");
    if (filepath) {
        DB::loadProject(filepath);
    }
}

void DB::loadCSVDialog() {
    char* filepath = Dialogs::showOpenFileDialog("Carregar CSV", "*.csv");
    if (!filepath) {
        LOG("ERROR", "Arquivo não encontrado.");
        return;
    }
    this->projectData.loadCSV(filepath);
}

void DB::saveProject(const std::filesystem::path& filepath) {
    if (this->projectData.serialize(filepath)) {
        this->projectData.currentProjectName = filepath.filename().string();
        SDLWrapper::changeWindowTitle(SDLWrapper::windowTitle + " - " + this->projectData.currentProjectName);
        LOG("INFO", "Projeto salvo: " + this->projectData.currentProjectName);
    }
}

void DB::loadProject(const std::filesystem::path& filepath) {
    if (this->projectData.deserialize(filepath)) {
        this->projectData.currentProjectName = filepath.filename().string();
        SDLWrapper::changeWindowTitle(SDLWrapper::windowTitle + " - " + this->projectData.currentProjectName);
        LOG("INFO", "Projeto carregado com sucesso.");
    }
}

const std::vector<double>& DB::getCSVData(const std::string& filepath, const std::string& columnName) const {
    static const std::vector<double> emptyVec{};

    for (const CSVFile& csvFile : this->projectData.csvFiles) {
        if (csvFile.getName() != filepath)
            continue;

        const std::vector<double>& data = csvFile.getColumnData(columnName);
        if (!data.empty()) {
            return data;
        }
    }
    LOG("ERROR",
        "Não foi possível encontrar os dados da coluna '" + columnName + "' no arquivo CSV '" + filepath + "'.");
    return emptyVec;
}

const std::vector<double>& DB::getTelemetryData(const std::string& packetId, const std::string& columnName) const {
    static const std::vector<double> emptyVec{};

    for (const TelemetryFile& telemetryFile : this->projectData.telemetryFiles) {
        if (telemetryFile.getPacketId() != packetId)
            continue;

        const std::vector<double>& data = telemetryFile.getColumnData(columnName);
        if (!data.empty()) {
            return data;
        }
    }

    LOG("ERROR", "Não foi possível encontrar os dados da coluna '" + columnName + "' no pacote de telemetria '" +
                     packetId + "'.");
    return emptyVec;
}

bool DB::processTelemetryPacket(const std::string& packetId, const std::vector<double>& data) {
    for (TelemetryFile& telemetryFile : this->projectData.telemetryFiles) {
        if (telemetryFile.getPacketId() == packetId) {
            return telemetryFile.insertData(data);
        }
    }
    return false;
}