#ifndef ASSET_MANAGER_HPP
#define ASSET_MANAGER_HPP

// Project
#include "Log.hpp"
#include "SDLWrapper.hpp"

// C++
#include <string>
#include <unordered_map>

#define GET_TEXTURE(filePath) AssetManager::getInstance().loadTexture(filePath);

class AssetManager {
    private:
        std::unordered_map<std::string, SDL_Texture*> textures;
        explicit AssetManager();

    public:
        AssetManager(AssetManager&&)            = delete;
        AssetManager& operator=(AssetManager&&) = delete;
        ~AssetManager();

        static AssetManager& getInstance();

        SDL_Texture* getTexture(const std::string& filePath);
        void         clearTextures();
};

#endif
