#ifndef PLOT_WINDOW_HPP
#define PLOT_WINDOW_HPP

// Project
#include "DB.hpp"
#include "ImGuiWrapper.hpp"
#include "ui/windows/iWindow.hpp"

// C++
#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

enum GraphType { GRAPH_LINE, GRAPH_BAR, GRAPH_SCATTER, GRAPH_FILLED_LINE };

struct GraphData {
        std::vector<std::string>         columns;
        std::vector<std::string>         archives;
        std::vector<std::vector<double>> x;
        std::vector<std::vector<double>> y;

        GraphType type       = GRAPH_LINE;
        bool      showXAxis   = false;
        bool      showYAxis   = true;
        double    plotHeight = 200;
};

namespace Window {

    class Plot : public IWindow {
        public:
            explicit Plot(bool* isOpen = nullptr);

            void render() override;

        private:
            // Funções de renderização da interface
            void drawMenuBar();

            // Funções auxiliares para processamento de dados e payload
            void processColumnDragDrop(GraphData& graphData);
            void generateSimulatedData(int numPoints, size_t graphIndex, float* x, float* y);

            // Funções de renderização dos gráficos
            void drawLegendPopup(GraphData& graphData, int graphIndex, int& graphToRemove);
            void renderGraph(size_t graphIndex, int& graphToRemove);
            void renderResizeButton(size_t graphIndex);
    };

} // namespace Window

#endif // PLOT_WINDOW_HPP
