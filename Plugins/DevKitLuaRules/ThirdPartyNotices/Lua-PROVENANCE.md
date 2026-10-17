# Lua provenance

- Version: **5.4.9**, pinned 5.4-series maintenance release.
- Official archive: <https://www.lua.org/ftp/lua-5.4.9.tar.gz>
- Official checksum index: <https://www.lua.org/ftp/>
- Archive SHA-256: `2335b6c582a52654f94612bf10d2f4672805d05329aa6568b1d8cd9e5c6fb8e6`
- Downloaded and independently SHA-256 verified: 2026-09-26.
- License: MIT; copied from the upstream `src/lua.h` copyright notice into `Lua-LICENSE.txt`, which is also staged with packaged builds.
- Reference API: <https://www.lua.org/manual/5.4/manual.html>

`Source/DevKitLuaRules/Private/ThirdParty/Lua` contains byte-for-byte upstream files: all `.h` files, README (renamed `README.upstream`), the Lua core C units, `lauxlib.c`, `lbaselib.c`, and `lmathlib.c`. No upstream source is modified. No interpreter/bytecode-compiler executable, IO, OS, package, debug, coroutine, string, table or UTF-8 library C unit is compiled.

The application adapter exposes only a whitelist of base functions and the mathematical library without random/randomseed. Text chunks only; binary bytecode is rejected. `luaL_openlibs` is never called. Dangerous base functions exist in the pristine upstream compilation unit but are not reachable from any script environment.

The module compiles the C source directly in both Editor and game targets, without Unreal Engine ScriptPlugin, an external interpreter, a dynamic Lua DLL or Editor module dependencies. Currently supported deployment platform: Win64.
