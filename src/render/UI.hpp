#pragma once

#include "backend/Backend.hpp"

class UI
{
  public:
    UI(Backend::Settings& settings);

    virtual ~UI() = default;

    /* ================== PUBLIC FUNCTIONS ================== */
  public:
    void draw(const Backend::Stats& stats);

    bool captureMouse() const;

    /* ====================== VARIABLES ===================== */
  private:
    Backend::Settings& _settings;

    float _uiWidth   = 200.0f;
    bool  _collapsed = false;
};