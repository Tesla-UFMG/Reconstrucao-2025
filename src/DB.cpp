#include "DB.hpp"

DB& DB::getInstance() {
    static DB instance;
    return instance;
}

DB::DB() { LOG("TRACE", "DB iniciada."); }

DB::~DB() { LOG("TRACE", "DB encerrado."); }

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
    char* filepath = saveFileDialog("Criar Projeto", this->projectData.currentProject, "*.tesla");
    if (filepath) {
        this->projectData.clear();
        DB::saveProject(filepath);
    }
}

void DB::saveProjectDialog() {
    char* filepath = saveFileDialog("Salvar Projeto", this->projectData.currentProject, "*.tesla");
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

void DB::saveProject(const std::filesystem::path& filepath) {
    if (this->projectData.serialize(filepath)) {
        this->projectData.currentProject = filepath.filename().string();
        SDLWrapper::changeWindowTitle(SDLWrapper::windowTitle + " - " + this->projectData.currentProject);
        LOG("INFO", "Projeto salvo com sucesso.");
    }
}

void DB::loadProject(const std::filesystem::path& filepath) {
    if (this->projectData.deserialize(filepath)) {
        this->projectData.currentProject = filepath.filename().string();
        SDLWrapper::changeWindowTitle(SDLWrapper::windowTitle + " - " + this->projectData.currentProject);
        LOG("INFO", "Projeto carregado com sucesso.");
    }
}

void DB::deleteCSV(const std::filesystem::path& filepath) { this->projectData.removeCSV(filepath); }

ProjectData DB::getProject() const { return this->projectData; }

std::vector<std::filesystem::path> DB::getCsvPaths() const { return this->projectData.csvPaths; }

std::vector<std::vector<std::string>> DB::getCsvColumns() const { return this->projectData.csvColumns; }

std::vector<double> DB::getCSVData(const std::string& filename, const std::string& columnName) const {

    for (size_t i = 0; i < this->projectData.csvData.size(); i++) {
        std::string csvName = this->projectData.csvPaths[i].filename().string();

        if (csvName == filename) {
            rapidcsv::Document csv = this->projectData.csvData[i];
            try {
                return csv.GetColumn<double>(columnName);
            } catch (const std::exception& e) {
                LOG("ERROR", "Coluna não encontrada. Erro: " + std::string(e.what()));
                return {};
            }
        }
    }

    LOG("ERROR", "Coluna não encontrada.");
    return {};
}