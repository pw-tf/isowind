#ifndef PLUGINSYSTEM3_LANGLUA_HPP
#define PLUGINSYSTEM3_LANGLUA_HPP

#include "lua.hpp"

#include <extern/LuaBridge/LuaBridge.h>
#include <LuaBridge.h>
#include <set>

#include <boost/any.hpp>
#include "../ScriptFunction.hpp"
#include "../Language.hpp"

struct LuaFunctionData
{
    const char* name;
    lua_CFunction func;
};

class LangLua: public Language
{
public:
    virtual lib_t GetInterface() override;
    lua_State *lua;
public:
    LangLua();
    LangLua(lua_State *lua);
    ~LangLua();

    static void AddPackagePath(const std::string &path);
    static void AddPackageCPath(const std::string &path);

    static int MakePublic(lua_State *lua) noexcept;
    static int CallPublic(lua_State *lua);

    static int CreateTimer(lua_State *lua) noexcept;
    static int CreateTimerEx(lua_State *lua);

    virtual void LoadProgram(const char *filename) override;
    virtual int FreeProgram() override;
    virtual bool IsCallbackPresent(const char *name) override;
    virtual boost::any Call(const char *name, const char *argl, int buf, ...) override;
    virtual boost::any Call(const char *name, const char *argl, const std::vector<boost::any> &args) override;
private:
    static std::set<std::string> packageCPath;
    static std::set<std::string> packagePath;
};


#endif //PLUGINSYSTEM3_LANGLUA_HPP
