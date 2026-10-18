#include<iostream>
#include <stdexcept>
#include "ScriptFunction.hpp"

#if defined (ENABLE_LUA)
#include "LangLua/LangLua.hpp"
#endif

ScriptFunction::ScriptFunction(ScriptFunc fCpp,char ret_type, const std::string &def) :
        fCpp(fCpp), ret_type(ret_type), def(def), script_type(SCRIPT_CPP)
{

}
#if defined (ENABLE_LUA)
ScriptFunction::ScriptFunction(const ScriptFuncLua &fLua, lua_State *lua, char ret_type, const std::string &def) :
        fLua({lua, fLua}), ret_type(ret_type), def(def), script_type(SCRIPT_LUA)
{

}
#endif


ScriptFunction::~ScriptFunction()
{
#if defined (ENABLE_LUA)
    if (script_type == SCRIPT_LUA)
        fLua.name.~ScriptFuncLua();
#endif
}

boost::any ScriptFunction::Call(const std::vector<boost::any> &args)
{
    boost::any result;

    if (def.length() != args.size())
        throw std::runtime_error("Script call: Number of arguments does not match definition");
#if defined (ENABLE_LUA)
    else if (script_type == SCRIPT_LUA)
    {
        LangLua langLua(fLua.lua);
        boost::any any = langLua.Call(fLua.name.c_str(), def.c_str(), args);

        switch (ret_type)
        {
            case 'i':
                result = boost::any_cast<luabridge::LuaRef>(any).cast<unsigned int>();
                break;
            case 'q':
                result = boost::any_cast<luabridge::LuaRef>(any).cast<signed int>();
                break;
            case 'f':
                result = boost::any_cast<luabridge::LuaRef>(any).cast<double>();
                break;
            case 's':
                result = boost::any_cast<luabridge::LuaRef>(any).cast<const char*>();
                break;
            case 'v':
                result = boost::any();
                break;
            default:
                throw std::runtime_error("Lua call: Unknown return type" + ret_type);
        }

        lua_settop(fLua.lua, 0);
    }
#endif

    return result;
}
