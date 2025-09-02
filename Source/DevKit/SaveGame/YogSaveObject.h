// Copyright (C), HEXWORKS by CIGames, 2020-2022.
// Authors: Jose David Tuero, jdtuero@cigames.com

#pragma once

#include "CoreMinimal.h"
#include "YogSaveObject.generated.h"

//LOTF2 Save Object base class. 
//Basic version control is provided with this, call SetVersionNumber in you constructor to set the current version
//on load OnBuiltFromOldVersion will be called if the system detect and old game version, it will do its best efford to match old with new data (based in variable names), and you can use OnBuiltFromOldVersion for any extra adjusment needed
UCLASS()
class DEVKIT_API UYogSaveObject : public UObject
{
	GENERATED_BODY()
	
public:

};

