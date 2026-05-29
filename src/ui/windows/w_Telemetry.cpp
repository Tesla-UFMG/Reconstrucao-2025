#include "ui/windows/w_Telemetry.hpp"

static std::string stripNulls(const std::string& s) {
    auto pos = s.find('\0');
    if (pos != std::string::npos)
        return s.substr(0, pos);
    return s;
}

Window::Telemetry::Telemetry(bool* isOpen) : IWindow(isOpen) {
    title = "Telemetria";
    flags = ImGuiWindowFlags_NoScrollbar;

    // Uart configuration
    this->serialPort = "";
    this->baudrate   = 115200;
    this->getAvailablePorts();
    this->selectedPortIndex = 0;
    this->saveToFile        = false;
    this->outputPacketFolder.resize(COLUMN_NAME_SIZE);

    // Packet configuration
    this->clearAndResizeInputBuffers();
    this->processingStatus = false;
}

void Window::Telemetry::clearAndResizeInputBuffers() {
    this->packetName.clear();
    this->packetName.resize(FILE_NAME_SIZE);

    this->packetId.clear();
    this->packetId.resize(COLUMN_NAME_SIZE);

    this->packetColumnNames.clear();
    this->packetColumnNames.resize(8);
    for (int i = 0; i < 8; ++i) {
        this->packetColumnNames[i].clear();
        this->packetColumnNames[i].resize(COLUMN_NAME_SIZE);
    }
}

void Window::Telemetry::savePacketsToFile(const std::string& outputFolder) {
    DB::getInstance().saveTelemetryPackets(std::string(outputFolder.data()));
}

Window::Telemetry::~Telemetry() { this->closeDevice(); }

void Window::Telemetry::openDevice(const char* port, int baud) {
    closeDevice();

    if (this->device.openDevice(port, baud) == 1) {
        LOG("INFO", "Conectado na porta: " + std::string(port));
        this->keepReading  = true;
        this->readerThread = std::thread(&Telemetry::readMessages, this);
    } else {
        std::string errorMsg = "Falha ao abrir porta: " + std::string(port);
        Dialogs::showErrorDialog(errorMsg);
        LOG("ERROR", errorMsg);
    }

    DB::getInstance().getProject().setTelemetryStatus(this->device.isDeviceOpen());
}

void Window::Telemetry::closeDevice() {
    this->keepReading = false;
    if (this->readerThread.joinable()) {
        this->readerThread.join();
    }

    if (this->device.isDeviceOpen()) {
        this->device.closeDevice();
        LOG("INFO", "Dispositivo fechado: " + this->serialPort);
    }
    DB::getInstance().getProject().setTelemetryStatus(this->device.isDeviceOpen());
}

void Window::Telemetry::getAvailablePorts() {
    serialib    temp_device;
    std::string device_port;
    this->closeDevice();
    this->serialPorts.clear();
    for (int i = 1; i < 99; i++) {
#if defined(_WIN32) || defined(_WIN64)
        device_port = std::string("\\\\.\\COM") + std::to_string(i);
#elif defined(__linux__)
        device_port = std::string("/dev/ttyACM") + std::to_string(i - 1);
#else
#error "Unsupported operating system"
#endif

        if (temp_device.openDevice(device_port.c_str(), 9600) == 1) {
            this->serialPorts.push_back(device_port);
            LOG("INFO", "Porta encontrada: " + device_port);
            temp_device.closeDevice();
        }
    }
}

void Window::Telemetry::readMessages() {
    char buf[512];
    while (keepReading) {
        int n = device.readString(buf, '\n', sizeof(buf) - 1, 300);
        if (n > 0) {
            buf[n] = '\0';
            std::lock_guard lk(queueMutex);
            messageQueue.push(std::string(buf));
        }
    }
}

void Window::Telemetry::drainQueueIntoRecent() {
    std::lock_guard lk(queueMutex);
    if (recentMessages.size() >= MAX_RECENT_MESSAGES) {
        if (this->saveToFile) {
            this->savePacketsToFile(this->outputPacketFolder);
        }
        recentMessages.clear();
    }

    while (!messageQueue.empty()) {
        recentMessages.push_back(std::move(messageQueue.front()));
        this->processingStatus = this->processPacket(recentMessages.back());
        DB::getInstance().getProject().setProcessingStatus(this->processingStatus);
        messageQueue.pop();
    }
}

void Window::Telemetry::renderConfigMenu() {

    ImGui::SeparatorText("Configuração da UART");
    ImGui::BeginGroup();
    if (ImGui::BeginTable("##cfg", 2, ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Widget", ImGuiTableColumnFlags_WidthStretch);

        // baudrate
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Baudrate");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(150.0f);
        ImGui::InputInt("##baudrate", &this->baudrate, 0, 0);

        // porta serial
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Porta Serial");
        ImGui::TableSetColumnIndex(1);
        {
            std::vector<const char*> items;
            items.reserve(serialPorts.size());
            for (auto& s : serialPorts)
                items.push_back(s.c_str());
            ImGui::SetNextItemWidth(150.0f);
            ImGui::Combo("##PortasSeriais", &this->selectedPortIndex, items.data(), (int)items.size());

            if (selectedPortIndex >= (int)items.size()) {
                this->serialPort = "";
            } else {
                this->serialPort = items[selectedPortIndex];
            }
        }

        // pasta output
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Salvar na Pasta");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(150.0f);
        ImGui::InputText("##outputPacketFolder", outputPacketFolder.data(), COLUMN_NAME_SIZE,
                         ImGuiInputTextFlags_CharsNoBlank);

        // salvar automaticamente
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Salvar Automat.");
        ImGui::TableSetColumnIndex(1);
        ImGui::Checkbox("##saveInFile", &this->saveToFile);
        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Status:");
    ImGui::SameLine();
    std::string status = this->device.isDeviceOpen() ? "Conectado" : "Desconectado";
    ImGui::TextColored(this->device.isDeviceOpen() ? HI(1) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f), status.c_str());

    ImGui::SameLine();
    ImGui::Text("Processamento:");
    std::string processing = this->processingStatus ? "Ok" : "Erro";
    ImGui::SameLine();
    ImGui::TextColored(this->processingStatus ? HI(1) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f), processing.c_str());

    ImGui::Spacing();

    // Botões
    if (ImGui::Button("Conectar") && !this->serialPort.empty()) {
        this->openDevice(this->serialPort.c_str(), this->baudrate);
    }

    ImGui::SameLine();
    if (ImGui::Button("Desconectar")) {
        this->closeDevice();
    }

    ImGui::SameLine();
    if (ImGui::Button("Atualizar Portas")) {
        this->getAvailablePorts();
    }

    ImGui::SameLine();
    if (ImGui::Button("Salvar Pacotes")) {
        this->savePacketsToFile(this->outputPacketFolder);
    }

    ImGui::EndGroup();
}

void Window::Telemetry::renderPacketConfigMenu() {
    std::string configTitle = m_editMode ? "Configuração dos Pacotes (Modo Edição)" : "Configuração dos Pacotes";
    ImGui::SeparatorText(configTitle.c_str());
    ImGui::BeginGroup();
    if (ImGui::BeginTable("##packetCfg", 2, ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

        // Nome do Pacote
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Nome do Pacote");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(150.0f);
        ImGui::InputText("##packet_name", this->packetName.data(), FILE_NAME_SIZE, ImGuiInputTextFlags_CharsNoBlank);

        // ID do Pacote
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("ID do Pacote");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(150.0f);
        if (m_editMode) {
            ImGui::TextDisabled("%s (Chave Única)", m_editPacketId.c_str());
        } else {
            ImGui::InputText("##packet_id", this->packetId.data(), COLUMN_NAME_SIZE, ImGuiInputTextFlags_CharsNoBlank);
        }
        ImGui::EndTable();
    }

    ImGui::Separator();

    if (ImGui::BeginTable("##packetCfg", 2, ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

        for (int i = 0; i < 8; ++i) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Coluna %d", i + 1);
            ImGui::TableSetColumnIndex(1);
            ImGui::PushID(i);
            ImGui::SetNextItemWidth(150.0f);
            ImGui::InputText("##colname", this->packetColumnNames[i].data(), COLUMN_NAME_SIZE,
                             ImGuiInputTextFlags_CharsNoBlank);
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    std::string buttonLabel = m_editMode ? "Salvar Alterações" : "Salvar Configuração";
    if (ImGui::Button(buttonLabel.c_str())) {
        // Verifica o nome do pacote
        std::string packetName_ = stripNulls(this->packetName);
        if (packetName_.empty()) {
            Dialogs::showErrorDialog("O nome do pacote não pode ser vazio.");
            ImGui::EndGroup();
            return;
        }

        // Verifica o id do pacote
        std::string packetId_ = m_editMode ? m_editPacketId : stripNulls(this->packetId);
        if (packetId_.empty()) {
            Dialogs::showErrorDialog("O ID do pacote não pode ser vazio.");
            ImGui::EndGroup();
            return;
        }

        // Filtra colunas preenchidas
        std::vector<std::string> packetColumnNames_;
        for (size_t i = 0; i < this->packetColumnNames.size(); ++i) {
            std::string colName = stripNulls(this->packetColumnNames[i]);
            if (!colName.empty()) {
                packetColumnNames_.push_back(colName);
            }
        }

        // Verifica se há pelo menos 1 coluna preenchida
        if (packetColumnNames_.empty()) {
            Dialogs::showErrorDialog("Você deve preencher pelo menos 1 coluna.");
            ImGui::EndGroup();
            return;
        }

        // Verifica se os nomes das colunas preenchidas são únicos
        std::set<std::string> uniq;
        for (const auto& colName : packetColumnNames_) {
            uniq.insert(colName);
        }
        if (uniq.size() != packetColumnNames_.size()) {
            Dialogs::showErrorDialog("Os nomes das colunas não podem se repetir.");
            ImGui::EndGroup();
            return;
        }

        // Tenta salvar ou atualizar
        if (m_editMode) {
            if (DB::getInstance().getProject().updatePacket(m_editPacketId, packetName_, packetColumnNames_)) {
                this->clearAndResizeInputBuffers();
                m_editMode = false;
                m_editPacketId = "";
                LOG("INFO", "Pacote atualizado: " + packetName_);
            } else {
                Dialogs::showErrorDialog("Erro ao atualizar o pacote.");
            }
        } else {
            if (DB::getInstance().getProject().loadPacket(packetName_, packetId_, packetColumnNames_)) {
                this->clearAndResizeInputBuffers();
                LOG("INFO", "Pacote salvo: " + packetName_);
            } else {
                std::string msg = "Pacote com o ID já existe. ID: " + packetId_;
                Dialogs::showErrorDialog(msg);
                LOG("ERROR", msg);
            }
        }
    }

    if (m_editMode) {
        ImGui::SameLine();
        if (ImGui::Button("Cancelar")) {
            this->clearAndResizeInputBuffers();
            m_editMode = false;
            m_editPacketId = "";
        }
    }

    ImGui::EndGroup();

    ImGui::BeginGroup();
    ImGui::SeparatorText("Pacotes Salvos:");

    ImGuiTableFlags tblFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                              ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_NoHostExtendY | ImGuiTableFlags_Resizable |
                              ImGuiTableFlags_HighlightHoveredColumn;
    if (ImGui::BeginTable("##TelemetryTable", 11, tblFlags)) {
        // Define cabeçalhos
        ImGui::TableSetupColumn("Nome do Pacote", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthStretch);
        for (int i = 0; i < 8; ++i) {
            ImGui::TableSetupColumn(("Col. " + std::to_string(i + 1)).c_str(), ImGuiTableColumnFlags_WidthStretch);
        }
        ImGui::TableSetupColumn("Ações", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableHeadersRow();

        const auto& telemetryFiles = DB::getInstance().getProject().getTelemetryFiles();
        for (const auto& telemetryFile : telemetryFiles) {
            ImGui::TableNextRow();

            // Coluna 0: Nome
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(telemetryFile.getName().c_str());

            // Coluna 1: ID
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(telemetryFile.getPacketId().c_str());

            // Coluna 2: Colunas
            for (int i = 0; i < 8; ++i) {
                ImGui::TableSetColumnIndex(i + 2);
                if ((size_t)i < telemetryFile.getColumnNames().size()) {
                    ImGui::TextUnformatted(telemetryFile.getColumnNames()[i].c_str());
                } else {
                    ImGui::TextUnformatted("");
                }
            }

            // Coluna 10: Botões Ações
            ImGui::TableSetColumnIndex(10);
            ImGui::PushID(telemetryFile.getPacketId().c_str());
            if (ImGui::Button("E")) {
                m_editMode = true;
                m_editPacketId = telemetryFile.getPacketId();

                // Carrega nome
                this->packetName.clear();
                this->packetName.resize(FILE_NAME_SIZE);
                std::strncpy(this->packetName.data(), telemetryFile.getName().c_str(), FILE_NAME_SIZE);

                // Carrega ID
                this->packetId.clear();
                this->packetId.resize(COLUMN_NAME_SIZE);
                std::strncpy(this->packetId.data(), telemetryFile.getPacketId().c_str(), COLUMN_NAME_SIZE);

                // Carrega colunas
                for (int i = 0; i < 8; ++i) {
                    this->packetColumnNames[i].clear();
                    this->packetColumnNames[i].resize(COLUMN_NAME_SIZE);
                }
                for (size_t i = 0; i < telemetryFile.getColumnNames().size() && i < 8; ++i) {
                    std::strncpy(this->packetColumnNames[i].data(), telemetryFile.getColumnNames()[i].c_str(), COLUMN_NAME_SIZE);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("X")) {
                if (Dialogs::showConfirmationDialog("Tem certeza que deseja remover o pacote " +
                                                    telemetryFile.getName() + "?")) {
                    if (m_editMode && m_editPacketId == telemetryFile.getPacketId()) {
                        m_editMode = false;
                        m_editPacketId = "";
                        this->clearAndResizeInputBuffers();
                    }
                    DB::getInstance().getProject().removePacket(telemetryFile.getPacketId());
                }
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::EndGroup();
}

void Window::Telemetry::renderRecentMessages() {
    ImGui::SeparatorText("Mensagens Recentes");
    ImVec2 available_size  = ImGui::GetContentRegionAvail();
    available_size.y      -= ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("TelemetryScroll", available_size, true, ImGuiWindowFlags_HorizontalScrollbar);
    {
        for (const std::string& line : this->recentMessages) {
            ImGui::TextUnformatted(line.c_str());
        }
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
    }
    ImGui::EndChild();
    if (ImGui::Button("Limpar")) {
        this->recentMessages.clear();
    }
}

void Window::Telemetry::render() {
    this->drainQueueIntoRecent();

    if (this->isOpen && *this->isOpen) {
        ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);
        if (ImGui::BeginTabBar("TabTelemetria",
                               ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_FittingPolicyScroll)) {
            if (ImGui::BeginTabItem("UART")) {
                this->renderConfigMenu();
                this->renderRecentMessages();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Pacotes")) {
                this->renderPacketConfigMenu();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::End();
    }
}

bool Window::Telemetry::processPacket(const std::string& packet) {
    // Pega a data e hora atual com milissegundos em unix time
    auto now = std::chrono::system_clock::now();
    auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::string date = std::to_string(timestamp_ms);

    // Processa os dados
    std::vector<double> data;
    std::stringstream   ss(packet);
    std::string         item;
    std::string         packetId;
    int                 index = 0;
    while (std::getline(ss, item, ',')) {
        if (index == 0) {
            packetId = item;
        } else {
            try {
                double value = std::stod(item);
                data.push_back(value);
            } catch (const std::exception& e) {
                return false;
            }
        }
        index++;
    }

    return DB::getInstance().processTelemetryPacket(packetId, data, date);
}
