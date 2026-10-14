-- Not mounted by default. Conditions are pure predicates, not arbitrary ability mutation.
return {
    functions = {
        player_only = function(ctx)
            return ctx.subject == "player"
        end,
        has_stamina = function(ctx)
            return (ctx.values["stamina"] or 0) >= (ctx.values["requested_cost"] or 0)
        end,
        in_cancel_window = function(ctx)
            return ctx.owned_tags["PlayerState.AbilityCast.CanCombo"] == true
        end
    }
}
