> 状态：归档。仅用于历史追溯，不作为当前实现依据。

# Valve-style water implementation contract

This is the first runtime implementation pass for cheap and interactive water.

## Runtime classes

- `ACheapWaterSurfaceActor`: static/decal-like water plane. It never receives gameplay impulses.
- `AWaterCausticsDecalActor`: standalone caustics decal actor for cheap water or floor-only caustics.
- `AInteractiveWaterVolume`: local expensive water tile. It owns the surface mesh, interaction bounds, optional caustics decal, runtime interaction render targets, and Niagara splash hooks.
- `UWaterInteractionBlueprintLibrary`: gameplay entry point for footstep, dash, explosion, falling-mesh, and magic/VFX impulses.

## Material parameter contract

Both cheap and interactive water materials should support:

- `BaseWaterColor`, `DepthColor`, `Opacity`, `Roughness`, `SpecularIntensity`, `WaveScale`
- `AttachmentLayer1Texture`, `AttachmentLayer1Tint`, `AttachmentLayer1Intensity`, `AttachmentLayer1TilingSpeed`, `AttachmentLayer1RoughnessInfluence`, `AttachmentLayer1Displaceable`
- Same fields for `AttachmentLayer2` and `AttachmentLayer3`

Interactive water additionally receives:

- `WaterInteractionRT`: RGBA runtime texture. Recommended channel use: R = ripple height, G = foam, B = turbidity, A = debris displacement/glint seed.
- `WaterInteractionWorldBounds`: XY = world min, ZW = world size for mapping world XY to RT UVs.
- `WaterInteractionRTSize`
- `ActiveWaterImpulseCount`
- `WaterImpulse0WorldLocationRadius`, `WaterImpulse0DirectionStrength`, `WaterImpulse0AgeFadeType`
- Same impulse fields up to the actor's `MaxMaterialImpulses`.

The stamp material should read `PreviousInteractionTexture` and output the combined old texture plus the new impulse. It receives `ImpulseWorldLocationRadius`, `ImpulseDirectionStrength`, `ImpulseChannels`, and `WaterInteractionWorldBounds`.

The decay material should read `PreviousInteractionTexture` and output decayed masks. It receives `DeltaSeconds`, `FoamDecay`, `TurbidityClearTime`, and `GlintLifetime`.

## Authoring defaults

- Use `ACheapWaterSurfaceActor` for long rivers, channels, distant flooded floors, and non-combat puddles.
- Use `AInteractiveWaterVolume` only for local puddles, combat hotspots, and player-near flooded rooms.
- Low/Deck/NS: set `PerformanceTier = Low`, RT size 256 or 512, 1-2 visible active tiles.
- High/PC: set `PerformanceTier = High`, RT size up to 1024 if GPU budget allows.
- Caustics should be a floor decal or plane using the existing `T_Caustics` texture. Do not use real optical caustics as the default path.

## Gameplay usage

Blueprint/C++ gameplay should call `AddWaterImpulseAtLocation`:

- `Footstep`: small radius, low strength, mass 1.
- `RollOrDash`: medium radius, directional impulse.
- `Explosion`: large radius, high strength, mass from damage/force.
- `FallingMesh`: radius from bounds, strength from impact velocity, mass from physics mass.
- `MagicOrVFX`: use configured radius/strength from the skill.

