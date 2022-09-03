#pragma once

#include <rococo.types.h>

namespace Rococo
{
	struct IMathsVenue;
	struct IDebuggerWindow;
	struct IInstallation;

	namespace Compiler
	{
		struct IStructure;
	}

	namespace Script
	{
		struct IPublicScriptSystem;

		struct ArchetypeCallback
		{
			ID_BYTECODE byteCodeId;
			const uint8* callerSF;
		};
	}

	namespace Sex
	{
		struct ISExpression;
		struct ISParserTree;
		typedef const ISExpression& cr_sex;
		class ParseException;
	}

	struct IArgStack
	{
		virtual void PushInt32(int32 value) = 0;
		virtual void PushInt64(int64 value) = 0;
		virtual void PushPointer(void* value) = 0;
	};

	struct IOutputStack
	{
		virtual int32 PopInt32() = 0;
	};

	struct IArgEnumerator
	{
		virtual void PushArgs(IArgStack& args) = 0;
		virtual void PopOutputs(IOutputStack& args) = 0;
	};

	ROCOCOAPI ISourceCache
	{
		[[nodiscard]] virtual IAllocator & Allocator() = 0;
		[[nodiscard]] virtual Rococo::Sex::ISParserTree* GetSource(cstr resourceName) = 0;
		virtual void Free() = 0;
		virtual void Release(cstr resourceName) = 0;
		virtual void ReleaseAll() = 0;
		[[nodiscard]] virtual IMathsVenue* Venue() = 0;

		// The package must remain valid for the lifetime of the source cache
		virtual void AddPackage(IPackage* package) = 0;

		virtual void RegisterPackages(Rococo::Script::IPublicScriptSystem& ss) = 0;
	};

	void DebuggerLoop(Rococo::Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger);

	struct ScriptCompileArgs
	{
		Rococo::Script::IPublicScriptSystem& ss;
	};

	struct ScriptPerformanceStats
	{
		OS::ticks loadTime;
		OS::ticks compileTime;
		OS::ticks executeTime;
	};

	void InitSexyScript(Rococo::Sex::ISParserTree& mainModule, IDebuggerWindow& debugger, Rococo::Script::IPublicScriptSystem& ss, ISourceCache& sources, IEventCallback<ScriptCompileArgs>& onCompile, Strings::StringBuilder* declarationBuilder);
	void ExecuteFunction(Rococo::ID_BYTECODE bytecodeId, IArgEnumerator& args, Rococo::Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger, bool trace);
	void ExecuteFunction(cstr name, IArgEnumerator& args, Rococo::Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger, bool trace);
	int32 ExecuteSexyScript(ScriptPerformanceStats& stats, Rococo::Sex::ISParserTree& mainModule, IDebuggerWindow& debugger, Rococo::Script::IPublicScriptSystem& ss, ISourceCache& sources, int32 param, IEventCallback<ScriptCompileArgs>& onCompile, bool trace, Strings::StringBuilder* declarationBuilder);
	[[nodiscard]] ISourceCache* CreateSourceCache(IInstallation& installation);

	void ThrowSex(Rococo::Sex::cr_sex s, cstr format, ...);
	void ScanExpression(Rococo::Sex::cr_sex s, cstr hint, const char* format, ...);
	void ValidateArgument(Rococo::Sex::cr_sex s, cstr arg);

	[[nodiscard]] Vec3 GetVec3Value(Rococo::Sex::cr_sex sx, Rococo::Sex::cr_sex sy, Rococo::Sex::cr_sex sz);
	[[nodiscard]] RGBAb GetColourValue(Rococo::Sex::cr_sex s);
	[[nodiscard]] Quat GetQuat(Rococo::Sex::cr_sex s);

	void LogParseException(Rococo::Sex::ParseException& ex, IDebuggerWindow& logger);

	[[nodiscard]] fstring GetAtomicArg(Rococo::Sex::cr_sex s);
}