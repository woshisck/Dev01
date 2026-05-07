# 512 Enemy/Room Rune Setup Report
- Mode: Apply
- Source root: `/Game/Docs/BuffDocs/V2-RuneCard/512Generated`
- Target root: `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/EnemyRoom`

## `DA_Rune512_EnemyRoom_Attack`
- Duplicated `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Attack` -> `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/EnemyRoom/DA_Rune512_EnemyRoom_Attack`.
- Configured enemy/room rune. Flow=`FA_Rune512_Attack_Base`; Trigger=ERuneTriggerType::Passive; RuneID=51251; SuggestedDifficultyScore=2; Note=Passive; stable baseline pressure buff..

## `DA_Rune512_EnemyRoom_Shield`
- Duplicated `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Shield` -> `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/EnemyRoom/DA_Rune512_EnemyRoom_Shield`.
- Configured enemy/room rune. Flow=`FA_Rune512_Shield_Base`; Trigger=ERuneTriggerType::Passive; RuneID=51252; SuggestedDifficultyScore=2; Note=Passive; replaces old Playtest_GA IronArmor recommendations..

## `DA_Rune512_EnemyRoom_ReduceDamage`
- Duplicated `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_ReduceDamage` -> `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/EnemyRoom/DA_Rune512_EnemyRoom_ReduceDamage`.
- Configured enemy/room rune. Flow=`FA_Rune512_ReduceDamage_Base`; Trigger=ERuneTriggerType::Passive; RuneID=51253; SuggestedDifficultyScore=2; Note=Passive; defensive room pressure buff..

## `DA_Rune512_EnemyRoom_Burn`
- Duplicated `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Burn` -> `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/EnemyRoom/DA_Rune512_EnemyRoom_Burn`.
- Configured enemy/room rune. Flow=`FA_Rune512_Burn_Base`; Trigger=ERuneTriggerType::OnAttackHit; RuneID=51254; SuggestedDifficultyScore=2; Note=OnAttackHit; requires enemy hit events..

## `DA_Rune512_EnemyRoom_Poison`
- Duplicated `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Poison` -> `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/EnemyRoom/DA_Rune512_EnemyRoom_Poison`.
- Configured enemy/room rune. Flow=`FA_Rune512_Poison_Base`; Trigger=ERuneTriggerType::OnAttackHit; RuneID=51255; SuggestedDifficultyScore=2; Note=OnAttackHit; requires enemy hit events..

## `DA_Rune512_EnemyRoom_Pierce`
- Duplicated `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Pierce` -> `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/EnemyRoom/DA_Rune512_EnemyRoom_Pierce`.
- Configured enemy/room rune. Flow=`FA_Rune512_Pierce_Base`; Trigger=ERuneTriggerType::OnAttackHit; RuneID=51256; SuggestedDifficultyScore=2; Note=OnAttackHit; verify current Flow tuning before using in core rooms..

## `DA_Rune512_EnemyRoom_Split`
- Duplicated `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Split` -> `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/EnemyRoom/DA_Rune512_EnemyRoom_Split`.
- Configured enemy/room rune. Flow=`FA_Rune512_Split_Base`; Trigger=ERuneTriggerType::OnAttackHit; RuneID=51257; SuggestedDifficultyScore=3; Note=OnAttackHit; prototype/high-risk tuning..

## `DA_Rune512_EnemyRoom_Moonlight`
- Duplicated `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Moonlight_Forward` -> `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/EnemyRoom/DA_Rune512_EnemyRoom_Moonlight`.
- Configured enemy/room rune. Flow=`FA_Rune512_Moonlight_Base`; Trigger=ERuneTriggerType::OnAttackHit; RuneID=51258; SuggestedDifficultyScore=3; Note=OnAttackHit; strong visual/projectile pressure..

## `DA_Rune512_EnemyRoom_Moonlight_Reversed`
- Duplicated `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Moonlight_Reversed` -> `/Game/Docs/BuffDocs/V2-RuneCard/512Generated/EnemyRoom/DA_Rune512_EnemyRoom_Moonlight_Reversed`.
- Configured enemy/room rune. Flow=`FA_Rune512_Moonlight_Base`; Trigger=ERuneTriggerType::OnAttackHit; RuneID=51259; SuggestedDifficultyScore=4; Note=OnAttackHit; high-risk elite/test buff..


## Current room/enemy BuffPool scan