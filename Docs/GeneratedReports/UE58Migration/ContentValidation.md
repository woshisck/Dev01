# UE5.8 Content Validation

- Validation date: 2026-06-25
- Engine: UE 5.8.0
- Worktree: `C:/Users/gongzhengang/.codex/worktrees/ue58-migration-verify`

## Passed

- `DevKitEditor Win64 Development` builds successfully with UE 5.8.
- GameplayTag migration dry-run completes with `EXIT=0`.
- GameplayTag migration reports `0` automatic tag changes.
- `Content/Maps/B_TeleportGate.uasset` loads during resave after adding a CoreRedirect from the deleted `B_PlayerBase_C` Blueprint class to `B_PlayerOne_C`.
- Controlled `ResavePackages` completed with `EXIT=0` for `4408` project packages after excluding known risky legacy packages.

## Fixed During Migration

- Removed deprecated `r.Mobile.VirtualTextures` from `Config/DefaultEngine.ini`; UE 5.8 reports it as an ensure and expects `r.VirtualTextures` instead.
- Updated `Plugins/ElectronicNodes/ElectronicNodes.uplugin` `EngineVersion` from `5.4.0` to `5.8.0`.
- Rewrote `Plugins/CelesLight/CelesLight.uplugin` as valid JSON.
- Replaced `AAltarActor` constructor default mesh from missing `/Game/Art/EnvironmentAsset/Prox_Box/SM_GothicAltar01` with `/Engine/BasicShapes/Cube.Cube`.
- Restored locally available source art folders needed for validation:
  - `Content/Art/Character/Elian`
  - `Content/Art/Enemy/Rats`
  - `Content/Art/Enemy/RottenGuard`

## Resave Scope

The successful resave used `Saved/Temp/ResavePackages-CleanProjectSubset-v2.txt`.

Excluded packages were not deleted or modified by design:

- MotionWarping startup-warning packages:
  - `/Game/Animation/1H-2HSword/Montage/1H_Attack_01_Seq_Montage`
  - `/Game/Animation/1H-2HSword/Montage/1H_Attack_02_Seq_Montage`
  - `/Game/Code/Characters/B_PlayerOne`
  - `/Game/Prototype/Blueprint/BP_PlayerCharacterBase`
- Legacy or broken content packages:
  - `/Game/_Deprecated/...`
  - `/Game/Code/Enemy/AI/Behaviour/BT_SaverTest`
  - `/Game/Code/Enemy/Minion/Shadow/ABL_ShadowEnemyLayer`
  - `/Game/Code/Enemy/Minion/Shadow/BS_Enemy_Shadow_Idle`
  - `/Game/Code/Environment/EnvModule/Data/Comp_ISM`
  - `/Game/Code/GAS/Abilities/Passive/GA_PlayHurtMontage`
  - `/Game/Code/GAS/Abilities/Shared/B_Effect_Bleed`
  - `/Game/Code/GAS/Abilities/Shared/GE_HurtMontage`
  - `/Game/Code/UtilityScript/LevelRelated/Temp_Building`
  - `/Game/Docs/BuffDocs/Playtest_GA/KnockBack/FA_Rune_Test_Konckback`
  - `/Game/Docs/Data/Buff/DT/DT_BuffDefinition`
  - `/Game/Docs/Data/DT_CharacterDefault`
  - `/Game/Docs/Data/DT_CharacterMovement`
  - `/Game/Docs/Data/DT_DefaultPlayer`
  - `/Game/Docs/Data/Enemy/Shadow/DA_Shadow_01`
  - `/Game/Docs/DT_PlayerDash`

## Residual Warnings

- UE commandlets still emit early startup warnings for `MotionWarpingComponent` on `BP_PlayerCharacterBase` and `B_PlayerOne`. A probe confirms `/Script/MotionWarping.MotionWarpingComponent` can be loaded later in the same process, but forcing the project module earlier caused a Niagara startup assertion, so this was left as a commandlet warning rather than a risky load-phase change.
- Restored Elian/Rat/RottenGuard source assets are read-only local files and are skipped by `ResavePackages`; they are present so dependent animations and Blueprints can resolve their skeleton references.
