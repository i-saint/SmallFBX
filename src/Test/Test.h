#pragma once

#ifdef _WIN32
    #define testExport extern "C" __declspec(dllexport)
#else
    #define testExport extern "C"
#endif

#define testPrint(...) ::test::PrintImpl(__VA_ARGS__)
#define testPuts(...) ::test::PutsImpl(__VA_ARGS__)

#define testRegisterInitializer(Name, Init, Fini)\
    static struct _TestInitializer_##Name {\
        _TestInitializer_##Name() { ::test::RegisterInitializer(Init, Fini); }\
    } g_TestInitializer_##Name;

#define testRegisterTestCase(Name)\
    static struct _TestCase_##Name {\
        _TestCase_##Name() { ::test::RegisterTestCaseImpl(#Name, Name); }\
    } g_TestCase_##Name;

#define testCase(Name) void Name(); testRegisterTestCase(Name); void Name()
#define testExpect(Body) if(!(Body)) { throw std::runtime_error(::test::Format("%s(%d): failed - " #Body "\n", __FILE__, __LINE__)); }


namespace test {

using nanosec = uint64_t;
nanosec Now();
inline float NS2MS(nanosec ns) { return float(double(ns) / 1000000.0); }

void RegisterInitializer(const std::function<void()>& init, const std::function<void()>& fini);
void RegisterTestCaseImpl(const char* name, const std::function<void()>& body);
std::string Format(const char* format, ...);
void PrintImpl(const char* format, ...);
void PutsImpl(const char* format);
template<class T> bool GetArg(const char* name, T& dst);

template<class Body>
inline void TestScope(const char *name, const Body& body, int num_try = 1)
{
    auto begin = Now();
    for (int i = 0; i < num_try; ++i)
        body();
    auto end = Now();

    float elapsed = NS2MS(end - begin);
    testPrint("    %s: %.2fms", name, elapsed / num_try);
    if (num_try > 1) {
        testPrint(" (%.2fms in total)", elapsed);
    }
    testPrint("\n");
}

} // namespace test
