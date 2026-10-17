#include "Modules/ModuleManager.h"

// DevKitEditor owns the existing workbench tab/menu entry points. The plugin owns
// their lighting implementation and content without depending on the host module.
IMPLEMENT_MODULE(FDefaultModuleImpl, YogLookDevEditor)
