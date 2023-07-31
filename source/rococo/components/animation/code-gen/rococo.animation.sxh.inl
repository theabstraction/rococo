// BennyHill generated Sexy native functions for Rococo::Components::Animation::IAnimationBase 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;


	void NativeGetHandleForRococoComponentsAnimationGetAnimation(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Components::Animation::IAnimationBase* nceContext = reinterpret_cast<Rococo::Components::Animation::IAnimationBase*>(_nce.context);
		// Uses: Rococo::Components::Animation::IAnimationBase* FactoryConstructRococoComponentsAnimationGetAnimation(Rococo::Components::Animation::IAnimationBase* _context);
		Rococo::Components::Animation::IAnimationBase* pObject = FactoryConstructRococoComponentsAnimationGetAnimation(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Components::Animation
{
	void AddNativeCalls_RococoComponentsAnimationIAnimationBase(Rococo::Script::IPublicScriptSystem& ss, Rococo::Components::Animation::IAnimationBase* _nceContext)
	{
		HIDE_COMPILER_WARNINGS(_nceContext);
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Components.Animation.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoComponentsAnimationGetAnimation, _nceContext, ("GetHandleForIAnimationBase0  -> (Pointer hObject)"), __FILE__, __LINE__);
	}
}
