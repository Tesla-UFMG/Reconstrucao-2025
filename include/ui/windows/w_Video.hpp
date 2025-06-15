#ifndef VIDEO_WINDOW_HPP
#define VIDEO_WINDOW_HPP

#include <string>

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;
struct SDL_Texture;
struct SDL_Renderer;

#include "ImGuiWrapper.hpp"
#include "ui/windows/iWindow.hpp"

namespace Window {
    class Video : public IWindow {
    public:
        explicit Video(SDL_Renderer* renderer, bool* isOpen = nullptr);
        ~Video();

        virtual void render() override;
        bool loadVideo(const std::string& filepath);

    private:
        void cleanup();

        SDL_Renderer* m_renderer = nullptr;

        AVFormatContext* m_formatCtx = nullptr;
        AVCodecContext* m_codecCtx = nullptr;
        AVFrame* m_frame = nullptr;
        AVPacket* m_packet = nullptr;
        SwsContext* m_swsCtx = nullptr;
        int m_videoStreamIndex = -1;

        SDL_Texture* m_texture = nullptr;
        int m_videoWidth = 0;
        int m_videoHeight = 0;
    };
}

#endif