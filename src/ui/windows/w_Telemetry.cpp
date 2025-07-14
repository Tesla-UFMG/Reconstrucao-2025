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

Window::Telemetry::~Telemetry() { this->closeDevice(); }

void Window::Telemetry::openDevice(const char* port, int baud) {
    closeDevice();

    if (this->device.openDevice(port, baud) == 1) {
        LOG("INFO", "Conectado na porta: " + std::string(port));
        this->keepReading  = true;
        this->readerThread = std::thread(&Telemetry::readMessages, this);
    } else {
        std::string errorMsg = "Falha ao abrir porta: " + std::string(port);
        DB::errorDialog(errorMsg);
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
        LOG("INFO", "Dispositivo fechado");
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
    while (!messageQueue.empty()) {
        recentMessages.push_back(std::move(messageQueue.front()));
        this->processingStatus = this->processPacket(recentMessages.back());
        DB::getInstance().getProject().setProcessingStatus(this->processingStatus);
        messageQueue.pop();
    }
}

void Window::Telemetry::renderConfigMenu() {
    ImGui::BeginGroup();

    ImGui::Text("Baudrate:");
    ImGui::SameLine();
    ImGui::InputInt("##baudrate", &this->baudrate, 0, 0, ImGuiInputTextFlags_None);

    std::vector<const char*> items;
    items.reserve(serialPorts.size());
    for (auto& s : serialPorts)
        items.push_back(s.c_str());

    ImGui::Text("Porta Serial:");
    ImGui::SameLine();
    ImGui::Combo("##PortasSeriais", &selectedPortIndex, items.data(), (int)items.size());
    if (selectedPortIndex >= (int)items.size()) {
        this->serialPort = "";
    } else {
        this->serialPort = items[selectedPortIndex];
    }

    ImGui::Checkbox("Salvar em Arquivo", &saveToFile);

    ImGui::Text("Status:");
    ImGui::SameLine();
    std::string status = this->device.isDeviceOpen() ? "Conectado" : "Desconectado";
    ImGui::TextColored(this->device.isDeviceOpen() ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                       status.c_str());

    ImGui::SameLine();
    ImGui::Text("Processamento:");
    std::string processing = this->processingStatus ? "Ok" : "Erro";
    ImGui::SameLine();
    ImGui::TextColored(this->processingStatus ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                       processing.c_str());

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

    ImGui::EndGroup();
}

void Window::Telemetry::renderPacketConfigMenu() {
    ImGui::BeginGroup();
    // Nome
    ImGui::Text("Nome do Pacote:");
    ImGui::SameLine();
    ImGui::InputText("##packet_name", this->packetName.data(), 128, ImGuiInputTextFlags_CharsNoBlank);

    // ID
    ImGui::Text("ID do Pacote:");
    ImGui::SameLine();
    ImGui::InputText("##packet_id", this->packetId.data(), 16, ImGuiInputTextFlags_CharsNoBlank);
    ImGui::Spacing();

    // Nomes das colunas
    ImGui::Text("Nomes das colunas:");
    for (int i = 0; i < 8; ++i) {
        ImGui::PushID(i);
        ImGui::InputText(std::string("##colname" + std::to_string(i)).c_str(), this->packetColumnNames[i].data(), 128,
                         ImGuiInputTextFlags_CharsNoBlank);
        ImGui::SameLine();
        ImGui::Text("Coluna %d", i + 1);
        ImGui::PopID();
    }

    ImGui::Spacing();
    if (ImGui::Button("Salvar Configuração")) {
        // Verifica o nome do pacote
        if (std::string(this->packetName.data()).empty()) {
            DB::errorDialog("O nome do pacote não pode ser vazio.");
            ImGui::EndGroup();
            return;
        }

        // Verifica o id do pacote
        if (std::string(this->packetId.data()).empty()) {
            DB::errorDialog("O ID do pacote não pode ser vazio.");
            ImGui::EndGroup();
            return;
        }

        // Verifica se o nome das colunas estão vazios
        for (const auto& colName : this->packetColumnNames) {
            if (std::string(colName.data()).empty()) {
                DB::errorDialog("Os nomes das colunas não podem ser vazios.");
                ImGui::EndGroup();
                return;
            }
        }

        // Verifica se os nomes das colunas são únicos
        std::set<std::string> uniq;
        for (auto& colBuf : packetColumnNames) {
            uniq.insert(colBuf.data());
        }
        if (uniq.size() != packetColumnNames.size()) {
            DB::errorDialog("Os nomes das colunas não podem se repetir.");
            ImGui::EndGroup();
            return;
        }

        std::string              packetName_ = stripNulls(this->packetName);
        std::string              packetId_   = stripNulls(this->packetId);
        std::vector<std::string> packetColumnNames_(this->packetColumnNames.size());
        for (size_t i = 0; i < this->packetColumnNames.size(); ++i) {
            packetColumnNames_[i] = stripNulls(this->packetColumnNames[i]);
        }

        // Tenta salvar
        if (DB::getInstance().getProject().loadPacket(packetName_, packetId_, packetColumnNames_)) {
            this->clearAndResizeInputBuffers();
            LOG("INFO", "Pacote salvo: " + packetName_);
        } else {
            std::string msg = "Pacote com o ID já existe. ID: " + packetId_;
            DB::errorDialog(msg);
            LOG("ERROR", msg);
        }
    }
    ImGui::EndGroup();

    ImGui::BeginGroup();
    ImGui::SeparatorText("Pacotes Salvos:");

    ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                            ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_NoHostExtendY | ImGuiTableFlags_Resizable |
                            ImGuiTableFlags_HighlightHoveredColumn;
    if (ImGui::BeginTable("##TelemetryTable", 11, flags)) {
        // Define cabeçalhos
        ImGui::TableSetupColumn("Nome do Pacote", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthStretch);
        for (int i = 0; i < 8; ++i) {
            ImGui::TableSetupColumn(("Col. " + std::to_string(i + 1)).c_str(), ImGuiTableColumnFlags_WidthStretch);
        }
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
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

            // Coluna 3: Botão Remover
            ImGui::TableSetColumnIndex(10);
            ImGui::PushID(telemetryFile.getPacketId().c_str());
            if (ImGui::Button("Remover")) {
                if (DB::ConfirmationDialog("Tem certeza que deseja remover o pacote " + telemetryFile.getName() +
                                           "?")) {
                    DB::getInstance().getProject().removePacket(telemetryFile.getPacketId());
                }
            }
            ImGui::PopID();
        }
    }
    ImGui::EndTable();
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

    return DB::getInstance().processTelemetryPacket(packetId, data);
}
