#include "DB.hpp"
#include "WindowManager.hpp"
#include "ui/windows/w_Warning.hpp"
#include <iomanip>
#include <limits>

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
        auto* warningWin = Window::Warning::getInstance();
        if (warningWin) {
            warningWin->setRules(this->projectData.warningRules);
            warningWin->clearLogs();
        }
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
    std::filesystem::path finalPath = filepath;
    if (finalPath.extension() != ".tesla") {
        finalPath += ".tesla";
    }
    auto* warningWin = Window::Warning::getInstance();
    if (warningWin) {
        this->projectData.warningRules = warningWin->getRules();
    }
    if (this->projectData.serialize(finalPath)) {
        this->projectData.currentProjectName = finalPath.filename().string();
        SDLWrapper::changeWindowTitle(SDLWrapper::windowTitle + " - " + this->projectData.currentProjectName);
        LOG("INFO", "Projeto salvo: " + this->projectData.currentProjectName);
    }
}

void DB::loadProject(const std::filesystem::path& filepath) {
    std::filesystem::path finalPath = filepath;
    if (finalPath.extension() != ".tesla") {
        finalPath += ".tesla";
    }
    if (this->projectData.deserialize(finalPath)) {
        this->projectData.currentProjectName = finalPath.filename().string();
        SDLWrapper::changeWindowTitle(SDLWrapper::windowTitle + " - " + this->projectData.currentProjectName);
        LOG("INFO", "Projeto carregado com sucesso.");

        auto* warningWin = Window::Warning::getInstance();
        if (warningWin) {
            auto rules = this->projectData.warningRules;
            for (auto& rule : rules) {
                rule.lastProcessedIndex = -1;
                rule.wasTriggered = false;
            }
            warningWin->setRules(rules);
            warningWin->clearLogs();
        }

        std::string currentProject = this->projectData.currentProjectName;
        std::string pathIni = "./cache/layouts/" + currentProject + "/.layout_1.ini";
        if (std::filesystem::exists(pathIni)) {
            WindowManager::getInstance().loadWindowVisibility("./cache/layouts/" + currentProject + "/.visibility_1.bin");
            ImGuiWrapper::loadLayout(pathIni);
            LOG("INFO", "Autocarregado Layout 1 do projeto: " + currentProject);
        }
    }
}

bool DB::columnExists(const std::string& fileType, const std::string& fileName, const std::string& columnName) const {
    if (fileType == "CSV") {
        for (const CSVFile& csvFile : this->projectData.csvFiles) {
            if (csvFile.getName() == fileName) {
                for (const auto& col : csvFile.getColumnNames()) {
                    if (col == columnName) return true;
                }
            }
        }
    } else if (fileType == "Telemetry") {
        for (const TelemetryFile& telFile : this->projectData.telemetryFiles) {
            if (telFile.getPacketId() == fileName) {
                for (const auto& col : telFile.getColumnNames()) {
                    if (col == columnName) return true;
                }
            }
        }
    } else if (fileType == "Text") {
        for (const auto& textFile : this->projectData.textFiles) {
            if (textFile.getName() == fileName) {
                for (const auto& col : textFile.getColumnNames()) {
                    if (col == columnName) return true;
                }
            }
        }
    }
    return false;
}

const std::vector<double>& DB::getCSVData(const std::string& filepath, const std::string& columnName) const {
    static const std::vector<double> emptyVec{};

    for (const CSVFile& csvFile : this->projectData.csvFiles) {
        if (csvFile.getName() != filepath)
            continue;

        const std::vector<std::string>& cols = csvFile.getColumnNames();
        for (const auto& col : cols) {
            if (col == columnName) {
                return csvFile.getColumnData(columnName);
            }
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

        const std::vector<std::string>& cols = telemetryFile.getColumnNames();
        for (const auto& col : cols) {
            if (col == columnName) {
                return telemetryFile.getColumnData(columnName);
            }
        }
    }

    LOG("ERROR", "Não foi possível encontrar os dados da coluna '" + columnName + "' no pacote de telemetria '" +
                     packetId + "'.");
    return emptyVec;
}

const std::vector<double>& DB::getTextData(const std::string& filename, const std::string& columnName) const {
    static const std::vector<double> emptyVec{};
    for (const auto& textFile : this->projectData.textFiles) {
        if (textFile.getName() == filename) {
            static std::vector<double> tsVec;
            tsVec.clear();
            for (const auto& dateStr : textFile.getDates()) {
                try {
                    tsVec.push_back(std::stod(dateStr));
                } catch (...) {
                    tsVec.push_back(0.0);
                }
            }
            return tsVec;
        }
    }
    return emptyVec;
}

bool DB::processTelemetryPacket(const std::string& packetId, const std::vector<double>& data, const std::string& date) {
    for (TelemetryFile& telemetryFile : this->projectData.telemetryFiles) {
        if (telemetryFile.getPacketId() == packetId) {
            if (telemetryFile.insertData(data)) {
                return telemetryFile.insertDate(date);
            }
        }
    }
    return false;
}

bool DB::saveTelemetryPackets(const std::string& outputFolder) {

    std::filesystem::path baseDir = outputFolder.empty() ? std::filesystem::path(TELEMETRY_OUTPUT)
                                                         : TELEMETRY_OUTPUT / std::filesystem::path(outputFolder);

    if (std::filesystem::create_directories(baseDir)) {
        LOG("INFO", "Criada pasta de saída: " + baseDir.string());
    }

    for (const TelemetryFile& file : this->getProject().telemetryFiles) {
        const std::string                      packetName  = file.getName();
        const std::string                      packetId    = file.getPacketId();
        std::vector<std::vector<double>>       data        = file.getData();
        const std::vector<std::string>         date        = file.getDate();
        const std::vector<std::string>         columnNames = file.getColumnNames();

        // nome do arquivo
        std::string           filename = packetId + "_" + packetName + ".csv";
        std::filesystem::path outPath  = baseDir / filename;

        // abre o arquivo
        std::ofstream ofs(outPath, std::ios::trunc);
        if (!ofs.is_open()) {
            LOG("ERROR", "Não foi possível criar arquivo: " + outPath.string());
            return false;
        }

        // Set max precision for doubles
        ofs << std::setprecision(std::numeric_limits<double>::max_digits10);

        // cabeçalho (only non-empty column names)
        ofs << "index,date,";
        std::vector<std::string> validNames;
        for (const auto &colName : columnNames) {
            if (!colName.empty())
                validNames.push_back(colName);
        }
        for (size_t i = 0; i < validNames.size(); ++i) {
            ofs << validNames[i];
            if (i + 1 < validNames.size())
                ofs << ',';
        }
        ofs << '\n';

        // dados
        size_t numRows = data.empty() ? 0 : data[0].size();
        size_t numCols = data.size();
        // Verify column count matches number of valid column names
        size_t validColCount = 0;
        for (const auto &colName : columnNames) {
            if (!colName.empty())
                ++validColCount;
        }
        if (numCols != validColCount) {
            LOG("WARNING", "Mismatch between defined column names (" + std::to_string(validColCount) + ") and data columns (" + std::to_string(numCols) + "). Adjusting.");
            if (numCols > validColCount) {
                numCols = validColCount;
            } else {
                for (size_t i = numCols; i < validColCount; ++i) {
                    data.push_back(std::vector<double>());
                }
                numCols = validColCount;
            }
        }
        for (size_t row = 0; row < numRows; ++row) {
            ofs << row << "," << date[row] << ",";
            for (size_t col = 0; col < numCols; ++col) {
                // If data for this column is missing for this row, write empty
                if (row < data[col].size())
                    ofs << data[col][row];
                if (col + 1 < numCols)
                    ofs << ',';
            }
            ofs << '\n';
        }

        ofs.close();
        LOG("INFO", "Salvo CSV: " + outPath.string());
    }

    // Save TextFiles as CSVs!
    for (const TextFile& textFile : this->getProject().getTextFiles()) {
        if (textFile.getName() == "Avisos") {
            continue; // Skip saving the sparse "Avisos" TextFile to avoid duplication
        }
        std::string filename = (textFile.getName() == "Comentários") ? "comentarios.csv" : "avisos.csv";
        std::filesystem::path outPath = baseDir / filename;
        std::ofstream ofs(outPath, std::ios::trunc);
        if (ofs.is_open()) {
            // Set max precision for doubles
            ofs << std::setprecision(std::numeric_limits<double>::max_digits10);
            
            // Write BOM for Excel UTF-8 compatibility
            ofs << "\xEF\xBB\xBF";
            
            // Header: index,date,col1,col2,...
            ofs << "index,date";
            for (const auto& col : textFile.getColumnNames()) {
                ofs << "," << col;
            }
            ofs << "\n";
            
            const auto& dates = textFile.getDates();
            const auto& data = textFile.getData();
            size_t numRows = dates.size();
            for (size_t row = 0; row < numRows; ++row) {
                ofs << row << "," << dates[row];
                for (size_t col = 0; col < textFile.getColumnNames().size(); ++col) {
                    if (col < data.size() && row < data[col].size()) {
                        ofs << "," << data[col][row];
                    } else {
                        ofs << ",";
                    }
                }
                ofs << "\n";
            }
            ofs.close();
            LOG("INFO", "Salvo CSV de texto: " + outPath.string());
        }
    }

    auto* warningWin = Window::Warning::getInstance();
    if (warningWin) {
        std::filesystem::path warnPath = baseDir / "avisos.csv";
        warningWin->exportToCSV(warnPath.string());
    }

    return true;
}

