// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Reusable 'Persona' features for asset editors concerned with USkeleton-related assets
 */
class IComboGraphBlueprintEditor
{
public:
	/** Virtual destructor */
	virtual ~IComboGraphBlueprintEditor() {}

	/** Get the skeleton that we are editing */
	virtual USkeleton* GetSkeleton() const = 0;

	/** Set the preview mesh, according to context (mesh, skeleton or animation etc.) */
	virtual USkeletalMesh* GetPreviewMesh() const = 0;

	/**
	 * Set the preview mesh, according to context (mesh, skeleton or animation etc.)
	 * @param	InSkeletalMesh			The mesh to set
	 * @param	bSetPreviewMeshInAsset	If true, the mesh will be written to the asset so it can be permanently saved.
	 *									Otherwise the change is merely transient and will reset next time the editor is opened.
	 */
	virtual void SetPreviewMesh(USkeletalMesh* InSkeletalMesh, bool bSetPreviewMeshInAsset = true) = 0;
};
