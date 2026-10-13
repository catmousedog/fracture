#pragma once

class UI
{
  public:
    UI() = default;

    virtual ~UI() = default;

    /* ==================== CAPABILITIES ==================== */
  public:
    void draw();

    /* ====================== VARIABLES ===================== */
  private:
    float _uiWidth   = 200.0f;
    bool  _collapsed = false;
};