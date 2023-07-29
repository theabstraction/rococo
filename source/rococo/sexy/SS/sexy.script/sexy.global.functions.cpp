#include "sexy.script.stdafx.h"

#include <sexy.types.h>
#include <sexy.s-parser.h>
#include <sexy.vm.cpu.h>
#include <sexy.compiler.public.h>
#include <sexy.debug.types.h>
#include <sexy.script.h>

#include <rococo.io.h>
#include <rococo.visitors.h>
#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.hashtable.h>
#include <sexy.strings.h>

#include <stdarg.h>

#include <algorithm>

#include <string>
#include <sexy.vector.h>

#include <rococo.ide.h>
#include <rococo.os.h>
#include <rococo.sexy.api.h>
#include <rococo.package.h>
#include <rococo.debugging.h>
#include <rococo.time.h>

#include <rococo.sexy.map.expert.h>

#ifdef _WIN32
# include <malloc.h>
# include <excpt.h>
# define TRY_PROTECTED __try
# define CATCH_PROTECTED __except (EXCEPTION_EXECUTE_HANDLER)
#else
# include <CoreServices/CoreServices.h>
# include <mach/mach.h>
# include <mach/mach_time.h>
# define TRY_PROTECTED try
# define CATCH_PROTECTED catch (SignalException& sigEx)
namespace
{
   typedef void(*FN_SignalHandler)(int);

   struct SignalException
   {
      int signalCode;
   };

   struct ThrowOnSignal
   {
      FN_SignalHandler previousHandler;
      int code;

      static void OnSignal(int code)
      {
         throw SignalException{ code };
      }

      ThrowOnSignal(int code)
      {
         this->previousHandler = signal(code, ThrowOnSignal::OnSignal);
         this->code = code;
      }

      ~ThrowOnSignal()
      {
         signal(code, previousHandler);
      }
   };
}
#endif

#include <string.h>

#include <rococo.os.win32.h>

#include <rococo.maths.h>

#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>

using namespace Rococo::IO;

namespace Rococo::Script
{
	size_t GetAlignmentPadding(int alignment, int objectSize);
	SCRIPTEXPORT_API uint8* GetKeyPointer(MapNode* m);
	SCRIPTEXPORT_API uint8* GetValuePointer(MapNode* m);
}

namespace Rococo
{
	using namespace Rococo::Sex;

	void UpdateDebugger(Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger, Rococo::int32 stackDepth, bool refreshAll);
}

namespace Rococo::Memory
{
	[[nodiscard]] IAllocator& CheckedAllocator();
	[[nodiscard]] IAllocatorSupervisor* CreateBlockAllocator(size_t kilobytes, size_t maxkilobytes, const char* const name);
}

namespace
{
   using namespace Rococo;
   using namespace Rococo::Sex;
   using namespace Rococo::Compiler;
   using namespace Rococo::Visitors;

   const char* Sanitize(const char* s)
   {
      if (strcmp(s, "_Null_Sys_Type_IString") == 0)
      {
         return "IString";
      }
      else if (strcmp(s, "_Null_Sys_Type_IStringBuilder") == 0)
      {
         return "IStringBuilder";
      }
      return s;
   }

   void AddArguments(const IArchetype& f, Visitors::IUITree& tree, Visitors::TREE_NODE_ID typeId)
   {
      char desc[256];

      if (f.NumberOfInputs() > (f.IsVirtualMethod() ? 1 : 0))
      {
         auto inputId = tree.AddChild(typeId, "[Inputs]", Visitors::CheckState_Clear);
         for (int i = 0; i < f.NumberOfInputs(); ++i)
         {
            if (i == f.NumberOfInputs() - 1 && f.IsVirtualMethod())
            {
               // Skip the instance reference
            }
            else
            {
               auto& arg = f.GetArgument(i + f.NumberOfOutputs());
               auto argName = f.GetArgName(i + f.NumberOfOutputs());

			   if (&arg != nullptr)
			   {
				   // Hack, some build error created a partially initialized structure.
				   // => We skip it.
				   SafeFormat(desc, sizeof(desc), "%s %s", Sanitize(arg.Name()), argName);
				   tree.AddChild(inputId, desc, Visitors::CheckState_Clear);
			   }
            }
         }
      }

      if (f.NumberOfOutputs() > 0)
      {
         auto outputId = tree.AddChild(typeId, "[Outputs]", Visitors::CheckState_Clear);

         if (f.NumberOfOutputs() == 1 && (f.NumberOfInputs() == 0 || (f.NumberOfInputs() == 1 && f.IsVirtualMethod())))
         {
            auto& arg = f.GetArgument(0);
            auto argName = f.GetArgName(0);

            SafeFormat(desc, sizeof(desc), "%s %s - Get Accessor", Sanitize(arg.Name()), argName);
            tree.AddChild(outputId, desc, Visitors::CheckState_Clear);
         }
         else
         {  
            for (int i = 0; i < f.NumberOfOutputs(); ++i)
            {
               auto& arg = f.GetArgument(i);
               auto argName = f.GetArgName(i);

               SafeFormat(desc, sizeof(desc), "%s %s", Sanitize(arg.Name()), argName);
               tree.AddChild(outputId, desc, Visitors::CheckState_Clear);
            }
         }
      }
   }

   struct StardardDebugControl : IDebugControl
   {
      Rococo::VM::IVirtualMachine* vm;
      Rococo::Script::IPublicScriptSystem* ss;
      IDebuggerWindow* debugger;

      void Update()
      {
         UpdateDebugger(*ss, *debugger, 0, true);
      }

      void RecurseNamespaces(const Rococo::Compiler::INamespace& ns, IUITree& tree, Visitors::TREE_NODE_ID rootId)
      {
         if (ns.ChildCount() > 0)
         {
            auto childSubspacesId = tree.AddChild(rootId, "[Subspaces]", CheckState_Clear);

            for (int i = 0; i < ns.ChildCount(); ++i)
            {
               auto& child = ns.GetChild(i);
               auto childId = tree.AddChild(childSubspacesId, child.Name()->Buffer, CheckState_Clear);
               RecurseNamespaces(child, tree, childId);
            }
         }

         struct : public ICallback<const Rococo::Compiler::IStructure, cstr>
         {
            TREE_NODE_ID childStructuresId;
            TREE_NODE_ID rootId;
            IUITree* tree;
            const Rococo::Compiler::INamespace* ns;
            virtual CALLBACK_CONTROL operator()(const Rococo::Compiler::IStructure& s, cstr alias)
            {
               if (childStructuresId.value == 0) childStructuresId = tree->AddChild(rootId, "[Structures]", CheckState_Clear);

               char desc[256];
               SafeFormat(desc, sizeof(desc), "%s.%s", ns->FullName()->Buffer, alias);
               auto typeId = tree->AddChild(childStructuresId, desc, CheckState_Clear);

               SafeFormat(desc, sizeof(desc), "%s - %d bytes. Defined in %s", Parse::VarTypeName(s.VarType()), s.SizeOfStruct(), s.Module().Name());
               auto typeDescId = tree->AddChild(typeId, desc, CheckState_Clear);

               if (s.VarType() == VARTYPE_Derivative)
               {
                  for (int32 i = 0; i < s.MemberCount(); ++i)
                  {
                     cstr fieldType = s.GetMember(i).UnderlyingType() ? s.GetMember(i).UnderlyingType()->Name() : "Unknown Type";
                     SafeFormat(desc, sizeof(desc), "%s %s", fieldType, s.GetMember(i).Name());
                     tree->AddChild(typeId, desc, CheckState_Clear);
                  }
               }
               return CALLBACK_CONTROL_CONTINUE;
            }
         } enumStructures;

         enumStructures.tree = &tree;
         enumStructures.childStructuresId.value = 0;
         enumStructures.rootId = rootId;
         enumStructures.ns = &ns;

         ns.EnumerateStrutures(enumStructures);

         struct : public ICallback<const Rococo::Compiler::IFunction, cstr>
         {
            TREE_NODE_ID childFunctionsId;
            TREE_NODE_ID rootId;
            IUITree* tree;
            const Rococo::Compiler::INamespace* ns;
            virtual CALLBACK_CONTROL operator()(const Rococo::Compiler::IFunction& f, cstr alias)
            {
               if (childFunctionsId.value == 0) childFunctionsId = tree->AddChild(rootId, "[Functions]", CheckState_Clear);

               TREE_NODE_ID typeId;
               char desc[256];

               SafeFormat(desc, sizeof(desc), "%s.%s", ns->FullName()->Buffer, alias);
               typeId = tree->AddChild(childFunctionsId, desc, CheckState_Clear);

               SafeFormat(desc, sizeof(desc), "Defined in %s", f.Module().Name());
               tree->AddChild(typeId, desc, CheckState_Clear);

               AddArguments(f, *tree, typeId);

               return CALLBACK_CONTROL_CONTINUE;
            }
         } enumFunctions;

         enumFunctions.tree = &tree;
         enumFunctions.childFunctionsId.value = 0;
         enumFunctions.rootId = rootId;
         enumFunctions.ns = &ns;

         ns.EnumerateFunctions(enumFunctions);

         if (ns.InterfaceCount() > 0)
         {
            char desc[256];

            auto interfaceId = tree.AddChild(rootId, "[Interfaces]", CheckState_Clear);

            for (int i = 0; i < ns.InterfaceCount(); ++i)
            {
               auto& inter = ns.GetInterface(i);
               auto* base = inter.Base();

               if (base)
               {
                  SafeFormat(desc, sizeof(desc), "%s.%s extending %s", ns.FullName()->Buffer, inter.Name(), base->Name());
               }
               else
               {
                  SafeFormat(desc, sizeof(desc), "%s.%s", ns.FullName()->Buffer, inter.Name());
               }

               auto interId = tree.AddChild(interfaceId, desc, CheckState_Clear);

               for (int j = 0; j < inter.MethodCount(); ++j)
               {
                  auto& method = inter.GetMethod(j);
                  if (&method != nullptr)
                  {
                     SafeFormat(desc, sizeof(desc), "method %s", method.Name());
                     auto methodId = tree.AddChild(interId, desc, CheckState_Clear);
                     AddArguments(method, tree, methodId);
                  }
               }
            }
         }

         struct : public ICallback<const Rococo::Compiler::IFactory, cstr>
         {
            TREE_NODE_ID childFactoryId;
            TREE_NODE_ID rootId;
            IUITree* tree;
            const Rococo::Compiler::INamespace* ns;
            virtual CALLBACK_CONTROL operator()(const Rococo::Compiler::IFactory& f, cstr alias)
            {
               if (childFactoryId.value == 0) childFactoryId = tree->AddChild(rootId, "[Factories]", CheckState_Clear);

               TREE_NODE_ID typeId;
               char desc[256];

               SafeFormat(desc, sizeof(desc), "%s.%s - creates objects of type %s", ns->FullName()->Buffer, alias, f.InterfaceType()->Buffer);
               typeId = tree->AddChild(childFactoryId, desc, CheckState_Clear);

               SafeFormat(desc, sizeof(desc), "Defined in %s", f.Constructor().Module().Name());
               tree->AddChild(typeId, desc, CheckState_Clear);

               AddArguments(f.Constructor(), *tree, typeId);

               return CALLBACK_CONTROL_CONTINUE;
            }
         } enumFactories;

         enumFactories.tree = &tree;
         enumFactories.childFactoryId.value = 0;
         enumFactories.rootId = rootId;
         enumFactories.ns = &ns;

         ns.EnumerateFactories(enumFactories);
      }

      void RefreshAtDepth(int stackDepth) override
      {
         UpdateDebugger(*ss, *debugger, stackDepth, false);
      }

      void PopulateAPITree(Visitors::IUITree& tree) override
      {
         auto& root = ss->PublicProgramObject().GetRootNamespace();
         auto nsid = tree.AddRootItem("[Namespaces]", CheckState_Clear);
         RecurseNamespaces(root, tree, nsid);
      }

      void Continue() override
      {
         vm->ContinueExecution(VM::ExecutionFlags(true, true, false));
         Update();
      }

      void StepOut() override
      {
         vm->StepOut();
         Update();
      }

      void StepOver() override
      {
         vm->StepOver();
         Update();
      }

      void StepNext() override
      {
         vm->StepInto();
         Update();
      }

      void StepNextSymbol() override
      {
         //ISymbols& symbols = ss->PublicProgramObject().ProgramMemory().
         //vm->StepNextSymbol()
      }
   };
}

namespace Rococo
{

	using namespace Rococo::Sex;
	using namespace Rococo::Visitors;

	SCRIPTEXPORT_API void ThrowSex(cr_sex s, cstr format, ...)
	{
		va_list args;
		va_start(args, format);

		char msg[512];
		SafeVFormat(msg, sizeof(msg), format, args);

		auto start = s.Start();
		auto end = s.End();

		char specimen[64];
		Rococo::Sex::GetSpecimen(specimen, s);

		ParseException ex(start, end, "ParseException", msg, specimen, &s);

		OS::TripDebugger();

		throw ex;
	}

	void ValidateArgument(cr_sex s, const char* arg)
	{
		auto txt = s.String();

		if (!IsAtomic(s) || strcmp(arg, txt->Buffer) != 0)
		{
			if (arg[0] == '\'' && arg[1] == 0)
			{
				ThrowSex(s, "Expecting quote character");
			}
			else
			{
				ThrowSex(s, "Expecting atomic argument: '%s'", arg);
			}
		}
	}

	float GetValue(cr_sex s, float minValue, float maxValue, cstr hint)
	{
		sexstring txt = s.String();

		float value = 0;
		if (!IsAtomic(s) || Parse::PARSERESULT_GOOD != Parse::TryParseFloat(value, txt->Buffer))
		{
			ThrowSex(s, "%s: Expecting atomic argument float", hint);
		}

		if (value < minValue || value > maxValue)
		{
			ThrowSex(s, "%s: Value %g must be in domain [%g,%g]", hint, value, minValue, maxValue);
		}

		return value;
	}

	RGBAb GetColourValue(cr_sex s)
	{
		int32 value = 0;
		if (!IsAtomic(s) || Parse::PARSERESULT_GOOD != Parse::TryParseHex(value, s.String()->Buffer))
		{
			ThrowSex(s, "Cannot parse hex colour value");
		}

		if (value > 0x00FFFFFF)
		{
			ThrowSex(s, "Expecting hex digits RRGGBB");
		}

		int red = (value >> 16) & 0x000000FF;
		int green = (value >> 8) & 0x000000FF;
		int blue = value & 0x000000FF;

		return RGBAb(red, green, blue);
	}

	Quat GetQuat(cr_sex s)
	{
		if (s.NumberOfElements() != 4) Throw(s, "Expecting quat (Vx Vy Vz S)");
		float Vx = GetValue(s[0], -1.0e10f, 1e10f, "Vx component");
		float Vy = GetValue(s[1], -1.0e10f, 1e10f, "Vy component");
		float Vz = GetValue(s[2], -1.0e10f, 1e10f, "Vz component");
		float S = GetValue(s[3], -1.0e10f, 1e10f, "scalar component");

		return Quat{ Vec3{ Vx,Vy,Vz }, S };
	}

	Vec3 GetVec3Value(cr_sex sx, cr_sex sy, cr_sex sz)
	{
		float x = GetValue(sx, -1.0e10f, 1e10f, "x component");
		float y = GetValue(sy, -1.0e10f, 1e10f, "y component");
		float z = GetValue(sz, -1.0e10f, 1e10f, "z component");
		return Vec3{ x, y, z };
	}

	int32 GetValue(cr_sex s, int32 minValue, int32 maxValue, cstr hint)
	{
		sexstring txt = s.String();

		int32 value = 0;
		if (!IsAtomic(s) || Parse::PARSERESULT_GOOD != Parse::TryParseDecimal(value, txt->Buffer))
		{
			ThrowSex(s, "%s: Expecting atomic argument int32", hint);
		}

		if (value < minValue || value > maxValue)
		{
			ThrowSex(s, "%s: Value %d must be in domain [%d,%d]", hint, value, minValue, maxValue);
		}

		return value;
	}

	void ScanExpression(cr_sex sExpr, cstr hint, cstr format, ...)
	{
		va_list args;
		va_start(args, format);

		int nElements = sExpr.NumberOfElements();

		int elementIndex = 0;
		for (const char* p = format; *p != 0; ++p)
		{
			if (*p == 'a')
			{
				if (elementIndex >= nElements)
				{
					ThrowSex(sExpr, "Too few elements in expression. Format is : %s", hint);
				}

				const ISExpression** ppExpr = va_arg(args, const ISExpression**);
				*ppExpr = &sExpr[elementIndex++];

				cr_sex child = **ppExpr;

				const auto s = child.String();

				if (!IsAtomic(child))
				{
					ThrowSex(child, "Expecting atomic element in expression. Format is : %s", hint);
				}
			}
			else if (*p == 'c')
			{
				if (elementIndex >= nElements)
				{
					ThrowSex(sExpr, "Too few elements in expression. Format is : %s", hint);
				}

				const ISExpression** ppExpr = va_arg(args, const ISExpression**);
				*ppExpr = &sExpr[elementIndex++];

				cr_sex child = **ppExpr;

				if (!IsCompound(child))
				{
					ThrowSex(child, "Expecting compound element in expression. Format is : %s", hint);
				}
			}
			else if (*p == ' ')
			{

			}
			else
			{
				Throw(0, "Bad format character %c", *p);
			}
		}
	}

	ISourceCode* DuplicateSourceCode(IOS& os, IExpandingBuffer& rbuffer, ISParser& parser, const IBuffer& rawData, const char* resourcePath)
	{
		const char* utf8data = (const char*)rawData.GetData();
		size_t rawLength = rawData.Length();

		if (rawLength < 2)
		{
			Throw(0, "Script file '%s' was too small", resourcePath);
		}

		if (rawLength % 2 == 0)
		{
			char bom = *utf8data;
			if (bom == 0 || (bom & 0x80))
			{
				Throw(0, "Script file '%s' was not UTF-8 or began with non-ASCII character", resourcePath);
			}
		}

		return parser.DuplicateSourceBuffer((cstr)rawData.GetData(), (int)rawData.Length(), Vec2i{ 1,1 }, resourcePath);
	}

	fstring GetAtomicArg(cr_sex s)
	{
		if (!IsAtomic(s)) ThrowSex(s, "Expecting atomic argument");
		auto st = s.String();
		return fstring{ st->Buffer, st->Length };
	}

	void PrintExpression(cr_sex s, int &totalOutput, int maxOutput, ILogger& logger)
	{
		switch (s.Type())
		{
		case EXPRESSION_TYPE_ATOMIC:
			totalOutput += logger.Log(" %s", (cstr)s.String()->Buffer);
			break;
		case EXPRESSION_TYPE_STRING_LITERAL:
			totalOutput += logger.Log(" \"%s\"", (cstr)s.String()->Buffer);
			break;
		case EXPRESSION_TYPE_COMPOUND:

			totalOutput += logger.Log(" (");

			for (int i = 0; i < s.NumberOfElements(); ++i)
			{
				if (totalOutput > maxOutput)
				{
					return;
				}

				cr_sex child = s.GetElement(i);
				PrintExpression(child, totalOutput, maxOutput, logger);
			}

			totalOutput += logger.Log(" )");
			break;
		case EXPRESSION_TYPE_NULL:
			totalOutput += logger.Log(" ()");
			break;
		}
	}

	SCRIPTEXPORT_API void LogParseException(ParseException& ex, IDebuggerWindow& debugger)
	{
		Vec2i a = ex.Start();
		Vec2i b = ex.End();

		debugger.AddLogSection(RGBAb(128, 0, 0), "ParseException\n");
		debugger.AddLogSection(RGBAb(64, 64, 64), " Name: ");
		debugger.AddLogSection(RGBAb(0, 0, 128), "%s\n", ex.Name());

		debugger.AddLogSection(RGBAb(64, 64, 64), " Message: ");
		debugger.AddLogSection(RGBAb(0, 0, 128), "%s\n", ex.Message());

		debugger.AddLogSection(RGBAb(64, 64, 64), " Specimen: (");
		debugger.AddLogSection(RGBAb(0, 0, 128), "%d", a.x);
		debugger.AddLogSection(RGBAb(64, 64, 64), ",");
		debugger.AddLogSection(RGBAb(0, 0, 128), "%d", a.y);
		debugger.AddLogSection(RGBAb(64, 64, 64), ") to (");
		debugger.AddLogSection(RGBAb(0, 0, 128), "%d", b.x);
		debugger.AddLogSection(RGBAb(64, 64, 64), ",");
		debugger.AddLogSection(RGBAb(0, 0, 128), "%d", b.y);
		debugger.AddLogSection(RGBAb(64, 64, 64), ")\n");

		debugger.AddLogSection(RGBAb(0, 127, 0), "%s\n", ex.Specimen());

		a = a - Vec2i{ 1, 0 };
		b = b - Vec2i{ 1, 0 };

		debugger.SetCodeHilight(ex.Name(), a, b, ex.Message());

		struct ANON : ILogger
		{
			char buf[4096];
			StackStringBuilder sb;

			ANON() :
				sb(buf, sizeof(buf))
			{

			}

			virtual void AddLogSection(RGBAb colour, cstr format, ...)
			{
				va_list args;
				va_start(args, format);

				char section[4096];
				SafeVFormat(section, sizeof(section), format, args);

				sb << section;
			}

			virtual void ClearLog()
			{
				buf[0] = 0;
				sb.Clear();
			}

			virtual int Log(cstr format, ...)
			{
				va_list args;
				va_start(args, format);

				char section[4096];
				int chars = SafeVFormat(section, sizeof(section), format, args);

				sb << section;

				return chars;
			}
		} subLogger;

		int depth = 0;
		for (const ISExpression* s = ex.Source(); s != NULL; s = s->GetOriginal())
		{
			if (depth++ > 0)  debugger.Log("Macro expansion %d:\n", depth);

			int totalOutput = 0;
			PrintExpression(*s, totalOutput, 1024, subLogger);

			debugger.Log("%s", subLogger.buf);

			subLogger.ClearLog();

			debugger.Log("\n");
		}
	}

	class SourceCache : public ISourceCache, private IMathsVenue
	{
	private:
		struct Binding
		{
			ISParserTree* tree;
			ISourceCode* code;
			Time::ticks loadTime;
		};
		TSexyStringMap<Binding> sources;
		// TODO -> allocator using the SourceCache allocator
		AutoFree<IExpandingBuffer> fileBuffer;
		// TODO -> allocator using the SourceCache allocator
		AutoFree<IExpandingBuffer> unicodeBuffer;
		IInstallation& installation;
		IAllocator& allocator;
		Auto<ISParser> parser;

		struct VisitorInfo
		{
			U8FilePath pingPath;
			size_t fileLength;
			char time[64];
		};
		TSexyVector<VisitorInfo> visitorData;
		TSexyVector<IPackage*> packages;
	public:
		SourceCache(IInstallation& _installation, IAllocator& _allocator) :
			fileBuffer(CreateExpandingBuffer(64_kilobytes)),
			unicodeBuffer(CreateExpandingBuffer(64_kilobytes)),
			installation(_installation),
			allocator(_allocator),
			parser(Sexy_CreateSexParser_2_0(_allocator))
		{
		}

		IAllocator& Allocator() override
		{
			return allocator;
		}

		ISourceCache* GetInterface()
		{
			return static_cast<ISourceCache*>(this);
		}

		~SourceCache()
		{
			for (auto i : sources)
			{
				i.second.code->Release();
				if (i.second.tree) i.second.tree->Release();
			}
		}

		IMathsVenue* Venue() override
		{
			return static_cast<IMathsVenue*>(this);
		}

		void ShowVenue(IMathsVisitor& visitor) override
		{
			if (visitorData.empty())
			{
				visitorData.reserve(sources.size());

				for (auto i : sources)
				{
					VisitorInfo info;
					Time::FormatTime(i.second.loadTime, info.time, sizeof info.time);
					info.fileLength = i.second.code->SourceLength();
					Format(info.pingPath, "%s", (cstr) i.first);
					visitorData.push_back(info);
				}

				std::sort(visitorData.begin(), visitorData.end(),
					[](const VisitorInfo& a, const VisitorInfo& b)
					{
						return b.fileLength < a.fileLength;
					}
					);
			}

			visitor.ShowString("", "   File Length     Timestamp");

			for (auto i : visitorData)
			{
				visitor.ShowString(i.pingPath, "%8d bytes      %8.8s", i.fileLength, i.time);
			}
		}
		 
		void Free() override
		{
			this->~SourceCache();
			allocator.FreeData(this);
		}

		ISParserTree* GetSource(cstr pingName) override
		{
			auto i = sources.find(pingName);
			if (i != sources.end())
			{
				if (i->second.tree == nullptr)
				{
					// a null tree indicates the src was cached, but the tree generation threw an exception
					i->second.code->Release();
					sources.erase(i);
				}
				else
				{
					return i->second.tree;
				}
			}

			visitorData.clear();

			ISourceCode* src = nullptr;

			bool success = installation.TryLoadResource(pingName, *fileBuffer, 64_megabytes);
			if (success)
			{
				src = DuplicateSourceCode(installation.OS(), *unicodeBuffer, *parser, *fileBuffer, pingName);
			}
			else
			{
				WideFilePath sysPath;
				installation.ConvertPingPathToSysPath(pingName, sysPath);
				U8FilePath expandedPath;
				installation.ConvertSysPathToPingPath(sysPath, expandedPath);

				auto scripts = "!scripts/"_fstring;
				if (StartsWith(expandedPath, scripts))
				{
					const char* packagePath = expandedPath.buf + scripts.length;
					for (auto p : packages)
					{
						PackageFileData pfd;
						if (p->TryGetFileInfo(packagePath, pfd))
						{
							char fullname[256];
							SafeFormat(fullname, "Package[%s]@%s", p->FriendlyName(), packagePath);

							if (pfd.filesize > 0x7FFFFFFFLL)
							{
								Throw(0, "%s file length too long", fullname);
							}

							src = parser->ProxySourceBuffer(pfd.data, (int32) pfd.filesize, { 1,1 }, fullname);
							break;
						}
					}
				}

				if (src == nullptr)
				{
					Throw(0, "\nCould not find file [%s] in content directory or packages", pingName);
				}
			}

			sources[pingName] = Binding{ nullptr, src, 0 };

			// We have cached the source, so that if tree generation creates an exception, the source codes is still existant

			ISParserTree* tree = parser->CreateTree(*src);
			sources[pingName] = Binding{ tree, src, Time::UTCTime() };

			return tree;
		}

		void Release(cstr resourceName) override
		{
			visitorData.clear();

			for (auto i = sources.begin(); i != sources.end(); ++i)
			{
				if (installation.DoPingsMatch(i->first, resourceName))
				{
					i->second.tree->Release();
					i->second.code->Release();					
					sources.erase(i);
					break;
				}
			}
		}

		void ReleaseAll() override
		{
			visitorData.clear();

			for (auto i : sources)
			{
				if (i.second.tree) i.second.tree->Release();
				i.second.code->Release();
			}

			sources.clear();
		}

		void AddPackage(IPackage* package) override
		{
			// Only add a package if a package with the same hash code is not found in the source
			auto i = std::find_if(packages.begin(), packages.end(),
				[package](const IPackage* p)
				{ 
					return package->HashCode() == p->HashCode();
				});

			if (i == packages.end())
			{
				packages.push_back(package);
			}
		}

		void RegisterPackages(Rococo::Script::IPublicScriptSystem& ss) override
		{
			for (auto p : packages)
			{
				ss.RegisterPackage(p);
			}
		}
	};

	SCRIPTEXPORT_API ISourceCache* CreateSourceCache(IInstallation& installation, IAllocator& allocator)
	{
		void* buffer = allocator.Allocate(sizeof SourceCache);
		auto* cache = new (buffer) SourceCache(installation, allocator);
		return cache->GetInterface();
	}

	using namespace Rococo::Compiler;
	using namespace Rococo::VM;
	using namespace Rococo::Visitors;
	using namespace Rococo::Script;

	void PopulateCallstack(IPublicScriptSystem& ss, IDebuggerWindow& debugger)
	{
		struct : Visitors::IListPopulator
		{
			IPublicScriptSystem* ss;

			void Populate(Visitors::IUIList& list) override
			{
				auto& vm = ss->PublicProgramObject().VirtualMachine();

				const Rococo::uint8* sf = nullptr;
				const Rococo::uint8* pc = nullptr;
				const IFunction* f = nullptr;

				size_t fnOffset;
				size_t pcOffset;

				int depth = 0;
				while (GetCallDescription(sf, pc, f, fnOffset, *ss, depth++, pcOffset) && f)
				{
					cstr values[] = { f->Name(), f->Module().Name(), nullptr };
					list.AddRow(values);
				}
			}
		} callstackPopulator;

		callstackPopulator.ss = &ss;
		debugger.PopulateCallStackView(callstackPopulator);
	}

	using namespace Rococo::Debugger;

	struct MemberEnumeratorPopulator: MemberEnumeratorCallback
	{
		TREE_NODE_ID parentId;
		IUITree* tree;
		int depth;
		const uint8* instance;
		const IStructure* parentStruct = nullptr;
		const IStructure* concreteStruct = nullptr;
		ObjectStub* parentHeader = nullptr;
		int index = 0;
		int firstUnkIndex = 1;

		void OnMember(IPublicScriptSystem& ss, cstr childName, const Rococo::Compiler::IMember& member, const uint8* sfItem, int offset, int recurseDepth) override
		{
			index++;

			if (parentStruct)
			{
				if (index == 1)
				{
					auto* header = (ObjectStub*)sfItem;
					auto* t = header->Desc;

					concreteStruct = t->TypeInfo;
					parentHeader = header;

					char classDesc[256];
					if (header->refCount == ObjectStub::NO_REF_COUNT)
					{
						SafeFormat(classDesc, "[%+d] %s DestructorId: %lld. Refcount: n/a", (sfItem - instance), concreteStruct->Name(), header->Desc->DestructorId);
					}
					else
					{
						SafeFormat(classDesc, "[%+d] %s DestructorId: %lld. Refcount: %lld", (sfItem - instance), concreteStruct->Name(), header->Desc->DestructorId, header->refCount);
					}


					auto node = tree->AddChild(parentId, classDesc, CheckState_NoCheckBox);

					firstUnkIndex = 3 + concreteStruct->InterfaceCount();

					for (int i = 0; i < concreteStruct->InterfaceCount(); ++i)
					{
						auto& interface = concreteStruct->GetInterface(i);
						auto* vTable = concreteStruct->GetVirtualTable(i + 1);
						char interfaceDesc[256];
						SafeFormat(interfaceDesc, "Implements %s. vTable %p", interface.Name(), vTable);

						auto inode = tree->AddChild(node, interfaceDesc, CheckState_NoCheckBox);

						auto* instanceVTable = header->pVTables[i];
						if (instanceVTable != (VirtualTable*)vTable)
						{
							char vTableDesc[256];
							SafeFormat(vTableDesc, "vTable mismatch. Expecting %p, but found %p", vTable, instanceVTable);
							auto vnode = tree->AddChild(inode, vTableDesc, CheckState_NoCheckBox);
						}

						if (header->Desc->TypeInfo == &interface.NullObjectType())
						{
							// The object is a null object so there are no other valid interface pointers
							break;
						}
					}
					return;
				}
				else if (index < firstUnkIndex)
				{
					return;
				}
			}

			char value[256];

			auto* name = member.UnderlyingType()->Name();
			if (Eq(name, "_Null_Sys_Type_IStringBuilder") || (Eq(name, "_Null_Sys_Type_IString")))
			{
				TRY_PROTECTED
				{
					// In the event that we have an IString, or IString derived interface, such as IStringBuilder
					// Then we can use the pointer member of string constant to determine the text buffer.
					// This is guaranteed to work for all implementations of IString, including a null IString;
					InterfacePointer p = InterfacePointer(sfItem);
					auto* s = (CStringConstant*)(sfItem + (*p)->OffsetToInstance);
					SafeFormat(value, "%s", s->pointer);
				}
				CATCH_PROTECTED
				{
					SafeFormat(value, "IString: <Bad pointer>");
				}
			}
			else
			{
				FormatValue(ss, value, sizeof value, member.UnderlyingType()->VarType(), sfItem);
			}

			char memberDesc[256];

			if (member.UnderlyingType()->InterfaceCount() != 0)
			{
				SafeFormat(memberDesc, "[%+d] ->  %p %s: %s", offset, sfItem, member.Name(), value);
			}
			else
			{
				SafeFormat(memberDesc, "[%+d] %p %s: %s", (sfItem - instance), sfItem, member.Name(), value);
			}

			auto node = tree->AddChild(parentId, memberDesc, CheckState_NoCheckBox);
			tree->SetId(node, depth + 1);

			if (member.UnderlyingType()->VarType() == VARTYPE_Derivative)
			{
				MemberEnumeratorPopulator subMember;

				if (member.UnderlyingType()->InterfaceCount() != 0)
				{
					subMember.parentStruct = member.UnderlyingType();
				}

				auto* subInstance = sfItem;
				// auto* subInstance = sfItem && member.UnderlyingType() ? (IsNullType(*member.UnderlyingType()) ? *(const uint8**)sfItem : sfItem) : nullptr;
				if (member.UnderlyingType() && IsNullType(*member.UnderlyingType()))
				{
					subInstance = sfItem;
				}
				subMember.instance = subInstance;
				subMember.parentId = node;
				subMember.tree = tree;
				subMember.depth = depth;

				GetMembers(ss, *member.UnderlyingType(), member.Name(), subInstance, 0, subMember, recurseDepth);
			}
			else if (member.UnderlyingType()->VarType() == VARTYPE_Array)
			{
				MemberEnumeratorPopulator subMember;

				if (member.UnderlyingType()->InterfaceCount() != 0)
				{
					subMember.parentStruct = member.UnderlyingType();
				}

				auto* subInstance = *(const uint8**) sfItem;

				char arrayDesc[256];

				if (subInstance == nullptr)
				{
					tree->AddChild(parentId, "null reference", CheckState_NoCheckBox);
					return;
				}
					
				ArrayImage& a = *(ArrayImage*)subInstance;

				SafeFormat(arrayDesc, "%d of %d elements of type %s and %d bytes each (total %d KB)", a.NumberOfElements, a.ElementCapacity, GetFriendlyName(*a.ElementType), a.ElementLength, (a.ElementLength * a.NumberOfElements) / 1024);
				
				tree->AddChild(node, arrayDesc, CheckState_NoCheckBox);

				SafeFormat(arrayDesc, "RefCount: %d. Modification %s", a.RefCount, a.LockNumber > 0 ? "prohibited" : "allowed");

				tree->AddChild(node, arrayDesc, CheckState_NoCheckBox);

				if (a.Start == nullptr)
				{
					tree->AddChild(node, "item pointer is null", CheckState_NoCheckBox);
				}
				else
				{
					const uint8* pInstance = (const uint8*) a.Start;

					char itemDesc[256];
					SafeFormat(itemDesc, "C-array start 0x%llX", pInstance);
					auto indexNode = tree->AddChild(node, itemDesc, CheckState_NoCheckBox);
					for (int i = 0; i < a.NumberOfElements; ++i)
					{
						enum { MAX_ITEMS_DEBUGGED_PER_ARRAY = 20 };
						if (i > MAX_ITEMS_DEBUGGED_PER_ARRAY)
						{
							tree->AddChild(indexNode, "...", CheckState_NoCheckBox);
							break;
						}

						char item[256];
						SafeFormat(item, "#%d", i);

						char itemEx[256];

						if (Eq(a.ElementType->Name(), "_Null_Sys_Type_IString"))
						{
							InterfacePointer pInterface = *(InterfacePointer*)pInstance;
							auto* stub = (CStringConstant*)InterfaceToInstance(pInterface);
							SafeFormat(itemEx, "0x%llX - #%d: %s", pInstance, i, stub->pointer);
							auto childNode = tree->AddChild(indexNode, itemEx, CheckState_NoCheckBox);
						}
						else
						{

							SafeFormat(itemEx, "0x%llX - #%d", pInstance, i);
							auto childNode = tree->AddChild(indexNode, itemEx, CheckState_NoCheckBox);

							if (a.ElementType->InterfaceCount() != 0)
							{
								subMember.parentStruct = a.ElementType;
							}

							auto* subInstanceInner = pInstance;

							if (IsNullType(*a.ElementType))
							{
								InterfacePointer pInterface = *(InterfacePointer*)pInstance;
								subInstanceInner = (const uint8*)pInterface;
							}
							subMember.instance = subInstanceInner;
							subMember.parentId = childNode;
							subMember.tree = tree;
							subMember.depth = depth;

							GetMembers(ss, *a.ElementType, item, subInstanceInner, 0, subMember, recurseDepth);
						}
						pInstance += a.ElementLength;
					}
				}
			}
		}

		void OnListMember(IPublicScriptSystem& ss, cstr childName, const Rococo::Compiler::IMember& member, const ListImage* l, const uint8* sfItem, int offset, int recurseDepth) override
		{
			index++;

			char name[256];

			if (!l)
			{
				SafeFormat(name, "list<%s> %s: null", GetFriendlyName(*member.UnderlyingGenericArg1Type()), childName);
			}
			else
			{
				SafeFormat(name, "list<%s> %s", GetFriendlyName(*member.UnderlyingGenericArg1Type()), childName);
			}

			auto node = tree->AddChild(parentId, name, CheckState_NoCheckBox);

			if (!l)
			{
				return;
			}

			char metrics[256];
			SafeFormat(metrics, "%d elements", l->NumberOfElements);
			tree->AddChild(node, metrics, CheckState_NoCheckBox);

			char refCount[256];
			SafeFormat(refCount, "%lld references", l->refCount);
			tree->AddChild(node, refCount, CheckState_NoCheckBox);
		}

		bool IsIString(const IInterface& i)
		{
			if (Eq(i.NullObjectType().Name(), "_Null_Sys_Type_IString"))
			{
				return true;
			}
			
			auto* base = i.Base();
			if (!base)
			{
				return false;
			}

			return IsIString(*base);
		}

		void OnMapMember(IPublicScriptSystem& ss, cstr childName, const Rococo::Compiler::IMember& member, const MapImage* m, const uint8* sfItem, int offset, int recurseDepth) override
		{
			index++;

			char name[256];

			if (!m)
			{
				SafeFormat(name, "map<%s to %s> %s: null", GetFriendlyName(*member.UnderlyingGenericArg1Type()), GetFriendlyName(*member.UnderlyingGenericArg2Type()), childName);
			}
			else
			{
				SafeFormat(name, "map<%s to %s> %s", GetFriendlyName(*member.UnderlyingGenericArg1Type()), GetFriendlyName(*member.UnderlyingGenericArg2Type()), childName);
			}

			auto node = tree->AddChild(parentId, name, CheckState_NoCheckBox);

			if (!m)
			{
				return;
			}

			char metrics[256];
			SafeFormat(metrics, "%d entries", m->NumberOfElements);
			tree->AddChild(node, metrics, CheckState_NoCheckBox);

			char refCount[256];
			SafeFormat(refCount, "%d references", m->refCount);
			tree->AddChild(node, refCount, CheckState_NoCheckBox);

			char mapInfo[256];
			SafeFormat(mapInfo, "Address %p", m);
			tree->AddChild(node, mapInfo, CheckState_NoCheckBox);

			int index = -1;
			for (auto* p = m->Head; p != nullptr; p = p->Next)
			{
				index++;

				auto* keyType = m->KeyType;
				if (keyType && keyType->InterfaceCount() == 1 && IsIString(keyType->GetInterface(0)))
				{
					InlineString* s = *(InlineString**)Rococo::Script::GetKeyPointer(p);
					if (s != nullptr)
					{
						auto* value = Rococo::Script::GetValuePointer(p);
						SafeFormat(mapInfo, "[%d] '%s' -> %p", index, s->buffer ? s->buffer : "<null>", value);
						tree->AddChild(node, mapInfo, CheckState_NoCheckBox);
					}
				}
			}
		}

		void OnArrayMember(IPublicScriptSystem& ss, cstr childName, const Rococo::Compiler::IMember& member, const struct ArrayImage* array, const uint8* sfItem, int offset, int recurseDepth) override
		{
			index++;

			char name[256];

			if (!array)
			{
				SafeFormat(name, "array<%s> %s: null", GetFriendlyName(*member.UnderlyingGenericArg1Type()), childName);
			}
			else
			{
				SafeFormat(name, "array<%s> %s", GetFriendlyName(*member.UnderlyingGenericArg1Type()), childName);
			}
			
			auto node = tree->AddChild(parentId, name, CheckState_NoCheckBox);

			if (!array)
			{
				return;
			}

			char metrics[256];
			SafeFormat(metrics, "%d of %d elements", array->NumberOfElements, array->ElementCapacity);
			tree->AddChild(node, metrics, CheckState_NoCheckBox);

			char info[256];
			SafeFormat(info, "Private: Refcount %d. Enum: %s ", array->RefCount, array->LockNumber > 0 ? "locked" : "unlocked");
			tree->AddChild(node, info, CheckState_NoCheckBox);

			if (array->NumberOfElements > 0)
			{
				auto elements = tree->AddChild(node, "#-Elements-#", CheckState_NoCheckBox);
				for (int i = 0; i < array->NumberOfElements; ++i)
				{
					char sindex[16];
					SafeFormat(sindex, "[%d]", i);
					auto element = tree->AddChild(elements, sindex, CheckState_NoCheckBox);

					auto* subInstance = ((const uint8*)array->Start) + array->ElementLength * i;

					if (array->ElementType->InterfaceCount() > 0)
					{
						const uint8* puint8Interface = *(const uint8**)subInstance;
						subInstance = puint8Interface;
						auto* object = InterfaceToInstance((InterfacePointer)subInstance);

						char concreteInfo[256];
						SafeFormat(concreteInfo, "%s", GetFriendlyName(*object->Desc->TypeInfo));
						tree->AddChild(element, concreteInfo, CheckState_NoCheckBox);
					}
					else
					{
						char concreteInfo[256];
						SafeFormat(concreteInfo, "%s", GetFriendlyName(*array->ElementType));
						tree->AddChild(element, concreteInfo, CheckState_NoCheckBox);
					}

					if (i > 20) break;
				}
			}
		}
	};

	struct VariableEnumeratorPopulator : public IVariableEnumeratorCallback
	{
		virtual void OnVariable(size_t index, const VariableDesc& v, const MemberDef& def)
		{
			if (AreEqual(v.Location, "CPU"))
			{
				return;
			}

			char desc[256];
			if (v.Value[0] != 0)
			{
				SafeFormat(desc, sizeof(desc), "[%d] %p: %s %s = %s", v.Address, v.Address + SF, v.Type, v.Name, v.Value);
			}
			else
			{
				SafeFormat(desc, sizeof(desc), "[%d] %p: %s %s", v.Address, v.Address + SF, v.Type, v.Name);
			}
			auto node = tree->AddRootItem(desc, CheckState_NoCheckBox);
			tree->SetId(node, depth + 1);

			// At this level of enumeration v.instance refers to a stack address
			TRY_PROTECTED
			{
				MemberEnumeratorPopulator addMember;
				addMember.parentId = node;
				addMember.tree = tree;
				addMember.depth = depth + 1;
				addMember.instance = v.instance;

				InterfacePointer pInterf = (InterfacePointer)(v.instance);
				if (v.s) GetMembers(*ss, *v.s, v.parentName, v.instance, 0, addMember, 1);
			}
			CATCH_PROTECTED
			{

			}
		}

		int32 depth;
		TREE_NODE_ID sfNode;
		IUITree* tree;
		const uint8* SF;
		Script::IPublicScriptSystem* ss;
	};

	void PopulateVariableList(Visitors::IUIList& list, IPublicScriptSystem& ss, int depth)
	{
		auto& vm = ss.PublicProgramObject().VirtualMachine();

		const Rococo::uint8* sf = nullptr;
		const Rococo::uint8* pc = nullptr;
		const IFunction* f = nullptr;

		size_t fnOffset;
		size_t pcOffset;
		if (!GetCallDescription(sf, pc, f, fnOffset, ss, depth, pcOffset) || !f)
		{
			return;
		}

		struct ANON : public IVariableEnumeratorCallback
		{
			virtual void OnVariable(size_t index, const VariableDesc& v, const MemberDef& def)
			{
				char offset[32];
				SafeFormat(offset, sizeof(offset), "%d", v.Address);
				char descAddress[256];
				SafeFormat(descAddress, sizeof(descAddress), "0x%llX", v.Address + SF);

				cstr values[] = { offset, descAddress, v.Location, v.Type, v.Name, v.Value, nullptr };
				list->AddRow(values);
			}

			int32 depth;
			IUIList* list;
			const uint8* SF;
			Script::IPublicScriptSystem* ss;
		} addToList;

		addToList.ss = &ss;
		addToList.depth = depth;
		addToList.list = &list;
		addToList.SF = sf;

		TRY_PROTECTED
		{
			Script::ForeachVariable(ss, addToList, depth);
		}
		CATCH_PROTECTED
		{

		}
	}

	void PopulateMemberTree(Visitors::IUITree& tree, IPublicScriptSystem& ss, int depth)
	{
		auto& vm = ss.PublicProgramObject().VirtualMachine();

		const Rococo::uint8* sf = nullptr;
		const Rococo::uint8* pc = nullptr;
		const IFunction* f = nullptr;

		size_t fnOffset;
		size_t pcOffset;
		if (!GetCallDescription(sf, pc, f, fnOffset, ss, depth, pcOffset) || !f)
		{
			return;
		}

		VariableEnumeratorPopulator addToTree;

		addToTree.tree = &tree;
		addToTree.SF = sf;
		addToTree.depth = depth;
		addToTree.ss = &ss;

		TRY_PROTECTED
		{
			Script::ForeachVariable(ss, addToTree, depth);
		}
		CATCH_PROTECTED
		{

		}
	}

	void PopulateMemberTree(IPublicScriptSystem& ss, IDebuggerWindow& debugger, int depth)
	{
		struct : Visitors::ITreePopulator
		{
			IPublicScriptSystem* ss;
			int depth;

			void Populate(Visitors::IUITree& tree) override
			{
				Rococo::PopulateMemberTree(tree, *ss, depth);
			}
		} memberPopulator;

		memberPopulator.ss = &ss;
		memberPopulator.depth = depth;
		debugger.PopulateMemberView(memberPopulator);
	}

	void PopulateVariables(IPublicScriptSystem& ss, IDebuggerWindow& debugger, int depth)
	{
		struct : Visitors::IListPopulator
		{
			IPublicScriptSystem* ss;
			int depth;

			void Populate(Visitors::IUIList& list) override
			{
				Rococo::PopulateVariableList(list, *ss, depth);
			}
		} variablePopulator;

		variablePopulator.ss = &ss;
		variablePopulator.depth = depth;

		debugger.PopulateVariableView(variablePopulator);
	}

	void PopulateSourceView(Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger, int64 stackDepth)
	{
		const Rococo::uint8* sf = nullptr;
		const Rococo::uint8* pc = nullptr;
		const Rococo::Compiler::IFunction* f;

		size_t fnOffset;
		size_t pcOffset;
		if (Rococo::Script::GetCallDescription(sf, pc, f, fnOffset, ss, stackDepth, pcOffset) && f)
		{
			auto* tree = ss.GetSourceCode(f->Module());
			if (tree)
			{
				debugger.AddSourceCode(f->Module().Name(), tree->Source().SourceStart());
			}
		}
	}

	void PopulateRegisterWindow(Script::IPublicScriptSystem& ss, Visitors::IUIList& registerListView)
	{
		auto& vm = ss.PublicProgramObject().VirtualMachine();

		using namespace Rococo::Debugger;

		struct : public IRegisterEnumerationCallback
		{
			int count;
			int maxCount;

			virtual void OnRegister(const char* name, const char* value)
			{
				if (count < maxCount)
				{
					char wname[128], wvalue[128];
					SafeFormat(wname, 128, "%s", name);
					SafeFormat(wvalue, 128, "%s", value);

					cstr row[] = { wname, wvalue, nullptr };
					uiList->AddRow(row);
				}

				count++;
			}

			IUIList* uiList;
		} addToList;

		addToList.uiList = &registerListView;
		addToList.count = 0;
		addToList.maxCount = 9;

		registerListView.ClearRows();

		Script::EnumerateRegisters(vm.Cpu(), addToList);
	}

	const IFunction* DisassembleCallStackAndAppendToView(IDisassembler& disassembler, IDebuggerWindow& debugger, Rococo::Script::IPublicScriptSystem& ss, CPU& cpu, size_t callDepth, const ISExpression** ppExpr, const uint8** ppSF, size_t populateAtDepth)
	{
		const Rococo::uint8* sf = nullptr;
		const Rococo::uint8* pc = nullptr;
		const IFunction* f = nullptr;
		*ppExpr = nullptr;
		*ppSF = nullptr;

		size_t fnOffset;
		size_t pcOffset;
		if (!Rococo::Script::GetCallDescription(sf, pc, f, fnOffset, ss, callDepth, pcOffset) || !f)
		{
			return nullptr;
		}

		*ppSF = sf;

		if (callDepth != populateAtDepth || populateAtDepth == (size_t)-1)
		{
			return f;
		}

		CodeSection section;
		f->Code().GetCodeSection(section);

		IPublicProgramObject& po = ss.PublicProgramObject();
		IVirtualMachine& vm = po.VirtualMachine();

		size_t functionLength = po.ProgramMemory().GetFunctionLength(section.Id);

		debugger.InitDisassembly(section.Id);

		char metaData[256];
		SafeFormat(metaData, sizeof(metaData), "%s %s (Id #%d) - %d bytes\n\n", f->Name(), f->Module().Name(), (int32)section.Id, (int32)functionLength);
		debugger.AddDisassembly(RGBAb(128, 128, 0), metaData);

		int lineCount = 1;

		const Rococo::uint8* fstart = vm.Cpu().ProgramStart + po.ProgramMemory().GetFunctionAddress(section.Id);

		size_t hilightIndex = pc - fstart;
		// size_t dissassembleLength = min(64_kilobytes, functionLength);
		size_t i = 0;
		while (i < functionLength)
		{
			SymbolValue symbol = f->Code().GetSymbol(i);

			IDisassembler::Rep rep;
			disassembler.Disassemble(fstart + i, rep);

			if (fstart + i <= pc)
			{
				auto* s = (const ISExpression*)symbol.SourceExpression;
				if (s)
				{
					*ppExpr = s;
				}
			}

			bool isHighlight = (fstart + i == pc);
			if (isHighlight)
			{
				/*
				if (*ppExpr == nullptr)
				{
					*ppExpr = (const ISExpression*)symbol.SourceExpression;
				}
				*/
			}

			char assemblyLine[256];

			if (isHighlight)
			{
				debugger.AddDisassembly(RGBAb(128, 0, 0), "*", RGBAb(255, 255, 255), true);
				SafeFormat(assemblyLine, sizeof(assemblyLine), "%p", fstart + i);
				debugger.AddDisassembly(RGBAb(255, 255, 255), assemblyLine, RGBAb(0, 0, 255));
				SafeFormat(assemblyLine, sizeof(assemblyLine), " %s %s ", rep.OpcodeText, rep.ArgText);
				debugger.AddDisassembly(RGBAb(255, 255, 255), assemblyLine, RGBAb(0, 0, 255));

				if (symbol.Text[0] != 0)
				{
					SafeFormat(assemblyLine, sizeof(assemblyLine), "// %s", symbol.Text);
					debugger.AddDisassembly(RGBAb(32, 128, 0), assemblyLine);
				}

				debugger.AddDisassembly(RGBAb(0, 0, 0), "\n");
			}
			else if ((i < hilightIndex && (hilightIndex - i) < 1024) || (i > hilightIndex && (i - hilightIndex) < 1024))
			{
				SafeFormat(assemblyLine, sizeof(assemblyLine), " %p", fstart + i);
				debugger.AddDisassembly(RGBAb(0, 0, 0), assemblyLine);
				SafeFormat(assemblyLine, sizeof(assemblyLine), " %s %s ", rep.OpcodeText, rep.ArgText);
				debugger.AddDisassembly(RGBAb(128, 0, 0), assemblyLine);

				if (symbol.Text[0] != 0)
				{
					SafeFormat(assemblyLine, sizeof(assemblyLine), "// %s", symbol.Text);
					debugger.AddDisassembly(RGBAb(0, 128, 0), assemblyLine);
				}

				debugger.AddDisassembly(RGBAb(0, 0, 0), "\n");
			}

			if (rep.ByteCount == 0)
			{
				debugger.AddDisassembly(RGBAb(128, 0, 0), "Bad disassembly");
				break;
			}

			i += rep.ByteCount;
			lineCount++;
		}

		debugger.AddDisassembly(RGBAb(0, 0, 0), nullptr);

		return f;
	}

	struct DebuggerPopulator : IDebuggerPopulator
	{
		Script::IPublicScriptSystem* ss;
		int stackDepth;
		bool refreshAll;

		virtual void Populate(IDebuggerWindow& debugger)
		{
			if (refreshAll)
			{
				struct : Visitors::IListPopulator
				{
					Script::IPublicScriptSystem* ss;
					IDebuggerWindow* debugger;

					virtual void Populate(Visitors::IUIList& registerListView)
					{
						PopulateRegisterWindow(*ss, registerListView);
					}
				} anon;

				anon.ss = ss;
				anon.debugger = &debugger;

				debugger.PopulateRegisterView(anon);
			}

			auto& vm = ss->PublicProgramObject().VirtualMachine();
			AutoFree<VM::IDisassembler> disassembler(vm.Core().CreateDisassembler());

			for (int depth = 0; depth < 10; depth++)
			{
				const ISExpression* s = nullptr;
				const uint8* SF;
				auto* f = DisassembleCallStackAndAppendToView(*disassembler, debugger, *ss, vm.Cpu(), depth, &s, &SF, stackDepth);
				if (depth == stackDepth && f != nullptr)
				{
					if (s != nullptr)
					{
						if (s->GetOriginal() != nullptr)
						{
							s = s->GetOriginal();
						}
		
						auto origin = s->Tree().Source().Origin();
						auto p0 = s->Start() - Vec2i{ 1,0 };
						auto p1 = s->End() - Vec2i{ 1,0 };
						debugger.SetCodeHilight(s->Tree().Source().Name(), p0, p1, "!");
					}
				}

				if (SF == nullptr) break;
			}

			PopulateMemberTree(*ss, debugger, stackDepth);
			PopulateVariables(*ss, debugger, stackDepth);

			if (refreshAll)
			{
				PopulateCallstack(*ss, debugger);
			}
				
			PopulateSourceView(*ss, debugger, stackDepth);
		}

	};

	void LogStack(IException& ex, ILog& logger)
	{
		char buf[16384];
		Rococo::OS::BuildExceptionString(buf, sizeof buf, ex, true);

		logger.Write(buf);

		logger.Write("\n");

		/*

		auto* sf = ex.StackFrames();
		if (!sf) return;

		using namespace Rococo::Debugging;

		struct Formatter : public IStackFrameFormatter
		{
			ILog& logger;
			Formatter(ILog& argLogger) : logger(argLogger)
			{

			}

			void Format(const StackFrame& frame) override
			{
				char buf[1024];
				SafeFormat(buf, "%64s - %s line %d", frame.functionName, frame.sourceFile, frame.lineNumber);
				logger.Write(buf);
			}
		} sfFormatter(logger);

		sf->FormatEachStackFrame(sfFormatter);
		*/
	}

	EXECUTERESULT ExecuteAndCatchIException(IVirtualMachine& vm, Rococo::Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger, bool trace)
	{
		try
		{
			if (trace && Rococo::OS::IsDebugging())
			{
				struct ANON : VM::ITraceOutput
				{
					Rococo::Script::IPublicScriptSystem& ss;
					IVirtualMachine& vm;
					AutoFree<VM::IDisassembler> disassembler;

					ANON(Rococo::Script::IPublicScriptSystem& _ss, IVirtualMachine& _vm) : vm(_vm), ss(_ss)
					{
						disassembler = vm.Core().CreateDisassembler();
					}

					void Report() override
					{
						VM::IDisassembler::Rep rep;
						disassembler->Disassemble(vm.Cpu().PC(), rep);

						auto id = ss.PublicProgramObject().ProgramMemory().GetFunctionContaingAddress(vm.Cpu().PC() - vm.Cpu().ProgramStart);
						auto* f = GetFunctionFromBytecode(ss.PublicProgramObject(), id);

						char line[256];

						if (f)
						{
							SafeFormat(line, 256, "[ %s ] %s: %s\n", f->Name(), rep.OpcodeText, rep.ArgText);
						}
						else
						{
							SafeFormat(line, 256, "[ ] %s: %s\n", rep.OpcodeText, rep.ArgText);
						}

						if (OS::IsDebugging())
						{
#ifdef _WIN32
							OutputDebugStringA(line);
#else
							printf("%s", line);
#endif
						}
					}
				} tracer(ss, vm);
				EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, false), &tracer);
				return result;
			}
			else
			{
				EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, false));
				return result;
			}
		}
		catch (ParseException& pex)
		{
			auto* s = pex.Source();
			cstr sourceFile = "unknown source";
			if (s != nullptr)
			{
				sourceFile = s->Tree().Source().Name();
			}
			debugger.SetCodeHilight(sourceFile, pex.Start(), pex.End(), pex.Message());
			UpdateDebugger(ss, debugger, 0, true);
			//ss.PublicProgramObject().Log().Write(pex.Message());
			LogStack(pex, ss.PublicProgramObject().Log());
			if (s) Throw(*s, pex.Message());
			else Throw(pex.ErrorCode(), "%s", pex.Message());
			return EXECUTERESULT_THROWN;
		}
		catch (IException& ex)
		{
			UpdateDebugger(ss, debugger, 0, true);
			//ss.PublicProgramObject().Log().Write(ex.Message());
			LogStack(ex, ss.PublicProgramObject().Log());
			Throw(ex.ErrorCode(), "%s", ex.Message());
			return EXECUTERESULT_THROWN;
		}
	}

	void Preprocess(cr_sex sourceRoot, ISourceCache& sources, IScriptEnumerator& implicitIncludes, Rococo::Script::IPublicScriptSystem& ss)
	{
		bool hasIncludedFiles = false;
		bool hasIncludedNatives = false;
		bool hasIncludedImports = false;

		size_t nImplicitIncludes = implicitIncludes.Count();
		for (size_t i = 0; i < nImplicitIncludes; ++i)
		{
			cstr implicitFile = implicitIncludes.ResourceName(i);

			if (implicitFile && *implicitFile)
			{
				try
				{
					auto includedModule = sources.GetSource(implicitFile);

					ss.ValidateSecureFile(implicitFile, includedModule->Source().SourceStart(), includedModule->Source().SourceLength());

					ss.AddTree(*includedModule);
				}
				catch (IException& ex)
				{
					Throw(0, "Error adding implicit include file %s. %s", implicitFile, ex.Message());
				}
			}
		}

		for (int i = 0; i < sourceRoot.NumberOfElements(); ++i)
		{
			cr_sex sincludeExpr = sourceRoot[i];
			if (IsCompound(sincludeExpr) && sincludeExpr.NumberOfElements() >= 3)
			{
				cr_sex squot = sincludeExpr[0];
				cr_sex stype = sincludeExpr[1];

				if (squot == "'" && stype == "#include")
				{
					if (hasIncludedFiles)
					{
						Throw(sincludeExpr, "An include directive is already been stated. Merge directives.");
					}

					hasIncludedFiles = true;

					for (int j = 2; j < sincludeExpr.NumberOfElements(); j++)
					{
						cr_sex sname = sincludeExpr[j];
						if (!IsStringLiteral(sname))
						{
							Throw(sname, "expecting string literal in include directive (' #include \"<name1>\" \"<name2 etc>\" ...) ");
						}

						auto name = sname.String();

						try
						{
							auto includedModule = sources.GetSource(name->Buffer);
							ss.AddTree(*includedModule);
						}
						catch (IException& ex)
						{
							Throw(sname, "Error with include file. %s", ex.Message());
						}
					}
				}
				else if (squot == "'" && stype == "#natives")
				{
					if (hasIncludedNatives)
					{
						Throw(sincludeExpr, "A natives directive is already been stated. Merge directives.");
					}

					hasIncludedNatives = true;

					for (int j = 2; j < sincludeExpr.NumberOfElements(); j++)
					{
						cr_sex sname = sincludeExpr[j];
						if (!IsStringLiteral(sname))
						{
							Throw(sname, "expecting string literal in natives directive (' #natives \"<name1>\" \"<name2 etc>\" ...) ");
						}

						auto name = sname.String();

						ss.AddNativeLibrary(name->Buffer);
					}

					break;
				}
				else if (squot == "'" && stype == "#import")
				{
					if (hasIncludedImports)
					{
						Throw(sincludeExpr, "An import directive is already been stated. Merge directives.");
					}

					hasIncludedImports = true;

					for (int j = 2; j < sincludeExpr.NumberOfElements(); j++)
					{
						cr_sex simport = sincludeExpr[j];
						if (simport.NumberOfElements() != 2)
						{
							Throw(simport, "expecting (<package> <namespace_filter>) literal in import directive");
						}

						auto packageName = GetAtomicArg(simport[0]);
						AssertStringLiteral(simport[1]);
						cstr namespaceFilter = simport[1].String()->Buffer;

						try
						{
							ss.LoadSubpackages(namespaceFilter, packageName);
						}
						catch (ParseException&)
						{
							throw;
						}
						catch (IException& ex)
						{
							Throw(simport, "%s", ex.Message());
						}
					}
				}
			}
		}
	}

	SCRIPTEXPORT_API void InitSexyScript(ISParserTree& mainModule, IDebuggerWindow& debugger, Script::IPublicScriptSystem& ss, ISourceCache& sources, IScriptEnumerator& implicitIncludes, IScriptCompilationEventHandler& onCompile, StringBuilder* declarationBuilder)
	{
		using namespace Rococo::Script;
		using namespace Rococo::Compiler;

		ScriptCompileArgs args{ ss };
		onCompile.OnCompile(args);

		sources.RegisterPackages(ss);

		Preprocess(mainModule.Root(), sources, implicitIncludes, ss);

		ss.AddTree(mainModule);
		ss.Compile(declarationBuilder);
	}

	void Execute(VM::IVirtualMachine& vm, Rococo::Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger, bool trace)
	{
		EXECUTERESULT result = EXECUTERESULT_YIELDED;

		bool captureStructedException = false;

		if (captureStructedException && !OS::IsDebugging())
		{
			TRY_PROTECTED
			{
				result = ExecuteAndCatchIException(vm, ss, debugger, trace);
			}
			CATCH_PROTECTED
			{
				result = EXECUTERESULT_SEH;
			}
		}
		else
		{
			result = ExecuteAndCatchIException(vm, ss, debugger, trace);
		}

		switch (result)
		{
		case EXECUTERESULT_BREAKPOINT:
			UpdateDebugger(ss, debugger, 0, true);
			Throw(0, "Script hit breakpoint");
			break;
		case EXECUTERESULT_ILLEGAL:
			UpdateDebugger(ss, debugger, 0, true);
			Throw(0, "Script did something bad.");
			break;
		case EXECUTERESULT_NO_ENTRY_POINT:
			Throw(0, "No entry point");
			break;
		case EXECUTERESULT_NO_PROGRAM:
			Throw(0, "No program");
			break;
		case EXECUTERESULT_RETURNED:
			Throw(0, "Unexpected EXECUTERESULT_RETURNED");
			break;
		case EXECUTERESULT_SEH:
			UpdateDebugger(ss, debugger, 0, true);
			Throw(0, "The script triggered a structured exception handler");
			break;
		case EXECUTERESULT_THROWN:
			UpdateDebugger(ss, debugger, 0, true);
			Throw(0, "The script triggered a virtual machine exception");
			break;
		case EXECUTERESULT_YIELDED:
			UpdateDebugger(ss, debugger, 0, true);
			Throw(0, "The script yielded");
			break;
		case EXECUTERESULT_TERMINATED:
			break;
		default:
			Rococo::Throw(0, "Unexpected EXECUTERESULT %d", result);
			break;
		}
	}

	SCRIPTEXPORT_API void ExecuteFunction(ID_BYTECODE bytecodeId, IArgEnumerator& args, Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger, bool trace)
	{
		ss.PublicProgramObject().SetProgramAndEntryPoint(bytecodeId);

		auto& vm = ss.PublicProgramObject().VirtualMachine();

		struct : IArgStack, IOutputStack
		{
			virtual void PushInt32(int32 value)
			{
				vm->Push(value);
			}
			virtual void PushInt64(int64 value)
			{
				vm->Push(value);
			}
			virtual void PushPointer(void * value)
			{
				vm->Push(value);
			}
			virtual int32 PopInt32()
			{
				return vm->PopInt32();
			}

			IVirtualMachine* vm;
		} argStack;

		argStack.vm = &vm;
		args.PushArgs(argStack);

		Execute(vm, ss, debugger, trace);

		args.PopOutputs(argStack);
	};

	SCRIPTEXPORT_API void ExecuteFunction(cstr name, IArgEnumerator& args, Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger, bool trace)
	{
		auto& module = ss.PublicProgramObject().GetModule(ss.PublicProgramObject().ModuleCount() - 1);
		auto f = module.FindFunction(name);
		if (f == nullptr)
		{
			Throw(0, "Cannot find function <%s> in <%s>", name, module.Name());
		}

		ss.PublicProgramObject().SetProgramAndEntryPoint(*f);

		auto& vm = ss.PublicProgramObject().VirtualMachine();

		struct : IArgStack, IOutputStack
		{
			virtual void PushInt32(int32 value)
			{
				vm->Push(value);
			}
			virtual void PushInt64(int64 value)
			{
				vm->Push(value);
			}
			virtual void PushPointer(void * value)
			{
				vm->Push(value);
			}
			virtual int32 PopInt32()
			{
				return vm->PopInt32();
			}

			IVirtualMachine* vm;
		} argStack;

		argStack.vm = &vm;
		args.PushArgs(argStack);

		Execute(vm, ss, debugger, trace);

		args.PopOutputs(argStack);
	};

	SCRIPTEXPORT_API int ExecuteSexyScript(ScriptPerformanceStats& stats, ISParserTree& mainModule, IDebuggerWindow& debugger, Script::IPublicScriptSystem& ss, ISourceCache& sources, IScriptEnumerator& implicitIncludes, int32 param, IScriptCompilationEventHandler& onCompile, bool trace, StringBuilder* declarationBuilder)
	{
		using namespace Rococo::Script;
		using namespace Rococo::Compiler;

		Time::ticks start = Time::TickCount();
		InitSexyScript(mainModule, debugger, ss, sources, implicitIncludes, onCompile, declarationBuilder);

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		if (ns == nullptr)
		{
			Throw(0, "Cannot find (namespace EntryPoint) in %s", mainModule.Source().Name());
		}

		const IFunction* f = ns->FindFunction("Main");
		if (f == nullptr)
		{
			Throw(0, "Cannot find function EntryPoint.Main in %s", mainModule.Source().Name());
		}

		ss.PublicProgramObject().SetProgramAndEntryPoint(*f);

		auto& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Core().SetLogger(&ss.PublicProgramObject().Log());

		vm.Push(param);

		stats.compileTime = Time::TickCount() - start;

		start = Time::TickCount();

		Execute(vm, ss, debugger, trace);

		stats.executeTime = Time::TickCount() - start;

		int exitCode = vm.PopInt32();

		vm.Core().SetLogger(nullptr);
		return exitCode;
	}

	void UpdateDebugger(Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger, int32 stackDepth, bool refreshAll)
	{
		DebuggerPopulator populator;
		populator.ss = &ss;
		populator.stackDepth = stackDepth;
		populator.refreshAll = refreshAll;
		populator.Populate(debugger);
	}

	SCRIPTEXPORT_API void DebuggerLoop(Rococo::Script::IPublicScriptSystem &ss, IDebuggerWindow& debugger)
	{
		StardardDebugControl dc;

		dc.vm = &ss.PublicProgramObject().VirtualMachine();
		dc.ss = &ss;
		dc.debugger = &debugger;

		DebuggerPopulator populator;
		populator.ss = &ss;
		populator.stackDepth = 0;
		populator.refreshAll = true;

		debugger.Run(populator, dc);
	}

	namespace Script
	{
		SCRIPTEXPORT_API void AddNativeCallSecurity_ToSysNatives(Rococo::Script::IPublicScriptSystem& ss)
		{
			AddNativeCallSecurity(ss, "Sys.Native", "!scripts/native/Sys.Type.sxy");
			AddNativeCallSecurity(ss, "Sys.Reflection.Native", "!scripts/native/Sys.Reflection.sxy");
			AddNativeCallSecurity(ss, "Sys.IO.Native", "!scripts/native/Sys.IO.sxy");
			AddNativeCallSecurity(ss, "Sys.Strings.Native", "!scripts/native/Sys.Type.Strings.sxy");
		}
	}

	SCRIPTEXPORT_API IScriptEnumerator* NoImplicitIncludes()
	{
		struct NONE : IScriptEnumerator
		{
			size_t Count() const override
			{
				return 0;

			};

			cstr ResourceName(size_t index) const override
			{
				return nullptr;
			}
		};

		static NONE none;
		return &none;
	}
}

namespace Rococo::Script
{
	void PopulateStringBuilder(InterfacePointerToStringBuilder sb, const fstring& text)
	{
		InterfacePointer ip = (InterfacePointer) sb.pSexyInterfacePointer;
		ObjectStub* stub = InterfaceToInstance(ip);		

		if (!Eq(stub->Desc->TypeInfo->Name(), "FastStringBuilder"))
		{
			Throw(0, __FUNCTION__ ": Expecting the object to be of type FastStringBuilder. It was of type %s", stub->Desc->TypeInfo->Name());
		}

		auto& fsb = *(FastStringBuilder*)stub;
		int32 bufferLeft = fsb.capacity - fsb.length;

		if (bufferLeft < 0)
		{
			Throw(0, __FUNCTION__ ": FastStringBuilder had length > capacity");
		}

		if (text.length > 0 && bufferLeft > 1)
		{
			CopyString(fsb.buffer, bufferLeft, text);

			fsb.length += text.length;
			fsb.length = min(fsb.capacity - 1, fsb.length);
		}
	}
}