#pragma once

#include <string>
#include <mutex>
#include <vlc/vlc.h>
#include <SDL2/SDL.h>

class VideoPlayer {
public:
    VideoPlayer(SDL_Renderer* renderer);
    ~VideoPlayer();

    void load(const std::string& path);
    void play();
    void pause();
    void stop();
    bool isPlaying() const;
    void setVolume(int volume); // 0 to 100

    void setTime(int64_t timeMs);
    int64_t getLength();
    int64_t getTime();
    void setRate(float rate);
    
    // Updates the SDL_Texture with the latest frame if needed
    void updateTexture();
    
    SDL_Texture* getTexture() const { return m_texture; }
    
    // Size of the video
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    
    // Callbacks for libvlc
    static void* lock(void* data, void** p_pixels);
    static void unlock(void* data, void* id, void* const* p_pixels);
    static void display(void* data, void* id);
    static unsigned format_setup(void** opaque, char* chroma, unsigned* width, unsigned* height, unsigned* pitches, unsigned* lines);
    static void format_cleanup(void* opaque);

private:
    void cleanup();

    SDL_Renderer* m_renderer = nullptr;
    SDL_Texture* m_texture = nullptr;
    
    libvlc_instance_t* m_vlcInstance = nullptr;
    libvlc_media_player_t* m_mediaPlayer = nullptr;
    
    std::mutex m_mutex;
    uint8_t* m_pixelBuffer = nullptr;
    
    unsigned m_width = 0;
    unsigned m_height = 0;
    unsigned m_pitch = 0;
    
    bool m_textureNeedsCreation = false;
    bool m_textureNeedsUpdate = false;
};
