Yes. The clean setup is:

**Asset Chain**
`WeaponDefinition`
→ `WeaponComboConfig`
→ combo node
→ `MontageConfig`
→ `AnimMontage`

For the new system, the important field is **each combo node’s `MontageConfig`**. That is what tells the selected combo step which montage to play.

**Example: Light / Heavy**
Create a `WeaponComboConfigDA` like this:

| NodeId | ParentNodeId | InputAction | AbilityTag | MontageConfig |
|---|---|---|---|---|
| `L1` | none | `Light` | `PlayerState.AbilityCast.LightAtk.Combo1` | `MC_L1` |
| `LH` | `L1` | `Heavy` | `PlayerState.AbilityCast.HeavyAtk.Combo2` | `MC_LH` |

**Example: Light / Light / Heavy**
Add:

| NodeId | ParentNodeId | InputAction | AbilityTag | MontageConfig |
|---|---|---|---|---|
| `LL` | `L1` | `Light` | `PlayerState.AbilityCast.LightAtk.Combo2` | `MC_LL` |
| `LLH` | `LL` | `Heavy` | `PlayerState.AbilityCast.HeavyAtk.Combo3` | `MC_LLH` |

Set `RootNodes = [L1]`.

**MontageConfig Setup**
For each `MC_*` asset:

1. Set `Montage` to the animation montage you want.
2. Set `TotalFrames`.
3. Add `Entries`:
   - `Hit Detection Window`: damage active frames.
   - `Combo Window`: frames where next input is allowed.
   - Optional `Early Exit`.
   - Optional `Gameplay Tag Window`.

If `L1` should branch into `LH` or `LL`, `MC_L1` must have a `Combo Window`. If the player presses heavy during that window, runtime finds child `LH`. If they press light, runtime finds child `LL`.

**Weapon Setup**
In your `WeaponDefinition` asset:

1. Set `WeaponType = Melee`.
2. Set `WeaponAbilityData` to your player ability/montage data asset.
3. Set `WeaponComboConfig` to the combo config above.
4. Make sure the weapon is equipped through the normal weapon setup path, because that calls `ComboRuntime->LoadComboConfig(WeaponComboConfig)`.

The montage priority is:

1. Combo node `MontageConfig`
2. AbilityData `MontageConfigMap`
3. AbilityData old `MontageMap`

So for this new combo system, prefer assigning montage configs directly on the combo nodes. That makes `Light/Heavy` and `Light/Light/Heavy` explicit and easy to read.