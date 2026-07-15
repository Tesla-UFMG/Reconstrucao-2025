#include "VideoPlayer.hpp"
#include "Log.hpp"
#include <cstring>
#include <iostream>

VideoPlayer::VideoPlayer(SDL_Renderer* renderer) : m_renderer(renderer) {
    const char* vlc_args[] = {"--no-xlib", "--drop-late-frames", "--skip-frames", "--quiet"};
    m_vlcInstance          = libvlc_new(sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);
    if (!m_vlcInstance) {
        LOG("ERROR", "Falha ao inicializar o libvlc.");
    }
}

VideoPlayer::~VideoPlayer() {
    cleanup();
    if (m_vlcInstance) {
        libvlc_release(m_vlcInstance);
        m_vlcInstance = nullptr;
    }
}

void VideoPlayer::cleanup() {
    // IMPORTANTE: libvlc_media_player_stop bloqueia a execução até que a thread
    // do VLC termine de decodificar. Se segurarmos o m_mutex aqui, o VLC não
    // conseguirá fazer o lock() do frame atual, e o app inteiro trava (deadlock).
    if (m_mediaPlayer) {
        libvlc_media_player_stop(m_mediaPlayer);
        libvlc_media_player_release(m_mediaPlayer);
        m_mediaPlayer = nullptr;
    }

    // Agora que o VLC parou, é seguro limpar os buffers
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_texture) {
        SDL_DestroyTexture(m_texture);
        m_texture = nullptr;
    }

    if (m_pixelBuffer) {
        delete[] m_pixelBuffer;
        m_pixelBuffer = nullptr;
    }

    m_width                = 0;
    m_height               = 0;
    m_pitch                = 0;
    m_textureNeedsCreation = false;
    m_textureNeedsUpdate   = false;
}

void VideoPlayer::load(const std::string& path) {
    cleanup();

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_vlcInstance)
        return;

    libvlc_media_t* media = libvlc_media_new_path(m_vlcInstance, path.c_str());
    if (!media) {
        LOG("ERROR", "Falha ao abrir a mídia: " + path);
        return;
    }

    m_mediaPlayer = libvlc_media_player_new_from_media(media);
    libvlc_media_release(media);

    if (!m_mediaPlayer) {
        LOG("ERROR", "Falha ao criar media player para: " + path);
        return;
    }

    libvlc_video_set_callbacks(m_mediaPlayer, VideoPlayer::lock, VideoPlayer::unlock, VideoPlayer::display, this);
    libvlc_video_set_format_callbacks(m_mediaPlayer, VideoPlayer::format_setup, VideoPlayer::format_cleanup);

    LOG("INFO", "Vídeo carregado: " + path);
}

void VideoPlayer::play() {
    if (m_mediaPlayer) {
        libvlc_media_player_play(m_mediaPlayer);
    }
}

void VideoPlayer::pause() {
    if (m_mediaPlayer) {
        libvlc_media_player_set_pause(m_mediaPlayer, 1);
    }
}

void VideoPlayer::stop() {
    if (m_mediaPlayer) {
        libvlc_media_player_stop(m_mediaPlayer);
    }
}

bool VideoPlayer::isPlaying() const {
    if (m_mediaPlayer) {
        return libvlc_media_player_is_playing(m_mediaPlayer) == 1;
    }
    return false;
}

void VideoPlayer::setVolume(int volume) {
    if (m_mediaPlayer) {
        libvlc_audio_set_volume(m_mediaPlayer, volume);
    }
}

void VideoPlayer::setTime(int64_t timeMs) {
    if (m_mediaPlayer) {
        libvlc_media_player_set_time(m_mediaPlayer, timeMs);
    }
}

int64_t VideoPlayer::getLength() {
    if (m_mediaPlayer) {
        return libvlc_media_player_get_length(m_mediaPlayer);
    }
    return 0;
}

int64_t VideoPlayer::getTime() {
    if (m_mediaPlayer) {
        return libvlc_media_player_get_time(m_mediaPlayer);
    }
    return 0;
}

void VideoPlayer::setRate(float rate) {
    if (m_mediaPlayer) {
        libvlc_media_player_set_rate(m_mediaPlayer, rate);
    }
}

void VideoPlayer::updateTexture() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_textureNeedsCreation) {
        if (m_texture) {
            SDL_DestroyTexture(m_texture);
            m_texture = nullptr;
        }
        m_texture =
            SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, m_width, m_height);
        m_textureNeedsCreation = false;
    }

    if (m_textureNeedsUpdate && m_texture && m_pixelBuffer) {
        SDL_UpdateTexture(m_texture, nullptr, m_pixelBuffer, m_pitch);
        m_textureNeedsUpdate = false;
    }
}

void* VideoPlayer::lock(void* data, void** p_pixels) {
    auto* player = static_cast<VideoPlayer*>(data);
    player->m_mutex.lock();
    *p_pixels = player->m_pixelBuffer;
    return nullptr;
}

void VideoPlayer::unlock(void* data, void* /*id*/, void* const* /*p_pixels*/) {
    auto* player                 = static_cast<VideoPlayer*>(data);
    player->m_textureNeedsUpdate = true;
    player->m_mutex.unlock();
}

void VideoPlayer::display(void* /*data*/, void* /*id*/) {
    // A flag m_textureNeedsUpdate foi setada no unlock, a thread de UI do ImGui irá atualizar a textura.
}

unsigned VideoPlayer::format_setup(void** opaque, char* chroma, unsigned* width, unsigned* height, unsigned* pitches,
                                   unsigned* lines) {
    auto*                       player = static_cast<VideoPlayer*>(*opaque);
    std::lock_guard<std::mutex> lock(player->m_mutex);

    // SDL Textures in ImGui typically use RGBA32
    // libvlc RV32 corresponds to RGBA
    memcpy(chroma, "RV32", 4);

    player->m_width  = *width;
    player->m_height = *height;
    player->m_pitch  = (*width) * 4;

    *pitches = player->m_pitch;
    *lines   = player->m_height;

    player->m_textureNeedsCreation = true;

    if (player->m_pixelBuffer) {
        delete[] player->m_pixelBuffer;
    }
    player->m_pixelBuffer = new uint8_t[player->m_pitch * player->m_height];

    return 1;
}

void VideoPlayer::format_cleanup(void* /*opaque*/) {
    // Cleanup will be handled during destruction or reload
}
