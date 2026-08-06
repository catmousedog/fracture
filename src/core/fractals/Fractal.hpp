#pragma once

class Fractal
{
    /* ==================== NESTED CLASS ==================== */
  public:
    struct Settings
    {
        float offsetX = 0.0f, offsetY = 0.0f;
        float zoom    = 1.0f;
        float maxIter = 100;
    };

    /* ======================== SETUP ======================= */

    Fractal();

    virtual ~Fractal();

    /* ================= GETTERS AND SETTERS ================ */

    const Settings& getSettings() const
    {
        return _settings;
    }
    Settings& getSettings()
    {
        return _settings;
    }

    /* ====================== VARIABLES ===================== */

    Settings _settings;
};