#pragma once

#ifndef TEXT
# define TEXT(x) L ## x
#endif

namespace Rococo::UE::Rocks
{
	typedef void (*FN_OF_ONE_POINTER)(void* rock);

	struct StructMethods
	{
		FN_OF_ONE_POINTER Construct;
		FN_OF_ONE_POINTER Destruct;
	};

	StructMethods GetStructMethods(const TCHAR* structPathName);
}