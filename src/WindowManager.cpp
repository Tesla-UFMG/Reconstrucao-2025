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
    windows.emplace_back(std::make_unique<Window::Warning>(&visibility.showWarnings));
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
    int maxIdx = 0;
    for (const auto& w : windows) {
        if (w->isDynamic() && w->getDynamicType() == "Numeric") {
            auto* numWin = dynamic_cast<Window::Numeric*>(w.get());
            if (numWin) {
                std::string wTitle = numWin->getTitle();
                size_t hashPos = wTitle.find('#');
                if (hashPos != std::string::npos) {
                    try {
                        int idx = std::stoi(wTitle.substr(hashPos + 1));
                        if (idx > maxIdx) {
                            maxIdx = idx;
                        }
                    } catch (...) {}
                }
            }
        }
    }
    std::string windowTitle = "Numérico #" + std::to_string(maxIdx + 1);

    auto numericWindow = std::make_unique<Window::Numeric>(windowTitle);
    windows.emplace_back(std::move(numericWindow));

    LOG("INFO", "Criada nova janela numérica: " + windowTitle);
}

void WindowManager::createGraphWindow() {
    int maxIdx = 0;
    for (const auto& w : windows) {
        if (w->isDynamic() && w->getDynamicType() == "Graph") {
            auto* graphWin = dynamic_cast<Window::Graph*>(w.get());
            if (graphWin) {
                std::string wTitle = graphWin->getTitle();
                size_t hashPos = wTitle.find('#');
                if (hashPos != std::string::npos) {
                    try {
                        int idx = std::stoi(wTitle.substr(hashPos + 1));
                        if (idx > maxIdx) {
                            maxIdx = idx;
                        }
                    } catch (...) {}
                }
            }
        }
    }
    std::string windowTitle = "Gráfico #" + std::to_string(maxIdx + 1);

    auto graphWindow = std::make_unique<Window::Graph>(windowTitle);
    windows.emplace_back(std::move(graphWindow));

    LOG("INFO", "Criada nova janela gráfica: " + windowTitle);
}

void WindowManager::createBarWindow() {
    int maxIdx = 0;
    for (const auto& w : windows) {
        if (w->isDynamic() && w->getDynamicType() == "Bar") {
            auto* barWin = dynamic_cast<Window::Bar*>(w.get());
            if (barWin) {
                std::string wTitle = barWin->getTitle();
                size_t hashPos = wTitle.find('#');
                if (hashPos != std::string::npos) {
                    try {
                        int idx = std::stoi(wTitle.substr(hashPos + 1));
                        if (idx > maxIdx) {
                            maxIdx = idx;
                        }
                    } catch (...) {}
                }
            }
        }
    }
    std::string windowTitle = "Barra #" + std::to_string(maxIdx + 1);

    auto barWindow = std::make_unique<Window::Bar>(windowTitle);
    windows.emplace_back(std::move(barWindow));

    LOG("INFO", "Criada nova janela de barra: " + windowTitle);
}

void WindowManager::createMatrixWindow() {
    int maxIdx = 0;
    for (const auto& w : windows) {
        if (w->isDynamic() && w->getDynamicType() == "Matrix") {
            auto* matWin = dynamic_cast<Window::Matrix*>(w.get());
            if (matWin) {
                std::string wTitle = matWin->getTitle();
                size_t hashPos = wTitle.find('#');
                if (hashPos != std::string::npos) {
                    try {
                        int idx = std::stoi(wTitle.substr(hashPos + 1));
                        if (idx > maxIdx) {
                            maxIdx = idx;
                        }
                    } catch (...) {}
                }
            }
        }
    }
    std::string windowTitle = "Matriz #" + std::to_string(maxIdx + 1);

    auto matWindow = std::make_unique<Window::Matrix>(windowTitle);
    windows.emplace_back(std::move(matWindow));

    LOG("INFO", "Criada nova janela de matriz: " + windowTitle);
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
                    file << numWin->getLoadedColumns().size() << "\n";
                    for (const auto& col : numWin->getLoadedColumns()) {
                        file << col.fileType << "\n";
                        file << col.archive << "\n";
                        file << col.column << "\n";
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

                    file << numWin->m_gradMinVal << "\n";
                    file << numWin->m_gradMaxVal << "\n";
                    file << numWin->m_gradMinColor[0] << " " << numWin->m_gradMinColor[1] << " " << numWin->m_gradMinColor[2] << " " << numWin->m_gradMinColor[3] << "\n";
                    file << numWin->m_gradMaxColor[0] << " " << numWin->m_gradMaxColor[1] << " " << numWin->m_gradMaxColor[2] << " " << numWin->m_gradMaxColor[3] << "\n";
                    file << numWin->m_statModeAll << "\n";
                    file << numWin->m_customLabel << "\n";
                }
            } else if (w->getDynamicType() == "Bar") {
                auto* barWin = dynamic_cast<Window::Bar*>(w.get());
                if (barWin) {
                    file << barWin->getTitle() << "\n";
                    file << barWin->hasData() << "\n";
                    if (barWin->hasData()) {
                        file << barWin->getFileType() << "\n";
                        file << barWin->getArchiveName() << "\n";
                        file << barWin->getColumnName() << "\n";
                    }
                    file << static_cast<int>(barWin->getMetricType()) << "\n";

                    // Orientation and scale limits
                    file << barWin->m_orientation << "\n";
                    file << barWin->m_useManualLimits << "\n";
                    file << barWin->m_minVal << "\n";
                    file << barWin->m_maxVal << "\n";

                    // Colors
                    file << barWin->m_barColor[0] << " " << barWin->m_barColor[1] << " " << barWin->m_barColor[2] << " " << barWin->m_barColor[3] << "\n";
                    file << barWin->m_bgColor[0] << " " << barWin->m_bgColor[1] << " " << barWin->m_bgColor[2] << " " << barWin->m_bgColor[3] << "\n";
                    file << barWin->m_fgColor[0] << " " << barWin->m_fgColor[1] << " " << barWin->m_fgColor[2] << " " << barWin->m_fgColor[3] << "\n";

                    // Text properties
                    file << barWin->m_fontScale << "\n";
                    file << barWin->m_showPercentage << "\n";
                    file << barWin->m_showValue << "\n";
                    file << barWin->m_showColumnName << "\n";
                    file << barWin->m_stripPattern << "\n";
                    file << barWin->m_prefix << "\n";
                    file << barWin->m_suffix << "\n";

                    // Thresholds
                    file << barWin->m_useThresholds << "\n";
                    file << barWin->m_threshLL << "\n";
                    file << barWin->m_threshL << "\n";
                    file << barWin->m_threshH << "\n";
                    file << barWin->m_threshHH << "\n";

                    auto saveBarConf = [&](const BarThresholdConfig& conf) {
                        file << conf.enabled << "\n";
                        file << conf.color[0] << " " << conf.color[1] << " " << conf.color[2] << " " << conf.color[3] << "\n";
                    };

                    saveBarConf(barWin->m_confLL);
                    saveBarConf(barWin->m_confL);
                    saveBarConf(barWin->m_confNormal);
                    saveBarConf(barWin->m_confH);
                    saveBarConf(barWin->m_confHH);

                    // Bar Gradient Configs
                    file << barWin->m_useGradient << "\n";
                    file << barWin->m_gradMinColor[0] << " " << barWin->m_gradMinColor[1] << " " << barWin->m_gradMinColor[2] << " " << barWin->m_gradMinColor[3] << "\n";
                    file << barWin->m_gradMaxColor[0] << " " << barWin->m_gradMaxColor[1] << " " << barWin->m_gradMaxColor[2] << " " << barWin->m_gradMaxColor[3] << "\n";

                    // Unified color mode + gradient limits
                    file << barWin->m_colorBarMode << "\n";
                    file << barWin->m_gradMinVal << "\n";
                    file << barWin->m_gradMaxVal << "\n";
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
            } else if (w->getDynamicType() == "Matrix") {
                auto* matWin = dynamic_cast<Window::Matrix*>(w.get());
                if (matWin) {
                    file << matWin->getTitle() << "\n";
                    file << matWin->m_colorMode << "\n";
                    file << matWin->m_minVal << "\n";
                    file << matWin->m_maxVal << "\n";
                    file << matWin->m_minColor[0] << " " << matWin->m_minColor[1] << " " << matWin->m_minColor[2] << " " << matWin->m_minColor[3] << "\n";
                    file << matWin->m_maxColor[0] << " " << matWin->m_maxColor[1] << " " << matWin->m_maxColor[2] << " " << matWin->m_maxColor[3] << "\n";
                    file << matWin->m_threshLL << "\n";
                    file << matWin->m_threshL  << "\n";
                    file << matWin->m_threshH  << "\n";
                    file << matWin->m_threshHH << "\n";

                    auto saveMatConf = [&](const ColorThresholdConfig& conf) {
                        file << conf.enabled << "\n";
                        file << conf.bg[0] << " " << conf.bg[1] << " " << conf.bg[2] << " " << conf.bg[3] << "\n";
                        file << conf.fg[0] << " " << conf.fg[1] << " " << conf.fg[2] << " " << conf.fg[3] << "\n";
                    };
                    saveMatConf(matWin->m_confLL);
                    saveMatConf(matWin->m_confL);
                    saveMatConf(matWin->m_confNormal);
                    saveMatConf(matWin->m_confH);
                    saveMatConf(matWin->m_confHH);

                    // Columns
                    file << matWin->m_columns.size() << "\n";
                    for (const auto& col : matWin->m_columns) {
                        file << col.fileType    << "\n";
                        file << col.archiveName << "\n";
                        file << col.variables.size() << "\n";
                        for (const auto& var : col.variables) {
                            file << var << "\n";
                        }
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
        if (line == "Numeric" || line == "Graph" || line == "Bar" || line == "Matrix") {
            type = line;
            if (!std::getline(file, title)) break;
        } else {
            // Retrocompatibilidade: se não for "Numeric", "Graph" ou "Bar", a primeira linha é o título do tipo Numeric
            title = line;
        }

        if (type == "Numeric") {
            size_t numColumns = 0;
            file >> numColumns;
            std::getline(file, dummy); // Consome o newline

            auto numWin = std::make_unique<Window::Numeric>(title);
            for (size_t c = 0; c < numColumns; ++c) {
                std::string fileType, archive, column;
                std::getline(file, fileType);
                std::getline(file, archive);
                std::getline(file, column);
                numWin->addColumn(fileType, archive, column);
            }

            int metricVal = 0;
            file >> metricVal;
            std::getline(file, dummy); // Consome o newline
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

            // Safe check for backward compatibility of Gradient config
            double gradMinVal = 20.0;
            if (file >> gradMinVal) {
                numWin->m_gradMinVal = gradMinVal;
                file >> numWin->m_gradMaxVal;
                file >> numWin->m_gradMinColor[0] >> numWin->m_gradMinColor[1] >> numWin->m_gradMinColor[2] >> numWin->m_gradMinColor[3];
                file >> numWin->m_gradMaxColor[0] >> numWin->m_gradMaxColor[1] >> numWin->m_gradMaxColor[2] >> numWin->m_gradMaxColor[3];
                std::getline(file, dummy); // consume newline
            }

            bool statModeAll = true;
            if (file >> statModeAll) {
                numWin->m_statModeAll = statModeAll;
                std::getline(file, dummy); // consume newline
                
                std::string customLabel;
                if (std::getline(file, customLabel)) {
                    strncpy(numWin->m_customLabel, customLabel.c_str(), sizeof(numWin->m_customLabel));
                }
            } else {
                file.clear();
            }

            windows.emplace_back(std::move(numWin));
        } else if (type == "Bar") {
            bool hasData = false;
            file >> hasData;
            std::getline(file, dummy); // consume newline

            std::string fileType, archive, column;
            if (hasData) {
                std::getline(file, fileType);
                std::getline(file, archive);
                std::getline(file, column);
            }

            int metricVal = 0;
            file >> metricVal;
            std::getline(file, dummy); // consume newline

            auto barWin = std::make_unique<Window::Bar>(title);
            if (hasData) {
                barWin->addColumn(fileType, archive, column);
            }
            barWin->setMetricType(static_cast<MetricType>(metricVal));

            // Orientation and scale limits
            file >> barWin->m_orientation;
            file >> barWin->m_useManualLimits;
            file >> barWin->m_minVal;
            file >> barWin->m_maxVal;

            // Colors
            file >> barWin->m_barColor[0] >> barWin->m_barColor[1] >> barWin->m_barColor[2] >> barWin->m_barColor[3];
            file >> barWin->m_bgColor[0] >> barWin->m_bgColor[1] >> barWin->m_bgColor[2] >> barWin->m_bgColor[3];
            file >> barWin->m_fgColor[0] >> barWin->m_fgColor[1] >> barWin->m_fgColor[2] >> barWin->m_fgColor[3];

            // Text properties
            file >> barWin->m_fontScale;
            file >> barWin->m_showPercentage;
            file >> barWin->m_showValue;
            file >> barWin->m_showColumnName;
            std::getline(file, dummy); // consume newline

            std::string stripPattern, prefix, suffix;
            std::getline(file, stripPattern);
            strncpy(barWin->m_stripPattern, stripPattern.c_str(), sizeof(barWin->m_stripPattern));
            std::getline(file, prefix);
            strncpy(barWin->m_prefix, prefix.c_str(), sizeof(barWin->m_prefix));
            std::getline(file, suffix);
            strncpy(barWin->m_suffix, suffix.c_str(), sizeof(barWin->m_suffix));

            // Thresholds
            file >> barWin->m_useThresholds;
            file >> barWin->m_threshLL;
            file >> barWin->m_threshL;
            file >> barWin->m_threshH;
            file >> barWin->m_threshHH;

            auto loadBarConf = [&](BarThresholdConfig& conf) {
                file >> conf.enabled;
                file >> conf.color[0] >> conf.color[1] >> conf.color[2] >> conf.color[3];
            };

            loadBarConf(barWin->m_confLL);
            loadBarConf(barWin->m_confL);
            loadBarConf(barWin->m_confNormal);
            loadBarConf(barWin->m_confH);
            loadBarConf(barWin->m_confHH);
            std::getline(file, dummy); // consume newline

            // Safe check for backward compatibility of Bar gradient config
            bool useGradient = false;
            if (file >> useGradient) {
                barWin->m_useGradient = useGradient;
                file >> barWin->m_gradMinColor[0] >> barWin->m_gradMinColor[1] >> barWin->m_gradMinColor[2] >> barWin->m_gradMinColor[3];
                file >> barWin->m_gradMaxColor[0] >> barWin->m_gradMaxColor[1] >> barWin->m_gradMaxColor[2] >> barWin->m_gradMaxColor[3];
                std::getline(file, dummy); // consume newline

                // Safe check for new unified color mode
                int colorBarMode = 0;
                if (file >> colorBarMode) {
                    barWin->m_colorBarMode = colorBarMode;
                    file >> barWin->m_gradMinVal;
                    file >> barWin->m_gradMaxVal;
                    std::getline(file, dummy);
                }
            }

            windows.emplace_back(std::move(barWin));
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
        } else if (type == "Matrix") {
            auto matWin = std::make_unique<Window::Matrix>(title);

            file >> matWin->m_colorMode;
            file >> matWin->m_minVal;
            file >> matWin->m_maxVal;
            file >> matWin->m_minColor[0] >> matWin->m_minColor[1] >> matWin->m_minColor[2] >> matWin->m_minColor[3];
            file >> matWin->m_maxColor[0] >> matWin->m_maxColor[1] >> matWin->m_maxColor[2] >> matWin->m_maxColor[3];
            file >> matWin->m_threshLL;
            file >> matWin->m_threshL;
            file >> matWin->m_threshH;
            file >> matWin->m_threshHH;

            auto loadMatConf = [&](ColorThresholdConfig& conf) {
                file >> conf.enabled;
                file >> conf.bg[0] >> conf.bg[1] >> conf.bg[2] >> conf.bg[3];
                file >> conf.fg[0] >> conf.fg[1] >> conf.fg[2] >> conf.fg[3];
            };
            loadMatConf(matWin->m_confLL);
            loadMatConf(matWin->m_confL);
            loadMatConf(matWin->m_confNormal);
            loadMatConf(matWin->m_confH);
            loadMatConf(matWin->m_confHH);

            size_t numCols = 0;
            file >> numCols;
            std::getline(file, dummy); // consume newline

            matWin->m_columns.clear();
            for (size_t ci = 0; ci < numCols; ci++) {
                MatrixColumn col;
                std::getline(file, col.fileType);
                std::getline(file, col.archiveName);
                size_t numVars = 0;
                file >> numVars;
                std::getline(file, dummy);
                col.variables.clear();
                for (size_t vi = 0; vi < numVars; vi++) {
                    std::string var;
                    std::getline(file, var);
                    col.variables.push_back(var);
                }
                matWin->m_columns.push_back(col);
            }

            windows.emplace_back(std::move(matWin));
        }
    }
}