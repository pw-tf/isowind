#ifndef _WIN32
    #include <dlfcn.h>
#endif

#include <stdexcept>
#include "LangNative.hpp"
#include <Script/SystemInterface.hpp>
#include <Script/Script.hpp>

template<typename R>
bool SetScript(lib_t lib, const char *name, R value)
{
    SystemInterface<R *> result(lib, name);

    if (result)
        *result.result = value;

    return result.operator bool();
}

void LangNative::LoadProgram(const char *filename)
{
    FILE *file = fopen(filename, "rb");

    if (!file)
        throw std::runtime_error("Script not found: " + std::string(filename));

    fclose(file);

#ifdef _WIN32
    lib = LoadLibraryA(filename);
#else
    lib = dlopen(filename, RTLD_LAZY);
#endif

    if (!lib)
        throw std::runtime_error("Was not able to load C++ script: " + std::string(filename));

    try
    {

        const char *prefix = SystemInterface<const char *>(lib, "prefix").result;
        std::string pf(prefix);

        for (const auto &function : ScriptFunctions::functions)
            if (!SetScript(lib, std::string(pf + function.name).c_str(), function.func.addr))
                LOG_MESSAGE_SIMPLE(TimedLog::LOG_WARN, "Script function pointer not found: %s", function.name);
    }
    catch (...)
    {
        FreeProgram();
        throw;
    }
}

int LangNative::FreeProgram()
{
#ifdef _WIN32
    FreeLibrary(lib);
#else
    dlclose(lib);
#endif
    return 0;
}

bool LangNative::IsCallbackPresent(const char *name)
{
    return true;
}

boost::any LangNative::Call(const char *name, const char *argl, int buf, ...)
{
    return nullptr;
}

boost::any LangNative::Call(const char *name, const char *argl, const std::vector<boost::any> &args)
{
    return nullptr;
}


lib_t LangNative::GetInterface()
{
    return lib;
}


LangNative::LangNative()
{

}

LangNative::~LangNative()
{

}
