#ifndef SCRIPTFUNCTION_HPP
#define SCRIPTFUNCTION_HPP

#include <boost/any.hpp>
#include <string>
#include <vector>
#if defined (ENABLE_LUA)
#include "LangLua/LangLua.hpp"
#endif

typedef unsigned long long(*ScriptFunc)();
#if defined (ENABLE_LUA)
typedef std::string ScriptFuncLua;
#endif

class ScriptFunction
{
protected:
    union
    {
        ScriptFunc fCpp;
#if defined (ENABLE_LUA)
        struct
        {
            lua_State *lua;
            ScriptFuncLua name;
        } fLua;
#endif
    };

protected:
    char ret_type;
    std::string def;
    int script_type;
    enum
    {
        SCRIPT_CPP,
        SCRIPT_LUA
    };

    ScriptFunction(ScriptFunc fCpp, char ret_type, const std::string &def);
#if defined (ENABLE_LUA)
    ScriptFunction(const ScriptFuncLua &fPawn, lua_State *lua, char ret_type, const std::string &def);
#endif
    virtual ~ScriptFunction();

    boost::any Call(const std::vector<boost::any> &args);
};

#endif //SCRIPTFUNCTION_HPP
