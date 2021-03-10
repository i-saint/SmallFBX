#pragma once

#ifdef _WIN32
    #define NOMINMAX
    #include <windows.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cstdarg>
#include <cmath>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <functional>
#include <memory>
#include <iostream>
#include <sstream>
#include <fstream>
#include <thread>
#include <condition_variable>
#include <future>
#include <random>
#include <regex>
#include <iterator>
#include <bit>
#include <ranges>

#ifdef __cpp_lib_span
    #include <span>
#endif
