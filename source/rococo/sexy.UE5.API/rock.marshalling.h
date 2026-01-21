#pragma once

#ifndef TEXT
# define TEXT(x) L ## x
#endif

namespace Rococo::Script
{
	DECLARE_ROCOCO_INTERFACE IPublicScriptSystem;
}

namespace Rococo::UE::Rocks
{
	ROCOCO_INTERFACE IRockFactory
	{
		virtual void Prep() = 0;
		virtual void Construct(void* pInstance) = 0;
		virtual void Destruct(void* pInstance) = 0;
	};

	ROCOCO_INTERFACE IRockFactories
	{
		virtual IRockFactory& BindRockFactory(crwstr classPath) = 0;
	};

	SEXY_MARSHALLING_API void RegisterRocks(Rococo::Script::IPublicScriptSystem& ss, IRockFactories& factories);
}