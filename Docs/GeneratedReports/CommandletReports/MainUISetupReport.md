# Main UI Setup Report
- Mode: Apply
- ForceLayout: true
- FrontendOnly: false
- HudOnly: true
- HudRootOnly: false

## HUD texture import
- Imported/reimported `X:/Project/Dev01/SourceArt/UI/HUD/T_GoldCoinIcon.png` -> `/Game/UI/Playtest_UI/UI_Tex/HUD`.
- Imported/reimported `X:/Project/Dev01/SourceArt/UI/HUD/T_MaterialQuestionIcon.png` -> `/Game/UI/Playtest_UI/UI_Tex/HUD`.

- Found existing `/Game/UI/Playtest_UI/HUD/WBP_PlayerCommonInfoHud`.
- Player common info HUD designer tree refreshed with gold and material rows.
- Found existing `/Game/UI/Playtest_UI/HUD/WBP_HUDRoot`.
- Missing widget class `/Game/UI/WB_PlayerHealthBar.WB_PlayerHealthBar_C`; using fallback for `PlayerHealthBar`.
- HUD designer tree refreshed with named regions and existing HUD widgets.
## HUD blueprint defaults
- Updated defaults on `/Game/UI/BP_YogHUD`.