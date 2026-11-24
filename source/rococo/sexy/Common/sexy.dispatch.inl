namespace Rococo
{
	namespace Script
	{
		// Pushes &args as the closure's only input. This shares IPublicScriptSystem::DispatchToSexyClosure, which refuses archetypes whose frame layout
		// does not match that (any outputs, more than one input, or an input that is not pointer-sized), and throws if the callback does not terminate normally
		template<typename DISPATCH_ARGS> void DispatchToSexyClosure(NativeCallEnvironment& nce, DISPATCH_ARGS& args, const ArchetypeCallback& target)
		{
		   nce.ss.DispatchToSexyClosure(&args, target);
		}
	} // Script
} // Rococo