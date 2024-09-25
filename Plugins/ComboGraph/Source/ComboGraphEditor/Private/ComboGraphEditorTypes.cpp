// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "ComboGraphEditorTypes.h"

const FName FComboGraphEditorModes::ComboGraphPersonaModeID("ComboGraph_ComboGraphPersonaMode");
const FName FComboGraphEditorModes::ComboGraphDefaultModeID("ComboGraph_ComboGraphDefaultMode");

const FName UComboGraphPinNames::PinCategory_EntryOut("Entry");
const FName UComboGraphPinNames::PinCategory_MultipleNodes("MultipleNodes");
const FName UComboGraphPinNames::PinCategory_Transition("Transition");
const FName UComboGraphPinNames::PinCategory_Edge("Edge");

const FName UComboGraphPinNames::PinName_InLeft("In_Left");
const FName UComboGraphPinNames::PinName_InTop("In_Top");
const FName UComboGraphPinNames::PinName_OutRight("Out_Right");
const FName UComboGraphPinNames::PinName_OutBottom("Out_Bottom");

UComboGraphPinNames::UComboGraphPinNames(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}
