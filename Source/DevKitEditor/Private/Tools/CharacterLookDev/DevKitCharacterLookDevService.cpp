#include "Tools/CharacterLookDev/DevKitCharacterLookDevService.h"

#include "Tools/CharacterLookDev/DevKitCharacterLookDevPresets.h"
#include "Animation/SkeletalMeshActor.h"
#include "Character/PlayerCharacterBase.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/LocalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/StylizedCharacterShadowPolicyComponent.h"
#include "Dom/JsonObject.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "EditorViewportClient.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PointLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/RectLight.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextureCube.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HighResScreenshot.h"
#include "LevelEditorViewport.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "UnrealClient.h"

#define LOCTEXT_NAMESPACE "DevKitCharacterLookDevService"

namespace
{
const FName StageTag(TEXT("Dev01.CharacterLookDev"));
const FName RigTag(TEXT("Dev01.CharacterLookDev.Rig"));
const FName SubjectTag(TEXT("Dev01.CharacterLookDev.Subject"));
const FName ReferenceTag(TEXT("Dev01.CharacterLookDev.Reference"));
const TCHAR* PresetTagPrefix = TEXT("Dev01.CharacterLookDev.Preset=");
const TCHAR* ModeTagPrefix = TEXT("Dev01.CharacterLookDev.Mode=");
const TCHAR* CameraTagPrefix = TEXT("Dev01.CharacterLookDev.Camera=");
const TCHAR* GreyCubePath = TEXT("/Engine/EditorMaterials/AssetViewer/T_GreyAmbient.T_GreyAmbient");
// Tags describe state, but ownership belongs only to this live, unsaved editor session.
TWeakObjectPtr<UWorld> ActiveStageWorld;

UWorld* EditorWorld()
{
	return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
}

bool IsStage(UWorld* World)
{
	if (!World || World != ActiveStageWorld.Get() || World->WorldType != EWorldType::Editor
		|| !World->GetWorldSettings() || !World->GetWorldSettings()->ActorHasTag(StageTag)) return false;
	if (FPackageName::DoesPackageExist(World->GetOutermost()->GetName()))
	{
		// Save / Save As permanently releases ownership, even when the actor tags survive.
		ActiveStageWorld.Reset();
		return false;
	}
	return true;
}

bool RequireStage(FText& OutMessage)
{
	if (!GEditor || GEditor->IsPlaySessionInProgress())
	{
		OutMessage = LOCTEXT("StopPIE", "请先停止 PIE / Simulate，再操作 LookDev 舞台。");
		return false;
	}
	if (!IsStage(EditorWorld()))
	{
		OutMessage = LOCTEXT("NeedStage", "请先创建本会话的 LookDev 临时舞台。舞台仅用于未保存预览；保存或重新载入后不再受此工具管理。");
		return false;
	}
	return true;
}

FString ReadTag(const AActor* Actor, const TCHAR* Prefix)
{
	if (Actor)
	{
		for (FName Tag : Actor->Tags)
		{
			const FString Text = Tag.ToString();
			if (Text.StartsWith(Prefix)) return Text.RightChop(FCString::Strlen(Prefix));
		}
	}
	return FString();
}

void WriteTag(AActor* Actor, const TCHAR* Prefix, const FString& Value)
{
	Actor->Tags.RemoveAll([Prefix](FName Tag) { return Tag.ToString().StartsWith(Prefix); });
	Actor->Tags.Add(FName(*(FString(Prefix) + Value)));
}

FLevelEditorViewportClient* FindViewport(UWorld* World)
{
	FLevelEditorViewportClient* Fallback = nullptr;
	if (!GEditor) return nullptr;
	for (FLevelEditorViewportClient* Client : GEditor->GetLevelViewportClients())
	{
		if (Client && Client->GetWorld() == World && Client->Viewport && Client->IsPerspective())
		{
			if (GEditor->GetActiveViewport() == Client->Viewport) return Client;
			if (!Fallback) Fallback = Client;
		}
	}
	return Fallback;
}

template<typename ActorT>
ActorT* Spawn(UWorld* World, const TCHAR* Label, FName Tag, const FVector& Location = FVector::ZeroVector,
	const FRotator& Rotation = FRotator::ZeroRotator, bool bTransient = true)
{
	FActorSpawnParameters Params;
	Params.ObjectFlags = bTransient ? RF_Transient | RF_Transactional : RF_Transactional;
	Params.OverrideLevel = World->PersistentLevel;
	ActorT* Actor = World->SpawnActor<ActorT>(ActorT::StaticClass(), Location, Rotation, Params);
	if (Actor)
	{
		Actor->Tags.Add(Tag);
		Actor->SetActorLabel(Label);
		if (!bTransient)
		{
			// Persistent lighting maps must retain the actor and every owned component on reload.
			for (UActorComponent* Component : Actor->GetComponents())
			{
				Component->ClearFlags(RF_Transient);
				Component->SetFlags(RF_Transactional);
			}
		}
	}
	return Actor;
}

void DestroyTaggedActors(UWorld* World, FName Tag)
{
	TArray<AActor*> Actors;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(Tag)) Actors.Add(*It);
	}
	for (AActor* Actor : Actors) World->DestroyActor(Actor);
}

AActor* FindSubject(UWorld* World)
{
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(SubjectTag)) return *It;
	}
	return nullptr;
}

UMaterial* MakeReferenceMaterial(UStaticMeshComponent* Owner, float BaseColor, float Metallic, float Roughness)
{
	// Referenced by the component; no standalone package, asset registry entry or global root.
	UMaterial* Material = NewObject<UMaterial>(Owner, NAME_None, RF_Transient);
	Material->SetShadingModel(MSM_DefaultLit);
	UMaterialExpressionConstant3Vector* Color = NewObject<UMaterialExpressionConstant3Vector>(Material);
	Color->Constant = FLinearColor(BaseColor, BaseColor, BaseColor);
	UMaterialExpressionConstant* Metal = NewObject<UMaterialExpressionConstant>(Material);
	Metal->R = Metallic;
	UMaterialExpressionConstant* Rough = NewObject<UMaterialExpressionConstant>(Material);
	Rough->R = Roughness;
	auto* Data = Material->GetEditorOnlyData();
	Data->ExpressionCollection.Expressions.Add(Color);
	Data->ExpressionCollection.Expressions.Add(Metal);
	Data->ExpressionCollection.Expressions.Add(Rough);
	Data->BaseColor.Connect(0, Color);
	Data->Metallic.Connect(0, Metal);
	Data->Roughness.Connect(0, Rough);
	Material->PostEditChange();
	Owner->SetMaterial(0, Material);
	return Material;
}

void AddReferences(UWorld* World, UStaticMesh* Cube, UStaticMesh* Sphere)
{
	struct FReference { const TCHAR* Label; FVector Location; FVector Scale; float Color; float Metal; float Rough; UStaticMesh* Mesh; };
	const FReference References[] = {
		{ TEXT("LookDev Floor - 18 percent linear grey"), FVector(0,0,-5), FVector(20,20,0.1), 0.18f, 0, 0.8f, Cube },
		{ TEXT("LookDev Grey - 18 percent linear reflectance"), FVector(0,-150,45), FVector(0.9), 0.18f, 0, 0.6f, Sphere },
		{ TEXT("LookDev Metal - 0.8 linear / metallic 1 / roughness 0.15"), FVector(0,150,45), FVector(0.9), 0.8f, 1, 0.15f, Sphere }
	};
	for (const FReference& Reference : References)
	{
		if (AStaticMeshActor* Actor = Spawn<AStaticMeshActor>(World, Reference.Label, ReferenceTag, Reference.Location))
		{
			UStaticMeshComponent* Component = Actor->GetStaticMeshComponent();
			Component->SetMobility(EComponentMobility::Movable);
			Component->SetStaticMesh(Reference.Mesh);
			Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Actor->SetActorScale3D(Reference.Scale);
			MakeReferenceMaterial(Component, Reference.Color, Reference.Metal, Reference.Rough);
		}
	}
}

void ConfigureLight(ALight* Actor, float Intensity, FLinearColor Color, float AttenuationRadiusCm)
{
	if (!Actor) return;
	Actor->SetMobility(EComponentMobility::Movable);
	ULightComponent* Light = Actor->GetLightComponent();
	if (ULocalLightComponent* Local = Cast<ULocalLightComponent>(Light))
	{
		Local->SetIntensityUnits(ELightUnits::Lumens);
		Local->SetAttenuationRadius(AttenuationRadiusCm);
		Local->InverseExposureBlend = 0.f;
	}
	if (UPointLightComponent* Point = Cast<UPointLightComponent>(Light))
	{
		Point->SetUseInverseSquaredFalloff(true);
	}
	Light->SetIntensity(Intensity);
	Light->SetDiffuseScale(1.f);
	Light->SetSpecularScale(1.f);
	Light->SetIndirectLightingIntensity(1.f);
	// ULightComponent stores FColor as sRGB and GetLightColor decodes it back to linear.
	Light->SetLightColor(Color, true);
	// Keep native temperature and custom character-light mode defaults; report their actual values.
	Light->MarkRenderStateDirty();
}

bool BuildRig(UWorld* World, const FDevKitCharacterLookDevPreset& Preset, UTextureCube* SkyCube,
	FText& OutMessage, bool bTransient = true)
{
	ALight* Key = nullptr;
	switch (Preset.KeyType)
	{
	case EDevKitLookDevKeyType::Point:
		Key = Spawn<APointLight>(World, TEXT("LookDev Candidate Key (lumens)"), RigTag, Preset.KeyLocation, Preset.KeyRotation, bTransient);
		break;
	case EDevKitLookDevKeyType::Rect:
		Key = Spawn<ARectLight>(World, TEXT("LookDev Candidate Key (lumens)"), RigTag, Preset.KeyLocation, Preset.KeyRotation, bTransient);
		break;
	default:
		Key = Spawn<ADirectionalLight>(World, TEXT("LookDev Candidate Key (lux)"), RigTag, Preset.KeyLocation, Preset.KeyRotation, bTransient);
		break;
	}
	const FVector FillPosition = Preset.FillLocation;
	const FRotator FillRotation = Preset.FillRotation;
	ARectLight* Fill = Spawn<ARectLight>(World, TEXT("LookDev Candidate Broad Fill (lumens)"), RigTag,
		FillPosition, FillRotation, bTransient);
	ASkyLight* Sky = Preset.SkyIntensity > 0.f && SkyCube
		? Spawn<ASkyLight>(World, TEXT("LookDev Candidate Grey Cubemap (multiplier)"), RigTag,
			FVector::ZeroVector, FRotator::ZeroRotator, bTransient) : nullptr;
	APostProcessVolume* Post = Spawn<APostProcessVolume>(World, TEXT("LookDev Candidate Fixed Exposure"), RigTag,
		FVector::ZeroVector, FRotator::ZeroRotator, bTransient);
	if (!Key || !Fill || !Post || (Preset.SkyIntensity > 0.f && !Sky))
	{
		DestroyTaggedActors(World, RigTag);
		OutMessage = LOCTEXT("RigFailed", "创建灯光舞台失败；请查看编辑器日志。");
		return false;
	}
	// Native light CDO root transforms are composed during SpawnActor (Directional adds -46 degrees / 2.5 scale).
	// Candidate rig transforms are absolute. Normalize only these lights, not subjects or other Spawn() users.
	Key->SetActorTransform(FTransform(Preset.KeyRotation, Preset.KeyLocation, FVector::OneVector));
	Fill->SetActorTransform(FTransform(FillRotation, FillPosition, FVector::OneVector));
	ConfigureLight(Key, Preset.KeyIntensity, Preset.KeyColor, Preset.AttenuationRadiusCm);
	ConfigureLight(Fill, Preset.FillLumens, Preset.FillColor, Preset.AttenuationRadiusCm);
	if (UPointLightComponent* Point = Cast<UPointLightComponent>(Key->GetLightComponent()))
	{
		Point->SetSourceRadius(Preset.SourceRadiusCm);
	}
	if (UDirectionalLightComponent* Directional = Cast<UDirectionalLightComponent>(Key->GetLightComponent()))
	{
		Directional->SetLightSourceAngle(Preset.SourceAngleDegrees);
	}
	if (URectLightComponent* Rect = Cast<URectLightComponent>(Key->GetLightComponent()))
	{
		Rect->SetSourceWidth(Preset.KeySourceSizeCm.X);
		Rect->SetSourceHeight(Preset.KeySourceSizeCm.Y);
	}
	Fill->RectLightComponent->SetSourceWidth(Preset.FillSourceSizeCm.X);
	Fill->RectLightComponent->SetSourceHeight(Preset.FillSourceSizeCm.Y);
	if (Sky)
	{
		USkyLightComponent* SkyComponent = Sky->GetLightComponent();
		SkyComponent->SetMobility(EComponentMobility::Movable);
		SkyComponent->SourceType = SLS_SpecifiedCubemap;
		SkyComponent->SetCubemap(SkyCube);
		SkyComponent->SetIntensity(Preset.SkyIntensity);
		SkyComponent->MarkRenderStateDirty();
	}
	Post->bUnbound = true;
	Post->Priority = 100.f;
	FPostProcessSettings& Settings = Post->Settings;
	Settings.bOverride_AutoExposureMethod = true;
	Settings.AutoExposureMethod = AEM_Manual;
	Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	Settings.AutoExposureApplyPhysicalCameraExposure = true;
	Settings.bOverride_CameraISO = true;
	Settings.CameraISO = 100.f;
	Settings.bOverride_DepthOfFieldFstop = true;
	Settings.DepthOfFieldFstop = 4.f;
	Settings.bOverride_CameraShutterSpeed = true;
	Settings.CameraShutterSpeed = FMath::Pow(2.f, Preset.EV100) / 16.f;
	Settings.bOverride_AutoExposureBias = true;
	Settings.AutoExposureBias = 0.f;
	// White temperature/tint and grading stay at project defaults, without overrides.
	if (Preset.FogDensity > 0.f)
	{
		if (AExponentialHeightFog* Fog = Spawn<AExponentialHeightFog>(World, TEXT("LookDev Candidate Atmosphere"), RigTag,
			FVector::ZeroVector, FRotator::ZeroRotator, bTransient))
		{
			Fog->GetComponent()->SetFogDensity(Preset.FogDensity);
			Fog->GetComponent()->SetFogInscatteringColor(Preset.FogColor);
		}
	}
	WriteTag(World->GetWorldSettings(), PresetTagPrefix, Preset.Id);
	return true;
}

FString PropertyText(const UStruct* Type, const void* Container, const TCHAR* Name)
{
	if (const FProperty* Property = FindFProperty<FProperty>(Type, Name))
	{
		FString Value;
		Property->ExportTextItem_Direct(Value, Property->ContainerPtrToValuePtr<void>(Container), nullptr, nullptr, PPF_None);
		return Value;
	}
	return TEXT("missing_in_this_engine");
}

void AddProperties(const TSharedRef<FJsonObject>& Json, UObject* Object, std::initializer_list<const TCHAR*> Names)
{
	for (const TCHAR* Name : Names) Json->SetStringField(Name, PropertyText(Object->GetClass(), Object, Name));
}

TSharedRef<FJsonObject> DescribeActor(AActor* Actor)
{
	TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetStringField(TEXT("name"), Actor->GetActorLabel());
	Json->SetStringField(TEXT("class"), Actor->GetClass()->GetPathName());
	Json->SetStringField(TEXT("location_cm"), Actor->GetActorLocation().ToString());
	Json->SetStringField(TEXT("rotation_degrees"), Actor->GetActorRotation().ToString());
	Json->SetStringField(TEXT("scale"), Actor->GetActorScale3D().ToString());
	Json->SetBoolField(TEXT("tool_generated"), Actor->ActorHasTag(RigTag));
	return Json;
}

FString HashFile(const FString& Filename)
{
	TArray<uint8> Bytes;
	if (!IFileManager::Get().FileExists(*Filename)) return TEXT("missing_or_unreadable");
	if (!FFileHelper::LoadFileToArray(Bytes, *Filename)) return TEXT("missing_or_unreadable");
	FSHAHash Hash;
	FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num(), Hash.Hash);
	return Hash.ToString();
}

FString ReadBuildId(const FString& Filename)
{
	FString Text;
	TSharedPtr<FJsonObject> Json;
	if (!FFileHelper::LoadFileToString(Text, *Filename)
		|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) || !Json.IsValid()) return TEXT("missing_or_unreadable");
	FString Id;
	return Json->TryGetStringField(TEXT("BuildId"), Id) ? Id : TEXT("missing");
}

bool WriteReport(UWorld* World, const FString& CapturePath, const FString& CaptureState, FString& OutPath)
{
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("utc"), FDateTime::UtcNow().ToIso8601());
	Root->SetNumberField(TEXT("catalogue_revision"), FDevKitCharacterLookDevPresets::Revision);
	Root->SetStringField(TEXT("preset_id"), ReadTag(World->GetWorldSettings(), PresetTagPrefix));
	Root->SetStringField(TEXT("stage_mode"), ReadTag(World->GetWorldSettings(), ModeTagPrefix));
	Root->SetStringField(TEXT("status"), TEXT("unapproved_candidate_or_template_copy_requires_visual_review"));
	Root->SetStringField(TEXT("limitations"), TEXT("Editor-world material/light preview; runtime LookVolume is Game/PIE only; no animation, gameplay camera BP, final room GI or SP pixel parity certification."));
	Root->SetStringField(TEXT("world"), World->GetPathName());
	Root->SetStringField(TEXT("project"), FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()));
	Root->SetStringField(TEXT("engine_root"), FPaths::ConvertRelativePathToFull(FPaths::EngineDir()));
	Root->SetStringField(TEXT("engine_version"), FEngineVersion::Current().ToString());
	Root->SetStringField(TEXT("engine_build_id"), ReadBuildId(FPaths::EngineDir() / TEXT("Binaries/Win64/UnrealEditor.modules")));
	Root->SetStringField(TEXT("project_build_id"), ReadBuildId(FPaths::ProjectDir() / TEXT("Binaries/Win64/UnrealEditor.modules")));
	Root->SetStringField(TEXT("capture_status"), CaptureState);
	Root->SetStringField(TEXT("capture_path"), CapturePath);
	Root->SetStringField(TEXT("hash_algorithm"), TEXT("SHA1"));
	TSharedRef<FJsonObject> Hashes = MakeShared<FJsonObject>();
	const FString Files[] = {
		FPaths::EngineDir() / TEXT("Build/Build.version"),
		FPaths::EngineDir() / TEXT("Shaders/Private/StylizedCharacterLighting.ush"),
		FPaths::EngineDir() / TEXT("Shaders/Private/StylizedCharacterReadability.usf"),
		FPaths::EngineDir() / TEXT("Shaders/Private/DeferredLightingCommon.ush"),
		FPaths::EngineDir() / TEXT("Shaders/Private/DiffuseIndirectComposite.usf"),
		FPaths::EngineDir() / TEXT("Shaders/Private/ShadingModels.ush"),
		FPaths::EngineDir() / TEXT("Shaders/Private/StylizedLumenLighting.ush"),
		FPaths::ProjectConfigDir() / TEXT("DefaultEngine.ini"),
		FPaths::ProjectConfigDir() / TEXT("DefaultScalability.ini"),
		FPaths::ProjectConfigDir() / TEXT("DefaultGame.ini"),
		FPaths::ProjectDir() / TEXT("Build/Dev01EngineRelease.txt"),
		FPaths::ProjectDir() / TEXT("Plugins/YogLookDev/Source/YogLookDevEditor/Private/Tools/CharacterLookDev/DevKitCharacterLookDevPresets.cpp")
	};
	for (const FString& File : Files) Hashes->SetStringField(FPaths::ConvertRelativePathToFull(File), HashFile(File));
	Root->SetObjectField(TEXT("source_and_config_hashes"), Hashes);
	TSharedRef<FJsonObject> CVars = MakeShared<FJsonObject>();
	IConsoleManager::Get().ForEachConsoleObjectThatStartsWith(FConsoleObjectVisitor::CreateLambda(
		[&CVars](const TCHAR* Name, IConsoleObject* Object)
		{
			if (IConsoleVariable* Variable = Object->AsVariable()) CVars->SetStringField(Name, Variable->GetString());
		}), TEXT("r.Stylized"));
	for (const TCHAR* Name : { TEXT("r.DynamicGlobalIlluminationMethod"), TEXT("r.ReflectionMethod"), TEXT("r.DefaultFeature.AutoExposure.ExtendDefaultLuminanceRange"), TEXT("r.EyeAdaptationQuality"), TEXT("r.LocalExposure"), TEXT("sg.GlobalIlluminationQuality"), TEXT("sg.PostProcessQuality") })
	{
		if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name)) CVars->SetStringField(Name, Variable->GetString());
	}
	Root->SetObjectField(TEXT("runtime_cvars_read_only"), CVars);
	TArray<TSharedPtr<FJsonValue>> Lights, PostProcess, Subjects, FogActors;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		TInlineComponentArray<ULightComponentBase*> Components(Actor);
		for (ULightComponentBase* Component : Components)
		{
			TSharedRef<FJsonObject> Item = DescribeActor(Actor);
			Item->SetStringField(TEXT("component"), Component->GetPathName());
			Item->SetStringField(TEXT("actual_light_color_linear"), Component->GetLightColor().ToString());
			AddProperties(Item, Component, { TEXT("LightingChannels") });
			Item->SetStringField(TEXT("unit"), Component->IsA<UDirectionalLightComponent>() ? TEXT("lux") : Component->IsA<USkyLightComponent>() ? TEXT("cubemap_multiplier") : PropertyText(Component->GetClass(), Component, TEXT("IntensityUnits")));
			AddProperties(Item, Component, { TEXT("Intensity"), TEXT("LightColor"), TEXT("Mobility"), TEXT("bVisible"), TEXT("bUseTemperature"), TEXT("Temperature"), TEXT("CastShadows"), TEXT("SpecularScale"), TEXT("DiffuseScale"), TEXT("StylizedCharacterLightMode"), TEXT("StylizedCharacterShadowStrength"), TEXT("AttenuationRadius"), TEXT("InverseExposureBlend"), TEXT("bUseInverseSquaredFalloff"), TEXT("SourceWidth"), TEXT("SourceHeight"), TEXT("SourceType"), TEXT("Cubemap"), TEXT("IndirectLightingIntensity") });
			Lights.Add(MakeShared<FJsonValueObject>(Item));
		}
		if (APostProcessVolume* Volume = Cast<APostProcessVolume>(Actor))
		{
			TSharedRef<FJsonObject> Item = DescribeActor(Actor);
			AddProperties(Item, Volume, { TEXT("bEnabled"), TEXT("bUnbound"), TEXT("Priority"), TEXT("BlendWeight"), TEXT("BlendRadius") });
			// Export the actual settings, including every override bit and any LUT/blendable references.
			Item->SetStringField(TEXT("settings_export_text"), PropertyText(Volume->GetClass(), Volume, TEXT("Settings")));
			PostProcess.Add(MakeShared<FJsonValueObject>(Item));
		}
		if (AExponentialHeightFog* Fog = Cast<AExponentialHeightFog>(Actor))
		{
			TSharedRef<FJsonObject> Item = DescribeActor(Actor);
			AddProperties(Item, Fog->GetComponent(), { TEXT("FogDensity"), TEXT("FogInscatteringLuminance"), TEXT("FogHeightFalloff"), TEXT("bEnableVolumetricFog"), TEXT("VolumetricFogAlbedo") });
			FogActors.Add(MakeShared<FJsonValueObject>(Item));
		}
		if (Actor->ActorHasTag(SubjectTag))
		{
			TSharedRef<FJsonObject> Item = DescribeActor(Actor);
			UMeshComponent* Mesh = Actor->FindComponentByClass<UMeshComponent>();
			if (UStaticMeshComponent* Static = Cast<UStaticMeshComponent>(Mesh)) Item->SetStringField(TEXT("mesh"), GetPathNameSafe(Static->GetStaticMesh()));
			if (USkeletalMeshComponent* Skeletal = Cast<USkeletalMeshComponent>(Mesh))
			{
				Item->SetStringField(TEXT("mesh"), GetPathNameSafe(Skeletal->GetSkeletalMeshAsset()));
				Item->SetStringField(TEXT("pose"), TEXT("reference_pose_no_animation"));
			}
			TArray<TSharedPtr<FJsonValue>> Materials;
			TArray<TSharedPtr<FJsonValue>> MaterialResponses;
			if (Mesh)
			{
				AddProperties(Item, Mesh, { TEXT("LightingChannels"), TEXT("CastShadow"), TEXT("bCastShadowOnOtherObjectsOnly"), TEXT("bCastContactShadow"), TEXT("bCastCapsuleDirectShadow") });
				for (int32 Index = 0; Index < Mesh->GetNumMaterials(); ++Index)
				{
					UMaterialInterface* Material = Mesh->GetMaterial(Index);
					Materials.Add(MakeShared<FJsonValueString>(GetPathNameSafe(Material)));
					if (!Material) continue;
					TSharedRef<FJsonObject> Response = MakeShared<FJsonObject>();
					Response->SetNumberField(TEXT("slot"), Index);
					Response->SetStringField(TEXT("material"), Material->GetPathName());
					Response->SetStringField(TEXT("base_material"), GetPathNameSafe(Material->GetMaterial()));
					Response->SetBoolField(TEXT("effective_unlit"), Material->GetShadingModels().HasShadingModel(MSM_Unlit));
					Response->SetBoolField(TEXT("effective_stylized_character_lit"), Material->GetShadingModels().HasShadingModel(MSM_StylizedCharacterLit));
					Response->SetNumberField(TEXT("shading_model_field"), Material->GetShadingModels().GetShadingModelField());
					MaterialResponses.Add(MakeShared<FJsonValueObject>(Response));
				}
			}
			Item->SetArrayField(TEXT("materials_by_slot"), Materials);
			Item->SetArrayField(TEXT("material_responses"), MaterialResponses);
			Subjects.Add(MakeShared<FJsonValueObject>(Item));
		}
	}
	Root->SetArrayField(TEXT("actual_lights"), Lights);
	Root->SetArrayField(TEXT("actual_post_process_volumes"), PostProcess);
	Root->SetArrayField(TEXT("actual_fog"), FogActors);
	Root->SetArrayField(TEXT("subjects"), Subjects);
	if (FLevelEditorViewportClient* Client = FindViewport(World))
	{
		TSharedRef<FJsonObject> Camera = MakeShared<FJsonObject>();
		Camera->SetStringField(TEXT("mode"), ReadTag(World->GetWorldSettings(), CameraTagPrefix));
		Camera->SetStringField(TEXT("location_cm"), Client->GetViewLocation().ToString());
		Camera->SetStringField(TEXT("rotation"), Client->GetViewRotation().ToString());
		Camera->SetNumberField(TEXT("fov_degrees"), Client->ViewFOV);
		Camera->SetStringField(TEXT("exposure_override"), Client->ExposureSettings.ToString());
		Camera->SetBoolField(TEXT("eye_adaptation_show_flag"), Client->EngineShowFlags.EyeAdaptation);
		Root->SetObjectField(TEXT("viewport_camera"), Camera);
	}
	const FString Directory = FPaths::ProjectSavedDir() / TEXT("CharacterLookDev");
	if (!IFileManager::Get().MakeDirectory(*Directory, true)) return false;
	OutPath = FPaths::ConvertRelativePathToFull(Directory / (TEXT("LookDev_") + FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S")) + TEXT("_") + FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8) + TEXT(".json")));
	FString Text;
	return FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Text))
		&& FFileHelper::SaveStringToFile(Text, *OutPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
}

bool FDevKitCharacterLookDevService::BuildLightingLevel(UWorld* World, const FString& PresetId, FText& OutMessage)
{
	const FDevKitCharacterLookDevPreset* Preset = FDevKitCharacterLookDevPresets::Find(PresetId);
	if (!World || !World->PersistentLevel || !World->GetWorldSettings() || !Preset
		|| World->WorldType == EWorldType::PIE
		|| !World->GetOutermost()->GetName().StartsWith(TEXT("/Game/Art/Map/LookDev/LightDev/")))
	{
		OutMessage = LOCTEXT("InvalidLightingLevel", "灯光构建需要 LookDev/LightDev 下的新关卡和有效灯光方案。");
		return false;
	}
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(RigTag))
		{
			OutMessage = LOCTEXT("ExistingLightingRig", "灯光关卡已包含灯光方案，保留当前可编辑内容。");
			return false;
		}
	}
	if (!BuildRig(World, *Preset, nullptr, OutMessage, false)) return false;
	World->MarkPackageDirty();
	OutMessage = FText::FromString(Preset->Label);
	return true;
}

bool FDevKitCharacterLookDevService::IsStageWorld()
{
	return IsStage(EditorWorld());
}

FText FDevKitCharacterLookDevService::GetStageDescription()
{
	if (GEditor && GEditor->IsPlaySessionInProgress())
		return LOCTEXT("StagePIEActive", "PIE / Simulate 正在运行，LookDev 编辑操作暂停。");
	UWorld* World = EditorWorld();
	if (!IsStage(World)) return LOCTEXT("NoActiveStage", "当前没有本会话未保存的 LookDev 舞台。保存或重载后的地图不再受此工具管理。");
	const FString Id = ReadTag(World->GetWorldSettings(), PresetTagPrefix);
	const FDevKitCharacterLookDevPreset* Preset = FDevKitCharacterLookDevPresets::Find(Id);
	const bool bTemplate = ReadTag(World->GetWorldSettings(), ModeTagPrefix) == TEXT("ExistingTemplateCopy");
	FText MaterialWarning;
	if (AActor* Subject = FindSubject(World))
	{
		if (UMeshComponent* Mesh = Subject->FindComponentByClass<UMeshComponent>())
		{
			for (int32 Index = 0; Index < Mesh->GetNumMaterials(); ++Index)
			{
				UMaterialInterface* Material = Mesh->GetMaterial(Index);
				if (Material && Material->GetShadingModels().HasShadingModel(MSM_Unlit))
				{
					MaterialWarning = LOCTEXT("PersistentUnlitWarning", "\n注意：角色含 Unlit 材质槽，不能作为原生灯角色受光验收。请核对正式材质覆盖。");
					break;
				}
			}
		}
	}
	return FText::Format(LOCTEXT("ActualStage", "实际舞台：{0} | {1} | 未保存临时预览 / 待验收{2}"),
		FText::FromString(bTemplate && Preset ? Preset->Layer : Preset ? Preset->Label : Id),
		bTemplate ? LOCTEXT("ActualTemplate", "现有灯光模板副本（不应用候选参数）") : LOCTEXT("ActualCandidate", "候选灯光档"), MaterialWarning);
}

bool FDevKitCharacterLookDevService::CreateStage(const FString& PresetId, bool bUseExistingTemplate, FText& OutMessage)
{
	if (!GEditor || GEditor->IsPlaySessionInProgress())
	{
		OutMessage = LOCTEXT("CreateStopPIE", "请先停止 PIE / Simulate，再创建 LookDev 舞台。");
		return false;
	}
	const FDevKitCharacterLookDevPreset* Preset = FDevKitCharacterLookDevPresets::Find(PresetId);
	if (!Preset)
	{
		OutMessage = LOCTEXT("PresetMissing", "未找到所选 LookDev 环境档。");
		return false;
	}
	UWorld* PreviousWorld = EditorWorld();
	TArray<UPackage*> Dirty;
	FEditorFileUtils::GetDirtyPackages(Dirty);
	for (UPackage* Package : Dirty)
	{
		if (IsStage(PreviousWorld) && Package == PreviousWorld->GetOutermost()) continue;
		OutMessage = FText::Format(LOCTEXT("DirtyPackages", "存在未保存内容：{0}。请先手动保存或处理，再新建 LookDev 舞台。"), FText::FromString(Package->GetName()));
		return false;
	}
	FString Template;
	if (bUseExistingTemplate)
	{
		Template = FDevKitCharacterLookDevPresets::GetLightTemplate(Preset->Layer);
		if (Template.IsEmpty() || !FPackageName::DoesPackageExist(Template))
		{
			OutMessage = FText::Format(LOCTEXT("TemplateMissing", "该环境没有可用的现有灯光模板：{0}。未创建或替换当前关卡。"), FText::FromString(Template));
			return false;
		}
	}
	// NewMap can collect the previous world's objects. Hold preflight resources until the new actors own them.
	TStrongObjectPtr<UStaticMesh> Cube(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	TStrongObjectPtr<UStaticMesh> Sphere(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
	const bool bNeedsSkyCube = !bUseExistingTemplate && Preset->SkyIntensity > 0.f;
	TStrongObjectPtr<UTextureCube> SkyCube(bNeedsSkyCube ? LoadObject<UTextureCube>(nullptr, GreyCubePath) : nullptr);
	if (!Cube.IsValid() || !Sphere.IsValid() || (bNeedsSkyCube && !SkyCube.IsValid()))
	{
		OutMessage = LOCTEXT("EngineResourcesMissing", "引擎基础网格或 T_GreyAmbient 资源缺失；未替换当前关卡。");
		return false;
	}
	const TWeakObjectPtr<UWorld> PreviousWorldIdentity(PreviousWorld);
	UWorld* World = nullptr;
	if (bUseExistingTemplate)
	{
		const FString ObjectPath = Template + TEXT(".") + FPackageName::GetLongPackageAssetName(Template);
		TStrongObjectPtr<UWorld> TemplateAsset(LoadObject<UWorld>(nullptr, *ObjectPath, nullptr, LOAD_NoWarn));
		if (!TemplateAsset.IsValid())
		{
			OutMessage = LOCTEXT("TemplateCannotLoad", "灯光模板无法加载为 World；当前关卡未替换，请查看日志。");
			return false;
		}
		// Unlike NewMapFromTemplate, propagate LoadMap's failure instead of receiving the old world.
		const FString Filename = FPackageName::LongPackageNameToFilename(Template, FPackageName::GetMapPackageExtension());
		// The source may itself be the clean, currently open map. Do not pin it across world teardown.
		TemplateAsset.Reset();
		if (!FEditorFileUtils::LoadMap(Filename, /*LoadAsTemplate=*/true, /*bShowProgress=*/false))
		{
			OutMessage = LOCTEXT("TemplateLoadFailed", "创建模板副本失败；未向当前世界添加标签、灯光或参照物，请查看日志。");
			return false;
		}
		World = EditorWorld();
	}
	else
	{
		World = GEditor->NewMap();
	}
	if (!World || World == PreviousWorldIdentity.Get() || World->WorldType != EWorldType::Editor
		|| FPackageName::DoesPackageExist(World->GetOutermost()->GetName()))
	{
		OutMessage = LOCTEXT("NewMapFailed", "没有得到新的未保存编辑器世界；未添加工具标签或舞台对象，请查看日志。");
		return false;
	}
	ActiveStageWorld = World;
	World->GetWorldSettings()->Tags.AddUnique(StageTag);
	WriteTag(World->GetWorldSettings(), PresetTagPrefix, PresetId);
	WriteTag(World->GetWorldSettings(), ModeTagPrefix, bUseExistingTemplate ? TEXT("ExistingTemplateCopy") : TEXT("CandidateRig"));
	AddReferences(World, Cube.Get(), Sphere.Get());
	if (!bUseExistingTemplate && !BuildRig(World, *Preset, SkyCube.Get(), OutMessage)) return false;
	FText CameraMessage;
	SetCamera(false, CameraMessage);
	World->MarkPackageDirty();
	GEditor->RedrawLevelEditingViewports();
	OutMessage = bUseExistingTemplate
		? LOCTEXT("TemplateStageReady", "已打开灯光模板的未保存副本；原灯光与后处理保留。仅临时预览，保存后不再受工具管理；不包含完整关卡 GI 或 PIE LookVolume。")
		: LOCTEXT("CandidateStageReady", "已创建未保存的候选灯光舞台与灰球/金属球。仅临时预览，保存后不再受工具管理；候选效果待确认，俯视仅为源码镜头近似。");
	return true;
}

bool FDevKitCharacterLookDevService::ApplyPreset(const FString& PresetId, FText& OutMessage)
{
	if (!RequireStage(OutMessage)) return false;
	UWorld* World = EditorWorld();
	if (ReadTag(World->GetWorldSettings(), ModeTagPrefix) != TEXT("CandidateRig"))
	{
		OutMessage = LOCTEXT("TemplateProtected", "模板对照模式保留原始灯光。切换模板请新建对照舞台；应用候选档请新建候选舞台。");
		return false;
	}
	const FDevKitCharacterLookDevPreset* Preset = FDevKitCharacterLookDevPresets::Find(PresetId);
	UTextureCube* Cube = Preset && Preset->SkyIntensity > 0.f ? LoadObject<UTextureCube>(nullptr, GreyCubePath) : nullptr;
	if (!Preset || (Preset->SkyIntensity > 0.f && !Cube))
	{
		OutMessage = LOCTEXT("ApplyMissing", "环境档或中性天空 Cubemap 缺失，未改变舞台。");
		return false;
	}
	DestroyTaggedActors(World, RigTag);
	if (!BuildRig(World, *Preset, Cube, OutMessage)) return false;
	if (FLevelEditorViewportClient* Client = FindViewport(World))
	{
		Client->ExposureSettings.bFixed = false;
		Client->EngineShowFlags.SetEyeAdaptation(true);
	}
	GEditor->RedrawLevelEditingViewports();
	OutMessage = FText::Format(LOCTEXT("Applied", "已应用候选档：{0}。角色材质与项目 Profile 沿用当前引擎设置。"), FText::FromString(Preset->Label));
	return true;
}

bool FDevKitCharacterLookDevService::SetSubject(UObject* MeshAsset, FText& OutMessage)
{
	if (!RequireStage(OutMessage)) return false;
	UStaticMesh* StaticMesh = Cast<UStaticMesh>(MeshAsset);
	USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(MeshAsset);
	if (!StaticMesh && !SkeletalMesh)
	{
		OutMessage = LOCTEXT("SelectMesh", "请在内容浏览器选择一个 StaticMesh 或 SkeletalMesh 角色资源。");
		return false;
	}
	UWorld* World = EditorWorld();
	AActor* NewSubject = nullptr;
	UPrimitiveComponent* Component = nullptr;
	if (StaticMesh)
	{
		if (AStaticMeshActor* Actor = Spawn<AStaticMeshActor>(World, TEXT("LookDev Subject"), SubjectTag))
		{
			Actor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
			Actor->GetStaticMeshComponent()->SetStaticMesh(StaticMesh);
			Component = Actor->GetStaticMeshComponent();
			NewSubject = Actor;
		}
	}
	else if (ASkeletalMeshActor* Actor = Spawn<ASkeletalMeshActor>(World, TEXT("LookDev Subject (reference pose)"), SubjectTag))
	{
		Actor->GetSkeletalMeshComponent()->SetSkeletalMesh(SkeletalMesh);
		Actor->GetSkeletalMeshComponent()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		Actor->GetSkeletalMeshComponent()->bForceRefpose = true;
		Component = Actor->GetSkeletalMeshComponent();
		NewSubject = Actor;
	}
	if (!NewSubject || !Component)
	{
		OutMessage = LOCTEXT("SubjectFailed", "角色创建失败，原角色未移除。");
		return false;
	}
	if (SkeletalMesh)
	{
		// Reuse the same native shadow policy as AYogCharacterBase instead of a second preview policy.
		UStylizedCharacterShadowPolicyComponent* Policy = NewObject<UStylizedCharacterShadowPolicyComponent>(NewSubject, NAME_None, RF_Transient);
		NewSubject->AddInstanceComponent(Policy);
		Policy->RegisterComponent();
	}
	TArray<AActor*> PreviousSubjects;
	for (TActorIterator<AActor> It(World); It; ++It) if (*It != NewSubject && It->ActorHasTag(SubjectTag)) PreviousSubjects.Add(*It);
	for (AActor* Actor : PreviousSubjects) World->DestroyActor(Actor);
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->UpdateBounds();
	FVector Origin, Extent;
	NewSubject->GetActorBounds(false, Origin, Extent);
	NewSubject->SetActorLocation(FVector(-Origin.X, -Origin.Y, Extent.Z - Origin.Z));
	FText CameraMessage;
	SetCamera(false, CameraMessage);
	GEditor->RedrawLevelEditingViewports();
	OutMessage = LOCTEXT("SubjectReady", "已加载角色，保留网格原材质并将包围盒底部对齐舞台。骨骼网格使用参考姿态。");
	if (UMeshComponent* MeshComponent = Cast<UMeshComponent>(Component))
	{
		for (int32 Index = 0; Index < MeshComponent->GetNumMaterials(); ++Index)
		{
			UMaterialInterface* Material = MeshComponent->GetMaterial(Index);
			if (Material && Material->GetShadingModels().HasShadingModel(MSM_Unlit))
			{
				OutMessage = LOCTEXT("SubjectUnlit", "已保留原材质载入，但检测到 Unlit 材质槽：可能使用旧 Celes 数据灯，不能据此验收原生灯角色受光。请核对正式角色的材质覆盖；工具不会自动换材质。");
				break;
			}
		}
	}
	return true;
}

bool FDevKitCharacterLookDevService::RotateSubject(float DeltaDegrees, FText& OutMessage)
{
	if (!RequireStage(OutMessage)) return false;
	AActor* Subject = FindSubject(EditorWorld());
	if (!Subject || !FMath::IsFinite(DeltaDegrees))
	{
		OutMessage = LOCTEXT("NeedSubject", "请先载入角色网格。");
		return false;
	}
	const FRotator Rotation(0, DeltaDegrees, 0);
	Subject->AddActorWorldRotation(Rotation);
	Subject->SetActorLocation(Rotation.RotateVector(Subject->GetActorLocation()));
	GEditor->RedrawLevelEditingViewports();
	OutMessage = LOCTEXT("SubjectRotated", "已旋转角色；灯光与曝光保持当前环境档。");
	return true;
}

bool FDevKitCharacterLookDevService::SetCamera(bool bGameplay, FText& OutMessage)
{
	if (!RequireStage(OutMessage)) return false;
	UWorld* World = EditorWorld();
	FLevelEditorViewportClient* Client = FindViewport(World);
	if (!Client)
	{
		OutMessage = LOCTEXT("NeedViewport", "没有可用的关卡透视视口，请打开一个关卡透视视口。");
		return false;
	}
	const APlayerCharacterBase* Defaults = GetDefault<APlayerCharacterBase>();
	const float FOV = Defaults->DefaultCameraFOV;
	FVector Target(0,0,90), Extent(60,60,90);
	if (AActor* Subject = FindSubject(World)) Subject->GetActorBounds(false, Target, Extent);
	const float Distance = bGameplay ? Defaults->DefaultCameraBoomLength
		: FMath::Max(300.f, Extent.Size() / FMath::Tan(FMath::DegreesToRadians(FOV * 0.5f)) * 1.5f);
	const FVector Direction = bGameplay ? FVector(-1,-1,1.5).GetSafeNormal() : FVector(-1,-0.25,0.15).GetSafeNormal();
	const FVector Location = Target + Direction * Distance;
	Client->SetViewLocation(Location);
	Client->SetLookAtLocation(Target);
	Client->SetViewRotation((Target - Location).Rotation());
	Client->ViewFOV = FOV;
	Client->FOVAngle = FOV;
	Client->SetViewMode(VMI_Lit);
	Client->SetGameView(true); // Hide editor icons and the grid from material comparison captures.
	Client->SetRealtime(true);
	Client->ExposureSettings.bFixed = false;
	Client->EngineShowFlags.SetEyeAdaptation(true);
	Client->Invalidate();
	WriteTag(World->GetWorldSettings(), CameraTagPrefix, bGameplay ? TEXT("GameplayProxy_SourceDefaults_NotBlueprintCamera") : TEXT("MaterialCloseView"));
	OutMessage = bGameplay ? LOCTEXT("GameplayProxy", "已切换俯视近似镜头：使用源码默认 FOV/臂长，未读取角色 Blueprint 实际镜头。") : LOCTEXT("CloseView", "已切换材质近景，使用场景曝光。");
	return true;
}

bool FDevKitCharacterLookDevService::ExportReport(FText& OutMessage)
{
	if (!RequireStage(OutMessage)) return false;
	FString Path;
	if (!WriteReport(EditorWorld(), FString(), TEXT("not_requested"), Path))
	{
		OutMessage = LOCTEXT("ReportFailed", "无法写入 Saved/CharacterLookDev 检查报告。");
		return false;
	}
	OutMessage = FText::Format(LOCTEXT("ReportReady", "已导出实际灯光、后处理、角色与版本证据：{0}"), FText::FromString(Path));
	return true;
}

bool FDevKitCharacterLookDevService::Capture(FText& OutMessage)
{
	if (!RequireStage(OutMessage)) return false;
	FLevelEditorViewportClient* Client = FindViewport(EditorWorld());
	if (!Client || !Client->Viewport || Client->Viewport->GetSizeXY().GetMin() <= 0)
	{
		OutMessage = LOCTEXT("CaptureViewportMissing", "没有可用于截图的关卡透视视口。");
		return false;
	}
	const FString Directory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("CharacterLookDev"));
	if (!IFileManager::Get().MakeDirectory(*Directory, true))
	{
		OutMessage = LOCTEXT("CaptureFolderFailed", "无法创建 Saved/CharacterLookDev 截图目录。");
		return false;
	}
	const FString Filename = Directory / (TEXT("LookDev_") + FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S")) + TEXT("_") + FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8) + TEXT(".png"));
	GetHighResScreenshotConfig().FilenameOverride = Filename;
	if (!Client->Viewport->TakeHighResScreenShot())
	{
		OutMessage = LOCTEXT("CaptureRequestFailed", "编辑器拒绝截图请求，请检查输出日志。");
		return false;
	}
	Client->Invalidate();
	FString ReportPath;
	const bool bReportWritten = WriteReport(EditorWorld(), Filename, TEXT("requested_not_verified_complete"), ReportPath);
	OutMessage = FText::Format(LOCTEXT("CaptureRequested", "已请求编辑器截图：{0}。请求已发送，完成情况请检查文件/日志。报告：{1}"),
		FText::FromString(Filename), FText::FromString(bReportWritten ? ReportPath : TEXT("写入失败")));
	return true;
}

#undef LOCTEXT_NAMESPACE
