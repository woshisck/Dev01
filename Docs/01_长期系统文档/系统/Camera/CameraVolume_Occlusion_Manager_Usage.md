# Camera Volume, Occlusion, and Camera Manager Usage

Status: current C++ implementation reference  
Scope: `AYogPlayerCameraManager`, `AYogCameraVolume`, `UYogCameraOcclusionFadeComponent`, and their player-character wiring

---

## Quick Ownership Map

```text
AYogPlayerControllerBase
  - Uses AYogPlayerCameraManager as PlayerCameraManagerClass.
  - Forwards Input_CameraLook to AYogPlayerCameraManager::SetCameraInputAxis().

APlayerCharacterBase
  - Owns UYogSpringArmComponent CameraBoom.
  - Owns UCameraComponent FollowCamera.
  - Owns UYogCameraOcclusionFadeComponent CameraOcclusionFadeComponent.
  - Exposes GetCameraBoom() and GetFollowCamera().

AYogPlayerCameraManager
  - Reads SpringArm + Camera base POV through Super::UpdateViewTarget().
  - Applies camera state offsets, input offsets, dash behavior, and camera-volume arm length.
  - Stores the currently active AYogCameraVolume as ConstraintVolume.

AYogCameraVolume
  - Level brush volume.
  - Registers/unregisters itself with AYogPlayerCameraManager on player overlap.
  - Also directly toggles the player spring arm follow state and arm length.

UYogCameraOcclusionFadeComponent
  - Ticks on the player.
  - Sweeps from camera to player target point.
  - Fades tagged occluding primitive components by driving a material scalar parameter.
```

---

## Player Character Camera Setup

Source:

- `Source/DevKit/Public/Character/PlayerCharacterBase.h`
- `Source/DevKit/Private/Character/PlayerCharacterBase.cpp`

`APlayerCharacterBase` creates the runtime camera stack:

```cpp
CameraBoom = CreateDefaultSubobject<UYogSpringArmComponent>(TEXT("CameraBoom"));
CameraBoom->SetupAttachment(GetCapsuleComponent());
CameraBoom->SetUsingAbsoluteRotation(true);
CameraBoom->TargetArmLength = DefaultCameraBoomLength;
CameraBoom->bUsePawnControlRotation = false;
CameraBoom->bInheritPitch = false;
CameraBoom->bInheritYaw = false;
CameraBoom->bInheritRoll = false;
CameraBoom->bfollowPlayer = true;

FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
FollowCamera->FieldOfView = DefaultCameraFOV;

CameraOcclusionFadeComponent =
    CreateDefaultSubobject<UYogCameraOcclusionFadeComponent>(TEXT("CameraOcclusionFadeComponent"));
```

Important defaults:

| Field | Owner | Default | Use |
|---|---|---:|---|
| `DefaultCameraBoomLength` | `APlayerCharacterBase` | `2300.0` | Normal spring-arm length. |
| `DefaultCameraFOV` | `APlayerCharacterBase` | `50.0` | Initial camera FOV. |
| `CameraBoom` | `APlayerCharacterBase` | native component | Custom spring arm used by camera volume and camera manager. |
| `FollowCamera` | `APlayerCharacterBase` | native component | Actual view camera attached to the spring-arm socket. |
| `CameraOcclusionFadeComponent` | `APlayerCharacterBase` | native component | Runtime occlusion fade handler. |

`BeginPlay()` currently re-applies arm attachment, absolute rotation, arm length, rotation inheritance, and FOV. If a Blueprint overrides these values, confirm whether the constructor or `BeginPlay()` value is the one you intend to keep.

---

## Camera Manager Usage

Source:

- `Source/DevKit/Public/Camera/YogPlayerCameraManager.h`
- `Source/DevKit/Private/Camera/YogPlayerCameraManager.cpp`
- `Source/DevKit/Private/Character/YogPlayerControllerBase.cpp`

### Activation

`AYogPlayerControllerBase` sets:

```cpp
PlayerCameraManagerClass = AYogPlayerCameraManager::StaticClass();
```

Camera input is forwarded from the player controller:

```cpp
void AYogPlayerControllerBase::CameraLook(const FInputActionValue& Value)
{
    if (IsGameplayInputBlocked()) return;

    if (AYogPlayerCameraManager* CM = Cast<AYogPlayerCameraManager>(PlayerCameraManager))
    {
        CM->SetCameraInputAxis(Value.Get<FVector2D>());
    }
}
```

`Input_CameraLook` is expected to be a `Vector2D` input action, currently assigned through player-controller defaults / Blueprint assets.

### Runtime Update Flow

`AYogPlayerCameraManager::UpdateViewTarget()` is the central camera update:

```text
1. Super::UpdateViewTarget()
   - Lets SpringArm + FollowCamera produce the base POV.

2. Resolve viewed pawn as APlayerCharacterBase.

3. Read player position and velocity.

4. Update movement timing and LookAhead alpha.

5. Determine camera state.

6. Compute state offset.

7. Compute right-stick input offset.

8. Combine offsets into a candidate camera location.

9. Read PlayerCharacter->GetCameraBoom().

10. Apply camera-volume follow mode and spring-arm length interpolation.

11. If inside camera volume, cancel extra XY camera offsets.

12. Apply final camera location using VInterpTo or dash follow rules.
```

### Camera States

Current state enum comes from `YogCameraPawn.h` and is reused by the camera manager.

The manager currently uses:

| State | Trigger | Effect |
|---|---|---|
| `Dash` | `SetDashMode(true)` | Uses `DashFollowSpeed` or direct snap when speed is `0`. |
| `PickupFocus` | Arrangement phase and nearest reward pickup exists | Offsets toward pickup. |
| `LookAhead` | Moving and `bEnableLookAhead == true` | Offsets toward last movement direction. |
| `FocusCharacter` | Default / stationary / LookAhead disabled | Keeps camera centered on character with smooth follow. |
| `CombatFocus` / `CombatSearch` | Present in code paths | Offset functions exist, but current `DetermineState()` does not select them in the active snippet. |

### Main Editable Parameters

Set these in the active PlayerCameraManager Blueprint, usually:

```text
Content/Code/Core/Camera/BP_PlayerCameraManager.uasset
```

| Field | Default | Category | Notes |
|---|---:|---|---|
| `bEnableLookAhead` | `false` | `Camera|LookAhead` | Enables movement-direction camera lead. |
| `MovingFollowSpeed` | `8.0` | `Camera|LookAhead` | Follow speed when LookAhead is disabled. |
| `LookAheadBuildupTime` | `0.5` | `Camera|LookAhead` | Time to build full lead. |
| `LookAheadDistance` | `280.0` | `Camera|LookAhead` | Max movement-direction lead distance. |
| `LookAheadLerpSpeed` | `4.0` | `Camera|LookAhead` | Lead interpolation speed. |
| `InitialFollowLerpSpeed` | `2.0` | `Camera|LookAhead` | Early movement follow speed. |
| `LookAheadAlphaDecaySpeed` | `5.0` | `Camera|LookAhead` | Lead fade-out speed after stopping. |
| `FocusLerpSpeed` | `1.5` | `Camera|Focus` | Focus state interpolation speed. |
| `MaxInputOffset` | `200.0` | `Camera|Input` | Right-stick camera offset scale. |
| `InputOffsetLerpSpeed` | `8.0` | `Camera|Input` | Right-stick offset smoothing. |
| `MovingSpeedThreshold` | `10.0` | `Camera|Movement` | Velocity threshold for moving state. |
| `StationarySettleSpeed` | `5.0` | `Camera|Movement` | Return-to-center speed when stationary. |
| `DashFollowSpeed` | `18.0` | `Camera|Dash` | Dash camera follow speed. |

Current note: the requested "move camera toward player's move direction with distance 100" is not the current default. The current implementation has `LookAheadDistance = 280.0` and `bEnableLookAhead = false`.

### Camera Input During Tutorials / Pause

Camera look input is blocked when `IsGameplayInputBlocked()` returns true. Tutorial popups or menu layers can also disable controller input through `UYogUIManagerSubsystem`.

For a tutorial section where gameplay is frozen but the player can move the camera, prefer a dedicated tutorial freeze mode:

```text
- Block gameplay actions.
- Keep PlayerController input enabled.
- Allow CameraLook even while gameplay actions are blocked.
- Avoid full game pause for camera-move tutorial moments.
- If full pause is required, make camera manager and relevant components tick while paused.
```

---

## Camera Volume Usage

Source:

- `Source/DevKit/Public/Volume/YogCameraVolume.h`
- `Source/DevKit/Private/Volume/YogCameraVolume.cpp`

### What It Is

`AYogCameraVolume` is an `AVolume` brush actor placed in a level. It uses overlap callbacks to tell the camera manager which camera volume is active.

Constructor setup:

```cpp
PrimaryActorTick.bCanEverTick = true;
PrimaryActorTick.bStartWithTickEnabled = true;

GetBrushComponent()->SetHiddenInGame(false);
GetBrushComponent()->SetCollisionProfileName(TEXT("Trigger"));
GetBrushComponent()->SetGenerateOverlapEvents(true);
```

Runtime binding:

```cpp
OnActorBeginOverlap.AddDynamic(this, &AYogCameraVolume::OnOverlapBegin);
OnActorEndOverlap.AddDynamic(this, &AYogCameraVolume::OnOverlapEnd);
```

### Editable Volume Parameters

| Field | Default | Category | Notes |
|---|---:|---|---|
| `bShowDebugInGame` | `true` | `Camera Volume|Debug` | Draws bounds in non-shipping builds. |
| `DebugColor` | `Cyan` | `Camera Volume|Debug` | Debug box color. |
| `DebugLineThickness` | `4.0` | `Camera Volume|Debug` | Debug box thickness. |
| `ExtendedArmLength` | `3200.0` | `Camera Volume|Camera` | Target arm length while volume is active. |
| `ArmLengthBlendSpeed` | `3.0` | `Camera Volume|Camera` | Camera manager interpolation speed into volume arm length. |

### Overlap Begin Flow

```text
Player enters AYogCameraVolume
  -> Cast OtherActor to APlayerCharacterBase
  -> Get player controller
  -> Cast PlayerCameraManager to AYogPlayerCameraManager
  -> CM->SetConstraintVolume(this)
  -> Player->GetCameraBoom()
  -> CameraBoom->SetFollowPlayer(false)
  -> CameraBoom->TargetArmLength = ExtendedArmLength
```

### Overlap End Flow

```text
Player exits AYogCameraVolume
  -> Cast OtherActor to APlayerCharacterBase
  -> Get player controller
  -> Cast PlayerCameraManager to AYogPlayerCameraManager
  -> CM->SetConstraintVolume(nullptr)
  -> Player->GetCameraBoom()
  -> CameraBoom->SetFollowPlayer(true)
  -> CameraBoom->TargetArmLength = Player->DefaultCameraBoomLength
```

### Camera Manager Volume Handling

After the volume registers itself, the camera manager does this each frame:

```cpp
const bool bInCameraVolume = bVolumeFreezeFollow && ConstraintVolume.IsValid();
CameraBoom->SetFollowPlayer(!bInCameraVolume);
CameraBoom->TargetArmLength = FMath::FInterpTo(
    CameraBoom->TargetArmLength,
    bInCameraVolume ? ConstraintVolume->ExtendedArmLength : PlayerCharacter->DefaultCameraBoomLength,
    DeltaTime,
    bInCameraVolume ? ConstraintVolume->ArmLengthBlendSpeed : 6.f);

if (bInCameraVolume)
{
    Candidate = OutVT.POV.Location;
}
```

Meaning:

```text
Inside active camera volume:
  - Spring arm follow is disabled.
  - Arm length moves toward volume ExtendedArmLength.
  - Extra state/input XY offsets are cancelled.

Outside active camera volume:
  - Spring arm follow is enabled.
  - Arm length moves back toward player DefaultCameraBoomLength.
```

### Current Caveats

The current implementation has overlapping responsibility:

```text
AYogCameraVolume:
  - Directly sets TargetArmLength immediately on begin/end overlap.

AYogPlayerCameraManager:
  - Also interpolates TargetArmLength every frame.
```

If smooth arm-length transitions are required, prefer letting the camera manager own the interpolation and remove the direct `TargetArmLength = ...` assignments from the volume.

There is currently no camera-volume FOV change. `DefaultCameraFOV` is only applied by `PlayerCharacterBase`, and the volume does not store or interpolate a target FOV.

---

## Yog Spring Arm Usage

Source:

- `Source/DevKit/Public/Camera/YogSpringArmComponent.h`
- `Source/DevKit/Private/Camera/YogSpringArmComponent.cpp`

`UYogSpringArmComponent` extends `USpringArmComponent`.

The important custom flag is:

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Lag)
uint32 bfollowPlayer : 1;
```

Use the setter instead of writing the flag directly:

```cpp
CameraBoom->SetFollowPlayer(false);
CameraBoom->SetFollowPlayer(true);
```

Behavior:

```text
SetFollowPlayer(false):
  - Stores FrozenArmOrigin = GetComponentLocation() + TargetOffset.
  - Spring arm origin stops following the player.

SetFollowPlayer(true):
  - Clears the frozen origin.
  - Spring arm origin follows GetComponentLocation() + TargetOffset again.
```

In `UpdateDesiredArmLocation()`:

```cpp
FVector ArmOrigin = bfollowPlayer
    ? GetComponentLocation() + TargetOffset
    : FrozenArmOrigin;
```

The component still applies normal spring-arm target arm length, socket offset, camera collision trace, and child transform updates.

---

## Camera Occlusion Fade Component Usage

Source:

- `Source/DevKit/Public/Component/YogCameraOcclusionFadeComponent.h`
- `Source/DevKit/Private/Component/YogCameraOcclusionFadeComponent.cpp`
- Existing guide: `Docs/01_长期系统文档/系统/Camera/CameraOcclusionFade_Guide.md`

### What It Does

`UYogCameraOcclusionFadeComponent` fades world geometry between the camera and the player. It is attached to `APlayerCharacterBase` by default.

Tick group:

```cpp
PrimaryComponentTick.bCanEverTick = true;
PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
```

This makes it run after camera update work, so it traces using the current camera location.

### Runtime Flow

```text
Every tick:
  -> UpdateFadeInterpolation(DeltaTime)

If occlusion fade disabled:
  -> Mark all targets visible
  -> Skip tracing

Every TraceInterval seconds:
  -> RunOcclusionTrace()
  -> Start = current PlayerCameraManager camera location
  -> End = player actor location + PlayerTargetOffset
  -> SweepMultiByChannel with TraceRadius and TraceChannel
  -> Filter hit components through IsFadeAllowedForComponent()
  -> AddOrUpdateFadeTarget() for valid occluders
  -> Interpolate material scalar toward MinVisibleAlpha

When no longer hit:
  -> TargetAlpha returns to 1.0
  -> Original materials are restored when fully visible
```

### Editable Parameters

| Field | Default | Category | Notes |
|---|---:|---|---|
| `bEnableOcclusionFade` | `true` | `Camera Occlusion` | Master enable. |
| `TraceInterval` | `0.05` | `Camera Occlusion` | Seconds between sweeps. |
| `TraceRadius` | `28.0` | `Camera Occlusion` | Sphere sweep radius. |
| `PlayerTargetOffset` | `(0,0,80)` | `Camera Occlusion` | Trace end point relative to player. |
| `TraceChannel` | `ECC_Visibility` | `Camera Occlusion` | Collision channel used by sweep. |
| `bOnlyFadeTaggedOccluders` | `true` | `Camera Occlusion` | If true, only tagged actors/components fade. |
| `OccluderTag` | `CameraOccluder` | `Camera Occlusion` | Required actor/component tag by default. |
| `MinVisibleAlpha` | `0.15` | `Camera Occlusion` | Target alpha while occluded. |
| `FadeOutSpeed` | `12.0` | `Camera Occlusion` | Interp speed toward fade-out. |
| `FadeInSpeed` | `8.0` | `Camera Occlusion` | Interp speed back to visible. |
| `FadeScalarParameterName` | `CameraOcclusionAlpha` | `Camera Occlusion` | Scalar parameter driven on dynamic materials. |
| `OcclusionFadeMaterial` | `null` | `Camera Occlusion|Material` | Optional override material. |
| `bDrawDebugOcclusionTrace` | `false` | `Camera Occlusion|Debug` | Debug trace drawing. |
| `DebugDrawDuration` | `0.08` | `Camera Occlusion|Debug` | Debug draw lifetime. |

### Fade Eligibility

`IsFadeAllowedForComponent()` rejects:

```text
- Null components.
- Hidden-in-game components.
- Components owned by the player actor.
- Attached-to-player components unless explicitly tagged.
- Untagged components when bOnlyFadeTaggedOccluders is true.
- Components with zero material slots.
```

By default, a target is valid when either:

```text
Actor has Actor Tag: CameraOccluder
or
PrimitiveComponent has Component Tag: CameraOccluder
```

### Material Requirements

The component creates dynamic material instances and sets:

```cpp
DynamicMaterial->SetScalarParameterValue(FadeScalarParameterName, Target.CurrentAlpha);
```

Default scalar name:

```text
CameraOcclusionAlpha
```

Recommended material setup:

```text
ScalarParameter CameraOcclusionAlpha
  -> DitherTemporalAA Alpha
  -> Opacity Mask
```

Use a masked material path for level geometry. This avoids the heavier translucent path and is better for many static occluders.

If `OcclusionFadeMaterial` is null, dynamic material instances are created from each component's existing material. If it is set, the component uses that override material for all faded material slots on the occluder.

### Cleanup

On `EndPlay()`, the component restores all original materials:

```cpp
RestoreAllMaterials();
```

It also restores individual targets after they fade fully back to alpha `1.0`.

---

## Level / Blueprint Setup Checklist

### Player

```text
- Use a player class derived from APlayerCharacterBase.
- Confirm CameraBoom is UYogSpringArmComponent.
- Confirm FollowCamera is attached to CameraBoom socket.
- Confirm CameraOcclusionFadeComponent exists on the player.
```

### Player Controller

```text
- Use AYogPlayerControllerBase or a derived Blueprint.
- Confirm PlayerCameraManagerClass resolves to AYogPlayerCameraManager or BP_PlayerCameraManager.
- Assign Input_CameraLook to the right-stick / camera-look Vector2D input action.
```

### Camera Manager Blueprint

```text
Open Content/Code/Core/Camera/BP_PlayerCameraManager.uasset:
- Enable bEnableLookAhead if movement-direction camera lead is wanted.
- Tune LookAheadDistance, MovingFollowSpeed, StationarySettleSpeed.
- Tune MaxInputOffset for right-stick camera offset.
- Assign camera shake classes if needed.
```

### Camera Volume

```text
In a level:
- Place AYogCameraVolume.
- Shape it with brush/geometry editing.
- Ensure the brush collision profile is Trigger.
- Tune ExtendedArmLength and ArmLengthBlendSpeed.
- Keep in mind current implementation has no FOV blending.
```

Known asset references found by string scan:

```text
Content/Developers/sunchuankai/LootTest/RuneTest.umap
Content/Art/Map/Map_Data/L1_CommonLevel_Prison_S_03/L_Prison_S_03_Gameplay.umap
```

### Occlusion Fade

```text
For each occluder actor or component:
- Add Actor Tag or Component Tag: CameraOccluder
- Use a material that responds to CameraOcclusionAlpha
- Keep bOnlyFadeTaggedOccluders enabled unless broad fade behavior is intended
```

---

## Current Gaps / Improvement Candidates

1. Camera volume does not currently change FOV.

2. Camera volume directly snaps spring-arm length on overlap begin/end, while camera manager also interpolates spring-arm length. The cleaner model is to let the camera manager own interpolation.

3. `LookAheadDistance` defaults to `280.0` and `bEnableLookAhead` defaults to false. If design wants movement lead distance `100.0` by default, update `AYogPlayerCameraManager` or the active Blueprint default.

4. Camera look input is blocked when `IsGameplayInputBlocked()` is true. Tutorial freeze sections that allow camera movement need a separate camera-allowed path.

5. Camera occlusion depends on material support for `CameraOcclusionAlpha`. Tagged occluders without compatible materials will not visually fade correctly.

