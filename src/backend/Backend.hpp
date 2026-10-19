#pragma once

#include "fractals/Fractal.hpp"

#include <algorithm>
#include <cstdint>
#include <glm/glm.hpp>
#include <optional>

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
#include <vulkan/vulkan_raii.hpp>

class Backend
{
    /* ==================== NESTED CLASS ==================== */
  public:
    enum class Type // WIP: better reflection?
    {
        GPU = 0
    };

    struct Settings
    {
        Fractal  fractal;
        Type     backendType = Type::GPU;
        uint32_t scale       = 1;
        bool     frozen      = false;

        bool operator==(const Settings& other) const = default;

        glm::uvec2 computeSize(uint32_t width, uint32_t height) const
        {
            return {std::max(1u, width / scale), std::max(1u, height / scale)};
        }
    };

    struct LastCompute
    {
        Fractal    fractal;
        glm::uvec2 computeSize = {0, 0};
    };

    struct Stats
    {
        double ns = 0; // nanoseconds of previous compute
    };

    /* ======================== SETUP ======================= */

    Backend()                          = default;
    virtual ~Backend()                 = default;
    Backend(const Backend&)            = delete;
    Backend(Backend&&)                 = delete;
    Backend& operator=(const Backend&) = delete;

    /* ================== PUBLIC FUNCTIONS ================== */

    void record();

    void resize();

    virtual std::optional<vk::SemaphoreSubmitInfo> getReady() const = 0;

    /* ================== HELPER FUNCTIONS ================= */
  private:
    virtual void recompute() = 0;

    virtual void onResize() = 0;

    /* ================= GETTERS AND SETTERS ================ */
  public:
    void setSettings(const Settings& settings);

    const Settings& getSettings() const { return _settings; }

    const LastCompute& getLastCompute() const { return _lastCompute; }

    const Stats& getStats() const { return _stats; }

    /* ====================== VARIABLES ===================== */

  protected:
    Settings    _settings;    // current settings
    LastCompute _lastCompute; // contents of the previously computed image
    Stats       _stats;

  private:
    bool _dirty = true;
};