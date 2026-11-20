#include "Precision.hpp"

struct PushConstants
{
    alignas(sizeof(real)) real offsetX = 0.;
    alignas(sizeof(real)) real offsetY = 0.;
    alignas(sizeof(real)) real zoom    = 1.;
    alignas(4) int maxIter             = 100;
};