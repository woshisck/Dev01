#include "DevKitLuaRuleVM.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitLuaConditionTest, "DevKit.ActionRules.Lua.ConditionAndIsolation",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FDevKitLuaConditionTest::RunTest(const FString& Parameters)
{
    FDevKitLuaRuleVM VM;
    FString Error;
    bool bMatch = false;
    FDevKitLuaRuleContext Context;
    Context.OwnedTags.Add(TEXT("Character.State.Skill.Dash"));
    Context.RequestTags.Add(TEXT("Character.State.Skill.Attack"));
    Context.Values.Add(TEXT("stamina"), 25.0);
    Context.Subject = TEXT("Player");
    TestTrue(TEXT("Load actual Lua module"), VM.LoadSource(TEXT(R"lua(
local count = 0
return { functions = {
  stamina_gate = function(ctx)
    count = count + 1
    global_count = (global_count or 0) + 1
    return count == 1 and global_count == 1 and ctx.subject == "Player"
      and ctx.owned_tags["Character.State.Skill.Dash"] == true
      and ctx.request_tags["Character.State.Skill.Attack"] == true
      and math.floor(ctx.values.stamina) >= 20
  end
} }
)lua"), Error));
    TestTrue(TEXT("Evaluate"), VM.EvaluateCondition(TEXT("stamina_gate"), Context, bMatch, Error));
    TestTrue(TEXT("Expected match"), bMatch);
    TestTrue(TEXT("Evaluate fresh state"), VM.EvaluateCondition(TEXT("stamina_gate"), Context, bMatch, Error));
    TestTrue(TEXT("Closures and globals never leak across calls"), bMatch);
    Context.Values[TEXT("stamina")] = 0.0;
    TestTrue(TEXT("False is a valid condition result"), VM.EvaluateCondition(TEXT("stamina_gate"), Context, bMatch, Error));
    TestFalse(TEXT("Condition does not match"), bMatch);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitLuaFailuresTest, "DevKit.ActionRules.Lua.ErrorsAndReloadRollback",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FDevKitLuaFailuresTest::RunTest(const FString& Parameters)
{
    FDevKitLuaRuleVM VM;
    FString Error;
    bool bMatch = true;
    const FDevKitLuaRuleContext Context;
    TestFalse(TEXT("Unloaded evaluation fails"), VM.EvaluateCondition(TEXT("ok"), Context, bMatch, Error));
    TestFalse(TEXT("Failure clears output"), bMatch);
    TestTrue(TEXT("Load baseline"), VM.LoadSource(TEXT("return {functions={ok=function(ctx) return true end}}"), Error));
    TestTrue(TEXT("Export cached"), VM.HasCondition(TEXT("ok")));
    TestFalse(TEXT("Unknown export absent"), VM.HasCondition(TEXT("missing")));
    TestFalse(TEXT("Reject syntax error"), VM.LoadSource(TEXT("return {broken"), Error));
    TestTrue(TEXT("Previous valid source retained"), VM.EvaluateCondition(TEXT("ok"), Context, bMatch, Error));
    TestTrue(TEXT("Old result intact"), bMatch);
    TestTrue(TEXT("Old exports retained"), VM.HasCondition(TEXT("ok")));
    TestFalse(TEXT("Reject non-module"), VM.LoadSource(TEXT("return true"), Error));
    TestFalse(TEXT("Reject non-function export"), VM.LoadSource(TEXT("return {functions={oops=123}}"), Error));
    TestFalse(TEXT("Reject C function export"), VM.LoadSource(TEXT("return {functions={oops=type}}"), Error));
    TestFalse(TEXT("Missing function fails"), VM.EvaluateCondition(TEXT("missing"), Context, bMatch, Error));
    TestTrue(TEXT("Load runtime error"), VM.LoadSource(TEXT("return {functions={oops=function(ctx) error('expected fault') end}}"), Error));
    TestTrue(TEXT("Export validation does not execute condition"), VM.HasCondition(TEXT("oops")));
    TestFalse(TEXT("Successful reload replaces exports"), VM.HasCondition(TEXT("ok")));
    TestFalse(TEXT("Runtime error protected"), VM.EvaluateCondition(TEXT("oops"), Context, bMatch, Error));
    TestTrue(TEXT("Error contains cause"), Error.Contains(TEXT("expected fault")));
    TestTrue(TEXT("Load bad return"), VM.LoadSource(TEXT("return {functions={oops=function(ctx) return 1 end}}"), Error));
    TestFalse(TEXT("Reject truthy non-boolean"), VM.EvaluateCondition(TEXT("oops"), Context, bMatch, Error));
    TestFalse(TEXT("Reject excessive source"), VM.LoadSource(FString::ChrN(FDevKitLuaRuleVM::MaxSourceBytes + 1, TEXT(' ')), Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitLuaLimitsTest, "DevKit.ActionRules.Lua.BudgetsAndCapabilities",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FDevKitLuaLimitsTest::RunTest(const FString& Parameters)
{
    FDevKitLuaRuleVM VM;
    FString Error;
    bool bMatch = false;
    const FDevKitLuaRuleContext Context;
    TestFalse(TEXT("Top-level infinite loop bounded"), VM.LoadSource(TEXT("while true do end"), Error));
    TestTrue(TEXT("Loop diagnostic"), Error.Contains(TEXT("instruction budget")));
    TestTrue(TEXT("Load looping condition"), VM.LoadSource(TEXT("return {functions={loop=function(ctx) while true do end end}}"), Error));
    TestFalse(TEXT("Condition infinite loop bounded"), VM.EvaluateCondition(TEXT("loop"), Context, bMatch, Error));
    TestTrue(TEXT("Load allocation stress"), VM.LoadSource(TEXT("return {functions={oom=function(ctx) local s='12345678'; for i=1,30 do s=s..s end return true end}}"), Error));
    TestFalse(TEXT("Allocation bounded"), VM.EvaluateCondition(TEXT("oom"), Context, bMatch, Error));
    TestTrue(TEXT("Memory diagnostic"), Error.Contains(TEXT("memory")));
    TestTrue(TEXT("Load capability probe"), VM.LoadSource(TEXT(R"lua(
return {functions={safe=function(ctx)
  return io==nil and os==nil and package==nil and debug==nil and require==nil
    and load==nil and loadfile==nil and dofile==nil and pcall==nil and xpcall==nil
    and coroutine==nil and collectgarbage==nil and rawset==nil and setmetatable==nil
    and getmetatable==nil and string==nil and table==nil and print==nil and _G==nil
    and math.random==nil and math.randomseed==nil
end}}
)lua"), Error));
    TestTrue(TEXT("Capability probe executes"), VM.EvaluateCondition(TEXT("safe"), Context, bMatch, Error));
    TestTrue(TEXT("No unsafe capabilities exposed"), bMatch);
    return true;
}

#endif
