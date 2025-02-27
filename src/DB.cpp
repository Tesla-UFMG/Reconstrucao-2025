#include "DB.hpp"

DB& DB::getInstance() {
    static DB instance;
    return instance;
}

DB::DB() : currentProject("a") { LOG("TRACE", "DB iniciada."); }

DB::~DB() { LOG("TRACE", "DB encerrado."); }

void DB::saveFileDialog(const std::string& title, const std::string& defaultName, const char* filter,
                        void (DB::*action)(const std::filesystem::path&)) {
    const char* filters[] = {filter, nullptr};
    char*       filepath  = tinyfd_saveFileDialog(title.c_str(), ("./" + defaultName).c_str(), 1, filters, filter);
    if (!filepath) {
        LOG("ERROR", "Não foi encontrado o local de salvamento do projeto.");
        return;
    }
    (this->*action)(filepath);
}

void DB::createProjectDialog() { saveFileDialog("Criar Projeto", currentProject, "*.tesla", &DB::saveProject); }

void DB::saveProjectDialog() { saveFileDialog("Salvar Projeto", currentProject, "*.tesla", &DB::saveProject); }

void DB::openFileDialog(const std::string& title, const char* filter,
                        void (DB::*action)(const std::filesystem::path&)) {
    const char* filters[] = {filter, nullptr};
    char*       filepath  = tinyfd_openFileDialog(title.c_str(), "./", 1, filters, filter, 0);
    if (!filepath) {
        LOG("ERROR", "Arquivo não encontrado.");
        return;
    }
    (this->*action)(filepath);
}

void DB::loadProjectDialog() { openFileDialog("Carregar Projeto", "*.tesla", &DB::loadProject); }

void DB::loadDataDialog() { openFileDialog("Carregar dados", "*.csv", &DB::loadData); }

void DB::loadData(const std::filesystem::path& filepath) {
    std::ifstream file(filepath);
    if (!file) {
        LOG("ERROR", "Não foi possível carregar o CSV: " + filepath.string());
        return;
    }
    try {
        rapidcsv::Document doc(file, rapidcsv::LabelParams(0, 0));
        csvPaths.push_back(filepath);
        csvData.push_back(doc);
        csvColumns.push_back(doc.GetColumnNames());
        LOG("INFO", "CSV carregado com sucesso.");
    } catch (const std::exception& e) {
        LOG("ERROR", std::string("Erro ao abrir o CSV: ") + e.what());
    }
}

void DB::saveProject(const std::filesystem::path& filepath) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file) {
        LOG("ERROR", "Não foi possível salvar o projeto: " + filepath.string());
        return;
    }
    size_t dataSize = csvPaths.size();
    file.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
    for (const auto& path : csvPaths) {
        std::string pathStr  = path.string();
        size_t      pathSize = pathStr.size();
        file.write(reinterpret_cast<const char*>(&pathSize), sizeof(pathSize));
        file.write(pathStr.data(), pathSize);
    }
    currentProject = filepath.filename().string();
    SDLWrapper::changeWindowTitle(SDLWrapper::windowTitle + " - " + currentProject);
    LOG("INFO", "Projeto salvo com sucesso.");
}

void DB::loadProject(const std::filesystem::path& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        LOG("ERROR", "Não foi possível carregar o projeto: " + filepath.string());
        return;
    }
    size_t dataSize;
    file.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));
    csvPaths.clear();
    for (size_t i = 0; i < dataSize; ++i) {
        size_t pathSize;
        file.read(reinterpret_cast<char*>(&pathSize), sizeof(pathSize));
        std::string pathStr(pathSize, '\0');
        file.read(pathStr.data(), pathSize);
        loadData(pathStr);
    }
    currentProject = filepath.filename().string();
    SDLWrapper::changeWindowTitle(SDLWrapper::windowTitle + " - " + currentProject);
    LOG("INFO", "Projeto carregado com sucesso.");
}

std::string                           DB::getCurrentProject() const { return currentProject; }
std::vector<rapidcsv::Document>       DB::getCsvData() const { return csvData; }
std::vector<std::filesystem::path>    DB::getCsvPaths() const { return csvPaths; }
std::vector<std::vector<std::string>> DB::getCsvColumns() const { return csvColumns; }