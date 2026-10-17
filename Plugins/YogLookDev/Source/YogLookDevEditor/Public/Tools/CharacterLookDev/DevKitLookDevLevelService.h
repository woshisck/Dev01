#pragma once

#include "CoreMinimal.h"

/** A saved review or lighting sublevel. Additional maps in the same folders appear automatically. */
struct FDevKitLookDevLevelEntry
{
	FString Id;
	FText Label;
	FString PackagePath;
	FText Description;
	bool bStandalone = false;
};

/** Session-only camera reference; never serialized into an authored Review map. */
struct YOGLOOKDEVEDITOR_API FDevKitLookDevCameraReference
{
	FVector TargetToCameraOffset = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;
	float FieldOfView = 50.f;
	float AspectRatio = 16.f / 9.f;
	FText Source;
	bool bFromPIE = false;
	bool IsValid() const;
};

/** Persistent LookDev levels; never replaces an authored review or lighting map. */
class YOGLOOKDEVEDITOR_API FDevKitLookDevLevelService
{
public:
	static FString MainPackagePath();
	static TArray<FDevKitLookDevLevelEntry> GetReviewEntries();
	static TArray<FDevKitLookDevLevelEntry> GetLightingEntries();
	static TArray<FString> GetRetiredLightingPackagePaths();
	static bool PruneRetiredLightingReferences(FText& OutMessage);
	static bool OpenMain(FText& OutMessage);
	static bool LoadReview(const FString& PackagePath, FText& OutMessage);
	static bool SwitchLighting(const FString& PackagePath, FText& OutMessage);
	static bool IsMainWorld();
	static FString GetCurrentReviewPath();
	static FString GetCurrentLightingPath();
	static bool CreateMissingAssets(FText& OutMessage, TArray<FString>* OutCreated = nullptr);
	// These actions move only the active LookDev viewport, never actors or map assets.
	static bool ReadPlayerCameraClass(UClass* PlayerClass, FDevKitLookDevCameraReference& OutCamera, FText& OutMessage);
	static bool ReadPlayerCameraBlueprint(const FString& ObjectPath, FDevKitLookDevCameraReference& OutCamera, FText& OutMessage);
	static bool CapturePIECamera(FDevKitLookDevCameraReference& OutCamera, FText& OutMessage);
	static bool MakeManualCamera(float DistanceCm, float FOV, float Pitch, float Yaw, FDevKitLookDevCameraReference& OutCamera, FText& OutMessage);
	static bool ApplyCameraReference(const FDevKitLookDevCameraReference& Camera, FText& OutMessage);
	static bool FrameSelectedSubject(FText& OutMessage);
	static bool SetPreviewViewMode(int32 Mode, FName VisualizationName, FText& OutMessage);
	static FText GetRenderingDiagnostics();
};
