#pragma once

#include "fractals/Fractal.hpp"

class Backend
{
    /* ==================== NESTED CLASS ==================== */
  public:
    enum class Type // WIP: better reflection?
    {
        GPU = 0,
        CPU = 1
    };

    struct Stats
    {
        double ns = 0;
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

    /* ================== HELPER FUNCTIONS ================= */
  private:
    virtual void recompute() = 0;

    virtual void onResize() = 0;

    /* ================= GETTERS AND SETTERS ================ */
  public:
    void setFractal(const Fractal& fractal);

    const Stats& getStats() const
    {
        return _stats;
    }

    /* ====================== VARIABLES ===================== */

  protected:
    Fractal _fractal;
    Stats   _stats;

  private:
    bool _dirty = true;
};