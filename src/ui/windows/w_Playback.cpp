#include "ui/windows/w_Playback.hpp"

Window::Playback::Playback(bool* isOpen) : IWindow(isOpen) {
    this->title = "Playback";
    this->flags = ImGuiWindowFlags_NoScrollbar;
}

void Window::Playback::render() {
    // Variáveis estáticas para manter o estado entre renderizações
    static float counter = 0.0f;      
    static int selectedButton = 0;     
    static float jumpStep = 1.0f;         
    const float MAX_TIME = 60.0f;          
    
    ImGuiIO& io = ImGui::GetIO();  // Para acessar o deltaTime

    if (this->isOpen && *this->isOpen) {
        ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);

        // Se estiver no modo "Iniciar", incrementa o contador com o deltaTime
        if (selectedButton == 1) {
            counter += io.DeltaTime;
            if (counter > MAX_TIME)
                counter = MAX_TIME; 
        }

        ImGui::SliderFloat("Tempo da Corrida", &counter, 0.0f, MAX_TIME, "%.2f s");

        if (ImGui::ArrowButton("##left", ImGuiDir_Left)) {
            counter -= jumpStep;
            if (counter < 0.0f)
                counter = 0.0f;
        }
        ImGui::SameLine();

        // Botões de controle: "Parar" e "Iniciar"
        ImGui::RadioButton("Parar", &selectedButton, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Iniciar", &selectedButton, 1);
        ImGui::SameLine();

        // Botão de avançar: aumenta o contador em 'jumpStep' segundos
        if (ImGui::ArrowButton("##right", ImGuiDir_Right)) {
            counter += jumpStep;
            if (counter > MAX_TIME)
                counter = MAX_TIME;
        }
        ImGui::SameLine();
        ImGui::Text("%.2f s", counter);

        // Slider para ajustar o intervalo de avanço/retrocesso (jumpStep)
        ImGui::SliderFloat("Intervalo (s)", &jumpStep, 0.1f, 5.0f, "%.1f s");

        ImGui::End();
    }
}
