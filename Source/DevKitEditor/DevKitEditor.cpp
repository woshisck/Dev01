// Fill out your copyright notice in the Description page of Project Settings.

#include "DevKitEditor.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "DevKitEditor"


class FDevKitEditorModule : public FDefaultGameModuleImpl {
	typedef FDevKitEditorModule ThisClass;

	virtual void StartupModule() override
	{
	}


	virtual void ShutdownModule() override
	{

	}

};



IMPLEMENT_MODULE(FDevKitEditorModule, DevKitEditor);
#undef LOCTEXT_NAMESPACE