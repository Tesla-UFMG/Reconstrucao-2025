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

void WindowManager::saveDynamicWindows(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file) {
        LOG("WARN", "Não foi possível salvar as janelas numéricas em '" + filepath + "'.");
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
        std::string title;
        std::getline(file, title);

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

        // Criamos a janela
        auto numWin = std::make_unique<Window::Numeric>(title);
        if (hasData) {
            numWin->addColumn(fileType, archive, column);
        }
        numWin->setMetricType(static_cast<MetricType>(metricVal));
        windows.emplace_back(std::move(numWin));
    }
}