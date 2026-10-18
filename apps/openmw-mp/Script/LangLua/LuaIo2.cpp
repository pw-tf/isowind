#include "LuaIo2.hpp"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>

#include "lua.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
    const char *const fileType = "io2.file";

    FILE *openUtf8(const char *path, const char *mode)
    {
#ifdef _WIN32
        auto widen = [](const char *text) {
            int size = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
            std::wstring wide(size > 0 ? size - 1 : 0, L'\0');
            if (size > 1)
                MultiByteToWideChar(CP_UTF8, 0, text, -1, wide.data(), size);
            return wide;
        };
        return _wfopen(widen(path).c_str(), widen(mode).c_str());
#else
        return std::fopen(path, mode);
#endif
    }

    FILE **toFile(lua_State *L)
    {
        return static_cast<FILE **>(luaL_checkudata(L, 1, fileType));
    }

    FILE *openFile(lua_State *L)
    {
        FILE *file = *toFile(L);
        if (!file)
            luaL_error(L, "attempt to use a closed file");
        return file;
    }

    // Reads the rest of the file; never fails (an empty string at the end).
    void pushAll(lua_State *L, FILE *file)
    {
        luaL_Buffer buffer;
        luaL_buffinit(L, &buffer);
        char chunk[LUAL_BUFFERSIZE];
        size_t count;
        while ((count = std::fread(chunk, 1, sizeof(chunk), file)) > 0)
            luaL_addlstring(&buffer, chunk, count);
        luaL_pushresult(&buffer);
    }

    // Pushes the next line without its newline, or returns false at the end of the file.
    bool pushLine(lua_State *L, FILE *file)
    {
        std::string line;
        int c;
        while ((c = std::fgetc(file)) != EOF && c != '\n')
            line.push_back(static_cast<char>(c));
        if (c == EOF && line.empty())
            return false;
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        lua_pushlstring(L, line.data(), line.size());
        return true;
    }

    int fileRead(lua_State *L)
    {
        FILE *file = openFile(L);
        int last = lua_gettop(L);
        if (last < 2)
            return pushLine(L, file) ? 1 : (lua_pushnil(L), 1);

        for (int arg = 2; arg <= last; ++arg)
        {
            if (lua_type(L, arg) == LUA_TNUMBER)
            {
                size_t wanted = static_cast<size_t>(lua_tointeger(L, arg));
                std::string data(wanted, '\0');
                size_t count = std::fread(data.data(), 1, wanted, file);
                if (count == 0 && wanted > 0)
                {
                    lua_pushnil(L);
                    return arg - 1;
                }
                lua_pushlstring(L, data.data(), count);
                continue;
            }
            const char *format = luaL_checkstring(L, arg);
            if (format[0] == '*')
                ++format;
            if (format[0] == 'a')
                pushAll(L, file);
            else if (format[0] == 'l')
            {
                if (!pushLine(L, file))
                {
                    lua_pushnil(L);
                    return arg - 1;
                }
            }
            else
                return luaL_argerror(L, arg, "invalid format");
        }
        return last - 1;
    }

    int fileWrite(lua_State *L)
    {
        FILE *file = openFile(L);
        int last = lua_gettop(L);
        for (int arg = 2; arg <= last; ++arg)
        {
            size_t size;
            const char *text = luaL_checklstring(L, arg, &size);
            if (std::fwrite(text, 1, size, file) != size)
            {
                lua_pushnil(L);
                lua_pushstring(L, std::strerror(errno));
                return 2;
            }
        }
        lua_settop(L, 1);
        return 1;
    }

    int fileClose(lua_State *L)
    {
        FILE **file = toFile(L);
        if (*file)
        {
            std::fclose(*file);
            *file = nullptr;
        }
        lua_pushboolean(L, 1);
        return 1;
    }

    int linesNext(lua_State *L)
    {
        FILE **file = static_cast<FILE **>(lua_touserdata(L, lua_upvalueindex(1)));
        if (!*file || !pushLine(L, *file))
            return 0;
        return 1;
    }

    int fileLines(lua_State *L)
    {
        openFile(L);
        lua_pushvalue(L, 1);
        lua_pushcclosure(L, linesNext, 1);
        return 1;
    }

    int io2Open(lua_State *L)
    {
        const char *path = luaL_checkstring(L, 1);
        const char *mode = luaL_optstring(L, 2, "r");

        FILE **file = static_cast<FILE **>(lua_newuserdata(L, sizeof(FILE *)));
        *file = nullptr;
        luaL_getmetatable(L, fileType);
        lua_setmetatable(L, -2);

        // Binary, so files read back exactly as written (the scripts write with "w+b" but read with "r").
        std::string binaryMode = mode;
        if (binaryMode.find('b') == std::string::npos)
            binaryMode.push_back('b');
        *file = openUtf8(path, binaryMode.c_str());
        if (!*file)
        {
            lua_pushnil(L);
            lua_pushfstring(L, "%s: %s", path, std::strerror(errno));
            return 2;
        }
        return 1;
    }

    int luaopenIo2(lua_State *L)
    {
        if (luaL_newmetatable(L, fileType))
        {
            static const luaL_Reg methods[] = { { "read", fileRead }, { "write", fileWrite }, { "lines", fileLines },
                { "close", fileClose }, { nullptr, nullptr } };
            lua_newtable(L);
            luaL_register(L, nullptr, methods);
            lua_setfield(L, -2, "__index");
            lua_pushcfunction(L, fileClose);
            lua_setfield(L, -2, "__gc");
        }
        lua_pop(L, 1);

        lua_newtable(L);
        lua_pushcfunction(L, io2Open);
        lua_setfield(L, -2, "open");
        return 1;
    }
}

void registerLuaIo2(lua_State *L)
{
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "preload");
    lua_pushcfunction(L, luaopenIo2);
    lua_setfield(L, -2, "io2");
    lua_pop(L, 2);
}
