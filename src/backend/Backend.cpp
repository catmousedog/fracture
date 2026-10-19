#include "Backend.hpp"

////////////////////////////////////////////////////////////

void Backend::update()
{
    if (_jobBusy && getJobDone())
    {
        onJobDone(); // swap
        _jobBusy = false;
    }
    if (!_jobBusy && _dirty && !_settings.frozen)
    {
        submitJob(); // recompute
        _dirty   = false;
        _jobBusy = true;
    }
}

////////////////////////////////////////////////////////////

void Backend::resize()
{
    onResize();
    _lastJob = {};
    _jobBusy = false; // image was just recreated
    _dirty   = true;
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