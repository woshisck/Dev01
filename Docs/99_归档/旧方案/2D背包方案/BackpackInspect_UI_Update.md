# Backpack Inspect UI Update

## Goal

Adjust the current backpack screen toward a restrained gothic 16:9 inspection panel:

- Left 25%: weapon pickup inspection, weapon icon, name, description, combo list.
- Center 50%: combat deck management, horizontal release-order conveyor.
- Right 25%: selected card/rune detail.

## Generated Source Art

Generated source PNGs are in:

`SourceArt/UI/BackpackInspect/`

Files:

- `T_BackpackInspect_MainPanelFrame.png`: 16:9 moonlit wrought-iron main frame.
- `T_BackpackInspect_CellFrame.png`: square inventory cell frame.
- `T_BackpackInspect_TarotCardFrame.png`: tall 1:3 deck card frame.
- `T_BackpackInspect_WeaponIcon_TrickBlade.png`: default gothic weapon icon fallback.

Import target recommendation:

- `/Game/Docs/UI/RunCard/BackpackInspect/T_BackpackInspect_MainPanelFrame`
- `/Game/Docs/UI/RunCard/BackpackInspect/T_BackpackInspect_CellFrame`
- `/Game/Docs/UI/RunCard/BackpackInspect/T_BackpackInspect_TarotCardFrame`
- `/Game/Docs/UI/RunCard/BackpackInspect/T_BackpackInspect_WeaponIcon_TrickBlade`

## DA_BackpackStyle Mapping

Open `Content/Docs/UI/RunCard/GlobalSet/DA_BackpackStyle` and set:

- `Inspect Panel / Main Panel Frame Texture` = imported main panel frame.
- `Inspect Panel / Deck Card Frame Texture` = imported tarot card frame.
- `Inspect Panel / Default Weapon Icon Texture` = imported weapon icon.
- `Cell Textures / Cell Empty Texture` = imported cell frame.
- `Cell Textures / Cell Occupied Active Texture` = imported cell frame.
- `Cell Textures / Cell Occupied Inactive Texture` = imported cell frame.
- `Cell Textures / Cell Selected Texture` = imported cell frame.
- `Cell Textures / Cell Hover Texture` = imported cell frame.
- `Inspect Panel / Deck Card Width` = `88`.
- `Inspect Panel / Deck Card Height` = `264`.
- `Inspect Panel / Deck Card Spacing` = `12`.

## WBP Notes

`WBP_BackpackScreen` should keep the existing C++ binding names:

- `WeaponIcon`
- `WeaponNameText`
- `WeaponDescText`
- `ComboHintText`
- `CombatDeckEditWidget`
- `RuneInfoCard`

Recommended visual layout:

- Canvas or root panel stays 16:9-centered.
- Left panel slot Fill weight `0.25`.
- Center panel slot Fill weight `0.50`.
- Right panel slot Fill weight `0.25`.
- `CombatDeckEditWidget.CardListBox` should be a horizontal panel so cards read left-to-right as release order.
- `WBP_CombatDeckEditCardSlot` can add an optional `Image` named `CardBG`; assign the imported tarot card frame in class defaults through `DefaultCardFrameTexture`.

## Runtime Changes

- Combat deck edit cards now default to fixed tall-card sizing with a 1:3 ratio.
- Combo hints render through `UYogCommonRichTextBlock` with `BP_InputActionDecorator`, using CommonUI input icons from `<input action="LightAttack"/>` / `<input action="HeavyAttack"/>` instead of hardcoded controller/keyboard rows.
- Weapon inspection uses the style DA default weapon icon when the equipped weapon has no thumbnail.
