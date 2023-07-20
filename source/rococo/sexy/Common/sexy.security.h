#pragma once

#include <rococo.types.h>
#include <rococo.stl.allocators.h>

namespace Rococo::Script
{
	// Protects a set of functions and methods from unauthorized access by a rogue script
	struct NativeCallSecurity
	{
		// Define a valid ping path for the native API. If the function caller's ping path does not match the security ping path, the script file will abort with a security exception
		U8FilePath callersPingPath;
	};

	struct NativeSecurityHandler;

	ROCOCO_INTERFACE INativeSecurity
	{
		// Returns true if the caller is permitted to invoke the function, method or other protected item. 
		// The implementation may throw an exception with more detailed information or just return false
		virtual bool IsCallerPermitted(const NativeSecurityHandler & handler, cr_sex callersCode) = 0;
	};

	struct NativeSecurityHandler
	{
		DEFINE_SEXY_ALLOCATORS_FOR_CLASS;

		NativeCallSecurity security;
		INativeSecurity* handler = nullptr;
	};
}