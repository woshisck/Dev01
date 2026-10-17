/* Compiles the exact shipping C adapter + pristine Lua units; no UE or external Lua executable. */
#include "DevKitLuaSandbox.h"
#include <stdio.h>
#include <string.h>

static int failures;
static int checks;

static void check(const char* name, int passed)
{
    ++checks;
    if (!passed) ++failures;
    printf("%s %s\n", passed ? "PASS" : "FAIL", name);
}

static int run(const char* source, const char* function, const DevKitLuaContext* context, int* match, char* error)
{
    return DevKitLuaSandbox_Run(source, strlen(source), function, context, 4u * 1024u * 1024u,
        100000, match, error, 1024);
}

int main(void)
{
    const char* owned[] = { "Character.State.Skill.Dash" };
    const char* requested[] = { "Character.State.Skill.Attack" };
    const DevKitLuaNumber values[] = { { "stamina", 25.0 } };
    const DevKitLuaContext context = { owned, 1, requested, 1, values, 1, "Player" };
    int match = 0;
    char error[1024];
    char names[256 * 65 + 1];
    const char* basic = "local n=0; return {functions={ok=function(ctx) n=n+1; g=(g or 0)+1; "
        "return n==1 and g==1 and ctx.owned_tags['Character.State.Skill.Dash']==true "
        "and ctx.request_tags['Character.State.Skill.Attack']==true and ctx.values.stamina==25 "
        "and ctx.subject=='Player' and math.floor(2.5)==2 end}}";
    const char* export_source = "return {functions={must_not_run=function(ctx) error('bad') end}}";
    check("valid module", run(basic, NULL, NULL, &match, error));
    check("export metadata without predicate evaluation", DevKitLuaSandbox_Validate(
        export_source, strlen(export_source),
        4u * 1024u * 1024u, 100000, names, sizeof(names), error, sizeof(error))
        && strstr(names, "must_not_run\n"));
    check("context and math", run(basic, "ok", &context, &match, error) && match);
    check("state isolated", run(basic, "ok", &context, &match, error) && match);
    check("missing export", !run(basic, "missing", &context, &match, error) && !match);
    check("syntax error", !run("return { broken", NULL, NULL, &match, error));
    check("bad module shape", !run("return true", NULL, NULL, &match, error));
    check("bad export shape", !run("return {functions={x=1}}", NULL, NULL, &match, error));
    check("invalid name", !run("return {functions={['not-valid']=function(ctx) return true end}}", NULL, NULL, &match, error));
    check("C export rejected", !run("return {functions={x=type}}", NULL, NULL, &match, error));
    check("boolean false valid", run("return {functions={x=function(ctx) return false end}}", "x", &context, &match, error) && !match);
    check("nonboolean rejected", !run("return {functions={x=function(ctx) return 1 end}}", "x", &context, &match, error));
    check("runtime error", !run("return {functions={x=function(ctx) error('fault') end}}", "x", &context, &match, error) && strstr(error, "fault"));
    check("nonstring error", !run("return {functions={x=function(ctx) error({}) end}}", "x", &context, &match, error) && strstr(error, "non-string"));
    check("load instruction quota", !run("while true do end", NULL, NULL, &match, error) && strstr(error, "instruction budget"));
    check("eval instruction quota", !run("return {functions={x=function(ctx) while true do end end}}", "x", &context, &match, error) && strstr(error, "instruction budget"));
    check("allocation quota", !run("return {functions={x=function(ctx) local s='12345678'; for i=1,30 do s=s..s end return true end}}", "x", &context, &match, error) && strstr(error, "memory"));
    check("binary chunk rejected", !run("\x1bLua", NULL, NULL, &match, error));
    check("restricted capabilities", run("return {functions={x=function(ctx) return io==nil and os==nil and debug==nil "
        "and package==nil and require==nil and load==nil and loadfile==nil and dofile==nil "
        "and pcall==nil and xpcall==nil and coroutine==nil and collectgarbage==nil "
        "and rawset==nil and setmetatable==nil and getmetatable==nil and string==nil "
        "and table==nil and print==nil and _G==nil and math.random==nil and math.randomseed==nil end}}",
        "x", &context, &match, error) && match);
    check("healthy after failures", run(basic, "ok", &context, &match, error) && match);
    printf("%d checks; %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
