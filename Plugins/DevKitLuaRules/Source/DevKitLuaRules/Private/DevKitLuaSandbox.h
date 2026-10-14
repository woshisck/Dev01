#ifndef DEVKIT_LUA_SANDBOX_H
#define DEVKIT_LUA_SANDBOX_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DevKitLuaNumber
{
    const char* name;
    double value;
} DevKitLuaNumber;

typedef struct DevKitLuaContext
{
    const char* const* owned_tags;
    size_t owned_count;
    const char* const* request_tags;
    size_t request_count;
    const DevKitLuaNumber* values;
    size_t value_count;
    const char* subject;
} DevKitLuaContext;

/* A NULL function_name validates the module shape without evaluating a condition. */
int DevKitLuaSandbox_Run(const char* source, size_t source_size,
    const char* function_name, const DevKitLuaContext* context,
    size_t memory_limit, int instruction_limit, int* match,
    char* error, size_t error_capacity);

/* On success, copies newline-delimited export identifiers without executing any predicate. */
int DevKitLuaSandbox_Validate(const char* source, size_t source_size,
    size_t memory_limit, int instruction_limit, char* names, size_t names_capacity,
    char* error, size_t error_capacity);

#ifdef __cplusplus
}
#endif
#endif
