#pragma once

#include "CoreMinimal.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

//https://medium.com/@chrhaase_71293/an-unreal-engine-saving-loading-system-part-1-of-2-62244e55e4b2
//https://www.tomlooman.com/unreal-engine-cpp-save-system/
//https://unrealcommunity.wiki/savegame-pointers-and-structs-8wlg0qms

//first Need to create fundemantion stuff -> subsystem, custom serializer and custom save game class, I have the last one?
// 
// 
//

//new struct for serializer
struct FYogSaveGameArchive : public FObjectAndNameAsStringProxyArchive
{
	FYogSaveGameArchive(FArchive& InInnerArchive)
		:FObjectAndNameAsStringProxyArchive(InInnerArchive, false)
	{
		ArIsSaveGame = true;
		ArNoDelta = true;
	}

};