#pragma once

#include <float.h>
#include <string>
#include <vector>

#define DOUBLE_PRECISION

#ifdef DOUBLE_PRECISION
using real = double;
#define REAL_MAX DBL_MAX;
#else
using real = float;
#define REAL_MAX FLT_MAX;
#endif

using std::string;
using std::vector;
