#pragma once

#include "fractals/Fractal.hpp"

class Backend
{
    /* ======================== SETUP ======================= */
  public:
    Backend() = default;

    virtual ~Backend() = default;

    /* ================== PUBLIC FUNCTIONS ================== */

    void record();

    void resize();

    /* ================== VIRTUAL FUNCTIONS ================= */
  private:
    virtual void recompute() = 0;

    virtual void onResize() = 0;

    /* ================= GETTERS AND SETTERS ================ */
  public:
    void setFractal(const Fractal& fractal);

    /* ====================== VARIABLES ===================== */

  protected:
    Fractal _fractal;

  private:
    bool _dirty = true;
};