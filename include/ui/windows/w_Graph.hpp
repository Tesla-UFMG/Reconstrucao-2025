#ifndef GRAPH_WINDOW_HPP
#define GRAPH_WINDOW_HPP

// Project
#include "ui/windows/iWindow.hpp"
#include "ui/windows/w_Plot.hpp"
#include "DB.hpp"
#include "Log.hpp"
#include "Dialogs.hpp"

// C++
#include <string>
#include <vector>
#include <algorithm>

namespace Window {
    class Graph : public IWindow {
        public:
            explicit Graph(const std::string& title);
            virtual void render() override;
            virtual bool isDynamic() const override { return true; }
            virtual std::string getDynamicType() const override { return "Graph"; }

            // Accessors for serialization/deserialization
            std::string getTitle() const { return title; }
            const ::Graph& getGraph() const { return m_graph; }
            ::Graph& getGraph() { return m_graph; }

            void addColumn(const std::string& fileType, const std::string& fileName, const std::string& columnName);
            void removeColumn(size_t columnIndex);

        private:
            void processColumnDragDrop();
            void drawLegendPopup();
            void renderGraphPlot();

            bool     m_isOpen = true;
            ::Graph  m_graph;
    };
} // namespace Window

#endif // GRAPH_WINDOW_HPP
