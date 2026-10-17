#pragma once

#include "backend/Backend.hpp"
#include "vulkan/VulkanImage.hpp"

class CPUBackend : public Backend
{
    /* ======================== SETUP ======================= */
  public:
    CPUBackend(VulkanCore& vulkanCore, CommandQueue& commandQueue, VulkanImage& vulkanImage);

    virtual ~CPUBackend() = default;

    /* ================== VIRTUAL FUNCTIONS ================= */
  private:
    virtual void recompute() override;

    virtual void onResize() override;

    /* ================== HELPER FUNCTIONS ================== */

    void fillImage();

    /* ====================== VARIABLES ===================== */

    VulkanCore&   _vulkanCore;
    CommandQueue& _commandQueue;
    VulkanImage&  _vulkanImage;

    struct Image
    {
        using Pixel = std::array<uint8_t, 4>;

        uint32_t width;
        uint32_t height;

        Image() = default;
        Image(uint32_t width, uint32_t height)
            : width(width),
              height(height),
              pixels(width * height)
        {
        }

        std::vector<Pixel> pixels;

        uint32_t size() const
        {
            return width * height;
        }

        uint32_t flatten(uint32_t x, uint32_t y) const
        {
            return x + y * width;
        }

        Pixel& operator[](uint32_t x, uint32_t y)
        {
            return pixels[flatten(x, y)];
        }
    };
    Image _image;
};