#ifndef OPENMW_LUAIO2_HPP
#define OPENMW_LUAIO2_HPP

struct lua_State;

// The "io2" module CoreScripts loads on Windows, built in: Lua's own io.open takes the path in the system code page,
// so files named after players with non-ASCII names can't be opened. This open() takes UTF-8 paths. Its files offer
// what the scripts use: read ("*a", "*l" or a byte count), write, lines and close.
// (TES3MP shipped a separate io2.dll for this.)
void registerLuaIo2(lua_State *L);

#endif //OPENMW_LUAIO2_HPP
