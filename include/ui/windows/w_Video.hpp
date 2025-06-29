#ifndef VIDEO_WINDOW_HPP
#define VIDEO_WINDOW_HPP

#include <string>
#include "ui/windows/iWindow.hpp"

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;
struct SDL_Texture;
struct SDL_Renderer;

namespace Window {
    class Video : public IWindow {
    public:
        explicit Video(SDL_Renderer* renderer, bool* isOpen = nullptr);
        ~Video();

        void render() override;
        bool loadVideo(const std::string& filepath);

        void play();
        void pause();
        void seek(double time_in_seconds);

        bool isPlaying() const;
        bool isLoaded() const;
        double getCurrentTime() const;
        double getDuration() const;

    private:
        void cleanup();
        bool decodeFrame();

        SDL_Renderer* m_renderer = nullptr;
        SDL_Texture* m_texture  = nullptr;
        
        AVFormatContext* m_formatCtx = nullptr;
        AVCodecContext* m_codecCtx  = nullptr;
        AVFrame* m_frame     = nullptr;
        AVPacket* m_packet    = nullptr;
        SwsContext* m_swsCtx    = nullptr;
        int m_videoStreamIndex = -1;

        int m_videoWidth  = 0;
        int m_videoHeight = 0;

        bool   m_isPlaying   = false;
        double m_currentTime = 0.0;
        double m_duration    = 0.0;

        double m_frameDelay = 0.0;
        uint32_t m_lastFrameTime = 0;
    };
}

#endif