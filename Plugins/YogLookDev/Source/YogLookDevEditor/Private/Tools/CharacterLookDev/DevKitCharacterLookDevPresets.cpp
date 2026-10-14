#include "Tools/CharacterLookDev/DevKitCharacterLookDevPresets.h"

namespace
{
FDevKitCharacterLookDevPreset MakePreset(const TCHAR* Id, const TCHAR* Label, const TCHAR* Layer,
	const TCHAR* Intent, const TCHAR* Basis, EDevKitLookDevKeyType KeyType, float Key, FLinearColor KeyColor,
	float Fill, FLinearColor FillColor, float EV, float Fog, FLinearColor FogColor)
{
	FDevKitCharacterLookDevPreset P;
	P.Id = Id; P.Label = Label; P.Layer = Layer; P.Intent = Intent; P.Basis = Basis;
	P.KeyType = KeyType; P.KeyIntensity = Key; P.KeyColor = KeyColor;
	P.FillLumens = Fill; P.FillColor = FillColor;
	P.EV100 = EV; P.FogDensity = Fog; P.FogColor = FogColor;
	if (KeyType != EDevKitLookDevKeyType::Directional)
	{
		P.KeyRotation = (FVector(0, 0, 100) - P.KeyLocation).Rotation();
	}
	return P;
}

void SetLocalKeyLocation(FDevKitCharacterLookDevPreset& Preset, FVector Location)
{
	Preset.KeyLocation = Location;
	Preset.KeyRotation = (FVector(0, 0, 100) - Location).Rotation();
}
}

const TArray<FDevKitCharacterLookDevPreset>& FDevKitCharacterLookDevPresets::GetPresets()
{
	// Unit-based starting rigs. Paintings define art direction, not measured illuminance.
	// Broad Rect fill represents window/bounce lighting; unknown HDR radiance contributes no energy.
	// Colors are linear light colors; camera white balance and light temperature are not overridden.
	static const TArray<FDevKitCharacterLookDevPreset> Presets = []
	{
		using K = EDevKitLookDevKeyType;
		TArray<FDevKitCharacterLookDevPreset> P;
		P.Add(MakePreset(TEXT("Neutral"), TEXT("中性环境，灯光基准"), TEXT(""),
			TEXT("白光材质基准，检查原色、布料、皮革和金属；EV100 8。"),
			TEXT("1000 lx 主光；8000 lm / 2.4×2.4 m 面光模拟环境反射。目标主辅约 4:1，需用灰卡定标。"), K::Directional,
			1000, FLinearColor::White, 8000, FLinearColor::White, 8, 0, FLinearColor::White));
		P.Add(MakePreset(TEXT("L1_Torch"), TEXT("L1-标准"), TEXT("L1"),
			TEXT("近处火光塑形，冷青环境保留暗色衣料；EV100 3。"),
			TEXT("1600 lm 发光体距评审中心 1.9 m，白光近似 35 lx；320 lm 冷色面光代表石壁反射。"), K::Point,
			1600, FLinearColor(1, 0.48f, 0.20f), 320, FLinearColor(0.45f, 0.70f, 0.85f), 3, 0.008f, FLinearColor(0.08f, 0.13f, 0.15f)));
		SetLocalKeyLocation(P.Last(), FVector(-140, -100, 180));
		P.Add(MakePreset(TEXT("L2_Fire"), TEXT("L2-标准"), TEXT("L2"),
			TEXT("大面积橙金火场、橘红烟尘与冷灰反射；EV100 5。"),
			TEXT("16000 lm / 2.4×4 m 面光代表燃烧立面；1800 lm / 2.4×2.4 m 反射补光保留正面材质。"), K::Rect,
			16000, FLinearColor(1, 0.30f, 0.065f), 1800, FLinearColor(0.60f, 0.66f, 0.75f), 5, 0.025f, FLinearColor(0.30f, 0.095f, 0.025f)));
		P.Last().KeySourceSizeCm = FVector2D(240, 400);
		P.Add(MakePreset(TEXT("L3_Sanctuary"), TEXT("L3-标准"), TEXT("L3"),
			TEXT("蓝紫高窗环境与暖白焦点，检查浅衣和银饰层次；EV100 6。"),
			TEXT("9000 lm / 1.6×2.4 m 暖白面光代表高窗焦点；2000 lm / 2.4×2.4 m 蓝紫面光代表侧窗反射。"), K::Rect,
			9000, FLinearColor(1, 0.82f, 0.75f), 2000, FLinearColor(0.48f, 0.56f, 0.90f), 6, 0.012f, FLinearColor(0.12f, 0.14f, 0.25f)));
		return P;
	}();
	return Presets;
}

const FDevKitCharacterLookDevPreset* FDevKitCharacterLookDevPresets::Find(const FString& Id)
{
	return GetPresets().FindByPredicate([&Id](const auto& P) { return P.Id == Id; });
}

FString FDevKitCharacterLookDevPresets::GetLightTemplate(const FString& Layer)
{
	// Compatibility references to the project-authored templates. Keep the plugin
	// independent of DevKitEditor; the host contract test checks these against MapCreator.
	const FString Root(TEXT("/Game/Art/Map/Templates/LevelAsset"));
	if (Layer == TEXT("L1")) return Root / TEXT("L_Temp_Light_Dungeon");
	if (Layer == TEXT("L2")) return Root / TEXT("L_Temp_Light_Outside");
	if (Layer == TEXT("L3")) return Root / TEXT("L_Temp_Light_Inside");
	return FString();
}
