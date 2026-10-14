/* Keep all Lua longjmp frames in C. No Unreal allocation or C++ destructor crosses this boundary. */
#include "DevKitLuaSandbox.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include <stdlib.h>
#include <string.h>

typedef struct DevKitLuaCall
{
    size_t bytes_used;
    size_t memory_limit;
    int instructions_left;
    const char* source;
    size_t source_size;
    const char* function_name;
    const DevKitLuaContext* context;
    char* names;
    size_t names_capacity;
    size_t names_used;
} DevKitLuaCall;

static void* DevKitLua_Alloc(void* user, void* pointer, size_t old_size, size_t new_size)
{
    DevKitLuaCall* call = (DevKitLuaCall*)user;
    void* replacement;
    if (!pointer) old_size = 0; /* Lua supplies a type tag, not an allocation size, for NULL. */
    if (new_size == 0)
    {
        free(pointer);
        call->bytes_used -= old_size;
        return NULL;
    }
    if (old_size > call->bytes_used || new_size > call->memory_limit - (call->bytes_used - old_size))
        return NULL;
    replacement = realloc(pointer, new_size);
    if (replacement)
        call->bytes_used = call->bytes_used - old_size + new_size;
    return replacement;
}

static void DevKitLua_CountHook(lua_State* state, lua_Debug* info)
{
    DevKitLuaCall* call;
    (void)info;
    lua_getallocf(state, (void**)&call);
    call->instructions_left -= 100;
    if (call->instructions_left <= 0)
        luaL_error(state, "Lua rule instruction budget exceeded");
}

static int DevKitLua_ValidName(const char* name, size_t length)
{
    size_t index;
    if (!name || length == 0 || length > 64) return 0;
    for (index = 0; index < length; ++index)
    {
        const char value = name[index];
        if (!((value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z') || value == '_'
            || (index > 0 && value >= '0' && value <= '9'))) return 0;
    }
    return 1;
}

static void DevKitLua_OpenEnvironment(lua_State* state)
{
    static const char* const allowed_base[] = { "assert", "error", "ipairs", "select", "tonumber", "type", NULL };
    int index;
    /* Copy an explicit whitelist into a fresh environment; never call luaL_openlibs. */
    luaopen_base(state);
    lua_newtable(state);
    for (index = 0; allowed_base[index]; ++index)
    {
        lua_getfield(state, -2, allowed_base[index]);
        lua_setfield(state, -2, allowed_base[index]);
    }
    luaopen_math(state);
    lua_pushnil(state);
    lua_setfield(state, -2, "random");
    lua_pushnil(state);
    lua_setfield(state, -2, "randomseed");
    lua_setfield(state, -2, "math");
    /* Replace the registry global table too, so no hidden fallback can reach the original base table. */
    lua_pushvalue(state, -1);
    lua_rawseti(state, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
    lua_remove(state, -2);
}

static void DevKitLua_PushTagSet(lua_State* state, const char* const* tags, size_t count)
{
    size_t index;
    lua_createtable(state, 0, (int)count);
    for (index = 0; index < count; ++index)
    {
        lua_pushboolean(state, 1);
        lua_setfield(state, -2, tags[index]);
    }
}

static void DevKitLua_PushContext(lua_State* state, const DevKitLuaContext* context)
{
    size_t index;
    lua_createtable(state, 0, 4);
    DevKitLua_PushTagSet(state, context->owned_tags, context->owned_count);
    lua_setfield(state, -2, "owned_tags");
    DevKitLua_PushTagSet(state, context->request_tags, context->request_count);
    lua_setfield(state, -2, "request_tags");
    lua_createtable(state, 0, (int)context->value_count);
    for (index = 0; index < context->value_count; ++index)
    {
        lua_pushnumber(state, context->values[index].value);
        lua_setfield(state, -2, context->values[index].name);
    }
    lua_setfield(state, -2, "values");
    lua_pushstring(state, context->subject);
    lua_setfield(state, -2, "subject");
}

static int DevKitLua_ProtectedRun(lua_State* state)
{
    DevKitLuaCall* call;
    size_t function_count = 0;
    lua_getallocf(state, (void**)&call);
    DevKitLua_OpenEnvironment(state);
    if (luaL_loadbufferx(state, call->source, call->source_size, "@ActionRules", "t") != LUA_OK)
        return lua_error(state);
    lua_pushvalue(state, -2);
    lua_setupvalue(state, -2, 1); /* The text chunk gets only the whitelist environment. */
    lua_call(state, 0, 1);
    if (!lua_istable(state, -1)) return luaL_error(state, "Rule module must return a table");
    lua_getfield(state, -1, "functions");
    if (!lua_istable(state, -1)) return luaL_error(state, "Rule module must export a functions table");
    lua_pushnil(state);
    while (lua_next(state, -2))
    {
        size_t name_length = 0;
        const char* name = lua_type(state, -2) == LUA_TSTRING ? lua_tolstring(state, -2, &name_length) : NULL;
        if (!DevKitLua_ValidName(name, name_length) || !lua_isfunction(state, -1) || lua_iscfunction(state, -1))
            return luaL_error(state, "Every condition must be a named Lua function (ASCII identifier, max 64 characters)");
        if (++function_count > 256) return luaL_error(state, "Too many Lua conditions (maximum 256)");
        if (call->names)
        {
            if (call->names_used + name_length + 2 > call->names_capacity)
                return luaL_error(state, "Condition export buffer is too small");
            memcpy(call->names + call->names_used, name, name_length);
            call->names_used += name_length;
            call->names[call->names_used++] = '\n';
            call->names[call->names_used] = '\0';
        }
        lua_pop(state, 1);
    }
    if (!call->function_name)
    {
        lua_pushboolean(state, 1);
        return 1;
    }
    lua_getfield(state, -1, call->function_name);
    if (!lua_isfunction(state, -1)) return luaL_error(state, "Condition '%s' was not exported", call->function_name);
    DevKitLua_PushContext(state, call->context);
    lua_call(state, 1, 1);
    if (lua_type(state, -1) != LUA_TBOOLEAN) return luaL_error(state, "Condition '%s' must return a boolean", call->function_name);
    return 1;
}

static void DevKitLua_CopyError(char* destination, size_t capacity, const char* message)
{
    size_t length;
    if (!destination || capacity == 0) return;
    length = strlen(message);
    if (length >= capacity) length = capacity - 1;
    memcpy(destination, message, length);
    destination[length] = '\0';
}

static int DevKitLuaSandbox_RunInternal(const char* source, size_t source_size,
    const char* function_name, const DevKitLuaContext* context,
    size_t memory_limit, int instruction_limit, int* match, char* names, size_t names_capacity,
    char* error, size_t error_capacity)
{
    DevKitLuaCall call;
    lua_State* state;
    int status;
    memset(&call, 0, sizeof(call));
    call.memory_limit = memory_limit;
    call.instructions_left = instruction_limit;
    call.source = source;
    call.source_size = source_size;
    call.function_name = function_name;
    call.context = context;
    call.names = names;
    call.names_capacity = names_capacity;
    if (names && names_capacity) names[0] = '\0';
    *match = 0;
    if (error_capacity) error[0] = '\0';
    state = lua_newstate(DevKitLua_Alloc, &call);
    if (!state)
    {
        DevKitLua_CopyError(error, error_capacity, "Unable to allocate bounded Lua state");
        return 0;
    }
    lua_sethook(state, DevKitLua_CountHook, LUA_MASKCOUNT, 100);
    /* Zero-upvalue C functions need no allocation. All other fallible operations are protected. */
    lua_pushcfunction(state, DevKitLua_ProtectedRun);
    status = lua_pcall(state, 0, 1, 0);
    if (status == LUA_OK)
        *match = lua_toboolean(state, -1);
    else
    {
        const char* message = lua_type(state, -1) == LUA_TSTRING ? lua_tostring(state, -1) : NULL;
        DevKitLua_CopyError(error, error_capacity, message ? message : "Lua rule raised a non-string error");
    }
    lua_close(state);
    return status == LUA_OK;
}

int DevKitLuaSandbox_Run(const char* source, size_t source_size,
    const char* function_name, const DevKitLuaContext* context,
    size_t memory_limit, int instruction_limit, int* match,
    char* error, size_t error_capacity)
{
    return DevKitLuaSandbox_RunInternal(source, source_size, function_name, context,
        memory_limit, instruction_limit, match, NULL, 0, error, error_capacity);
}

int DevKitLuaSandbox_Validate(const char* source, size_t source_size,
    size_t memory_limit, int instruction_limit, char* names, size_t names_capacity,
    char* error, size_t error_capacity)
{
    int match;
    return DevKitLuaSandbox_RunInternal(source, source_size, NULL, NULL,
        memory_limit, instruction_limit, &match, names, names_capacity, error, error_capacity);
}
