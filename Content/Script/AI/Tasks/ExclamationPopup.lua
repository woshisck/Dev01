--
-- Pops a "!" bubble above the enemy once, when the state this task sits on is entered.
-- Used by a "Call Lua" StateTree task placed on the Combat state of ST_Rat.
--
-- Uses UYogBubbleSubsystem (the same world-anchored bubble the story system uses).
-- OnEnter returns nothing on purpose: the task stays Running for the whole state, so it
-- never completes the Combat state by itself.
--

local M = {}

local TEXT = "!"
local DURATION = 1.0
local PRIORITY = 60

local function ResolvePawn(Owner)
    if Owner:IsA(UE.AController) then
        return Owner:K2_GetPawn()
    end
    return Owner
end

function M.OnEnter(Owner)
    local Pawn = ResolvePawn(Owner)
    if not Pawn then
        return false
    end

    local Bubble = UE.USubsystemBlueprintLibrary.GetWorldSubsystem(Pawn, UE.UYogBubbleSubsystem)
    if not Bubble then
        return false
    end

    Bubble:RequestBubbleText(
        Pawn,
        UE.UKismetTextLibrary.Conv_StringToText(""),
        UE.UKismetTextLibrary.Conv_StringToText(TEXT),
        DURATION,
        UE.EBubbleAnchorMode.WorldOnly,
        PRIORITY,
        UE.FGameplayTag())
end

function M.OnExit(Owner)
    local Pawn = ResolvePawn(Owner)
    if not Pawn then
        return
    end

    local Bubble = UE.USubsystemBlueprintLibrary.GetWorldSubsystem(Pawn, UE.UYogBubbleSubsystem)
    if Bubble then
        Bubble:DismissBubbleFor(Pawn)
    end
end

return M
