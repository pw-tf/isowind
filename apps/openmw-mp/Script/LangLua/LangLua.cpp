#include <iostream>
#include "LangLua.hpp"
#include "LuaIo2.hpp"
#include <Script/Script.hpp>
#include <Script/Types.hpp>

std::set<std::string> LangLua::packagePath;
std::set<std::string> LangLua::packageCPath;

void setLuaPath(lua_State* L, const char* path, bool cpath = false)
{
    std::string field = cpath ? "cpath" : "path";
    lua_getglobal(L, "package");

    lua_getfield(L, -1, field.c_str());
    std::string cur_path = lua_tostring(L, -1);
    cur_path.append(";");
    cur_path.append(path);
    lua_pop(L, 1);
    lua_pushstring(L, cur_path.c_str());
    lua_setfield(L, -2, field.c_str());
    lua_pop(L, 1);
}

lib_t LangLua::GetInterface()
{
    return reinterpret_cast<lib_t>(lua);
}

LangLua::LangLua(lua_State *lua)
{
    this->lua = lua;
}

LangLua::LangLua()
{
    lua = luaL_newstate();
    luaL_openlibs(lua); // load all lua std libs
    registerLuaIo2(lua);

    std::string p, cp;
    for (auto& path : packagePath)
        p += path + ';';

    for (auto& path : packageCPath)
        cp += path + ';';

    setLuaPath(lua, p.c_str());
    setLuaPath(lua, cp.c_str(), true);

}

LangLua::~LangLua()
{

}

// LuaFunctionDispatcher template struct for Lua function dispatch
template <unsigned int ArgIndex, unsigned int FunctionIndex>
struct LuaFunctionDispatcher {
    // Dispatch Lua function with the given arguments
    template <typename ReturnType, typename... Args>
    inline static ReturnType Dispatch(lua_State*&& lua, Args&&... args) noexcept {
        // Retrieve function data
        constexpr ScriptFunctionData const& functionData = ScriptFunctions::functions[FunctionIndex];
        // Retrieve argument from the Lua stack
        auto argument = luabridge::Stack<typename CharType<functionData.func.types[ArgIndex - 1]>::type>::get(lua, ArgIndex);
        // Recursively dispatch the Lua function
        return LuaFunctionDispatcher<ArgIndex - 1, FunctionIndex>::template Dispatch<ReturnType>(
            std::forward<lua_State*>(lua), argument, std::forward<Args>(args)...);
    }
};

// Specialization for LuaFunctionDispatcher when ArgIndex is 0
template <unsigned int FunctionIndex>
struct LuaFunctionDispatcher<0, FunctionIndex> {
    // Dispatch Lua function with the given arguments
    template <typename ReturnType, typename... Args>
    inline static ReturnType Dispatch(lua_State*&&, Args&&... args) noexcept {
        // Call the C++ function using reinterpret_cast
        return reinterpret_cast<FunctionEllipsis<ReturnType>>(ScriptFunctions::addresses[FunctionIndex].addr)(
            std::forward<Args>(args)...);
    }
};

// Lua function wrapper for functions returning 'void'
template <unsigned int FunctionIndex>
static typename std::enable_if<ScriptFunctions::functions[FunctionIndex].func.ret == 'v', int>::type LuaFunctionWrapper(lua_State* lua) noexcept {
    // Dispatch the Lua function
    LuaFunctionDispatcher<ScriptFunctions::functions[FunctionIndex].func.numargs, FunctionIndex>::template Dispatch<void>(std::forward<lua_State*>(lua));
    return 0;
}

// Lua function wrapper for functions with non-void return types
template <unsigned int FunctionIndex>
static typename std::enable_if<ScriptFunctions::functions[FunctionIndex].func.ret != 'v', int>::type LuaFunctionWrapper(lua_State* lua) noexcept {
    // Dispatch the Lua function
    auto result = LuaFunctionDispatcher<ScriptFunctions::functions[FunctionIndex].func.numargs, FunctionIndex>::template Dispatch<
        typename CharType<ScriptFunctions::functions[FunctionIndex].func.ret>::type>(std::forward<lua_State*>(lua));
    // Push the result onto the Lua stack
    luabridge::Stack<typename CharType<ScriptFunctions::functions[FunctionIndex].func.ret>::type>::push(lua, result);
    return 1;
}

// Struct for defining Lua functions with names and wrappers
template <unsigned int FunctionIndex>
struct LuaFunctionDefinition {
    static constexpr LuaFunctionData FunctionInfo{
       ScriptFunctions::functions[FunctionIndex].name, LuaFunctionWrapper<FunctionIndex>
    };
};

template<> struct LuaFunctionDefinition<0> { static constexpr LuaFunctionData FunctionInfo{"CreateTimer", LangLua::CreateTimer}; };
template<> struct LuaFunctionDefinition<1> { static constexpr LuaFunctionData FunctionInfo{"CreateTimerEx", LangLua::CreateTimerEx}; };
template<> struct LuaFunctionDefinition<2> { static constexpr LuaFunctionData FunctionInfo{"MakePublic", LangLua::MakePublic}; };
template<> struct LuaFunctionDefinition<3> { static constexpr LuaFunctionData FunctionInfo{"CallPublic", LangLua::CallPublic}; };


#ifdef __arm__
template<std::size_t... Is>
struct indices {};
template<std::size_t N, std::size_t... Is>
struct build_indices : build_indices<N-1, N-1, Is...> {};
template<std::size_t... Is>
struct build_indices<0, Is...> : indices<Is...> {};
template<std::size_t N>
using IndicesFor = build_indices<N>;

template<size_t... Indices>
LuaFuctionData *functions(indices<Indices...>)
{

    static LuaFuctionData functions_[sizeof...(Indices)]{
            F_<Indices>::F...
    };

    static_assert(
            sizeof(functions_) / sizeof(functions_[0]) ==
            sizeof(ScriptFunctions::functions) / sizeof(ScriptFunctions::functions[0]),
            "Not all functions have been mapped to Lua");

    return functions_;
}
#else
template<unsigned int I>
struct LuaFunctionInitializer
{
    constexpr static void Initialize(LuaFunctionData *functions_)
    {
        functions_[I] = LuaFunctionDefinition<I>::FunctionInfo;
        LuaFunctionInitializer<I - 1>::Initialize(functions_);
    }
};

template<>
struct LuaFunctionInitializer<0>
{
    constexpr static void Initialize(LuaFunctionData *functions_)
    {
        functions_[0] = LuaFunctionDefinition<0>::FunctionInfo;
    }
};

template<size_t LastI>
LuaFunctionData *GetLuaFunctions()
{
    static LuaFunctionData functions_[LastI];
    LuaFunctionInitializer<LastI - 1>::Initialize(functions_);

    static_assert(
        sizeof(functions_) / sizeof(functions_[0]) ==
        sizeof(ScriptFunctions::functions) / sizeof(ScriptFunctions::functions[0]),
        "Not all functions have been mapped to Lua");

    return functions_;
}
#endif

void LangLua::LoadProgram(const char *filename)
{
    int err = 0;

    if ((err =luaL_loadfile(lua, filename)) != 0)
        throw std::runtime_error("Lua script " + std::string(filename) + " error (" + std::to_string(err) + "): \"" +
                            std::string(lua_tostring(lua, -1)) + "\"");

    constexpr auto functions_n = sizeof(ScriptFunctions::functions) / sizeof(ScriptFunctions::functions[0]);

#ifdef __arm__
    LuaFunctionData *functions_ = GetLuaFunctions(IndicesFor<functions_n>{});
#else
    LuaFunctionData *functions_ = GetLuaFunctions<sizeof(ScriptFunctions::functions) / sizeof(ScriptFunctions::functions[0])>();
#endif
luabridge::Namespace tes3mp = luabridge::getGlobalNamespace(lua).beginNamespace("tes3mp");

for (unsigned i = 0; i < functions_n; i++)
    tes3mp.addCFunction(functions_[i].name, functions_[i].func);

tes3mp.endNamespace();

if ((err = lua_pcall(lua, 0, 0, 0)) != 0) // Run once script for load in memory.
    throw std::runtime_error("Lua script " + std::string(filename) + " error (" + std::to_string(err) + "): \"" +
                        std::string(lua_tostring(lua, -1)) + "\"");

}

int LangLua::FreeProgram()
{
    lua_close(lua);
    return 0;
}

bool LangLua::IsCallbackPresent(const char *name)
{
    return luabridge::getGlobal(lua, name).isFunction();
}

boost::any LangLua::Call(const char *name, const char *argl, int buf, ...)
{
    va_list vargs;
    va_start(vargs, buf);

    int n_args = (int)(strlen(argl));

    lua_getglobal(lua, name);

    for (int index = 0; index < n_args; index++)
    {
        switch (argl[index])
        {
            case 'i':
                luabridge::Stack<unsigned int>::push(lua,va_arg(vargs, unsigned int));
                break;

            case 'q':
                luabridge::Stack<signed int>::push(lua,va_arg(vargs, signed int));
                break;

            case 'l':
                luabridge::Stack<unsigned long long>::push(lua, va_arg(vargs, unsigned long long));
                break;

            case 'w':
                luabridge::Stack<signed long long>::push(lua, va_arg(vargs, signed long long));
                break;

            case 'f':
                luabridge::Stack<double>::push(lua, va_arg(vargs, double));
                break;

            case 'p':
                luabridge::Stack<void*>::push(lua, va_arg(vargs, void*));
                break;

            case 's':
                luabridge::Stack<const char*>::push(lua, va_arg(vargs, const char*));
                break;

            case 'b':
                luabridge::Stack<bool>::push(lua, (bool) va_arg(vargs, int));
                break;

            default:
                throw std::runtime_error("C++ call: Unknown argument identifier " + argl[index]);
        }
    }

    va_end(vargs);

    luabridge::LuaException::pcall(lua, n_args, 1);
    return boost::any(luabridge::LuaRef::fromStack(lua, -1));
}

boost::any LangLua::Call(const char *name, const char *argl, const std::vector<boost::any> &args)
{
    int n_args = (int)(strlen(argl));

    lua_getglobal(lua, name);

    for (int index = 0; index < n_args; index++)
    {
        switch (argl[index])
        {
            case 'i':
                luabridge::Stack<unsigned int>::push(lua, boost::any_cast<unsigned int>(args.at(index)));
                break;

            case 'q':
                luabridge::Stack<signed int>::push(lua, boost::any_cast<signed int>(args.at(index)));
                break;

            case 'l':
                luabridge::Stack<unsigned long long>::push(lua, boost::any_cast<unsigned long long>(args.at(index)));
                break;

            case 'w':
                luabridge::Stack<signed long long>::push(lua, boost::any_cast<signed long long>(args.at(index)));
                break;

            case 'f':
                luabridge::Stack<double>::push(lua, boost::any_cast<double>(args.at(index)));
                break;

            case 'p':
                luabridge::Stack<void *>::push(lua, boost::any_cast<void *>(args.at(index)));
                break;

            case 's':
                luabridge::Stack<const char *>::push(lua, boost::any_cast<const char *>(args.at(index)));
                break;

            case 'b':
                luabridge::Stack<bool>::push(lua, boost::any_cast<int>(args.at(index)));
                break;
            default:
                throw std::runtime_error("Lua call: Unknown argument identifier " + argl[index]);
        }
    }

    luabridge::LuaException::pcall(lua, n_args, 1);
    return boost::any(luabridge::LuaRef::fromStack(lua, -1));
}

void LangLua::AddPackagePath(const std::string& path)
{
    packagePath.emplace(path);
}

void LangLua::AddPackageCPath(const std::string& path)
{
    packageCPath.emplace(path);
}
