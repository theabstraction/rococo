namespace Sys { namespace Animals { 
	struct ITiger
	{
		virtual void Write(Sys::SexString text) = 0;
	};
}}

namespace Sys::Animals
{
	void AddNativeCalls_SysAnimalsITiger(Sexy::Script::IPublicScriptSystem& ss, Sys::IZoo* nceContext);
}
