#pragma once

#include <miniaudio.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

class AudioSource
{
public:
    bool loop = false;

    static bool Init()
    {
        if (engineInitialized)
            return true;

        engineInitialized = (ma_engine_init(nullptr, &engine) == MA_SUCCESS);
        return engineInitialized;
    }

    static void StopAll()
    {
        for (AudioSource* src : audioSources)
        {
            if (src)
                src->activeSound = nullptr;
        }

        for (auto& entry : managedSounds)
        {
            if (entry && entry->initialized)
            {
                ma_sound_stop(&entry->sound);
                ma_sound_uninit(&entry->sound);
                entry->initialized = false;
            }
        }
        managedSounds.clear();
    }

    static void Shutdown()
    {
        if (!engineInitialized)
            return;

        StopAll();
        ma_engine_uninit(&engine);
        engineInitialized = false;
    }

    AudioSource(const std::string& path, bool loop = false)
        : loop(loop), audioFilePath(path)
    {
        audioSources.push_back(this);
    }

    AudioSource(const AudioSource& other)
        : loop(other.loop), volume(other.volume), audioFilePath(other.audioFilePath)
    {
        audioSources.push_back(this);
    }

    AudioSource& operator=(const AudioSource& other)
    {
        if (this != &other)
        {
            DetachSound();
            loop = other.loop;
            volume = other.volume;
            audioFilePath = other.audioFilePath;
        }
        return *this;
    }

    ~AudioSource()
    {
        DetachSound();
        audioSources.erase(std::remove(audioSources.begin(), audioSources.end(), this), audioSources.end());
    }

    void Play()
    {
        if (!engineInitialized && !Init())
            return;

        CleanupFinishedSounds();

        // Reuse existing sound instance for looping music or stopped one-shots;
        // if a non-looping SFX is still mid-play, detach it so overlapping hits play naturally.
        if (activeSound && activeSound->initialized)
        {
            if (!loop && ma_sound_is_playing(&activeSound->sound) == MA_TRUE)
            {
                DetachSound();
            }
            else
            {
                ma_sound_set_looping(&activeSound->sound, loop ? MA_TRUE : MA_FALSE);
                ma_sound_set_volume(&activeSound->sound, volume);
                ma_sound_seek_to_pcm_frame(&activeSound->sound, 0);
                ma_sound_start(&activeSound->sound);
                return;
            }
        }

        auto entry = std::make_unique<ManagedSound>();
        const ma_uint32 flags = MA_SOUND_FLAG_NO_SPATIALIZATION | (loop ? MA_SOUND_FLAG_LOOPING : 0);

        if (ma_sound_init_from_file(&engine, audioFilePath.c_str(), flags, nullptr, nullptr, &entry->sound) != MA_SUCCESS)
            return;

        entry->initialized = true;
        entry->owner = this;

        ma_sound_set_looping(&entry->sound, loop ? MA_TRUE : MA_FALSE);
        ma_sound_set_volume(&entry->sound, volume);
        ma_sound_start(&entry->sound);

        activeSound = entry.get();
        managedSounds.push_back(std::move(entry));
    }

    void Play(bool shouldLoop)
    {
        loop = shouldLoop;
        Play();
    }

    void Stop()
    {
        if (activeSound && activeSound->initialized)
        {
            ma_sound_stop(&activeSound->sound);
            ma_sound_seek_to_pcm_frame(&activeSound->sound, 0);
        }
    }

    void Pause()
    {
        if (activeSound && activeSound->initialized)
        {
            ma_sound_stop(&activeSound->sound);
        }
    }

    void Resume()
    {
        if (activeSound && activeSound->initialized)
        {
            ma_sound_start(&activeSound->sound);
        }
    }

    void SetLooping(bool shouldLoop)
    {
        loop = shouldLoop;
        if (activeSound && activeSound->initialized)
        {
            ma_sound_set_looping(&activeSound->sound, loop ? MA_TRUE : MA_FALSE);
        }
    }

    bool IsLooping() const
    {
        if (activeSound && activeSound->initialized)
            return ma_sound_is_looping(&activeSound->sound) == MA_TRUE;
        return loop;
    }

    void SetVolume(float newVolume)
    {
        volume = (std::max)(0.0f, newVolume);
        if (activeSound && activeSound->initialized)
        {
            ma_sound_set_volume(&activeSound->sound, volume);
        }
    }

    float GetVolume() const { return volume; }

    bool IsPlaying() const
    {
        return activeSound &&
               activeSound->initialized &&
               ma_sound_is_playing(&activeSound->sound) == MA_TRUE;
    }

    const char* GetAudioFilePath() const { return audioFilePath.c_str(); }
    const std::string& GetAudioFilePathString() const { return audioFilePath; }

private:
    struct ManagedSound
    {
        ma_sound sound{};
        AudioSource* owner = nullptr;
        bool initialized = false;
    };

    void DetachSound()
    {
        if (activeSound)
        {
            activeSound->owner = nullptr;
            activeSound = nullptr;
        }
    }

    static void CleanupFinishedSounds()
    {
        for (auto it = managedSounds.begin(); it != managedSounds.end(); )
        {
            ManagedSound* entry = it->get();
            if (entry &&
                entry->owner == nullptr &&
                entry->initialized &&
                ma_sound_is_looping(&entry->sound) == MA_FALSE &&
                ma_sound_at_end(&entry->sound) == MA_TRUE)
            {
                ma_sound_uninit(&entry->sound);
                entry->initialized = false;
                it = managedSounds.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    inline static ma_engine engine{};
    inline static bool engineInitialized = false;
    inline static std::vector<AudioSource*> audioSources;
    inline static std::vector<std::unique_ptr<ManagedSound>> managedSounds;

    ManagedSound* activeSound = nullptr;
    float volume = 1.0f;
    std::string audioFilePath;
};
