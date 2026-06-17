#pragma once

#include "CoreMinimal.h"
#include "CelesLightTypes.h"
#include "GameFramework/Actor.h"
#include "CelesLightCaptureBox.generated.h"

class UBillboardComponent;
class UBoxComponent;
class UPointLightComponent;
class UTextureRenderTarget2D;

UCLASS(BlueprintType, Blueprintable, ClassGroup = (Rendering), meta = (DisplayName = "Celes Light Capture Box"))
class CELESLIGHTRUNTIME_API ACelesLightCaptureBox : public AActor
{
	GENERATED_BODY()

public:
	ACelesLightCaptureBox(const FObjectInitializer& ObjectInitializer);

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

#if WITH_EDITOR
	virtual bool ShouldTickIfViewportsOnly() const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Celes Light|更新", meta = (DisplayName = "Update Celes Light", ToolTip = "重新读取盒体范围内的灯光，并写入 Light Info Render Target。"))
	void UpdateCelesLight();

	UFUNCTION(BlueprintCallable, Category = "Celes Light|采集", meta = (ToolTip = "读取盒体范围内的 Celes 灯光数据，不会修改材质。"))
	void CollectLightsInBox(TArray<FCelesLightSourceData>& OutLights) const;

	UFUNCTION(BlueprintPure, Category = "Celes Light|RT")
	UTextureRenderTarget2D* GetLightInfoRenderTarget() const;

	UFUNCTION(BlueprintPure, Category = "Celes Light|状态")
	int32 GetLastEncodedLightCount() const;

	UFUNCTION(BlueprintPure, Category = "Celes Light|状态")
	const TArray<FCelesLightSourceData>& GetLastCapturedLights() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Celes Light|组件")
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Celes Light|组件")
	TObjectPtr<UBoxComponent> CaptureBounds = nullptr;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Celes Light|组件")
	TObjectPtr<UBillboardComponent> Billboard = nullptr;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|RT", meta = (DisplayName = "Light Info Render Target", ToolTip = "写入灯光信息的 RT/CRT。可以直接拖入已有 Render Target，也可以在详情面板点击创建。"))
	TObjectPtr<UTextureRenderTarget2D> LightInfoRenderTarget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|RT", meta = (ClampMin = "1", ClampMax = "16", UIMin = "1", UIMax = "16", DisplayName = "Max Light Count", ToolTip = "这个盒体最多写入的灯光数量，默认 4，最高 16。超过数量的灯光会进入 Overflow 列表，不会写入 RT。"))
	int32 MaxLightCount = CelesLight::DefaultLightInfoCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|采集", meta = (ToolTip = "开启后，除了 CelesPointLight，也会采集盒体内普通 PointLightComponent。"))
	bool bIncludeNativePointLights = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|采集", meta = (ToolTip = "当盒体内灯光数量超过 Max Light Count 时，用这个策略决定哪些灯光写入 RT。"))
	ECelesLightSelectionMode LightSelectionMode = ECelesLightSelectionMode::NearestToBoxCenter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|采集", meta = (ToolTip = "为空时采集所有灯光；填写后，只采集带有任一匹配 Actor Tag 的灯光。"))
	TArray<FName> RequiredLightTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|编辑器更新", meta = (ToolTip = "在编辑器非运行状态下自动 Tick 更新 RT。"))
	bool bUpdateInEditor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|编辑器更新", meta = (ClampMin = "0.01", UIMin = "0.05", ToolTip = "编辑器自动更新间隔，单位秒。"))
	float EditorUpdateInterval = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|运行时更新", meta = (ToolTip = "运行时自动 Tick 写入 RT。默认关闭，避免额外采样成本。"))
	bool bUpdateAtRuntime = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Celes Light|运行时更新", meta = (ClampMin = "0.01", UIMin = "0.05", ToolTip = "运行时自动更新间隔，单位秒。"))
	float RuntimeUpdateInterval = 0.1f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Celes Light|状态")
	int32 LastAvailableLightCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Celes Light|状态")
	int32 LastEncodedLightCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Celes Light|状态")
	int32 LastOverflowLightCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Celes Light|状态")
	TArray<FCelesLightSourceData> LastCapturedLights;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Celes Light|状态")
	TArray<FCelesLightSourceData> LastOverflowLights;

private:
	bool IsLocationInsideCaptureBox(const FVector& WorldLocation) const;
	bool ShouldCaptureActor(const AActor* Actor) const;
	void AddNativePointLightData(const UPointLightComponent* PointLightComponent, TArray<FCelesLightSourceData>& OutLights) const;
	void SortLightData(TArray<FCelesLightSourceData>& InOutLights) const;
	void DrawPixelsToRenderTarget(UTextureRenderTarget2D* RenderTarget, const TArray<FLinearColor>& Pixels) const;
	int32 GetClampedMaxLightCount() const;
	bool ShouldRunEditorUpdate() const;

	float EditorTickAccumulator = 0.0f;
	float RuntimeTickAccumulator = 0.0f;
};
