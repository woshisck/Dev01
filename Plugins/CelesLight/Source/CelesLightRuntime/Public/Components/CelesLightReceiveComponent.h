#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CelesLightTypes.h"
#include "CelesLightReceiveComponent.generated.h"

class UMaterialInstanceDynamic;
class UTextureRenderTarget2D;

UCLASS(ClassGroup = (Rendering), meta = (DeprecatedNode, DeprecationMessage = "Use CelesLightCaptureBox to write light data into a render target."))
class CELESLIGHTRUNTIME_API UCelesLightReceiveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCelesLightReceiveComponent();

	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Celes Light")
	void InitCelesLightReceive(bool bInEditor = false);

	UFUNCTION(BlueprintCallable, Category = "Celes Light")
	void UpdateLightInfo(bool bInEditor = false);

	UFUNCTION(BlueprintCallable, Category = "Celes Light")
	void ApplyRTToMaterial(bool bInEditor = false);

	UFUNCTION(BlueprintCallable, Category = "Celes Light")
	UTextureRenderTarget2D* CreateRT(bool bInEditor = false);

	UFUNCTION(BlueprintCallable, Category = "Celes Light")
	void ClearNotvalidLight(bool bInEditor = false);

	UFUNCTION(BlueprintCallable, Category = "Celes Light")
	void RegistLight(UObject* RegistLightInterface, bool bInEditor = false);

	UFUNCTION(BlueprintCallable, Category = "Celes Light")
	void DeregistLight(UObject* RegistLightInterface, bool bInEditor = false);

	UFUNCTION(BlueprintCallable, Category = "Celes Light")
	void ClearRegisteredLights(bool bInEditor = false);

	UFUNCTION(BlueprintCallable, Category = "Celes Light")
	void RefreshLightSources(bool bInEditor = false);

	UFUNCTION(BlueprintPure, Category = "Celes Light")
	UTextureRenderTarget2D* GetActiveRenderTarget(bool bInEditor = false) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|Render Target", meta = (ClampMin = "1", ClampMax = "16", UIMin = "1", UIMax = "16"))
	int32 LightInfoCount = CelesLight::DefaultLightInfoCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|Render Target")
	TObjectPtr<UTextureRenderTarget2D> RT_LightInfo = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|Render Target")
	TObjectPtr<UTextureRenderTarget2D> RT_LightInfo_InEditor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|Material")
	FName LightInfoTextureParameterName = CelesLight::LightInfoTextureParameterName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|Material")
	FName LightInfoCountParameterName = CelesLight::LightInfoCountParameterName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|Material")
	FName ReceiveMeshTag = CelesLight::ReceiveMeshTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|Trace")
	bool bAutoAddReceiveActorTag = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|Trace")
	FName ReceiveActorTag = CelesLight::ReceiveActorTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|Trace", meta = (ClampMin = "0.0"))
	float TraceRadius = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|Trace")
	TArray<FName> FilterActorTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|Realtime")
	bool bRealtimeSampling = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|Realtime", meta = (ClampMin = "0.01"))
	float RealtimeUpdateInterval = 0.1f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Celes Light|Runtime")
	TArray<TObjectPtr<UObject>> RegistLightList;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Celes Light|Runtime")
	TArray<TObjectPtr<UObject>> RegistLightList_InEditor;

private:
	TArray<TObjectPtr<UObject>>& GetLightList(bool bInEditor);
	const TArray<TObjectPtr<UObject>>& GetLightList(bool bInEditor) const;
	void DrawPixelsToRenderTarget(UTextureRenderTarget2D* RenderTarget, const TArray<FLinearColor>& Pixels) const;
	void CollectLightData(bool bInEditor, TArray<FCelesLightSourceData>& OutSources) const;
};
