#pragma once

#include <cstdint>
#include <utility>
#include <vector>

namespace vk::raii
{

class SurfaceKHR;
class Instance;

} // namespace vk::raii

class Window
{

  public:
    Window();

    virtual ~Window();

    bool shouldClose();
    void pollEvents();

    void setResized(bool resized);
    bool wasResized() const;

    std::vector<const char*> getRequiredExtensions();

    [[nodiscard]] vk::raii::SurfaceKHR createSurface(vk::raii::Instance& instance);

    std::pair<uint32_t, uint32_t> getSize();

    class GLFWwindow* getWindow();

  private:
    class GLFWwindow* _glfwWindow = nullptr;

    bool _resized = false;
};