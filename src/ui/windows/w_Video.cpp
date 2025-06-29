#include "ui/windows/w_Video.hpp"
#include <SDL2/SDL.h>
#include <iostream>
#include <algorithm>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
}

#include "tinyfiledialogs.h"

Window::Video::Video(SDL_Renderer* renderer, bool* isOpen) : IWindow(isOpen), m_renderer(renderer) {
    this->title = "Vídeo";
    this->flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse;
}

Window::Video::~Video() {
    cleanup();
}

void Window::Video::play() {
    if (isLoaded()) {
        m_isPlaying = true;
        m_lastFrameTime = SDL_GetTicks();
    }
}

void Window::Video::pause() {
    m_isPlaying = false;
}

bool Window::Video::isPlaying() const {
    return m_isPlaying;
}

bool Window::Video::isLoaded() const {
    return m_formatCtx != nullptr;
}

double Window::Video::getCurrentTime() const {
    return m_currentTime;
}

double Window::Video::getDuration() const {
    return m_duration;
}

bool Window::Video::decodeFrame() {
    while (av_read_frame(m_formatCtx, m_packet) >= 0) {
        if (m_packet->stream_index == m_videoStreamIndex) {
            if (avcodec_send_packet(m_codecCtx, m_packet) == 0) {
                int ret = avcodec_receive_frame(m_codecCtx, m_frame);
                if (ret == 0) {
                    AVStream* videoStream = m_formatCtx->streams[m_videoStreamIndex];
                    m_currentTime = m_frame->pts * av_q2d(videoStream->time_base);
                    
                    uint8_t* pixels[4] = { nullptr };
                    int pitch[4] = { 0 };
                    SDL_LockTexture(m_texture, NULL, (void**)pixels, pitch);
                    sws_scale(m_swsCtx, (const uint8_t* const*)m_frame->data, m_frame->linesize,
                                0, m_videoHeight, pixels, pitch);
                    SDL_UnlockTexture(m_texture);
                    
                    av_packet_unref(m_packet);
                    return true; 
                }
            }
        }
        av_packet_unref(m_packet);
    }

    pause(); 
    return false; 
}

void Window::Video::seek(double time_in_seconds) {
    if (!isLoaded()) return;

    time_in_seconds = std::max(0.0, std::min(time_in_seconds, m_duration));

    AVStream* videoStream = m_formatCtx->streams[m_videoStreamIndex];
    int64_t target_ts = static_cast<int64_t>(time_in_seconds / av_q2d(videoStream->time_base));
    
    if (av_seek_frame(m_formatCtx, m_videoStreamIndex, target_ts, AVSEEK_FLAG_BACKWARD) >= 0) {
        avcodec_flush_buffers(m_codecCtx);

        while (true) {

            if (!decodeFrame()) {
                break;
            }

            if (m_currentTime >= time_in_seconds) {
                break;
            }
        }
        
        m_lastFrameTime = SDL_GetTicks();
    }
}

bool Window::Video::loadVideo(const std::string& filepath) {
    cleanup();

    m_frame = av_frame_alloc();
    m_packet = av_packet_alloc();
    if (!m_frame || !m_packet) {
        cleanup();
        return false;
    }

    if (avformat_open_input(&m_formatCtx, filepath.c_str(), nullptr, nullptr) != 0) {
        cleanup();
        return false;
    }

    if (avformat_find_stream_info(m_formatCtx, nullptr) < 0) {
        cleanup();
        return false;
    }

    const AVCodec* codec = nullptr;
    m_videoStreamIndex = av_find_best_stream(m_formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
    if (m_videoStreamIndex < 0) {
        cleanup();
        return false;
    }
    
    AVStream* videoStream = m_formatCtx->streams[m_videoStreamIndex];
    double fps = av_q2d(videoStream->avg_frame_rate);
    if (fps > 0) {
        m_frameDelay = 1000.0 / fps;
    } else {
        m_frameDelay = 1000.0 / 30.0;
    }

    m_duration = (double)m_formatCtx->duration / AV_TIME_BASE;
    m_currentTime = 0.0;
    m_isPlaying = false;

    m_codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(m_codecCtx, videoStream->codecpar);
    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) {
        cleanup();
        return false;
    }

    m_videoWidth = m_codecCtx->width;
    m_videoHeight = m_codecCtx->height;

    m_swsCtx = sws_getContext(m_videoWidth, m_videoHeight, m_codecCtx->pix_fmt,
                              m_videoWidth, m_videoHeight, AV_PIX_FMT_RGBA,
                              SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!m_swsCtx) {
        cleanup();
        return false;
    }
    
    m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGBA32,
                                  SDL_TEXTUREACCESS_STREAMING, m_videoWidth, m_videoHeight);

    if (!m_texture) {
        cleanup();
        return false;
    }
    
    decodeFrame();
    return true;
}

void Window::Video::render() {
    if (!this->isOpen || !*this->isOpen) return;

    ImGui::Begin(this->title.c_str(), this->isOpen, this->flags);

    if (ImGui::Button("Carregar Vídeo")) {
        const char* filterPatterns[2] = { "*.mp4", "*.mkv" };
        const char* filepath = tinyfd_openFileDialog("Selecione um vídeo", "", 2, filterPatterns, "Arquivos de Vídeo", 0);
        if (filepath) {
            loadVideo(filepath);
        }
    }

    if (isLoaded()) {
        if (m_isPlaying) {
            uint32_t currentTime = SDL_GetTicks();
            if (currentTime >= m_lastFrameTime + m_frameDelay) {
                decodeFrame();
                m_lastFrameTime = currentTime;
            }
        }

        if(m_texture) {
            ImVec2 window_size = ImGui::GetContentRegionAvail();
            float aspect_ratio = (float)m_videoWidth / (float)m_videoHeight;
            
            float img_width = window_size.x;
            float img_height = window_size.x / aspect_ratio;

            if (img_height > window_size.y) {
                img_height = window_size.y;
                img_width = window_size.y * aspect_ratio;
            }

            ImGui::SetCursorPosX((window_size.x - img_width) * 0.5f);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (window_size.y - img_height) * 0.5f);
            
            ImGui::Image(reinterpret_cast<ImTextureID>(m_texture), ImVec2(img_width, img_height));
        }
    } else {
        ImGui::Text("Nenhum vídeo carregado. Clique em 'Carregar Vídeo' para começar.");
    }

    ImGui::End();
}

void Window::Video::cleanup() {
    if (m_texture) {
        SDL_DestroyTexture(m_texture);
        m_texture = nullptr;
    }
    if (m_swsCtx) {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
    }
    if (m_codecCtx) {
        avcodec_free_context(&m_codecCtx);
    }
    if (m_formatCtx) {
        avformat_close_input(&m_formatCtx);
    }
    av_frame_free(&m_frame);
    av_packet_free(&m_packet);

    m_videoStreamIndex = -1;
    m_videoWidth = 0;
    m_videoHeight = 0;
    
    m_isPlaying = false;
    m_currentTime = 0.0;
    m_duration = 0.0;

    m_frameDelay = 0.0;
    m_lastFrameTime = 0;
}