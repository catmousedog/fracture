#pragma once

class Backend
{
    /* ======================== SETUP ======================= */
  public:
    Backend() = default;

    virtual ~Backend() = default;

    virtual void recompute() = 0;

    virtual void resize() = 0;
};