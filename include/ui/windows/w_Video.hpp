#pragma once

#include "ui/windows/iWindow.hpp"
#include "VideoPlayer.hpp"
#include <string>

namespace Window {

class Video : public IWindow {
public:
    Video(bool* showFlag, SDL_Renderer* renderer);
    ~Video() override = default;

    void render() override;
    
    // Metodos para setar o estado do video (usado pelo WindowManager ao carregar layout)
    void setLoadedVideo(const std::string& path);
    std::string getLoadedVideo() const { return m_currentArchiveName; }
    
    void setVolume(float volume);
    float getVolume() const { return m_volume; }

    VideoPlayer* getPlayer() { return &m_player; }

private:
    bool* m_showFlag = nullptr;
    std::string m_currentArchiveName;
    float m_volume = 100.0f; // 0 a 100
    
    VideoPlayer m_player;
};

} // namespace Window
