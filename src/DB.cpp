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

char* DB::saveFileDialog(const std::string& title, const std::string& defaultName, const char* filter) {
    const char* filters[] = {filter, nullptr};
    char*       filepath  = tinyfd_saveFileDialog(title.c_str(), ("./" + defaultName).c_str(), 1, filters, filter);
    if (!filepath) {
        LOG("ERROR", "Não foi encontrado o local de salvamento do projeto.");
        return nullptr;
    }
    return filepath;
}

char* DB::openFileDialog(const std::string& title, const char* filter) {
    const char* filters[] = {filter, nullptr};
    char*       filepath  = tinyfd_openFileDialog(title.c_str(), "./", 1, filters, filter, 0);
    if (!filepath) {
        LOG("ERROR", "Arquivo não encontrado.");
        return nullptr;
    }
    return filepath;
}

void DB::createProjectDialog() {
    char* filepath = saveFileDialog("Criar Projeto", this->projectData.currentProjectName, "*.tesla");
    if (filepath) {
        this->projectData.clear();
        DB::saveProject(filepath);
    }
}

void DB::saveProjectDialog() {
    char* filepath = saveFileDialog("Salvar Projeto", this->projectData.currentProjectName, "*.tesla");
    if (filepath) {
        DB::saveProject(filepath);
    }
}

void DB::loadProjectDialog() {
    char* filepath = openFileDialog("Carregar Projeto", "*.tesla");
    if (filepath) {
        DB::loadProject(filepath);
    }
}

void DB::loadCSVDialog() {
    const char* filters[] = {"*.csv", nullptr};
    char*       filepath  = tinyfd_openFileDialog("Carregar dados", "./", 1, filters, "*.csv", 0);
    if (!filepath) {
        LOG("ERROR", "Arquivo não encontrado.");
        return;
    }
    this->projectData.loadCSV(filepath);
}

void DB::errorDialog(const std::string& message) { tinyfd_messageBox("Erro", message.c_str(), "ok", "error", 1); }

void DB::saveProject(const std::filesystem::path& filepath) {
    if (this->projectData.serialize(filepath)) {
        this->projectData.currentProjectName = filepath.filename().string();
        SDLWrapper::changeWindowTitle(SDLWrapper::windowTitle + " - " + this->projectData.currentProjectName);
        LOG("INFO", "Projeto salvo com sucesso.");
    }
}

void DB::loadProject(const std::filesystem::path& filepath) {
    if (this->projectData.deserialize(filepath)) {
        this->projectData.currentProjectName = filepath.filename().string();
        SDLWrapper::changeWindowTitle(SDLWrapper::windowTitle + " - " + this->projectData.currentProjectName);
        LOG("INFO", "Projeto carregado com sucesso.");
    }
}

std::vector<double> DB::getCSVData(const std::string& filepath, const std::string& columnName) const {

    for (const CSVFile& csvFile : this->projectData.csvFiles) {
        const std::string& filepath_ = csvFile.getPath().filename().string();
        // std::cout << filepath << std::endl;
        // std::cout << filepath_ << std::endl;

        if (filepath_ == filepath) {
            try {
                return csvFile.getDocument()->GetColumn<double>(columnName);
            } catch (const std::exception& e) {
                LOG("ERROR", "Coluna não encontrada. Erro: " + std::string(e.what()));
                return {};
            }
        }
    }
    std::string msg = "Coluna '" + columnName + "' não encontrada no arquivo: " + filepath;
    LOG("ERROR", msg);
    return {};
}

std::vector<double> DB::getTelemetryData(const std::string& packetId, const std::string& columnName) const {

    for (const auto& telemetryFile : this->projectData.telemetryFiles) {
        const std::string& packetId_ = telemetryFile.getPacketId();

        if (packetId_ == packetId) {
            try {
                const auto data = telemetryFile.getData();
                const auto cols = telemetryFile.getColumnNames();
                for (size_t i = 0; i < cols.size(); ++i) {
                    if (cols[i] == columnName) {
                        return data[i];
                    }
                }
            } catch (const std::exception& e) {
                LOG("ERROR", "Coluna não encontrada. Erro: " + std::string(e.what()));
                return {};
            }
        }
    }
    std::string msg = "Coluna '" + columnName + "' não encontrada no arquivo: " + packetId;
    LOG("ERROR", msg);
    return {};
}

bool DB::processTelemetryPacket(const std::string& packetId, const std::vector<double>& data) {
    for (TelemetryFile& telemetryFile : this->projectData.telemetryFiles) {
        if (telemetryFile.getPacketId() == packetId) {
            return telemetryFile.insertData(data);
        }
    }
    return false;
}