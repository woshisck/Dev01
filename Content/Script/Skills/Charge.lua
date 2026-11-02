--
-- Charge as a Lua enemy skill. Used through a UEnemySkill_Lua asset listed in UEnemyData.Skills under an
-- Enemy.Skill.SkillN slot; the StateTree fires it with Activate Ability By Tag on that same slot tag.
--
-- The enemy winds up facing the target, then charges in a straight line past where the target stood.
--   Hits the player  -> PinPlayer on:  the player is grabbed and carried in front of the enemy.
--                         Full distance covered -> the player is dropped there with TargetEffect, enemy plays Recover.
--                         Player driven into a wall -> both stop at the wall, player gets TargetEffect, enemy plays Recover.
--                       PinPlayer off: the player gets TargetEffect on contact, the enemy plays Recover.
--   Hits a wall      -> (nobody carried) the enemy gets SelfEffect (a stun) and the skill ends at once.
--   Hits nothing     -> the enemy plays Recover once the full distance is covered.
--
-- Montage (Asset.Montage) sections:
--   Windup      telegraph; keep its link to ChargeLoop so the animation never blends out between them
--   ChargeLoop  run cycle whose next section is itself, so it loops until the script jumps out
--   Recover     skid/stop; the skill ends when it blends out
--
-- The charge starts on the WindupEnd event, which the skill raises itself once the Windup section's length
-- has played (WindupTime overrides it). An "Enemy Skill Event" notify named WindupEnd inside Windup can fire
-- it earlier; whichever arrives first starts the charge, the other is ignored.
--
-- Tuning on the skill asset:
--   NumberParams
--     ChargeSpeed        cm/s                                                       (default 1200)
--     MaxChargeDistance  cm; the charge runs at most this far                       (default 1500)
--     WallHitDot         a blocking hit counts as a wall below this dot product      (default -0.7)
--     HitPadding         cm added to the capsule radius for the player-contact check (default 30)
--     WindupTime         seconds from activation to the charge; 0 = length of the Windup section (default 0)
--     PinPlayer          1 = grab and carry the player to the end of the charge, 0 = stop on contact (default 1)
--   EffectParams
--     TargetEffect       applied to the player on a hit (default: native GE_Stun, 1.5 s)
--     SelfEffect         applied to the enemy when it hits a wall, e.g. GE_Stun
--

local M = {}

-- [Skill] = true once the charge has started, so a notify and the timer cannot both start it.
local Charging = setmetatable({}, { __mode = "k" })

local function Param(Asset, Name, Default)
    local Value = Asset.NumberParams:Find(Name)
    if Value == nil then
        return Default
    end
    return Value
end

local function HitTarget(Asset, Skill, Target)
    if not Target then
        return
    end
    -- Native GE_Stun (Buff.Status.Stunned, 1.5 s) unless the asset picks its own TargetEffect.
    local Effect = Asset.EffectParams:Find("TargetEffect") or UE.UGE_Stun
    Skill:ApplyEffectToActor(Target, Effect, 1)
end

local function Recover(Skill)
    if Skill:GetCurrentMontage() then
        Skill:JumpToSection("Recover")
    else
        Skill:EndSkill()
    end
end

function M.OnActivate(Skill, Owner, Asset)
    Charging[Skill] = nil
    Skill:FaceTarget()
    if not Skill:PlaySkillMontage("Windup") then
        Skill:EndSkill()
        return
    end

    local WindupTime = Param(Asset, "WindupTime", 0)
    if WindupTime <= 0 then
        WindupTime = Skill:GetSectionLength("Windup")
    end
    Skill:RaiseSkillEventAfter("WindupEnd", WindupTime)
end

function M.OnEvent(Skill, Owner, Asset, EventName, Other)
    if EventName == "WindupEnd" then
        if Charging[Skill] then
            return
        end
        Charging[Skill] = true
        Skill:JumpToSection("ChargeLoop")
        local bStarted = Skill:StartChargeMove(
            Param(Asset, "ChargeSpeed", 1200),
            Param(Asset, "MaxChargeDistance", 1500),
            Param(Asset, "WallHitDot", -0.7),
            Param(Asset, "HitPadding", 30),
            Param(Asset, "PinPlayer", 1) > 0)
        if not bStarted then
            Recover(Skill)
        end

    elseif EventName == "HitPlayer" then
        HitTarget(Asset, Skill, Other)
        Recover(Skill)

    elseif EventName == "GrabPlayer" then
        -- The charge carries on; the stun lands when the player is released.

    elseif EventName == "HitWall" then
        if Other then
            -- Other is the carried player, crushed against the wall ahead of the enemy.
            HitTarget(Asset, Skill, Other)
            Recover(Skill)
            return
        end
        -- End first so the stun's own reaction is not cut off by this skill's montage.
        Skill:EndSkill()
        local Effect = Asset.EffectParams:Find("SelfEffect")
        if Effect then
            Skill:ApplyEffectToActor(Owner, Effect, 1)
        end

    elseif EventName == "MoveFinished" then
        if Other then
            HitTarget(Asset, Skill, Other)
        end
        Recover(Skill)

    elseif EventName == "MontageEnded" or EventName == "MontageInterrupted" then
        Skill:EndSkill()
    end
end

function M.OnEnd(Skill, Owner, Asset, bWasCancelled)
    Charging[Skill] = nil
end

return M
