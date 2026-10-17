# DevKit Lua Rules

Project-owned, Win64 Runtime plugin with a Core-only API. This is a restricted **condition evaluator**, not a general-purpose Unreal scripting engine. Add `DevKitLuaRules` to the caller module dependencies and enable the project plugin.

```lua
return { functions = {
  low_stamina = function(ctx)
    return ctx.values.stamina < 20
  end,
  attack_during_dash = function(ctx)
    return ctx.owned_tags["Character.State.Movement.Dash"] == true
      and ctx.request_tags["Character.State.Skill.Attack"] == true
  end
} }
```

Use `FDevKitLuaRuleVM::LoadSource` once when activating a validated rule bundle. Failed loads preserve the previous validated source. `EvaluateCondition` returns **execution success**, while its `bMatch` output is the boolean decision. Callers must handle an execution failure as an explicit policy error; never confuse it with a condition that successfully returned false.

## Context

- `owned_tags`: exact string-keyed set, values `true`; caller decides whether ancestor tags are included.
- `request_tags`: same representation for the requested action.
- `values`: string-keyed finite numeric values; keys are ASCII identifiers. No automatic UObject/attribute access.
- `subject`: subject classification string, for example `player`.

Every context is a copied value snapshot. Mutating a Lua context cannot mutate the caller. Every evaluation constructs and closes a fresh VM and executes the source again, so closures, globals and nested tables cannot leak between requests or different characters. Calls on the same holder are serialized. Do not place stateful gameplay or per-frame work here: use event-driven conditions; C++ owns all action execution and lock lifetimes. Source parsing per evaluation is a deliberate v1 safety tradeoff and must be profiled before high-volume AI use.

## Limits and available functions

- 64 KiB UTF-8 text source; 4 MiB total Lua allocator budget; 100,000 VM instructions per load/evaluation, including module initialization.
- 256 exported functions, ASCII identifier names (1–64 characters).
- Up to 512 owned and 512 requested tags (512 characters each), 128 finite named numbers, 128-character subject.
- Base functions: `assert`, `error`, `ipairs`, `select`, `tonumber`, `type`.
- `math` functions/constants except `random` and `randomseed`.
- No filesystem, network, UObject, clock, environment, module loading, bytecode input, printing, metatable API, coroutine, `pcall` or `xpcall` access. Removing protected-call functions prevents script code from swallowing quota errors and looping forever.
- No string-pattern/table standard library loops that could run outside the VM instruction hook. Ordinary Lua operators, loops, local tables and string concatenation remain available and budgeted.

The instruction hook is a computation budget, not a hard wall-clock guarantee. Parser/GC/C numeric calls are not individually hook-counted; bounded inputs and restricted APIs reduce their cost. This is for trusted team-authored rules, not a hostile-code security boundary. Shipping uses the same interpreter; development hot reload and packaged source staging belong to the host rule-bundle subsystem.

## Tests

Unreal Automation filter: `DevKit.ActionRules.Lua`.
Tests cover actual execution, context values, cross-call state isolation, failed reload rollback, syntax/runtime/type errors, load/evaluation infinite loops, allocation limits, and capability restrictions. Engine/asset/PIE behavior is validated separately by the host action framework.

Independent C-adapter smoke (MSVC/Windows SDK required): `powershell -ExecutionPolicy Bypass -File Plugins/DevKitLuaRules/Tests/Standalone/Run-Smoke.ps1`. This compiles the exact adapter and vendored runtime and checks 20 conditions; it does not replace the Unreal wrapper/packaging tests. Generated binary, objects and `smoke.log` stay in the plugin's ignored `Intermediate/Standalone` folder.
