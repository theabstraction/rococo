// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once

#include <rococo.types.h>

#ifndef SCRIPTEXPORT_API
# define SCRIPTEXPORT_API ROCOCO_API_IMPORT
#endif

namespace Rococo
{
	DECLARE_ROCOCO_INTERFACE IMathsVenue;
	DECLARE_ROCOCO_INTERFACE IDebuggerWindow;

	namespace Compiler
	{
		DECLARE_ROCOCO_INTERFACE IStructure;
	}

	namespace Script
	{
		DECLARE_ROCOCO_INTERFACE IPublicScriptSystem;

		struct ArchetypeCallback
		{
			ID_BYTECODE byteCodeId;
			const uint8* callerSF;
		};
	}

	namespace Sex
	{
		DECLARE_ROCOCO_INTERFACE ISExpression;
		DECLARE_ROCOCO_INTERFACE ISParserTree;
		typedef const ISExpression& cr_sex;
		class ParseException;
	}

	ROCOCO_INTERFACE IArgStack
	{
		virtual void PushInt32(int32 value) = 0;
		virtual void PushInt64(int64 value) = 0;
		virtual void PushPointer(void* value) = 0;
	};

	ROCOCO_INTERFACE IOutputStack
	{
		virtual int32 PopInt32() = 0;
	};

	ROCOCO_INTERFACE IArgEnumerator
	{
		virtual void PushArgs(IArgStack& args) = 0;
		virtual void PopOutputs(IOutputStack& args) = 0;
	};

	// Enumerates scripts
	ROCOCO_INTERFACE IScriptEnumerator
	{
		virtual size_t Count() const = 0;
		virtual cstr ResourceName(size_t index) const = 0;
	};

	SCRIPTEXPORT_API IScriptEnumerator* NoImplicitIncludes();

	ROCOCO_INTERFACE ISourceCache
	{
		[[nodiscard]] virtual IAllocator & Allocator() const = 0;

		// Get/Build the tree for the specified resource. If an [owner] is supplied this defines which file & expression specified the GetSource request
		// Owners are required when the resource name is prefixed with some system macros such as #$/ (which replaces the #$/ with the owning directory of the of the expression file that is the [owner]
		[[nodiscard]] virtual Rococo::Sex::ISParserTree* GetSource(cstr resourceName, const Sex::ISExpression* owner = nullptr) = 0;

		virtual int LoadSourceAsTextFileElseReturnErrorCode(cstr resourceName, Strings::IStringPopulator& populator) = 0;
		virtual void Free() = 0;
		virtual void Release(cstr resourceName) = 0;
		virtual void ReleaseAll() = 0;
		[[nodiscard]] virtual IMathsVenue* Venue() = 0;

		// The package must remain valid for the lifetime of the source cache
		virtual void AddPackage(IPackage* package) = 0;

		virtual void RegisterPackages(Rococo::Script::IPublicScriptSystem& ss) = 0;
	};

	SCRIPTEXPORT_API void DebuggerLoop(Rococo::Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger);

	namespace Script
	{
		SCRIPTEXPORT_API void AddNativeCallSecurity_ToSysNatives(Rococo::Script::IPublicScriptSystem& ss);
	}

	struct ScriptCompileArgs
	{
		Rococo::Script::IPublicScriptSystem& ss;
	};

	struct ScriptPerformanceStats
	{
		Time::ticks loadTime;
		Time::ticks compileTime;
		Time::ticks executeTime;
	};

	SCRIPTEXPORT_API void InitSexyScript(Rococo::Sex::ISParserTree& mainModule, IDebuggerWindow& debugger, Rococo::Script::IPublicScriptSystem& ss, ISourceCache& sources, IScriptEnumerator& implicitIncludes, IScriptCompilationEventHandler& onCompile, Strings::StringBuilder* declarationBuilder);
	SCRIPTEXPORT_API void ExecuteFunction(Rococo::ID_BYTECODE bytecodeId, IArgEnumerator& args, Rococo::Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger, bool trace);
	SCRIPTEXPORT_API void ExecuteFunctionUntilYield(ID_BYTECODE bytecodeId, Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger, bool trace);
	SCRIPTEXPORT_API void ExecuteFunction(cstr name, IArgEnumerator& args, Rococo::Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger, bool trace);
	SCRIPTEXPORT_API int32 ExecuteSexyScript(ScriptPerformanceStats& stats, Rococo::Sex::ISParserTree& mainModule, IDebuggerWindow& debugger, Rococo::Script::IPublicScriptSystem& ss, ISourceCache& sources, IScriptEnumerator& implicitIncludes, int32 param, IScriptCompilationEventHandler& onCompile, bool trace, Strings::StringBuilder* declarationBuilder);
	SCRIPTEXPORT_API [[nodiscard]] ISourceCache* CreateSourceCache(IO::IInstallation& installation, Rococo::IAllocator& allocator, bool allowSysPath = false);
	SCRIPTEXPORT_API void ThrowSex(Rococo::Sex::cr_sex s, cstr format, ...);
	void ScanExpression(Rococo::Sex::cr_sex s, cstr hint, const char* format, ...);
	void ValidateArgument(Rococo::Sex::cr_sex s, cstr arg);

	[[nodiscard]] Vec3 GetVec3Value(Rococo::Sex::cr_sex sx, Rococo::Sex::cr_sex sy, Rococo::Sex::cr_sex sz);
	[[nodiscard]] RGBAb GetColourValue(Rococo::Sex::cr_sex s);
	[[nodiscard]] Quat GetQuat(Rococo::Sex::cr_sex s);

	SCRIPTEXPORT_API void LogParseException(Rococo::Sex::ParseException& ex, IDebuggerWindow& logger, bool jitCompileError = false);

	[[nodiscard]] fstring GetAtomicArg(Rococo::Sex::cr_sex s);
}