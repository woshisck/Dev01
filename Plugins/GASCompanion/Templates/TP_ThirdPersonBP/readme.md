## GAS Companion Template - Third Person BP

GAS Companion template project, with standard Third person pack features.

Includes standard movement from Third Person template, a Character Blueprint with GSCModularCharacter parent class, GSCAttributeSet as default attributes, an HUD added to player screen and a Jump Ability with GE cost and stamina regen effect.

The third person pack features a playable character where the camera is positioned behind and slightly above the character. As the character moves using mouse, keyboard, controller or virtual joystick on a touch device, the camera follows the character, observing from an over-the-shoulder point of view. This perspective emphasizes the main character and is popular in action and adventure games. The template map contains several objects that the player can jump upon, and push around.

## Plugins

GAS Companion enabled by default:

```json TP_ThirdPersonBP.uproject
	"Plugins": [
		{
			"Name": "GASCompanion",
			"Enabled": true,
			"MarketplaceURL": "com.epicgames.launcher://ue/marketplace/product/d83c6f34c3fb4b7092dde195c37c7413"
		}
  ]
```

## Config

Also includes the following settings custom to this template:

```ini Config/DefaultEditorPerProjectUserSettings.ini
[/Script/UnrealEd.EditorStyleSettings]
AssetEditorOpenLocation=MainWindow

[/Script/UnrealEd.EditorLoadingSavingSettings]
LoadLevelAtStartup=LastOpened
bForceCompilationAtStartup=True

[/Script/BlueprintGraph.BlueprintEditorSettings]
SaveOnCompile=SoC_SuccessOnly
```

```ini Config/DefaultInput.ini
[/Script/Engine.InputSettings]
+ConsoleKeys=²
```