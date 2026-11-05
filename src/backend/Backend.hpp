#pragma once

#include "fractals/Fractal.hpp"

#include <glm/glm.hpp>

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
#include <vulkan/vulkan_raii.hpp>

class Backend
{
  public:
    // Settings used to store the current state of the backend
    struct Settings
    {
        Fractal fractal;

        bool operator==(const Settings& other) const = default;
    };

    struct Stats
    {
        double ns = 0; // nanoseconds of previous compute
    };

    /* ======================== SETUP ======================= */

    Backend(Settings& settings)
        : _settings(settings)
    {
    }
    virtual ~Backend()                 = default;
    Backend(const Backend&)            = delete;
    Backend(Backend&&)                 = delete;
    Backend& operator=(const Backend&) = delete;

    /* ================== PUBLIC FUNCTIONS ================== */

    void update();

    void resize();

    void changeFractal();

    void markDirty();

    /* ================== HELPER FUNCTIONS ================= */
  private:
    virtual void onResize() = 0;

    virtual void onFractalChange() = 0;

    virtual void submitJob() = 0;

    /* ================= GETTERS AND SETTERS ================ */
  public:
    const Settings& getSettings() const { return _settings; }

    const Stats& getStats() const { return _stats; }

    /* ====================== VARIABLES ===================== */

  protected:
    Settings& _settings; // current settings
    Stats     _stats;

  private:
    bool _dirty = true;
};