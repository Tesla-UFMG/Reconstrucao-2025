#include "WindowManager.hpp"
#include "Log.hpp"

#include "ui/windows/iWindow.hpp"
#include "ui/windows/w_About.hpp"
#include "ui/windows/w_DataPicker.hpp"
#include "ui/windows/w_Demo.hpp"
#include "ui/windows/w_HomePage.hpp"
#include "ui/windows/w_Pedal.hpp"
#include "ui/windows/w_Playback.hpp"
#include "ui/windows/w_Reconstruction.hpp"
#include "ui/windows/w_Statistics.hpp"
#include "ui/windows/w_Telemetry.hpp"
#include "ui/windows/w_Terminal.hpp"
#include "ui/windows/w_Updates.hpp"
#include "ui/windows/w_WheelControl.hpp"

WindowManager& WindowManager::getInstance() {
    static WindowManager instance;
    return instance;
}

WindowManager::WindowManager() { LOG("TRACE", "Window Manager iniciado com sucesso."); }

WindowManager::~WindowManager() { LOG("TRACE", "Window Manager encerrado."); }

void WindowManager::init(SDL_Renderer* renderer) {
    this->m_renderer = renderer;
    this->setup();

    // Abre a janela de atualizações se o arquivo oculto local não existir
    if (!std::filesystem::exists(std::filesystem::current_path() / ".tesla_updates_seen")) {
        this->showUpdates = true;
    } else {
        this->showUpdates = false;
    }
}

void WindowManager::cleanup() {
    windows.clear();
    home.reset();
    m_reconstructionWindow = nullptr;
    m_aboutWindow          = nullptr;
    m_updatesWindow        = nullptr;
    m_playbackWindow       = nullptr;
    m_videoWindow          = nullptr;
    m_renderer             = nullptr;
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

    saveWindowCustomStates(filepath.string() + ".state");
}

void WindowManager::loadWindowVisibility(const std::filesystem::path& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        LOG("WARN", "Não foi possível carregar a visibilidade '" + filepath.string() + "'.");
        return;
    }

    file.read(reinterpret_cast<char*>(&visibility), sizeof(VisibilityFlags));
    LOG("INFO", "Visibilidade '" + filepath.string() + "' carregada com sucesso.");

    loadWindowCustomStates(filepath.string() + ".state");
}

void WindowManager::setup() {
    // --- Criação das Janelas "Reproduzíveis" ---

    // 1. Janela de Reconstrução
    auto temp_reconstruction_ptr = std::make_unique<Window::Reconstruction>(&visibility.showReconstruction);
    m_reconstructionWindow       = temp_reconstruction_ptr.get();

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
    m_aboutWindow       = temp_about_ptr.get();
    windows.emplace_back(std::move(temp_about_ptr));

    auto temp_updates_ptr = std::make_unique<Window::Updates>(&showUpdates);
    m_updatesWindow       = temp_updates_ptr.get();
    windows.emplace_back(std::move(temp_updates_ptr));

    windows.emplace_back(std::make_unique<Window::DataPicker>(&visibility.showDataPicker));
    windows.emplace_back(std::make_unique<Window::Terminal>(&visibility.showLog));
    windows.emplace_back(std::make_unique<Window::ImGuiDemo>(&visibility.showImGuiDemo));
    windows.emplace_back(std::make_unique<Window::ImPlotDemo>(&visibility.showImPlotDemo));
    windows.emplace_back(std::make_unique<Window::ImPlot3dDemo>(&visibility.showImPlot3dDemo));
    // windows.emplace_back(std::make_unique<Window::WheelControl>(&visibility.showWheelControl));
    windows.emplace_back(std::make_unique<Window::Telemetry>(&visibility.showTelemetry));
    windows.emplace_back(std::make_unique<Window::Warning>(&visibility.showWarnings));

    auto temp_playback_ptr = std::make_unique<Window::Playback>(&visibility.showPlayback);
    m_playbackWindow       = temp_playback_ptr.get();
    windows.emplace_back(std::move(temp_playback_ptr));

    auto temp_video_ptr = std::make_unique<Window::Video>(&visibility.showVideo, m_renderer);
    m_videoWindow       = temp_video_ptr.get();
    windows.emplace_back(std::move(temp_video_ptr));
}

void WindowManager::homePage() {
    MenuBar::render();
    if (m_aboutWindow) {
        m_aboutWindow->render();
    }
    if (m_updatesWindow) {
        m_updatesWindow->render();
    }
    home->render(); // Home page
}

void WindowManager::mainPage() {
    MenuBar::render();

    // Remove closed dynamic windows
    windows.erase(std::remove_if(windows.begin(), windows.end(),
                                 [](const std::unique_ptr<IWindow>& window) {
                                     return window->isDynamic() && !window->getIsOpen();
                                 }),
                  windows.end());

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
                std::string wTitle  = numWin->getTitle();
                size_t      hashPos = wTitle.find('#');
                if (hashPos != std::string::npos) {
                    try {
                        int idx = std::stoi(wTitle.substr(hashPos + 1));
                        if (idx > maxIdx) {
                            maxIdx = idx;
                        }
                    } catch (...) {
                    }
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
                std::string wTitle  = graphWin->getTitle();
                size_t      hashPos = wTitle.find('#');
                if (hashPos != std::string::npos) {
                    try {
                        int idx = std::stoi(wTitle.substr(hashPos + 1));
                        if (idx > maxIdx) {
                            maxIdx = idx;
                        }
                    } catch (...) {
                    }
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
                std::string wTitle  = barWin->getTitle();
                size_t      hashPos = wTitle.find('#');
                if (hashPos != std::string::npos) {
                    try {
                        int idx = std::stoi(wTitle.substr(hashPos + 1));
                        if (idx > maxIdx) {
                            maxIdx = idx;
                        }
                    } catch (...) {
                    }
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
                std::string wTitle  = matWin->getTitle();
                size_t      hashPos = wTitle.find('#');
                if (hashPos != std::string::npos) {
                    try {
                        int idx = std::stoi(wTitle.substr(hashPos + 1));
                        if (idx > maxIdx) {
                            maxIdx = idx;
                        }
                    } catch (...) {
                    }
                }
            }
        }
    }
    std::string windowTitle = "Matriz #" + std::to_string(maxIdx + 1);

    auto matWindow = std::make_unique<Window::Matrix>(windowTitle);
    windows.emplace_back(std::move(matWindow));

    LOG("INFO", "Criada nova janela de matriz: " + windowTitle);
}

void WindowManager::createTabelaWindow() {
    int maxIdx = 0;
    for (const auto& w : windows) {
        if (w->isDynamic() && w->getDynamicType() == "Tabela") {
            auto* tabWin = dynamic_cast<Window::Statistics*>(w.get());
            if (tabWin) {
                std::string wTitle  = tabWin->getTitle();
                size_t      hashPos = wTitle.find('#');
                if (hashPos != std::string::npos) {
                    try {
                        int idx = std::stoi(wTitle.substr(hashPos + 1));
                        if (idx > maxIdx) {
                            maxIdx = idx;
                        }
                    } catch (...) {
                    }
                }
            }
        }
    }
    std::string windowTitle = "Tabela #" + std::to_string(maxIdx + 1);

    auto tabelaWindow = std::make_unique<Window::Statistics>(windowTitle);
    windows.emplace_back(std::move(tabelaWindow));

    LOG("INFO", "Criada nova janela de tabela: " + windowTitle);
}

void WindowManager::saveWindowCustomStates(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file) {
        LOG("WARN", "Não foi possível salvar os estados customizados em '" + filepath + "'.");
        return;
    }

    // 1. Encontrar ponteiros para janelas estáticas
    Window::Reconstruction* reconWin = nullptr;
    Window::Pedal*          pedalWin = nullptr;
    Window::WheelControl*   wheelWin = nullptr;

    for (const auto& w : windows) {
        if (auto* r = dynamic_cast<Window::Reconstruction*>(w.get())) {
            reconWin = r;
        } else if (auto* p = dynamic_cast<Window::Pedal*>(w.get())) {
            pedalWin = p;
        } else if (auto* wh = dynamic_cast<Window::WheelControl*>(w.get())) {
            wheelWin = wh;
        }
    }

    // Salvando estado da janela de Reconstrução
    if (reconWin) {
        file << "RECONSTRUCTION_STATE\n";
        file << reconWin->m_panX << " " << reconWin->m_panY << " " << reconWin->m_zoomScale << "\n";
        file << reconWin->m_selectedLatFileType << "\n";
        file << reconWin->m_selectedLatFileName << "\n";
        file << reconWin->m_selectedLatCol << "\n";
        file << reconWin->m_selectedLonFileType << "\n";
        file << reconWin->m_selectedLonFileName << "\n";
        file << reconWin->m_selectedLonCol << "\n";
        file << static_cast<int>(reconWin->m_alignmentMode) << "\n";
        file << static_cast<int>(reconWin->m_colorAlignmentMode) << "\n";
        file << reconWin->m_selectedColorCol << "\n";
        file << reconWin->m_selectedColorFileName << "\n";
        file << reconWin->m_selectedColorFileType << "\n";
        file << reconWin->m_gradMinVal << "\n";
        file << reconWin->m_gradMaxVal << "\n";
        file << reconWin->m_gradMinColor[0] << " " << reconWin->m_gradMinColor[1] << " " << reconWin->m_gradMinColor[2]
             << " " << reconWin->m_gradMinColor[3] << "\n";
        file << reconWin->m_gradMaxColor[0] << " " << reconWin->m_gradMaxColor[1] << " " << reconWin->m_gradMaxColor[2]
             << " " << reconWin->m_gradMaxColor[3] << "\n";

        file << reconWin->m_centerLat << " " << reconWin->m_centerLon << "\n";
        file << reconWin->m_trackOffsetLat << " " << reconWin->m_trackOffsetLon << "\n";
        file << reconWin->m_colorLine[0] << " " << reconWin->m_colorLine[1] << " " << reconWin->m_colorLine[2] << " "
             << reconWin->m_colorLine[3] << "\n";
        file << reconWin->m_colorPoint[0] << " " << reconWin->m_colorPoint[1] << " " << reconWin->m_colorPoint[2] << " "
             << reconWin->m_colorPoint[3] << "\n";
        file << reconWin->m_colorLastPoint[0] << " " << reconWin->m_colorLastPoint[1] << " "
             << reconWin->m_colorLastPoint[2] << " " << reconWin->m_colorLastPoint[3] << "\n";
        file << reconWin->m_textAnnotations.size() << "\n";
        for (const auto& ann : reconWin->m_textAnnotations) {
            file << ann.archiveName << "\n";
            file << ann.columnName << "\n";
        }
        file << reconWin->m_colorMode << "\n";
        file << reconWin->m_colormap << "\n";
        file << reconWin->m_reverseColormap << "\n";
        file << reconWin->m_followTheEnd << "\n";
        file << reconWin->m_rotateMap << "\n";
        file << reconWin->m_limitPoints << "\n";
        file << reconWin->m_numPointsToShow << "\n";
        file << (reconWin->m_currentMapName.empty() ? "EMPTY" : reconWin->m_currentMapName) << "\n";
    } else {
        file << "NO_RECONSTRUCTION_STATE\n";
    }

    // Salvando estado da janela de Pedais
    if (pedalWin) {
        file << "PEDAL_STATE\n";
        file << pedalWin->m_isPlaying << "\n";
        file << pedalWin->m_playbackSpeed << "\n";
        file << pedalWin->m_currentTime << "\n";
        file << pedalWin->m_stepSize << "\n";
        file << pedalWin->m_showPedalImages << "\n";
        file << pedalWin->m_throttleIndex << "\n";
        file << pedalWin->m_brakeIndex << "\n";
        file << pedalWin->m_dataList.size() << "\n";
        for (const auto& pd : pedalWin->m_dataList) {
            file << pd.column << "\n";
            file << pd.archive << "\n";
            file << pd.fileType << "\n";
            file << pd.maxValue << "\n";
        }
    } else {
        file << "NO_PEDAL_STATE\n";
    }

    // Salvando estado da janela de Volante
    if (wheelWin) {
        file << "WHEEL_STATE\n";
        file << wheelWin->m_isPlaying << "\n";
        file << wheelWin->m_playbackSpeed << "\n";
        file << wheelWin->m_currentTime << "\n";
        file << wheelWin->m_stepSize << "\n";
        file << wheelWin->m_dataIsDegrees << "\n";
        file << wheelWin->m_steerIndex << "\n";
        file << wheelWin->m_dataList.size() << "\n";
        for (const auto& wd : wheelWin->m_dataList) {
            file << wd.column << "\n";
            file << wd.archive << "\n";
            file << wd.fileType << "\n";
            file << wd.maxValue << "\n";
            file << wd.minValue << "\n";
        }
    } else {
        file << "NO_WHEEL_STATE\n";
    }

    // Salvando estado da janela de Vídeo
    if (m_videoWindow) {
        file << "VIDEO_STATE\n";
        file << m_videoWindow->getVolume() << "\n";
        file << (m_videoWindow->getLoadedVideo().empty() ? "EMPTY" : m_videoWindow->getLoadedVideo()) << "\n";
    } else {
        file << "NO_VIDEO_STATE\n";
    }

    // Salvando estado da janela de Playback
    if (m_playbackWindow) {
        file << "PLAYBACK_STATE\n";
        file << m_playbackWindow->videoLengthMs << "\n";
        file << m_playbackWindow->videoBlockStart << "\n";
        file << m_playbackWindow->csvBlockStart << "\n";
        file << m_playbackWindow->csvBlockEnd << "\n";

        file << "PLAYBACK_FILES_STATE\n";
        file << (m_playbackWindow->selectedFileType.empty() ? "EMPTY" : m_playbackWindow->selectedFileType) << "\n";
        file << (m_playbackWindow->selectedFileName.empty() ? "EMPTY" : m_playbackWindow->selectedFileName) << "\n";
        file << (m_playbackWindow->selectedTimestampCol.empty() ? "EMPTY" : m_playbackWindow->selectedTimestampCol)
             << "\n";
        file << (m_playbackWindow->loadedVideoName.empty() ? "EMPTY" : m_playbackWindow->loadedVideoName) << "\n";

        file << "PLAYBACK_CURSOR_STATE\n";
        file << m_playbackWindow->globalTime << "\n";

        file << "PLAYBACK_LOCK_STATE\n";
        file << m_playbackWindow->tracksLocked << "\n";
    } else {
        file << "NO_PLAYBACK_STATE\n";
        file << "NO_PLAYBACK_FILES_STATE\n";
        file << "NO_PLAYBACK_CURSOR_STATE\n";
        file << "NO_PLAYBACK_LOCK_STATE\n";
    }

    // 2. Salvando janelas dinâmicas
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
                    file << numWin->m_prefix << "\n";
                    file << numWin->m_suffix << "\n";
                    file << numWin->m_useFormula << "\n";
                    file << numWin->m_multiplier << "\n";
                    file << numWin->m_offset << "\n";
                    file << numWin->m_useTranslation << "\n";
                    file << numWin->m_translationRules.size() << "\n";
                    for (const auto& rule : numWin->m_translationRules) {
                        file << rule.value << "\n";
                        file << rule.text << "\n";
                    }
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
                    file << numWin->m_gradMinColor[0] << " " << numWin->m_gradMinColor[1] << " "
                         << numWin->m_gradMinColor[2] << " " << numWin->m_gradMinColor[3] << "\n";
                    file << numWin->m_gradMaxColor[0] << " " << numWin->m_gradMaxColor[1] << " "
                         << numWin->m_gradMaxColor[2] << " " << numWin->m_gradMaxColor[3] << "\n";
                    file << numWin->m_statModeAll << "\n";
                    file << numWin->m_customLabel << "\n";
                    file << numWin->m_colormap << "\n";
                    file << numWin->m_reverseColormap << "\n";
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
                    file << barWin->m_orientation << "\n";
                    file << barWin->m_useManualLimits << "\n";
                    file << barWin->m_minVal << "\n";
                    file << barWin->m_maxVal << "\n";
                    file << barWin->m_barColor[0] << " " << barWin->m_barColor[1] << " " << barWin->m_barColor[2] << " "
                         << barWin->m_barColor[3] << "\n";
                    file << barWin->m_bgColor[0] << " " << barWin->m_bgColor[1] << " " << barWin->m_bgColor[2] << " "
                         << barWin->m_bgColor[3] << "\n";
                    file << barWin->m_fgColor[0] << " " << barWin->m_fgColor[1] << " " << barWin->m_fgColor[2] << " "
                         << barWin->m_fgColor[3] << "\n";
                    file << barWin->m_fontScale << "\n";
                    file << barWin->m_showPercentage << "\n";
                    file << barWin->m_showValue << "\n";
                    file << barWin->m_showColumnName << "\n";
                    file << barWin->m_customLabel << "\n";
                    file << barWin->m_prefix << "\n";
                    file << barWin->m_suffix << "\n";
                    file << barWin->m_useFormula << "\n";
                    file << barWin->m_multiplier << "\n";
                    file << barWin->m_offset << "\n";
                    file << barWin->m_useThresholds << "\n";
                    file << barWin->m_threshLL << "\n";
                    file << barWin->m_threshL << "\n";
                    file << barWin->m_threshH << "\n";
                    file << barWin->m_threshHH << "\n";

                    auto saveBarConf = [&](const BarThresholdConfig& conf) {
                        file << conf.enabled << "\n";
                        file << conf.color[0] << " " << conf.color[1] << " " << conf.color[2] << " " << conf.color[3]
                             << "\n";
                    };

                    saveBarConf(barWin->m_confLL);
                    saveBarConf(barWin->m_confL);
                    saveBarConf(barWin->m_confNormal);
                    saveBarConf(barWin->m_confH);
                    saveBarConf(barWin->m_confHH);

                    file << barWin->m_useGradient << "\n";
                    file << barWin->m_gradMinColor[0] << " " << barWin->m_gradMinColor[1] << " "
                         << barWin->m_gradMinColor[2] << " " << barWin->m_gradMinColor[3] << "\n";
                    file << barWin->m_gradMaxColor[0] << " " << barWin->m_gradMaxColor[1] << " "
                         << barWin->m_gradMaxColor[2] << " " << barWin->m_gradMaxColor[3] << "\n";
                    file << barWin->m_colorBarMode << "\n";
                    file << barWin->m_gradMinVal << "\n";
                    file << barWin->m_gradMaxVal << "\n";
                    file << barWin->m_colormap << "\n";
                    file << barWin->m_reverseColormap << "\n";
                }
            } else if (w->getDynamicType() == "Graph") {
                auto graphWin = dynamic_cast<Window::Graph*>(w.get());
                if (graphWin) {
                    auto& graph  = graphWin->getGraph();
                    auto& config = graph.config;
                    file << graphWin->getTitle() << "\n";
                    file << static_cast<int>(config.type) << "\n";
                    file << config.showXAxis << "\n";
                    file << config.showYAxis << "\n";
                    file << config.numPoints << "\n";
                    file << config.plotHeight << "\n";
                    file << config.followTheEnd << "\n";
                    file << config.autoFit << "\n";
                    file << config.showValueOnYAxis << "\n";
                    file << config.showCursorOnYAxis << "\n";
                    file << config.xColumn << "\n";
                    file << "XYALIGN:" << static_cast<int>(config.xyAlignmentMode) << "\n";

                    file << graph.textAnnotations.size() << "\n";
                    for (const auto& ann : graph.textAnnotations) {
                        file << ann.archiveName << "\n";
                        file << ann.columnName << "\n";
                    }

                    file << graph.data.size() << "\n";
                    for (const auto& col : graph.data) {
                        file << col.columnName << "\n";
                        file << col.fileName << "\n";
                        file << col.fileType << "\n";
                        file << col.multiplier << "\n";
                    }
                }
            } else if (w->getDynamicType() == "Matrix") {
                auto* matWin = dynamic_cast<Window::Matrix*>(w.get());
                if (matWin) {
                    file << matWin->getTitle() << "\n";
                    file << matWin->m_colorMode << "\n";
                    file << matWin->m_minVal << "\n";
                    file << matWin->m_maxVal << "\n";
                    file << matWin->m_minColor[0] << " " << matWin->m_minColor[1] << " " << matWin->m_minColor[2] << " "
                         << matWin->m_minColor[3] << "\n";
                    file << matWin->m_maxColor[0] << " " << matWin->m_maxColor[1] << " " << matWin->m_maxColor[2] << " "
                         << matWin->m_maxColor[3] << "\n";
                    file << matWin->m_threshLL << "\n";
                    file << matWin->m_threshL << "\n";
                    file << matWin->m_threshH << "\n";
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

                    file << matWin->m_columns.size() << "\n";
                    for (const auto& col : matWin->m_columns) {
                        if (col.isArtificial) {
                            file << "Artificial\n";
                            file << col.customName << "\n";
                            file << col.customCells.size() << "\n";
                            for (const auto& kv : col.customCells) {
                                file << kv.first << "\n";
                                file << kv.second.fileType << "\n";
                                file << kv.second.archiveName << "\n";
                                file << kv.second.columnName << "\n";
                            }
                        } else {
                            file << col.fileType << "\n";
                            file << col.archiveName << "\n";
                            file << matWin->m_rowVariables.size() << "\n";
                            for (const auto& v : matWin->m_rowVariables) {
                                file << v << "\n";
                            }
                        }
                    }

                    file << matWin->m_fontScale << "\n";
                    file << matWin->m_showVariableName << "\n";
                    file << matWin->m_suffix << "\n";
                    file << matWin->m_useFormula << "\n";
                    file << matWin->m_multiplier << "\n";
                    file << matWin->m_offset << "\n";
                    file << matWin->m_useTranslation << "\n";
                    file << matWin->m_translationRules.size() << "\n";
                    for (const auto& r : matWin->m_translationRules) {
                        file << r.value << "\n";
                        file << r.text << "\n";
                    }

                    // Save rowVariables explicitly (to preserve empty artificial rows)
                    file << matWin->m_rowVariables.size() << "\n";
                    for (const auto& v : matWin->m_rowVariables) {
                        file << v << "\n";
                    }
                    file << matWin->m_prefix << "\n";
                    file << matWin->m_colormap << "\n";
                    file << matWin->m_reverseColormap << "\n";
                    file << matWin->m_specificRules.size() << "\n";
                    for (const auto& rule : matWin->m_specificRules) {
                        file << rule.value << "\n";
                        file << rule.bg[0] << " " << rule.bg[1] << " " << rule.bg[2] << " " << rule.bg[3] << "\n";
                        file << rule.fg[0] << " " << rule.fg[1] << " " << rule.fg[2] << " " << rule.fg[3] << "\n";
                    }
                }
            } else if (w->getDynamicType() == "Tabela") {
                auto* tabWin = dynamic_cast<Window::Statistics*>(w.get());
                if (tabWin) {
                    file << tabWin->getTitle() << "\n";
                    file << tabWin->getMetrics().size() << "\n";
                    for (const auto& metric : tabWin->getMetrics()) {
                        file << metric.fileType << "\n";
                        file << metric.fileName << "\n";
                        file << metric.display_name << "\n";
                    }
                }
            }
        }
    }
}

void WindowManager::loadWindowCustomStates(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file) {
        return;
    }

    // 1. Encontrar ponteiros para janelas estáticas
    Window::Reconstruction* reconWin = nullptr;
    Window::Pedal*          pedalWin = nullptr;
    Window::WheelControl*   wheelWin = nullptr;

    for (const auto& w : windows) {
        if (auto* r = dynamic_cast<Window::Reconstruction*>(w.get())) {
            reconWin = r;
        } else if (auto* p = dynamic_cast<Window::Pedal*>(w.get())) {
            pedalWin = p;
        } else if (auto* wh = dynamic_cast<Window::WheelControl*>(w.get())) {
            wheelWin = wh;
        }
    }

    std::string line, dummy;

    auto seekToBlock = [&](const std::string& blockName) {
        file.clear();
        std::string l;
        // Se a linha atual já for o que procuramos (pode ocorrer se alinhado corretamente), não precisamos avançar.
        // Como o getline avança, primeiro verificamos a próxima linha real.
        while (std::getline(file, l)) {
            if (l.find(blockName) == 0 || l.find("NO_" + blockName) == 0) {
                line = l;
                return true;
            }
        }
        return false;
    };

    // Restaurando estado da janela de Reconstrução
    if (seekToBlock("RECONSTRUCTION_STATE")) {
        if (line == "RECONSTRUCTION_STATE" && reconWin) {
            file >> reconWin->m_panX >> reconWin->m_panY >> reconWin->m_zoomScale;
            std::getline(file, dummy); // consume newline
            std::string str1, str2, str3, str4;
            std::getline(file, str1);
            std::getline(file, str2);
            std::getline(file, str3);
            std::getline(file, str4);

            bool        isOldFormat = true;
            std::string line5;

            if (str4 == "CSV" || str4 == "Telemetry") {
                isOldFormat = false;
                std::getline(file, line5);
            } else if (str4.empty()) {
                std::getline(file, line5);
                if (line5.empty()) {
                    isOldFormat = false;
                }
            }

            int alignVal = 0, colorAlignVal = 0;
            if (isOldFormat) {
                reconWin->m_selectedLatFileType = str1;
                reconWin->m_selectedLatFileName = str2;
                reconWin->m_selectedLatCol      = str3;
                reconWin->m_selectedLonFileType = str1;
                reconWin->m_selectedLonFileName = str2;
                reconWin->m_selectedLonCol      = str4;

                if (!line5.empty()) {
                    try {
                        alignVal = std::stoi(line5);
                    } catch (...) {
                        alignVal = 2; // Default fallback
                    }
                } else {
                    file >> alignVal;
                }
                reconWin->m_alignmentMode = static_cast<XYAlignmentMode>(alignVal);
                file >> colorAlignVal;
                reconWin->m_colorAlignmentMode = static_cast<XYAlignmentMode>(colorAlignVal);
                std::getline(file, dummy); // consume newline
            } else {
                reconWin->m_selectedLatFileType = str1;
                reconWin->m_selectedLatFileName = str2;
                reconWin->m_selectedLatCol      = str3;
                reconWin->m_selectedLonFileType = str4;
                reconWin->m_selectedLonFileName = line5;
                std::getline(file, reconWin->m_selectedLonCol);

                file >> alignVal;
                reconWin->m_alignmentMode = static_cast<XYAlignmentMode>(alignVal);
                file >> colorAlignVal;
                reconWin->m_colorAlignmentMode = static_cast<XYAlignmentMode>(colorAlignVal);
                std::getline(file, dummy); // consume newline
            }
            std::getline(file, reconWin->m_selectedColorCol);
            std::getline(file, reconWin->m_selectedColorFileName);
            std::getline(file, reconWin->m_selectedColorFileType);
            file >> reconWin->m_gradMinVal;
            file >> reconWin->m_gradMaxVal;
            file >> reconWin->m_gradMinColor[0] >> reconWin->m_gradMinColor[1] >> reconWin->m_gradMinColor[2] >>
                reconWin->m_gradMinColor[3];
            file >> reconWin->m_gradMaxColor[0] >> reconWin->m_gradMaxColor[1] >> reconWin->m_gradMaxColor[2] >>
                reconWin->m_gradMaxColor[3];
            file >> reconWin->m_centerLat >> reconWin->m_centerLon;
            file >> reconWin->m_trackOffsetLat >> reconWin->m_trackOffsetLon;
            file >> reconWin->m_colorLine[0] >> reconWin->m_colorLine[1] >> reconWin->m_colorLine[2] >>
                reconWin->m_colorLine[3];
            file >> reconWin->m_colorPoint[0] >> reconWin->m_colorPoint[1] >> reconWin->m_colorPoint[2] >>
                reconWin->m_colorPoint[3];
            file >> reconWin->m_colorLastPoint[0] >> reconWin->m_colorLastPoint[1] >> reconWin->m_colorLastPoint[2] >>
                reconWin->m_colorLastPoint[3];
            size_t annSize = 0;
            file >> annSize;
            std::getline(file, dummy); // consume newline
            reconWin->m_textAnnotations.clear();
            for (size_t a = 0; a < annSize; ++a) {
                TrackTextAnnotation ann;
                std::getline(file, ann.archiveName);
                std::getline(file, ann.columnName);
                bool exists = DB::getInstance().columnExists("CSV", ann.archiveName, ann.columnName) ||
                              DB::getInstance().columnExists("Telemetry", ann.archiveName, ann.columnName) ||
                              DB::getInstance().columnExists("Text", ann.archiveName, ann.columnName);
                if (exists) {
                    reconWin->m_textAnnotations.push_back(ann);
                }
            }

            int colorMode = 1;
            if (file >> colorMode) {
                reconWin->m_colorMode = colorMode;
                int colormap          = 0;
                if (file >> colormap) {
                    reconWin->m_colormap = colormap;
                    bool rev             = false;
                    if (file >> rev) {
                        reconWin->m_reverseColormap = rev;

                        // New fields added later, wrap in safe read block
                        bool followEnd, rotateMap, limitPts;
                        int  numPts;
                        if (file >> followEnd) {
                            reconWin->m_followTheEnd = followEnd;
                            if (file >> rotateMap)
                                reconWin->m_rotateMap = rotateMap;
                            if (file >> limitPts)
                                reconWin->m_limitPoints = limitPts;
                            if (file >> numPts)
                                reconWin->m_numPointsToShow = numPts;
                        } else {
                            file.clear(); // Clear EOF flag if reading old file format
                        }

                        std::getline(file, dummy); // consume newline

                        std::streampos pos = file.tellg();
                        std::string    mapName;
                        if (std::getline(file, mapName)) {
                            if (mapName.find("_STATE") != std::string::npos) {
                                file.seekg(pos); // Rewind because it's the next block header, not a map name
                            } else if (mapName != "EMPTY" && !mapName.empty()) {
                                reconWin->setMap(mapName);
                            }
                        }
                    } else {
                        file.clear();
                    }
                } else {
                    file.clear();
                }
            } else {
                file.clear();
            }

            // Validar se latitude/longitude ainda existem
            if (!reconWin->m_selectedLatCol.empty() &&
                !DB::getInstance().columnExists(reconWin->m_selectedLatFileType, reconWin->m_selectedLatFileName,
                                                reconWin->m_selectedLatCol)) {
                reconWin->m_selectedLatCol.clear();
                reconWin->m_selectedLatFileName.clear();
            }
            if (!reconWin->m_selectedLonCol.empty() &&
                !DB::getInstance().columnExists(reconWin->m_selectedLonFileType, reconWin->m_selectedLonFileName,
                                                reconWin->m_selectedLonCol)) {
                reconWin->m_selectedLonCol.clear();
                reconWin->m_selectedLonFileName.clear();
            }

            // Validar se coluna de gradiente ainda existe
            if (!reconWin->m_selectedColorCol.empty() &&
                !DB::getInstance().columnExists(reconWin->m_selectedColorFileType, reconWin->m_selectedColorFileName,
                                                reconWin->m_selectedColorCol)) {
                reconWin->m_selectedColorCol.clear();
                reconWin->m_selectedColorFileName.clear();
                reconWin->m_selectedColorFileType.clear();
            }
        } else if (line == "NO_RECONSTRUCTION_STATE") {
            // Nenhuma ação necessária
        }
    }

    // Restaurando estado da janela de Pedais
    if (seekToBlock("PEDAL_STATE")) {
        if (line == "PEDAL_STATE" && pedalWin) {
            file >> pedalWin->m_isPlaying;
            file >> pedalWin->m_playbackSpeed;
            file >> pedalWin->m_currentTime;
            file >> pedalWin->m_stepSize;
            file >> pedalWin->m_showPedalImages;
            file >> pedalWin->m_throttleIndex;
            file >> pedalWin->m_brakeIndex;
            size_t dataListSize = 0;
            file >> dataListSize;
            std::getline(file, dummy); // consume newline
            int origThrottle          = pedalWin->m_throttleIndex;
            int origBrake             = pedalWin->m_brakeIndex;
            pedalWin->m_throttleIndex = -1;
            pedalWin->m_brakeIndex    = -1;
            pedalWin->m_dataList.clear();
            for (size_t d = 0; d < dataListSize; ++d) {
                PedalData pd;
                std::getline(file, pd.column);
                std::getline(file, pd.archive);
                std::getline(file, pd.fileType);
                file >> pd.maxValue;
                std::getline(file, dummy); // consume newline

                if (DB::getInstance().columnExists(pd.fileType, pd.archive, pd.column)) {
                    if (pd.fileType == "CSV") {
                    } else if (pd.fileType == "Telemetry") {
                    }
                    pedalWin->m_dataList.push_back(pd);
                    int newIdx = static_cast<int>(pedalWin->m_dataList.size() - 1);
                    if (static_cast<int>(d) == origThrottle) {
                        pedalWin->m_throttleIndex = newIdx;
                    }
                    if (static_cast<int>(d) == origBrake) {
                        pedalWin->m_brakeIndex = newIdx;
                    }
                }
            }

        } else if (line == "NO_PEDAL_STATE") {
            // Nenhuma ação necessária
        }
    }

    // Restaurando estado da janela de Volante
    if (seekToBlock("WHEEL_STATE")) {
        if (line == "WHEEL_STATE" && wheelWin) {
            file >> wheelWin->m_isPlaying;
            file >> wheelWin->m_playbackSpeed;
            file >> wheelWin->m_currentTime;
            file >> wheelWin->m_stepSize;
            file >> wheelWin->m_dataIsDegrees;
            file >> wheelWin->m_steerIndex;
            size_t dataListSize = 0;
            file >> dataListSize;
            std::getline(file, dummy); // consume newline
            int origSteer          = wheelWin->m_steerIndex;
            wheelWin->m_steerIndex = -1;
            wheelWin->m_dataList.clear();
            for (size_t d = 0; d < dataListSize; ++d) {
                WheelData wd;
                std::getline(file, wd.column);
                std::getline(file, wd.archive);
                std::getline(file, wd.fileType);
                file >> wd.maxValue;
                file >> wd.minValue;
                std::getline(file, dummy); // consume newline

                if (DB::getInstance().columnExists(wd.fileType, wd.archive, wd.column)) {
                    if (wd.fileType == "CSV") {
                    } else if (wd.fileType == "Telemetry") {
                    }
                    wheelWin->m_dataList.push_back(wd);
                    int newIdx = static_cast<int>(wheelWin->m_dataList.size() - 1);
                    if (static_cast<int>(d) == origSteer) {
                        wheelWin->m_steerIndex = newIdx;
                    }
                }
            }
        } else if (line == "NO_WHEEL_STATE") {
            // Nenhuma ação necessária
        }
    }

    // Restaurando estado da janela de Vídeo
    if (seekToBlock("VIDEO_STATE")) {
        if (line == "VIDEO_STATE" && m_videoWindow) {
            float vol = 100.0f;
            file >> vol;
            std::getline(file, dummy); // consume newline
            std::string path;
            std::getline(file, path);
            m_videoWindow->setVolume(vol);
            if (path != "EMPTY" && !path.empty()) {
                m_videoWindow->setLoadedVideo(path);
            }
        }
    }

    // Restaurando estado da janela de Playback
    if (seekToBlock("PLAYBACK_STATE")) {
        if (line == "PLAYBACK_STATE" && m_playbackWindow) {
            file >> m_playbackWindow->videoLengthMs;
            file >> m_playbackWindow->videoBlockStart;
            file >> m_playbackWindow->csvBlockStart;
            file >> m_playbackWindow->csvBlockEnd;
            std::getline(file, dummy); // consume newline
        }
    }

    // Tentamos ler os arquivos do playback se existirem (pode não existir em layouts antigos)
    std::streampos beforeFiles = file.tellg();
    if (seekToBlock("PLAYBACK_FILES_STATE")) {
        if (line == "PLAYBACK_FILES_STATE" && m_playbackWindow) {
            std::string temp;
            std::getline(file, temp);
            m_playbackWindow->selectedFileType = (temp == "EMPTY" ? "" : temp);
            std::getline(file, temp);
            m_playbackWindow->selectedFileName = (temp == "EMPTY" ? "" : temp);
            std::getline(file, temp);
            m_playbackWindow->selectedTimestampCol = (temp == "EMPTY" ? "" : temp);
            std::getline(file, temp);
            m_playbackWindow->loadedVideoName = (temp == "EMPTY" ? "" : temp);

            // Re-populate timestamp data and bounds based on loaded file/column
            m_playbackWindow->refreshData();
        }
    } else {
        // Se não achou PLAYBACK_FILES_STATE, volta o ponteiro para continuarmos procurando as janelas dinâmicas
        file.clear();
        file.seekg(beforeFiles);
    }

    // Tentamos ler o cursor do playback
    std::streampos beforeCursor = file.tellg();
    if (seekToBlock("PLAYBACK_CURSOR_STATE")) {
        if (line == "PLAYBACK_CURSOR_STATE" && m_playbackWindow) {
            file >> m_playbackWindow->globalTime;
            std::getline(file, dummy); // Consume newline
        }
    } else {
        file.clear();
        file.seekg(beforeCursor);
    }
    
    // Tentamos ler o estado do cadeado do playback
    std::streampos beforeLock = file.tellg();
    if (seekToBlock("PLAYBACK_LOCK_STATE")) {
        if (line == "PLAYBACK_LOCK_STATE" && m_playbackWindow) {
            file >> m_playbackWindow->tracksLocked;
            std::getline(file, dummy); // Consume newline
        }
    } else {
        file.clear();
        file.seekg(beforeLock);
    }

    // 2. Remove todas as janelas dinâmicas existentes
    windows.erase(std::remove_if(windows.begin(), windows.end(),
                                 [](const std::unique_ptr<IWindow>& w) { return w->isDynamic(); }),
                  windows.end());

    int count = 0;
    if (!(file >> count))
        return;
    std::getline(file, dummy); // Consome o newline

    for (int i = 0; i < count; i++) {
        std::string lineType;
        do {
            if (!std::getline(file, lineType))
                break;
        } while (lineType.empty());
        if (lineType.empty())
            break;

        std::string type = "Numeric";
        std::string title;
        if (lineType == "Numeric" || lineType == "Graph" || lineType == "Bar" || lineType == "Matrix" ||
            lineType == "Tabela") {
            type = lineType;
            if (!std::getline(file, title))
                break;
        } else {
            title = lineType;
        }

        if (title.empty()) {
            title = "Janela Recuperada " + std::to_string(i);
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
                if (DB::getInstance().columnExists(fileType, archive, column)) {
                    numWin->addColumn(fileType, archive, column);
                }
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
                double      rVal = 0.0;
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
                file >> numWin->m_gradMinColor[0] >> numWin->m_gradMinColor[1] >> numWin->m_gradMinColor[2] >>
                    numWin->m_gradMinColor[3];
                file >> numWin->m_gradMaxColor[0] >> numWin->m_gradMaxColor[1] >> numWin->m_gradMaxColor[2] >>
                    numWin->m_gradMaxColor[3];
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
                int colormap = 0;
                if (file >> colormap) {
                    numWin->m_colormap = colormap;
                    bool rev           = false;
                    if (file >> rev) {
                        numWin->m_reverseColormap = rev;
                        std::getline(file, dummy);
                    } else {
                        file.clear();
                    }
                } else {
                    file.clear();
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
                if (DB::getInstance().columnExists(fileType, archive, column)) {
                    barWin->addColumn(fileType, archive, column);
                }
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

            std::string customLabel, prefix, suffix;
            std::getline(file, customLabel);
            strncpy(barWin->m_customLabel, customLabel.c_str(), sizeof(barWin->m_customLabel));
            std::getline(file, prefix);
            strncpy(barWin->m_prefix, prefix.c_str(), sizeof(barWin->m_prefix));
            std::getline(file, suffix);
            strncpy(barWin->m_suffix, suffix.c_str(), sizeof(barWin->m_suffix));

            file >> barWin->m_useFormula;
            file >> barWin->m_multiplier;
            file >> barWin->m_offset;

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
                file >> barWin->m_gradMinColor[0] >> barWin->m_gradMinColor[1] >> barWin->m_gradMinColor[2] >>
                    barWin->m_gradMinColor[3];
                file >> barWin->m_gradMaxColor[0] >> barWin->m_gradMaxColor[1] >> barWin->m_gradMaxColor[2] >>
                    barWin->m_gradMaxColor[3];
                std::getline(file, dummy); // consume newline

                // Safe check for new unified color mode
                int colorBarMode = 0;
                if (file >> colorBarMode) {
                    barWin->m_colorBarMode = colorBarMode;
                    file >> barWin->m_gradMinVal;
                    file >> barWin->m_gradMaxVal;

                    int colormap = 0;
                    if (file >> colormap) {
                        barWin->m_colormap = colormap;
                        bool rev           = false;
                        if (file >> rev) {
                            barWin->m_reverseColormap = rev;
                            std::getline(file, dummy);
                        } else {
                            file.clear();
                        }
                    } else {
                        file.clear();
                    }
                } else {
                    file.clear();
                }
            } else {
                file.clear();
            }

            windows.emplace_back(std::move(barWin));
        } else if (type == "Graph") {
            auto  graphWin = std::make_unique<Window::Graph>(title);
            auto& graph    = graphWin->getGraph();
            auto& config   = graph.config;

            int typeVal = 0;
            file >> typeVal;
            config.type = static_cast<GraphType>(typeVal);

            file >> config.showXAxis;
            file >> config.showYAxis;
            file >> config.numPoints;
            file >> config.plotHeight;
            file >> config.followTheEnd;
            file >> config.autoFit;
            file >> config.showValueOnYAxis;
            file >> config.showCursorOnYAxis;
            std::getline(file, dummy); // Consome newline
            std::getline(file, config.xColumn);

            size_t dataSize        = 0;
            config.xyAlignmentMode = ALIGN_MIN_SIZE;
            std::string nextLine;
            if (std::getline(file, nextLine)) {
                if (nextLine.rfind("XYALIGN:", 0) == 0) {
                    try {
                        int alignVal           = std::stoi(nextLine.substr(8));
                        config.xyAlignmentMode = static_cast<XYAlignmentMode>(alignVal);
                    } catch (...) {
                    }

                    // Carregar anotações
                    size_t annSize = 0;
                    file >> annSize;
                    std::getline(file, dummy); // consume newline
                    graph.textAnnotations.clear();
                    for (size_t a = 0; a < annSize; ++a) {
                        GraphTextAnnotation ann;
                        std::getline(file, ann.archiveName);
                        std::getline(file, ann.columnName);
                        bool exists = DB::getInstance().columnExists("CSV", ann.archiveName, ann.columnName) ||
                                      DB::getInstance().columnExists("Telemetry", ann.archiveName, ann.columnName) ||
                                      DB::getInstance().columnExists("Text", ann.archiveName, ann.columnName);
                        if (exists) {
                            graph.textAnnotations.push_back(ann);
                        }
                    }

                    file >> dataSize;
                    std::getline(file, dummy); // Consome newline
                } else {
                    try {
                        dataSize = std::stoull(nextLine);
                    } catch (...) {
                    }
                }
            }

            for (size_t d = 0; d < dataSize; d++) {
                std::string colName, fileName, fileType;
                double      multiplier = 1.0;

                std::getline(file, colName);
                std::getline(file, fileName);
                std::getline(file, fileType);
                file >> multiplier;
                std::getline(file, dummy); // Consome newline

                if (DB::getInstance().columnExists(fileType, fileName, colName)) {
                    graphWin->addColumn(fileType, fileName, colName);
                    if (!graph.data.empty()) {
                        graph.data.back().multiplier = multiplier;
                    }
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
            matWin->m_rowVariables.clear();
            for (size_t ci = 0; ci < numCols; ci++) {
                MatrixColumn col;
                std::getline(file, col.fileType);
                if (col.fileType == "Artificial") {
                    col.isArtificial = true;
                    std::getline(file, col.customName);
                    col.archiveName = col.customName;
                    size_t numCells = 0;
                    file >> numCells;
                    std::getline(file, dummy);
                    for (size_t vi = 0; vi < numCells; vi++) {
                        std::string rowName;
                        std::getline(file, rowName);
                        MatrixCellSource src;
                        std::getline(file, src.fileType);
                        std::getline(file, src.archiveName);
                        std::getline(file, src.columnName);

                        if (DB::getInstance().columnExists(src.fileType, src.archiveName, src.columnName)) {
                            col.customCells[rowName] = src;
                            if (std::find(matWin->m_rowVariables.begin(), matWin->m_rowVariables.end(), rowName) ==
                                matWin->m_rowVariables.end()) {
                                matWin->m_rowVariables.push_back(rowName);
                            }
                        }
                    }
                    matWin->m_columns.push_back(col);
                } else {
                    std::getline(file, col.archiveName);
                    size_t numVars = 0;
                    file >> numVars;
                    std::getline(file, dummy);
                    for (size_t vi = 0; vi < numVars; vi++) {
                        std::string var;
                        std::getline(file, var);
                        if (DB::getInstance().columnExists(col.fileType, col.archiveName, var)) {
                            col.variables.push_back(var);
                            if (std::find(matWin->m_rowVariables.begin(), matWin->m_rowVariables.end(), var) ==
                                matWin->m_rowVariables.end()) {
                                matWin->m_rowVariables.push_back(var);
                            }
                        }
                    }
                    if (!col.variables.empty()) {
                        matWin->m_columns.push_back(col);
                    }
                }
            }

            float fontScale = 1.0f;
            if (file >> fontScale) {
                matWin->m_fontScale = fontScale;
                file >> matWin->m_showVariableName;
                std::getline(file, dummy); // consume newline

                // Backward compatible loading of new Matrix properties
                std::string suffix;
                if (std::getline(file, suffix)) {
                    strncpy(matWin->m_suffix, suffix.c_str(), sizeof(matWin->m_suffix));

                    if (file >> matWin->m_useFormula) {
                        file >> matWin->m_multiplier;
                        file >> matWin->m_offset;

                        if (file >> matWin->m_useTranslation) {
                            size_t transSize = 0;
                            if (file >> transSize) {
                                std::getline(file, dummy); // consume newline
                                matWin->m_translationRules.clear();
                                for (size_t r = 0; r < transSize; r++) {
                                    double      rVal = 0.0;
                                    std::string rTxt;
                                    file >> rVal;
                                    std::getline(file, dummy); // consume newline
                                    std::getline(file, rTxt);
                                    matWin->m_translationRules.push_back({rVal, rTxt});
                                }
                            }
                        }
                    }
                }

                // Read explicit row variables if they exist
                size_t extraRows = 0;
                if (file >> extraRows) {
                    std::getline(file, dummy); // consume newline
                    for (size_t r = 0; r < extraRows; r++) {
                        std::string rn;
                        std::getline(file, rn);
                        if (!rn.empty() && std::find(matWin->m_rowVariables.begin(), matWin->m_rowVariables.end(),
                                                     rn) == matWin->m_rowVariables.end()) {
                            matWin->m_rowVariables.push_back(rn);
                        }
                    }

                    std::string pfx;
                    if (std::getline(file, pfx)) {
                        strncpy(matWin->m_prefix, pfx.c_str(), sizeof(matWin->m_prefix));
                        int colormap = 0;
                        if (file >> colormap) {
                            matWin->m_colormap = colormap;
                            bool rev           = false;
                            if (file >> rev) {
                                matWin->m_reverseColormap = rev;
                                size_t spcSize            = 0;
                                if (file >> spcSize) {
                                    matWin->m_specificRules.resize(spcSize);
                                    for (size_t k = 0; k < spcSize; ++k) {
                                        file >> matWin->m_specificRules[k].value;
                                        file >> matWin->m_specificRules[k].bg[0] >> matWin->m_specificRules[k].bg[1] >>
                                            matWin->m_specificRules[k].bg[2] >> matWin->m_specificRules[k].bg[3];
                                        file >> matWin->m_specificRules[k].fg[0] >> matWin->m_specificRules[k].fg[1] >>
                                            matWin->m_specificRules[k].fg[2] >> matWin->m_specificRules[k].fg[3];
                                    }
                                    std::getline(file, dummy);
                                } else {
                                    file.clear();
                                }
                            } else {
                                file.clear();
                            }
                        } else {
                            file.clear();
                        }
                    }
                } else {
                    file.clear(); // Clear EOF flag if applicable
                }
            } else {
                file.clear();
            }

            windows.emplace_back(std::move(matWin));
        } else if (type == "Tabela") {
            size_t numMetrics = 0;
            file >> numMetrics;
            std::getline(file, dummy); // Consome o newline

            auto tabWin = std::make_unique<Window::Statistics>(title);
            for (size_t m = 0; m < numMetrics; ++m) {
                std::string fileType, fileName, columnName;
                std::getline(file, fileType);
                std::getline(file, fileName);
                std::getline(file, columnName);
                if (DB::getInstance().columnExists(fileType, fileName, columnName)) {
                    tabWin->addColumn(fileType, fileName, columnName);
                }
            }
            windows.emplace_back(std::move(tabWin));
        }
    }
}