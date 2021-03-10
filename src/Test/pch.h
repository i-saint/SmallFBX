#pragma once
#include <cstdio>
#include <string>
#include <cstdarg>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>
#include <memory>
#include <iostream>
#include <chrono>

#ifdef __cpp_lib_span
    #include <span>
#endif

#ifdef _WIN32
    #define NOMINMAX
    #include <windows.h>
#endif
