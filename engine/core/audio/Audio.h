#pragma once

#include <miniaudio.h>

#include <memory>
#include <string>
#include <glm/glm.hpp>

class Audio
{
public:
    static bool Init()
    {
        return ma_engine_init(nullptr, &engine) == MA_SUCCESS;
    }

    static void Shutdown()
    {
        music.reset();
        ma_engine_uninit(&engine);
    }

    static void PlayOneShot(const char* path)
    {
        ma_engine_play_sound(&engine, path, nullptr);
    }

    static void PlayMusic(const char* path, bool loop = true)
    {
        music = std::make_unique<ma_sound>();

        ma_sound_init_from_file(&engine, path, MA_SOUND_FLAG_STREAM, nullptr, nullptr, music.get());
        ma_sound_set_looping(music.get(), loop);
        ma_sound_start(music.get());
    }

    /*static void SetMasterVolume(float volume)
    {
        ma_engine_set_volume(&engine, volume);
    }

    static void UpdateListener(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up)
    {
        ma_engine_listener_set_position(&engine, 0, position.x, position.y, position.z);
        ma_engine_listener_set_direction(&engine, 0, forward.x, forward.y, forward.z);
        ma_engine_listener_set_world_up(&engine, 0, up.x, up.y, up.z);
    }*/

    static ma_engine* GetEngine() { return &engine; }

private:
    inline static ma_engine engine;
    inline static std::unique_ptr<ma_sound> music;
};