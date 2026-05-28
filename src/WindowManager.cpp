#include "WindowManager.hpp"
#include "Log.hpp"

#include "ui/windows/iWindow.hpp"
#include "ui/windows/w_About.hpp"
#include "ui/windows/w_DataPicker.hpp"
#include "ui/windows/w_Demo.hpp"
#include "ui/windows/w_HomePage.hpp"
#include "ui/windows/w_Pedal.hpp"
#include "ui/windows/w_Plot.hpp"
#include "ui/windows/w_Statistics.hpp"
#include "ui/windows/w_Telemetry.hpp"
#include "ui/windows/w_Terminal.hpp"
#include "ui/windows/w_WheelControl.hpp"
#include "ui/windows/w_Reconstruction.hpp" 

WindowManager& WindowManager::getInstance() {
    static WindowManager instance;
    return instance;
}

WindowManager::WindowManager() { LOG("TRACE", "Window Manager iniciado com sucesso."); }

WindowManager::~WindowManager() { LOG("TRACE", "Window Manager encerrado."); }

void WindowManager::init(SDL_Renderer* renderer) {
    this->m_renderer = renderer;
    this->setup();
}

void WindowManager::saveWindowVisibility(const std::filesystem::path& filepath) {
    std::filesystem::path parentPath = filepath.parent_path();
    if (!parentPath.empty() && !std::filesystem::exists(parentPath)) {
        std::filesystem::create_directories(parentPath);
        LOG("INFO", "Criada pasta '" + parentPath.string() + "'.");
    }

    std::ofstream file(filepath, std::ios::binary);
    if (!file) {
        LOG("WARN", "Não foi possível salvar a visibilidade '" + filepath.string() + "'.");
        return;
    }

    file.write(reinterpret_cast<const char*>(&visibility), sizeof(VisibilityFlags));
    LOG("INFO", "Visibilidade '" + filepath.string() + "' salvo com sucesso.");

    saveDynamicWindows(filepath.string() + ".numeric");
}

void WindowManager::loadWindowVisibility(const std::filesystem::path& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        LOG("WARN", "Não foi possível carregar a visibilidade '" + filepath.string() + "'.");
        return;
    }

    file.read(reinterpret_cast<char*>(&visibility), sizeof(VisibilityFlags));
    LOG("INFO", "Visibilidade '" + filepath.string() + "' carregada com sucesso.");

    loadDynamicWindows(filepath.string() + ".numeric");
}

void WindowManager::setup() {
    // --- Criação das Janelas "Reproduzíveis" ---

    // 1. Janela de Reconstrução
    auto temp_reconstruction_ptr = std::make_unique<Window::Reconstruction>(&visibility.showReconstruction);
    m_reconstructionWindow = temp_reconstruction_ptr.get();

    // 2. Janela de Pedal 
    auto temp_pedal_ptr = std::make_unique<Window::Pedal>(&visibility.showPedal);

    // 3. Janela de Volante 
    auto temp_wheel_ptr = std::make_unique<Window::WheelControl>(&visibility.showWheelControl);

    // --- Adiciona todos os ponteiros únicos ao vetor principal de janelas ---
    
    windows.emplace_back(std::move(temp_reconstruction_ptr));
    windows.emplace_back(std::move(temp_pedal_ptr));
    windows.emplace_back(std::move(temp_wheel_ptr));

    // --- Criação das Outras Janelas (não-reproduzíveis) ---
    
    home = std::make_unique<Window::HomePage>();
    
    auto temp_about_ptr = std::make_unique<Window::About>(&visibility.showAbout);
    m_aboutWindow = temp_about_ptr.get();
    windows.emplace_back(std::move(temp_about_ptr));

    windows.emplace_back(std::make_unique<Window::DataPicker>(&visibility.showDataPicker));
    windows.emplace_back(std::make_unique<Window::Plot>(&visibility.showPlot));
    windows.emplace_back(std::make_unique<Window::Terminal>(&visibility.showLog));
    windows.emplace_back(std::make_unique<Window::ImGuiDemo>(&visibility.showImGuiDemo));
    windows.emplace_back(std::make_unique<Window::ImPlotDemo>(&visibility.showImPlotDemo));
    windows.emplace_back(std::make_unique<Window::ImPlot3dDemo>(&visibility.showImPlot3dDemo));
    // windows.emplace_back(std::make_unique<Window::WheelControl>(&visibility.showWheelControl));
    windows.emplace_back(std::make_unique<Window::Statistics>(&visibility.showStatistics));
    windows.emplace_back(std::make_unique<Window::Telemetry>(&visibility.showTelemetry));
}

void WindowManager::homePage() {
    MenuBar::render();
    if (m_aboutWindow) {
        m_aboutWindow->render();
    }
    home->render();       // Home page
}

void WindowManager::mainPage() {
    MenuBar::render();

    // Remove closed dynamic windows
    windows.erase(std::remove_if(windows.begin(), windows.end(),
        [](const std::unique_ptr<IWindow>& window) {
            return window->isDynamic() && !window->getIsOpen();
        }), windows.end());

    for (auto& window : windows) {
        window->render();
    }
}

void WindowManager::createNumericWindow() {
    static int numericCount = 0;
    numericCount++;
    std::string windowTitle = "Numérico #" + std::to_string(numericCount);

    auto numericWindow = std::make_unique<Window::Numeric>(windowTitle);
    windows.emplace_back(std::move(numericWindow));

    LOG("INFO", "Criada nova janela numérica: " + windowTitle);
}

void WindowManager::createGraphWindow() {
    static int graphCount = 0;
    graphCount++;
    std::string windowTitle = "Gráfico #" + std::to_string(graphCount);

    auto graphWindow = std::make_unique<Window::Graph>(windowTitle);
    windows.emplace_back(std::move(graphWindow));

    LOG("INFO", "Criada nova janela gráfica: " + windowTitle);
}

void WindowManager::saveDynamicWindows(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file) {
        LOG("WARN", "Não foi possível salvar as janelas dinâmicas em '" + filepath + "'.");
        return;
    }

    int count = 0;
    for (const auto& w : windows) {
        if (w->isDynamic()) {
            count++;
        }
    }
    file << count << "\n";

    for (const auto& w : windows) {
        if (w->isDynamic()) {
            file << w->getDynamicType() << "\n";
            if (w->getDynamicType() == "Numeric") {
                auto* numWin = dynamic_cast<Window::Numeric*>(w.get());
                if (numWin) {
                    file << numWin->getTitle() << "\n";
                    file << numWin->hasData() << "\n";
                    if (numWin->hasData()) {
                        file << numWin->getFileType() << "\n";
                        file << numWin->getArchiveName() << "\n";
                        file << numWin->getColumnName() << "\n";
                    }
                    file << static_cast<int>(numWin->getMetricType()) << "\n";

                    // Prefix and Suffix
                    file << numWin->m_prefix << "\n";
                    file << numWin->m_suffix << "\n";

                    // Formula
                    file << numWin->m_useFormula << "\n";
                    file << numWin->m_multiplier << "\n";
                    file << numWin->m_offset << "\n";

                    // Translations
                    file << numWin->m_useTranslation << "\n";
                    file << numWin->m_translationRules.size() << "\n";
                    for (const auto& rule : numWin->m_translationRules) {
                        file << rule.value << "\n";
                        file << rule.text << "\n";
                    }

                    // Colors
                    file << numWin->m_colorMode << "\n";
                    file << numWin->m_threshLL << "\n";
                    file << numWin->m_threshL << "\n";
                    file << numWin->m_threshH << "\n";
                    file << numWin->m_threshHH << "\n";

                    auto saveConf = [&](const ColorThresholdConfig& conf) {
                        file << conf.enabled << "\n";
                        file << conf.bg[0] << " " << conf.bg[1] << " " << conf.bg[2] << " " << conf.bg[3] << "\n";
                        file << conf.fg[0] << " " << conf.fg[1] << " " << conf.fg[2] << " " << conf.fg[3] << "\n";
                    };

                    saveConf(numWin->m_confLL);
                    saveConf(numWin->m_confL);
                    saveConf(numWin->m_confNormal);
                    saveConf(numWin->m_confH);
                    saveConf(numWin->m_confHH);

                    file << numWin->m_specificRules.size() << "\n";
                    for (const auto& rule : numWin->m_specificRules) {
                        file << rule.value << "\n";
                        file << rule.bg[0] << " " << rule.bg[1] << " " << rule.bg[2] << " " << rule.bg[3] << "\n";
                        file << rule.fg[0] << " " << rule.fg[1] << " " << rule.fg[2] << " " << rule.fg[3] << "\n";
                    }
                    file << numWin->m_fontScale << "\n";
                    file << numWin->m_showColumnName << "\n";
                    file << numWin->m_stripPattern << "\n";
                }
            } else if (w->getDynamicType() == "Graph") {
                auto* graphWin = dynamic_cast<Window::Graph*>(w.get());
                if (graphWin) {
                    const auto& graph = graphWin->getGraph();
                    const auto& config = graph.config;
                    file << graphWin->getTitle() << "\n";
                    file << static_cast<int>(config.type) << "\n";
                    file << config.showXAxis << "\n";
                    file << config.showYAxis << "\n";
                    file << config.numPoints << "\n";
                    file << config.followTheEnd << "\n";
                    file << config.autoFit << "\n";
                    file << config.showValueOnYAxis << "\n";
                    file << config.showCursorOnYAxis << "\n";
                    file << config.xColumn << "\n";
                    file << graph.data.size() << "\n";
                    for (const auto& gd : graph.data) {
                        file << gd.columnName << "\n";
                        file << gd.fileName << "\n";
                        file << gd.fileType << "\n";
                        file << gd.multiplier << "\n";
                    }
                }
            }
        }
    }
}

void WindowManager::loadDynamicWindows(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file) {
        return;
    }

    // 1. Remove todas as janelas dinâmicas existentes
    windows.erase(std::remove_if(windows.begin(), windows.end(),
        [](const std::unique_ptr<IWindow>& w) {
            return w->isDynamic();
        }), windows.end());

    int count = 0;
    if (!(file >> count)) return;
    std::string dummy;
    std::getline(file, dummy); // Consome o newline

    for (int i = 0; i < count; i++) {
        std::string line;
        if (!std::getline(file, line)) break;

        std::string type = "Numeric";
        std::string title;
        if (line == "Numeric" || line == "Graph") {
            type = line;
            if (!std::getline(file, title)) break;
        } else {
            // Retrocompatibilidade: se não for "Numeric" ou "Graph", a primeira linha é o título do tipo Numeric
            title = line;
        }

        if (type == "Numeric") {
            bool hasData = false;
            file >> hasData;
            std::getline(file, dummy); // Consome o newline

            std::string fileType, archive, column;
            if (hasData) {
                std::getline(file, fileType);
                std::getline(file, archive);
                std::getline(file, column);
            }

            int metricVal = 0;
            file >> metricVal;
            std::getline(file, dummy); // Consome o newline

            auto numWin = std::make_unique<Window::Numeric>(title);
            if (hasData) {
                numWin->addColumn(fileType, archive, column);
            }
            numWin->setMetricType(static_cast<MetricType>(metricVal));

            // Prefix and Suffix
            std::string prefix, suffix;
            std::getline(file, prefix);
            strncpy(numWin->m_prefix, prefix.c_str(), sizeof(numWin->m_prefix));
            std::getline(file, suffix);
            strncpy(numWin->m_suffix, suffix.c_str(), sizeof(numWin->m_suffix));

            file >> numWin->m_useFormula;
            file >> numWin->m_multiplier;
            file >> numWin->m_offset;

            file >> numWin->m_useTranslation;
            size_t transSize = 0;
            file >> transSize;
            std::getline(file, dummy); // consume newline
            numWin->m_translationRules.clear();
            for (size_t r = 0; r < transSize; r++) {
                double rVal = 0.0;
                std::string rTxt;
                file >> rVal;
                std::getline(file, dummy); // consume newline
                std::getline(file, rTxt);
                numWin->m_translationRules.push_back({rVal, rTxt});
            }

            file >> numWin->m_colorMode;
            file >> numWin->m_threshLL;
            file >> numWin->m_threshL;
            file >> numWin->m_threshH;
            file >> numWin->m_threshHH;

            auto loadConf = [&](ColorThresholdConfig& conf) {
                file >> conf.enabled;
                file >> conf.bg[0] >> conf.bg[1] >> conf.bg[2] >> conf.bg[3];
                file >> conf.fg[0] >> conf.fg[1] >> conf.fg[2] >> conf.fg[3];
            };

            loadConf(numWin->m_confLL);
            loadConf(numWin->m_confL);
            loadConf(numWin->m_confNormal);
            loadConf(numWin->m_confH);
            loadConf(numWin->m_confHH);

            size_t specSize = 0;
            file >> specSize;
            numWin->m_specificRules.clear();
            for (size_t r = 0; r < specSize; r++) {
                SpecificColorRule rule;
                file >> rule.value;
                file >> rule.bg[0] >> rule.bg[1] >> rule.bg[2] >> rule.bg[3];
                file >> rule.fg[0] >> rule.fg[1] >> rule.fg[2] >> rule.fg[3];
                numWin->m_specificRules.push_back(rule);
            }
            std::getline(file, dummy); // consume newline

            // Font scale, showColumnName, and stripPattern with safe check for backward compatibility
            float fontScale = 1.0f;
            if (file >> fontScale) {
                numWin->m_fontScale = fontScale;
                std::getline(file, dummy); // consume newline

                bool showColumnName = true;
                if (file >> showColumnName) {
                    numWin->m_showColumnName = showColumnName;
                    std::getline(file, dummy); // consume newline

                    std::string stripPattern;
                    if (std::getline(file, stripPattern)) {
                        strncpy(numWin->m_stripPattern, stripPattern.c_str(), sizeof(numWin->m_stripPattern));
                    }
                }
            }

            windows.emplace_back(std::move(numWin));
        } else if (type == "Graph") {
            auto graphWin = std::make_unique<Window::Graph>(title);
            auto& graph = graphWin->getGraph();
            auto& config = graph.config;

            int typeVal = 0;
            file >> typeVal;
            config.type = static_cast<GraphType>(typeVal);

            file >> config.showXAxis;
            file >> config.showYAxis;
            file >> config.numPoints;
            file >> config.followTheEnd;
            file >> config.autoFit;
            file >> config.showValueOnYAxis;
            file >> config.showCursorOnYAxis;
            std::getline(file, dummy); // Consome newline
            std::getline(file, config.xColumn);

            size_t dataSize = 0;
            file >> dataSize;
            std::getline(file, dummy); // Consome newline

            for (size_t d = 0; d < dataSize; d++) {
                std::string colName, fileName, fileType;
                double multiplier = 1.0;

                std::getline(file, colName);
                std::getline(file, fileName);
                std::getline(file, fileType);
                file >> multiplier;
                std::getline(file, dummy); // Consome newline

                graphWin->addColumn(fileType, fileName, colName);
                if (!graph.data.empty()) {
                    graph.data.back().multiplier = multiplier;
                }
            }

            windows.emplace_back(std::move(graphWin));
        }
    }
}