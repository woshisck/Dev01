#include "MaterialBatch/MaterialBatchAuditHelpers.h"

#include "Misc/PackageName.h"

FString FMaterialBatchAuditHelpers::ResolveMapFilename(const FString& MapPackageNameOrFilename)
{
	if (MapPackageNameOrFilename.EndsWith(FPackageName::GetMapPackageExtension()))
	{
		return FPaths::ConvertRelativePathToFull(MapPackageNameOrFilename);
	}

	if (FPackageName::IsValidLongPackageName(MapPackageNameOrFilename))
	{
		const FString RelativeFilename = FPackageName::LongPackageNameToFilename(
			MapPackageNameOrFilename,
			FPackageName::GetMapPackageExtension());
		return FPaths::ConvertRelativePathToFull(RelativeFilename);
	}

	return MapPackageNameOrFilename;
}
