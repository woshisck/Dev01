#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CelesLightTypes.h"
#include "CelesLightSourceInterface.generated.h"

class UPointLightComponent;

UINTERFACE(BlueprintType)
class CELESLIGHTRUNTIME_API UCelesLightSourceInterface : public UInterface
{
	GENERATED_BODY()
};

class CELESLIGHTRUNTIME_API ICelesLightSourceInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Celes Light")
	UPointLightComponent* GetLight() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Celes Light")
	AActor* GetActor() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Celes Light")
	void GetCelesLightData(FCelesLightSourceData& OutData) const;
};
