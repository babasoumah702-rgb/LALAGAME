#include "LalalandUnreal.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogLalaland);
// Secondary module in the isolated receiver; original received source is untouched.
IMPLEMENT_GAME_MODULE(FDefaultGameModuleImpl, LalalandUnreal);
