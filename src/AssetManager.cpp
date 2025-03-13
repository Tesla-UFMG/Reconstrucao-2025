#include "AssetManager.hpp"

AssetManager& AssetManager::getInstance() {
    static AssetManager instance;
    return instance;
}

AssetManager::AssetManager() {
    LOG("TRACE", "AssetManager iniciado com sucesso.");
}

AssetManager::~AssetManager() {
    clearTextures();
    LOG("TRACE", "AssetManager encerrado.");
}

SDL_Texture* AssetManager::loadTexture(const std::string& filePath) {
    if (textures.find(filePath) != textures.end()) {
        return textures[filePath];
    }

    SDL_Texture* texture = IMG_LoadTexture(SDLWrapper::renderer, filePath.c_str());
    if (!texture) {
        LOG("ERROR", "Falha ao carregar textura: " + filePath);
        return nullptr;
    }

    textures[filePath] = texture;
    LOG("INFO", "Textura carregada: " + filePath);
    return texture;
}

SDL_Texture* AssetManager::getTexture(const std::string& filePath) const {
    auto it = textures.find(filePath);
    if (it != textures.end()) {
        return it->second;
    }
    return nullptr;
}

void AssetManager::clearTextures() {
    for (auto& pair : textures) {
        SDL_DestroyTexture(pair.second);
    }
    textures.clear();
}