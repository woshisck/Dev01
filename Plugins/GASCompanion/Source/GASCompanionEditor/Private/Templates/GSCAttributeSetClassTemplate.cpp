// Copyright 2021 Mickael Daniel. All Rights Reserved.


#include "Templates/GSCAttributeSetClassTemplate.h"

#include "GameProjectUtils.h"
#include "GeneralProjectSettings.h"
#include "Abilities/Attributes/GSCAttributeSetBase.h"
#include "Core/Logging/GASCompanionEditorLog.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"

#define LOCTEXT_NAMESPACE "AttributeSetClassTemplate"

UGSCAttributeSetClassTemplate::UGSCAttributeSetClassTemplate(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PluginName = TEXT("GASCompanion");
	SetGeneratedBaseClass(UGSCAttributeSetBase::StaticClass());
}

FString UGSCAttributeSetClassTemplate::GetFilename() const
{
	return TEXT("AttributeSetClass");
}

FString UGSCAttributeSetClassTemplate::GetDirectory() const
{
	return GetPluginTemplateDirectory();
}

FString UGSCAttributeSetClassTemplate::GetPluginTemplateDirectory()
{
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("GASCompanion"));
	if (Plugin.IsValid())
	{
		return Plugin->GetBaseDir() / TEXT("Templates") / TEXT("ClassTemplates");
	}

	return UClassTemplateEditorSubsystem::GetEngineTemplateDirectory();
}

bool UGSCAttributeSetClassTemplate::PrepareHeaderTemplate(TArray<FGSCAttributeDefinition> Attributes, FString TemplateText, FText& OutFailReason)
{
	const bool bReplaceDone = ReplaceAttributePlaceholder(Attributes, TemplateText, OutFailReason);
	if (!bReplaceDone)
	{
		EDITOR_LOG(Error, TEXT("ClassTemplate:PrepareHeaderTemplate() Error replacing attribute placeholders: %s"), *OutFailReason.ToString())
		return false;
	}

	const FString OutputFilename = GetPluginTemplateDirectory() / TEXT("AttributeSetClass.h.template");

	EDITOR_LOG(Display, TEXT("ClassTemplate:PrepareHeaderTemplate() Write Output to: %s"), *OutputFilename)
	return WriteOutputFile(OutputFilename, TemplateText, OutFailReason);
}

bool UGSCAttributeSetClassTemplate::PrepareSourceTemplate(TArray<FGSCAttributeDefinition> Attributes, FString TemplateText, FText& OutFailReason)
{
	const bool bReplaceDone = ReplaceAttributePlaceholder(Attributes, TemplateText, OutFailReason);
	if (!bReplaceDone)
	{
		EDITOR_LOG(Error, TEXT("ClassTemplate:PrepareSourceTemplate() Error replacing attribute placeholders: %s"), *OutFailReason.ToString())
		return false;
	}

	const FString OutputFilename = GetPluginTemplateDirectory() / TEXT("AttributeSetClass.cpp.template");

	EDITOR_LOG(Display, TEXT("ClassTemplate:PrepareSourceTemplate() Write Output to: %s"), *OutputFilename)
	return WriteOutputFile(OutputFilename, TemplateText, OutFailReason);
}

bool UGSCAttributeSetClassTemplate::ResetTemplates(FText& OutFailReason)
{
	FString OriginalHeaderTemplate;
	FString OriginalSourceTemplate;
	const FString OutputHeaderFilename = GetPluginTemplateDirectory() / TEXT("AttributeSetClass.h.template");
	const FString OutputSourceFilename = GetPluginTemplateDirectory() / TEXT("AttributeSetClass.cpp.template");

	if (!ReadTemplateFile(TEXT("AttributeSetClass.h.template.original"), OriginalHeaderTemplate, OutFailReason))
	{
		return false;
	}

	if (!ReadTemplateFile(TEXT("AttributeSetClass.cpp.template.original"), OriginalSourceTemplate, OutFailReason))
	{
		return false;
	}

	if (!WriteOutputFile(OutputHeaderFilename, OriginalHeaderTemplate, OutFailReason))
	{
		return false;
	}

	return WriteOutputFile(OutputSourceFilename, OriginalSourceTemplate, OutFailReason);
}

bool UGSCAttributeSetClassTemplate::ReplaceAttributePlaceholder(const TArray<FGSCAttributeDefinition> Attributes, FString& OutTemplateText, FText& OutFailReason)
{
	EDITOR_LOG(Verbose, TEXT("ClassTemplate:ReplaceAttributePlaceholder() Alter template here"))

	FString AttributeDeclarationTemplate;
	FString AttributeOnRepDeclarationTemplate;
	FString AttributeOnRepDefinitionTemplate;
	FString AttributeDOREPLIFETIMEDefinitionTemplate;


	if (!ReadTemplateFile(TEXT("FGameplayAttribute_OnRep_Declaration.template"), AttributeOnRepDeclarationTemplate, OutFailReason))
	{
		return false;
	}

	if (!ReadTemplateFile(TEXT("FGameplayAttribute_OnRep_Definition.template"), AttributeOnRepDefinitionTemplate, OutFailReason))
	{
		return false;
	}

	if (!ReadTemplateFile(TEXT("FGameplayAttribute_DOREPLIFETIME_Definition.template"), AttributeDOREPLIFETIMEDefinitionTemplate, OutFailReason))
	{
		return false;
	}

	// Not all of these will exist in every class template
	FString FinalOutput = OutTemplateText.Replace(TEXT("// %ATTRIBUTES_DECLARATION%"), *MakeAttributesDeclaration(Attributes), ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("// %ATTRIBUTES_ONREP_DECLARATION%"), *MakeAttributesOnRepDeclaration(Attributes), ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("// %ATTRIBUTES_DOREPLIFETIME_DEFINITION%"), *MakeAttributesDoRepLifetimeDefinition(Attributes), ESearchCase::CaseSensitive);
	FinalOutput = FinalOutput.Replace(TEXT("// %ATTRIBUTES_ONREP_DEFINITION%"), *MakeAttributesOnRepDefinition(Attributes), ESearchCase::CaseSensitive);

	OutTemplateText = FinalOutput;
	return true;
}

FString UGSCAttributeSetClassTemplate::MakeAttributesDeclaration(TArray<FGSCAttributeDefinition> InAttributesList)
{
	FText FailReason;
	FString Template;
	if (!ReadTemplateFile(TEXT("FGamplayAttributeDeclaration.template"), Template, FailReason))
	{
		EDITOR_LOG(Error, TEXT("ClassTemplate:MakeAttributesDeclaration() Error reading template: %s"), *FailReason.ToString())
		return FString();
	}

	TArray<FString> Outputs;
	for (FGSCAttributeDefinition Attribute : InAttributesList)
	{
		FString Output = Template.Replace(TEXT("%ATTRIBUTE_NAME%"), *Attribute.AttributeName, ESearchCase::CaseSensitive);
		Output = Output.Replace(TEXT("%ATTRIBUTE_CATEGORY%"), *Attribute.Category, ESearchCase::CaseSensitive);
		Output = Output.Replace(TEXT("%ATTRIBUTE_VALUE%"), *FString::SanitizeFloat(Attribute.DefaultValue), ESearchCase::CaseSensitive);
		Outputs.Add(Output);
	}

	return FString::Join(Outputs, LINE_TERMINATOR);
}

FString UGSCAttributeSetClassTemplate::MakeAttributesOnRepDeclaration(TArray<FGSCAttributeDefinition> InAttributesList)
{
	FText FailReason;
	FString Template;
	if (!ReadTemplateFile(TEXT("FGameplayAttribute_OnRep_Declaration.template"), Template, FailReason))
	{
		EDITOR_LOG(Error, TEXT("ClassTemplate:MakeAttributesOnRepDeclaration() Error reading template: %s"), *FailReason.ToString())
		return FString();
	}

	TArray<FString> Outputs;
	for (FGSCAttributeDefinition Attribute : InAttributesList)
	{
		FString Output = Template.Replace(TEXT("%ATTRIBUTE_NAME%"), *Attribute.AttributeName, ESearchCase::CaseSensitive);
		Outputs.Add(Output);
	}

	return FString::Join(Outputs, LINE_TERMINATOR);
}

FString UGSCAttributeSetClassTemplate::MakeAttributesDoRepLifetimeDefinition(TArray<FGSCAttributeDefinition> InAttributesList)
{
	FText FailReason;
	FString Template;
	if (!ReadTemplateFile(TEXT("FGameplayAttribute_DOREPLIFETIME_Definition.template"), Template, FailReason))
	{
		EDITOR_LOG(Error, TEXT("ClassTemplate:MakeAttributesDoRepLifetimeDefinition() Error reading template: %s"), *FailReason.ToString())
		return FString();
	}

	TArray<FString> Outputs;
	for (FGSCAttributeDefinition Attribute : InAttributesList)
	{
		FString Output = Template.Replace(TEXT("%ATTRIBUTE_NAME%"), *Attribute.AttributeName, ESearchCase::CaseSensitive);
		Outputs.Add(Output);
	}

	return FString::Join(Outputs, LINE_TERMINATOR);
}

FString UGSCAttributeSetClassTemplate::MakeAttributesOnRepDefinition(TArray<FGSCAttributeDefinition> InAttributesList)
{
	FText FailReason;
	FString Template;
	if (!ReadTemplateFile(TEXT("FGameplayAttribute_OnRep_Definition.template"), Template, FailReason))
	{
		EDITOR_LOG(Error, TEXT("ClassTemplate:MakeAttributesOnRepDefinition() Error reading template: %s"), *FailReason.ToString())
		return FString();
	}

	TArray<FString> Outputs;
	for (FGSCAttributeDefinition Attribute : InAttributesList)
	{
		FString Output = Template.Replace(TEXT("%ATTRIBUTE_NAME%"), *Attribute.AttributeName, ESearchCase::CaseSensitive);
		Outputs.Add(Output);
	}

	return FString::Join(Outputs, LINE_TERMINATOR);
}

bool UGSCAttributeSetClassTemplate::ReadTemplateFile(const FString& TemplateFileName, FString& OutFileContents, FText& OutFailReason)
{
	const FString FullFileName = GetPluginTemplateDirectory() / TemplateFileName;
	if (FFileHelper::LoadFileToString(OutFileContents, *FullFileName))
	{
		return true;
	}

	FFormatNamedArguments Args;
	Args.Add(TEXT("FullFileName"), FText::FromString(FullFileName));
	OutFailReason = FText::Format(LOCTEXT("FailedToReadTemplateFile", "Failed to read template file \"{FullFileName}\""), Args);
	return false;
}

bool UGSCAttributeSetClassTemplate::WriteOutputFile(const FString& OutputFilename, const FString& OutputFileContents, FText& OutFailReason)
{
	if (FFileHelper::SaveStringToFile(OutputFileContents, *OutputFilename))
	{
		return true;
	}

	FFormatNamedArguments Args;
	Args.Add(TEXT("OutputFilename"), FText::FromString(OutputFilename));
	OutFailReason = FText::Format(LOCTEXT("FailedToWriteOutputFile", "Failed to write output file \"{OutputFilename}\". Perhaps the file is Read-Only?"), Args);
	return false;
}

#undef LOCTEXT_NAMESPACE
