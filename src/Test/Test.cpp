#include "pch.h"
#include "Test.h"

namespace test {

nanosec Now()
{
    using namespace std::chrono;
    return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

static std::string g_log;

std::string FormatImpl(const char* format, va_list args)
{
    const int MaxBuf = 4096;
    char buf[MaxBuf];
    vsprintf(buf, format, args);
    return buf;
}

std::string Format(const char* format, ...)
{
    std::string ret;
    va_list args;
    va_start(args, format);
    ret = FormatImpl(format, args);
    va_end(args);
    fflush(stdout);
    return ret;
}

void PrintImpl(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    auto txt = FormatImpl(format, args);
    g_log += txt;
#ifdef _WIN32
    ::OutputDebugStringA(txt.c_str());
#endif
    printf("%s", txt.c_str());

    va_end(args);
    fflush(stdout);
}

void PutsImpl(const char* s)
{
#ifdef _WIN32
    ::OutputDebugStringA(s);
#endif
    printf(s);
}

struct ArgEntry
{
    std::string name;
    std::string value;

    template<class T> bool getValue(T& dst) const;
};
template<> bool ArgEntry::getValue(std::string& dst) const
{
    dst = value;
    return true;
}
template<> bool ArgEntry::getValue(bool& dst) const
{
    if (value == "true" || std::atoi(value.c_str()) != 0) {
        dst = true;
        return true;
    }
    else if (value == "false" || value == "0") {
        dst = false;
        return true;
    }
    return false;
}
template<> bool ArgEntry::getValue(int& dst) const
{
    dst = std::atoi(value.c_str());
    return dst != 0;
}
template<> bool ArgEntry::getValue(float& dst) const
{
    dst = (float)std::atof(value.c_str());
    return dst != 0.0f;
}

static std::vector<ArgEntry>& GetArgs()
{
    static std::vector<ArgEntry> s_instance;
    return s_instance;
}
template<class T> bool GetArg(const char *name, T& dst)
{
    auto& args = GetArgs();
    auto it = std::find_if(args.begin(), args.end(), [name](auto& e) { return e.name == name; });
    if (it != args.end())
        return it->getValue(dst);
    return false;
}
template bool GetArg(const char *name, std::string& dst);
template bool GetArg(const char* name, bool& dst);
template bool GetArg(const char* name, int& dst);
template bool GetArg(const char *name, float& dst);



struct TestInitializer
{
    std::function<void()> initializer;
    std::function<void()> finalizer;
};

struct TestEntry
{
    std::string name;
    std::function<void()> body;
};

static std::vector<TestInitializer>& GetInitializers()
{
    static std::vector<TestInitializer> s_instance;
    return s_instance;
}

static std::vector<TestEntry>& GetTests()
{
    static std::vector<TestEntry> s_instance;
    return s_instance;
}

void RegisterInitializer(const std::function<void()>& init, const std::function<void()>& fini)
{
    GetInitializers().push_back({ init, fini });
}

void RegisterTestCaseImpl(const char *name, const std::function<void()>& body)
{
    GetTests().push_back({name, body});
}

static void RunTestImpl(const TestEntry& v)
{
    testPrint("%s begin\n", v.name.c_str());
    auto begin = Now();
    try {
        v.body();
    }
    catch (const std::runtime_error& e) {
        testPrint("*** failed: %s ***\n", e.what());
    }
    auto end = Now();
    testPrint("%s end (%.2fms)\n\n", v.name.c_str(), NS2MS(end - begin));
}

} // namespace test

testExport const char* GetLogMessage()
{
    return test::g_log.c_str();
}

testExport void RunTest(char *name)
{
    test::g_log.clear();
    for (auto& entry : test::GetTests()) {
        if (entry.name == name) {
            test::RunTestImpl(entry);
        }
    }
}

testExport void RunAllTests()
{
    test::g_log.clear();
    for (auto& entry : test::GetTests()) {
        test::RunTestImpl(entry);
    }
}

int main(int argc, char *argv[])
{
    for (auto& i : test::GetInitializers())
        i.initializer();

    int run_count = 0;
    for (int i = 1; i < argc; ++i) {
        if (char *sep = std::strstr(argv[i], "=")) {
            *(sep++) = '\0';
            test::GetArgs().push_back({ argv[i] , sep });
        }
    }
    for (int i = 1; i < argc; ++i) {
        if (!std::strstr(argv[i], "=")) {
            RunTest(argv[i]);
            ++run_count;
        }
    }

    if (run_count == 0) {
        RunAllTests();
    }

    for (auto& i : test::GetInitializers())
        i.finalizer();
}
