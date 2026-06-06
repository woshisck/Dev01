# Poison Death Burst And Armor Shield VFX

Project: `DevKit`

Target view: top-down 3D.

Recommended UE content root:

- `/Game/Effect/Poison_Shield_VFX/Textures`
- `/Game/Effect/Poison_Shield_VFX/Materials`
- `/Game/Effect/Poison_Shield_VFX/Niagara/Emitters`
- `/Game/Effect/Poison_Shield_VFX/Niagara/Systems`
- `/Game/Effect/Poison_Shield_VFX/Blueprints`

This project already has blood splash assets under `/Game/Effect/Blood_VFX` and `/Game/Blood_VFX_Pack`. For the poison burst, duplicate the splash/drop/decal material pattern from the blood pack and swap in the green poison textures below.

## 1. Enemy Death Poison Burst And Poison Pool

### Visual Goal


When the enemy dies, poison bursts outward in a short radial splash. A sickly green puddle remains on the floor and expands for a moment. The puddle is the gameplay hazard. When the player overlaps it, apply the poison debuff and attach a subtle green aura to the player.

Recommended timing:

- Burst: `0.0s - 0.45s`
- Flying droplets: `0.05s - 1.1s`
- Pool growth: `0.0s - 0.75s`
- Pool active duration: `4.0s - 8.0s`
- Pool fade-out: `0.8s`
- Player poison aura: active while debuff is active, with `0.2s` fade-in and `0.35s` fade-out

### Image2 Texture Prompts

Use square power-of-two PNGs. If image2 cannot output transparency directly, generate on solid magenta `#ff00ff`, then remove the chroma key. Magenta is safer here because the subject is green.

#### `T_VFX_Poison_Drop_01`

Purpose: Niagara sprite for flying droplets.

Size: `512 x 512`, transparent PNG.

Prompt:

```text
Create a game VFX texture: a single stylized toxic slime droplet splat, bright acid green core with darker olive edges, glossy viscous liquid, irregular bulbous silhouette, high contrast alpha edges, centered with generous padding. Orthographic front view, no perspective, no floor, no shadow, no text, no watermark. Use a perfectly flat solid #ff00ff chroma-key background and do not use #ff00ff in the subject.
```

#### `T_VFX_Poison_Splash_Sheet_4x4`

Purpose: flipbook for initial burst splash.

Size: `2048 x 2048`, `4 x 4` frames.

Prompt:

```text
Create a 4 by 4 sprite sheet for a stylized toxic poison splash animation for a top-down 3D action game. The animation starts as a compact green liquid impact, then expands into sharp viscous radial spikes and droplets, then thins and breaks apart. Acid green glow in the center, darker murky green at edges, glossy slime highlights. Each frame centered in its cell with consistent scale and transparent-ready silhouette. No camera perspective, no ground, no text, no watermark. Use a perfectly flat solid #ff00ff chroma-key background and do not use #ff00ff in the subject.
```

#### `T_VFX_Poison_Pool_Mask_01`

Purpose: decal opacity mask for the floor puddle.

Size: `1024 x 1024`, grayscale or RGBA.

Prompt:

```text
Create a top-down game VFX alpha mask for a spreading toxic poison puddle decal. Irregular organic circular puddle silhouette, branching tendrils at the outer edge, several smaller satellite blobs, smooth liquid holes and islands inside. White shape on pure black background, high contrast, no color, no lighting, no shadows, no text, no watermark. The mask must be centered and tile-free.
```

#### `T_VFX_Poison_CausticNoise_01`

Purpose: panning internal motion and edge erosion.

Size: `1024 x 1024`, seamless.

Prompt:

```text
Create a seamless tileable grayscale texture for toxic liquid caustic noise. Swirling cloudy cellular patterns, soft marbling, medium contrast, no hard directional lighting, no objects, no text, no watermark. Designed for shader panning and erosion in a game material.
```

#### `T_VFX_Poison_Aura_Soft_01`

Purpose: player poison debuff aura sprites.

Size: `512 x 512`, transparent PNG.

Prompt:

```text
Create a soft circular toxic green aura sprite for a game status effect. Wispy radial glow, translucent smoky edges, faint small particles embedded in the glow, center mostly transparent so it can surround a character without hiding them. Orthographic front view, no ground, no text, no watermark. Use a perfectly flat solid #ff00ff chroma-key background and do not use #ff00ff in the subject.
```

### Materials

#### `M_VFX_Poison_Sprite`

Material domain: `Surface`

Blend mode: `Translucent` or `Additive` for glow-heavy style. Start with `Translucent`.

Shading model: `Unlit`

Two sided: `true`

Inputs:

- `TextureObject`: `T_VFX_Poison_Drop_01` or `T_VFX_Poison_Splash_Sheet_4x4`
- `BaseColor`: `ParticleColor.rgb * Texture.rgb`
- `EmissiveColor`: `ParticleColor.rgb * Texture.rgb * EmissiveStrength`
- `Opacity`: `Texture.a * ParticleColor.a`

Parameters:

- `EmissiveStrength`: default `2.0`
- `SoftOpacity`: default `0.85`
- `UseDepthFade`: default `true`
- `DepthFadeDistance`: default `25.0`

#### `M_VFX_Poison_Pool_Decal`

Material domain: `Deferred Decal`

Blend mode: `Translucent`

Decal blend mode: `DBuffer Translucent Color, Normal, Roughness` if using DBuffer decals; otherwise `Translucent`.

Key graph:

- `PoolMask = TextureSample(T_VFX_Poison_Pool_Mask_01).r`
- `NoiseA = TextureSample(T_VFX_Poison_CausticNoise_01, UV + Time * float2(0.04, 0.02)).r`
- `NoiseB = TextureSample(T_VFX_Poison_CausticNoise_01, UV * 1.8 - Time * float2(0.02, 0.05)).r`
- `Grow = DynamicParameter.x`
- `Fade = DynamicParameter.y`
- `Edge = smoothstep(Grow - 0.08, Grow + 0.04, PoolMask + NoiseA * 0.12)`
- `Opacity = saturate(Edge * Fade * 0.72)`
- `Emissive = lerp(DarkPoison, AcidGreen, NoiseB) * Opacity * 0.8`

Parameters:

- `DarkPoison`: `(0.02, 0.12, 0.035)`
- `AcidGreen`: `(0.25, 1.0, 0.08)`
- `OpacityMax`: `0.72`
- `EdgeGlowStrength`: `1.2`
- `NoiseTiling`: `1.0`
- `PoolRoughness`: `0.25`

Niagara can drive `DynamicParameter.x` from `0.15` to `1.0` over `0.75s`, and `DynamicParameter.y` from `1.0` to `0.0` during fade-out.

#### `M_VFX_Poisoned_Aura`

Material domain: `Surface`

Blend mode: `Additive`

Shading model: `Unlit`

Two sided: `true`

Inputs:

- `EmissiveColor`: `Texture.rgb * ParticleColor.rgb * 1.8`
- `Opacity`: `Texture.a * ParticleColor.a`

Use this for aura sprites attached to the player while poisoned.

### Niagara System: `NS_EnemyDeath_PoisonBurst`

Create a Niagara System with these emitters.

#### Emitter: `NE_Poison_BurstSplash`

Purpose: instant visible death splash.

- Sim target: `CPU`
- Renderer: `Sprite Renderer`
- Material: `M_VFX_Poison_Sprite`
- SubUV: `4 x 4` if using `T_VFX_Poison_Splash_Sheet_4x4`
- Spawn: `Spawn Burst Instantaneous`, count `1`
- Lifetime: `0.42 - 0.55`
- Sprite size: `180 - 260`
- Facing mode: `Face Camera`
- Rotation: random `0 - 360`
- Color over life:
  - `0.0`: `(0.45, 1.0, 0.08, 1.0)`
  - `0.45`: `(0.12, 0.55, 0.05, 0.75)`
  - `1.0`: `(0.02, 0.16, 0.03, 0.0)`
- Scale sprite size over life:
  - `0.0`: `0.45`
  - `0.18`: `1.15`
  - `1.0`: `1.35`

#### Emitter: `NE_Poison_FlyingDrops`

Purpose: droplets thrown outward from the corpse.

- Sim target: `CPU`
- Renderer: `Sprite Renderer`
- Material: `M_VFX_Poison_Sprite`
- Spawn: `Spawn Burst Instantaneous`, count `26 - 42`
- Lifetime: `0.65 - 1.15`
- Initial position: sphere radius `20`
- Velocity:
  - XY radial speed: `420 - 780`
  - Z speed: `120 - 280`
  - Cone angle: `18 - 32 degrees` above ground
- Gravity: `Z = -980`
- Drag: `1.2`
- Sprite size: `18 - 46`
- Rotation rate: `-360 - 360 deg/s`
- Collision:
  - Enabled
  - Restitution `0.08`
  - Friction `0.75`
  - Kill on max collisions: `1`
  - Optional event: spawn tiny floor stain decal on collision
- Color: acid green with random dark variation

#### Emitter: `NE_Poison_GroundMist`

Purpose: low green vapor over the poison pool.

- Sim target: `GPU` if many enemies can die together; otherwise `CPU`
- Renderer: `Sprite Renderer`
- Material: reuse smoke pack material if suitable, tinted green, or `M_VFX_Poisoned_Aura`
- Spawn rate: `8 - 14 / second`
- Lifetime: `1.2 - 2.0`
- Initial location: disk radius controlled by user parameter `PoolRadius`, default `220`
- Velocity: `Z = 15 - 45`, XY random `10 - 30`
- Sprite size: `70 - 150`
- Color over life:
  - start `(0.10, 0.85, 0.12, 0.0)`
  - mid `(0.10, 0.75, 0.08, 0.28)`
  - end `(0.03, 0.20, 0.04, 0.0)`

#### Emitter: `NE_Poison_PoolDecal`

Purpose: persistent hazardous puddle.

- Sim target: `CPU`
- Renderer: `Mesh Renderer` with a flat plane mesh, or `Decal Renderer` if available in your UE version
- Material: `M_VFX_Poison_Pool_Decal`
- Spawn: `1`
- Lifetime: `PoolDuration + FadeOut`, default `6.8`
- User parameters:
  - `User.PoolRadius`: default `220`
  - `User.PoolDuration`: default `6.0`
  - `User.FadeOut`: default `0.8`
- Scale:
  - `0.0s`: `0.25 * PoolRadius`
  - `0.75s`: `1.0 * PoolRadius`
  - hold until fade
- Dynamic material parameter:
  - `x Grow`: `0.15 -> 1.0` over first `0.75s`
  - `y Fade`: `1.0`, then `1.0 -> 0.0` over fade-out
  - `z EdgePulse`: `sin(Time * 5.0) * 0.5 + 0.5`

### Gameplay Hazard Actor

Do not use the visual decal for gameplay collision. Create a separate actor, for example `BP_PoisonPoolHazard`.

Components:

- `NiagaraComponent`: `NS_EnemyDeath_PoisonBurst`
- `SphereCollision` or `CylinderCollision`

Suggested defaults:

- Collision radius: `220`
- Active delay: `0.15s`
- Active duration: `6.0s`
- Tick interval for damage/debuff: `0.5s`
- Debuff duration applied to player: `3.0s`

Overlap behavior:

- On player begin overlap: apply poison debuff, attach `NS_Player_Poisoned_Aura`
- While overlapping: refresh debuff duration every tick interval
- On pool expired: stop Niagara, disable collision, destroy actor after fade-out

### Niagara System: `NS_Player_Poisoned_Aura`

Attach to player root or pelvis.

Emitters:

#### `NE_Poisoned_BodyGlow`

- Renderer: `Sprite Renderer`
- Material: `M_VFX_Poisoned_Aura`
- Spawn rate: `5 - 8 / second`
- Lifetime: `0.65 - 1.0`
- Location: cylinder around character, radius `35 - 55`, height `20 - 130`
- Velocity: `Z = 25 - 60`
- Sprite size: `55 - 95`
- Color: `(0.12, 1.0, 0.10, 0.20 - 0.45)`

#### `NE_Poisoned_FootRing`

- Renderer: `Sprite Renderer` or `Mesh Ring`
- Material: `M_VFX_Poisoned_Aura`
- Spawn: persistent one particle
- Lifetime: infinite while component active
- Size: `120 - 160`
- Color alpha: pulse `0.15 - 0.35`
- Rotation rate: `25 deg/s`

## 2. Enemy Armor Silver Fresnel Shield

### Visual Goal

Armored enemies have a circular/spherical shield around the body. It should read clearly from top-down camera: transparent center, stronger silver-white rim, with a Fresnel edge. It should not hide the enemy mesh.

Use a sphere or half-sphere mesh around the enemy rather than only sprites. For top-down view, a full sphere scaled slightly flat is readable and cheap.

### Optional Image2 Texture Prompts

The shield can be built procedurally in material nodes. Only generate a noise/ring texture if you want more surface detail.

#### `T_VFX_Shield_RippleNoise_01`

Size: `1024 x 1024`, seamless grayscale.

Prompt:

```text
Create a seamless grayscale texture for an energy shield ripple material. Fine circular wave interference, subtle hex-like energy distortions, soft cloudy variation, medium-low contrast, tileable edges, no objects, no text, no watermark. Designed for panning normal/opacity distortion in a transparent game shader.
```

#### `T_VFX_Shield_Impact_Ring_01`

Size: `512 x 512`, transparent PNG.

Prompt:

```text
Create a game VFX impact ring sprite for a silver-white magical armor shield. Thin circular shock ring, bright white rim with pale blue-silver glow, transparent center, soft fading outer edge, centered, orthographic front view, no ground, no shadow, no text, no watermark. Use a perfectly flat solid #00ff00 chroma-key background and do not use #00ff00 in the subject.
```

### Mesh

Create or reuse:

- `SM_VFX_ShieldSphere`: UE sphere mesh, radius `50`, enough segments for smooth Fresnel
- Scale per enemy: enemy capsule radius * `1.25 - 1.45`, height * `0.75 - 0.95`

For flatter top-down readability, scale:

- X/Y: `1.35`
- Z: `0.90`

### Material: `M_VFX_ArmorShield_Fresnel`

Material domain: `Surface`

Blend mode: `Translucent`

Shading model: `Unlit`

Two sided: `true`

Render after DOF: optional.

Disable depth test: normally `false`. Turn on only if shield must always be visible through clutter.

Core graph:

```text
FresnelRaw = Fresnel(Exponent = FresnelPower, BaseReflectFraction = FresnelBias)
Rim = pow(saturate(FresnelRaw), RimPower)
Center = 1 - Rim
Noise = TextureSample(T_VFX_Shield_RippleNoise_01, UV * NoiseTiling + Time * NoisePan).r
Pulse = sin(Time * PulseSpeed) * 0.5 + 0.5
Opacity = saturate(CenterOpacity + Rim * RimOpacity + Noise * NoiseOpacity + Pulse * PulseOpacity)
Emissive = ShieldColor * (CenterEmission + Rim * RimEmission + Pulse * PulseEmission)
```

Parameters:

- `ShieldColor`: `(0.82, 0.90, 1.00)`
- `CenterOpacity`: `0.06`
- `RimOpacity`: `0.58`
- `FresnelPower`: `3.2`
- `FresnelBias`: `0.03`
- `RimPower`: `1.25`
- `CenterEmission`: `0.08`
- `RimEmission`: `1.9`
- `PulseSpeed`: `2.2`
- `PulseOpacity`: `0.05`
- `PulseEmission`: `0.25`
- `NoiseTiling`: `2.0`
- `NoiseOpacity`: `0.04`
- `NoisePan`: `(0.03, -0.02)`

Recommended extras:

- Add `DepthFade` with distance `35 - 60` to soften intersections with the floor and enemy.
- Add `PixelDepthOffset` only if shield/floor z-fighting appears.
- If the shield looks too invisible from top-down, lower `FresnelPower` to `2.2` and raise `CenterOpacity` to `0.10`.

### Niagara System: `NS_Enemy_ArmorShield`

Use Niagara for the orbiting rim particles and hit response. Use the mesh component/material for the actual shield shell.

#### Component Setup

Enemy blueprint components:

- `StaticMeshComponent ShieldShell`
  - Mesh: `SM_VFX_ShieldSphere`
  - Material: `M_VFX_ArmorShield_Fresnel`
  - Collision: `NoCollision`
  - Hidden in game when armor <= 0
- `NiagaraComponent ShieldVFX`
  - System: `NS_Enemy_ArmorShield`
  - Attach: enemy root
  - Active while armor > 0

#### User Parameters

- `User.ShieldRadius`: default `95`
- `User.ShieldHeight`: default `130`
- `User.ArmorRatio`: `0.0 - 1.0`
- `User.HitPulse`: set to `1.0` on armor hit, decay to `0.0` over `0.25s`

#### Emitter: `NE_Shield_EquatorRim`

Purpose: readable circular outline from top-down.

- Sim target: `CPU`
- Renderer: `Sprite Renderer` or `Ribbon Renderer`
- Material: additive silver-white soft sprite or ribbon material
- Spawn rate: `18 / second`
- Lifetime: `0.7 - 1.0`
- Location: torus/ring around enemy at capsule mid height
- Ring radius: `User.ShieldRadius`
- Sprite size: `8 - 14`
- Velocity: orbit tangent `35 - 55`
- Color:
  - RGB `(0.82, 0.90, 1.0)`
  - Alpha `0.20 + User.ArmorRatio * 0.25 + User.HitPulse * 0.35`

#### Emitter: `NE_Shield_VerticalGlimmers`

Purpose: thin silver streaks sliding on the shield surface.

- Spawn rate: `4 - 7 / second`
- Lifetime: `0.55 - 0.9`
- Location: random point on sphere surface, biased toward rim from camera
- Sprite size: X `6 - 10`, Y `45 - 85`
- Color alpha: `0.12 - 0.32`
- Velocity: slight upward or tangential drift

#### Emitter: `NE_Shield_HitRing`

Purpose: flash when armor absorbs damage.

- Spawn: burst count `1` when `User.HitPulse` is triggered or via event
- Renderer: `Sprite Renderer`
- Material: `T_VFX_Shield_Impact_Ring_01` material
- Lifetime: `0.28 - 0.38`
- Size: `User.ShieldRadius * 1.6 -> User.ShieldRadius * 2.15`
- Color: silver white, alpha `0.75 -> 0.0`
- Facing mode: `Face Camera`

### Blueprint Logic Notes

Armor state:

- Armor > 0: show `ShieldShell`, activate `ShieldVFX`
- Armor hit: set dynamic material parameter `HitPulse = 1.0`, then timeline to `0.0` over `0.25s`; trigger `NE_Shield_HitRing`
- Armor broken: play a stronger shield shatter burst, fade shell opacity to `0.0` over `0.25s`, deactivate `ShieldVFX`

Recommended shield material hit pulse additions:

- `Opacity += HitPulse * 0.22`
- `Emissive += ShieldColor * HitPulse * 3.0`
- Optional quick scale pulse on `ShieldShell`: `1.0 -> 1.06 -> 1.0` over `0.18s`

## Naming Summary

Textures:

- `T_VFX_Poison_Drop_01`
- `T_VFX_Poison_Splash_Sheet_4x4`
- `T_VFX_Poison_Pool_Mask_01`
- `T_VFX_Poison_CausticNoise_01`
- `T_VFX_Poison_Aura_Soft_01`
- `T_VFX_Shield_RippleNoise_01`
- `T_VFX_Shield_Impact_Ring_01`

Materials:

- `M_VFX_Poison_Sprite`
- `M_VFX_Poison_Pool_Decal`
- `M_VFX_Poisoned_Aura`
- `M_VFX_ArmorShield_Fresnel`

Niagara:

- `NS_EnemyDeath_PoisonBurst`
- `NS_Player_Poisoned_Aura`
- `NS_Enemy_ArmorShield`

Blueprint:

- `BP_PoisonPoolHazard`

## First-Pass Tuning Presets

Small enemy:

- `PoolRadius`: `170`
- Drop burst count: `20 - 28`
- Pool duration: `4.0s`
- Shield radius: `75 - 90`

Medium enemy:

- `PoolRadius`: `220`
- Drop burst count: `26 - 42`
- Pool duration: `6.0s`
- Shield radius: `95 - 115`

Elite enemy:

- `PoolRadius`: `300`
- Drop burst count: `45 - 70`
- Pool duration: `8.0s`
- Shield radius: `130 - 160`
- Add `NE_Shield_VerticalGlimmers` spawn rate `10 / second`

## Implementation Order

1. Generate/import the seven textures.
2. Create the poison sprite, pool decal, poison aura, and shield Fresnel materials.
3. Duplicate existing blood splash Niagara emitters where useful, then replace material and color with poison settings.
4. Build `NS_EnemyDeath_PoisonBurst`.
5. Build `BP_PoisonPoolHazard` with separate collision and debuff logic.
6. Add `NS_Player_Poisoned_Aura` to the poison debuff state.
7. Add `ShieldShell` and `NS_Enemy_ArmorShield` to armored enemy blueprints.
8. Tune from top-down camera distance before tuning in close-up view.
