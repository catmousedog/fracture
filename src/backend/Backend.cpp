#include "Backend.hpp"

////////////////////////////////////////////////////////////

void Backend::record()
{
    if (_dirty && !_settings.frozen)
    {
        recompute();
        _dirty = false;
    }
}

////////////////////////////////////////////////////////////

void Backend::resize()
{
    onResize();
    _dirty = true;
}

////////////////////////////////////////////////////////////

void Backend::setSettings(const Settings& settings)
{
    if (settings != _settings)
    {
        _settings = settings;
        _dirty    = true;
    }
}

////////////////////////////////////////////////////////////