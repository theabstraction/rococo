// BennyHill generated Sexy native functions for Rococo::Components::Body::IBodyBase 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;


	void NativeGetHandleForRococoComponentsBodyGetBody(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Components::Body::IBodyBase* nceContext = reinterpret_cast<Rococo::Components::Body::IBodyBase*>(_nce.context);
		// Uses: Rococo::Components::Body::IBodyBase* FactoryConstructRococoComponentsBodyGetBody(Rococo::Components::Body::IBodyBase* _context);
		Rococo::Components::Body::IBodyBase* pObject = FactoryConstructRococoComponentsBodyGetBody(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Components::Body
{
	void AddNativeCalls_RococoComponentsBodyIBodyBase(Rococo::Script::IPublicScriptSystem& ss, Rococo::Components::Body::IBodyBase* _nceContext)
	{
		HIDE_COMPILER_WARNINGS(_nceContext);
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Components.Body.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoComponentsBodyGetBody, _nceContext, ("GetHandleForIBodyBase0  -> (Pointer hObject)"), __FILE__, __LINE__);
	}
}
