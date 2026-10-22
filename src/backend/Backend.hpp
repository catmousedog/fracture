#pragma once

#include "fractals/Fractal.hpp"

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
        Fractal fractal;
        Type    backendType = Type::GPU;
        bool    frozen      = false;

        bool operator==(const Settings& other) const = default;
    };

    struct Job
    {
        Fractal fractal;
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

    void update();

    void resize();

    virtual std::optional<vk::SemaphoreSubmitInfo> getReady() const = 0;

    /* ================== HELPER FUNCTIONS ================= */
  private:
    virtual void onResize() = 0;

    virtual void onJobDone() = 0;

    virtual void submitJob() = 0;

    /* ================= GETTERS AND SETTERS ================ */
  protected:
    virtual bool getJobDone() const = 0;

  public:
    void setSettings(const Settings& settings);

    const Settings& getSettings() const { return _settings; }

    const Job& getLastJob() const { return _lastJob; }

    const Stats& getStats() const { return _stats; }

    /* ====================== VARIABLES ===================== */

  protected:
    Settings _settings;      // current settings
    Job      _lastJob, _job; // contents of the previously computed image
    Stats    _stats;

  private:
    bool _jobBusy = false;
    bool _dirty   = true;
};