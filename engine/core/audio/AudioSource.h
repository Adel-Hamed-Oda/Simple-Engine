#pragma once

#include <miniaudio.h>

#include "../../Config.h"

#include <string>
#include <vector>

class AudioSource
{
public:
    static bool Init()
    {
        return ma_engine_init(nullptr, &engine) == MA_SUCCESS;
    }

    static void Shutdown()
    {
        ma_engine_uninit(&engine);
    }

    AudioSource(const std::string& path) : audioFilePath(CONFIG::DIRECTORY::AUDIO + path) 
    {
        audioSources.push_back(this);
    }
    ~AudioSource() 
    { 
        audioSources.erase(std::remove(audioSources.begin(), audioSources.end(), this), audioSources.end()); 
    }

    void Play()
    {
        ma_engine_play_sound(&engine, audioFilePath.c_str(), nullptr);
    }

    const char *GetAudioFilePath() const { return audioFilePath.c_str(); }
    const std::string& GetAudioFilePathString() const { return audioFilePath; }

private:
    inline static ma_engine engine;
    inline static std::vector<AudioSource*> audioSources;

    std::string audioFilePath;
};