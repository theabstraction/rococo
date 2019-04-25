#include <sexy.types.h>
#include <sexy.s-parser.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>
#include <sexy.compiler.public.h>
#include <sexy.debug.types.h>
#include <sexy.script.h>

#include <rococo.api.h>
#include <rococo.io.h>
#include <rococo.visitors.h>
#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>
#include <sexy.strings.h>

#include <stdarg.h>

#include <unordered_map>
#include <string>

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

namespace Rococo
{
   
   using namespace Rococo::Sex;

   void UpdateDebugger(Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger, Rococo::int32 stackDepth, bool refreshAll);
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

      virtual void RefreshAtDepth(int stackDepth)
      {
         UpdateDebugger(*ss, *debugger, stackDepth, false);
      }

      virtual void PopulateAPITree(Visitors::IUITree& tree)
      {
         auto& root = ss->PublicProgramObject().GetRootNamespace();
         auto nsid = tree.AddRootItem("[Namespaces]", CheckState_Clear);
         RecurseNamespaces(root, tree, nsid);
      }

      virtual void Continue()
      {
         vm->ContinueExecution(VM::ExecutionFlags(true, true, false));
         Update();
      }

      virtual void StepOut()
      {
         vm->StepOut();
         Update();
      }

      virtual void StepOver()
      {
         vm->StepOver();
         Update();
      }

      virtual void StepNext()
      {
         vm->StepInto();
         Update();
      }

      virtual void StepNextSymbol()
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

	void ThrowSex(cr_sex s, cstr format, ...)
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

		float value;
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
		int32 value;
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

		int32 value;
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

	void ScanExpression(cr_sex s, cstr hint, cstr format, ...)
	{
		va_list args;
		va_start(args, format);

		int nElements = s.NumberOfElements();

		int elementIndex = 0;
		for (const char* p = format; *p != 0; ++p)
		{
			if (*p == 'a')
			{
				if (elementIndex >= nElements)
				{
					ThrowSex(s, "Too few elements in expression. Format is : %s", hint);
				}

				const ISExpression** ppExpr = va_arg(args, const ISExpression**);
				*ppExpr = &s[elementIndex++];

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
					ThrowSex(s, "Too few elements in expression. Format is : %s", hint);
				}

				const ISExpression** ppExpr = va_arg(args, const ISExpression**);
				*ppExpr = &s[elementIndex++];

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

	ISourceCode* DuplicateSourceCode(IOS& os, IExpandingBuffer& rbuffer, ISParser& parser, const IBuffer& rawData, cstr resourcePath)
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

	void LogParseException(ParseException& ex, IDebuggerWindow& debugger)
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

		for (const ISExpression* s = ex.Source(); s != NULL; s = s->GetTransform())
		{
			if (s->TransformDepth() > 0)  debugger.Log("Macro expansion %d:\n", s->TransformDepth());

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
			OS::ticks loadTime;
		};
		std::unordered_map<std::string, Binding> sources;
		AutoFree<IExpandingBuffer> fileBuffer;
		AutoFree<IExpandingBuffer> unicodeBuffer;
		Rococo::Sex::CSParserProxy spp;
		IInstallation& installation;

	public:
		SourceCache(IInstallation& _installation) :
			fileBuffer(CreateExpandingBuffer(64_kilobytes)),
			unicodeBuffer(CreateExpandingBuffer(64_kilobytes)),
			installation(_installation)
		{
		}

		ISourceCache* GetInterface()
		{
			return this;
		}

		~SourceCache()
		{
			for (auto i : sources)
			{
				i.second.code->Release();
				if (i.second.tree) i.second.tree->Release();
			}
		}

		IMathsVenue* Venue()
		{
			return this;
		}

		virtual void ShowVenue(IMathsVisitor& visitor)
		{
			visitor.ShowString("", "   File Length     Timestamp");

			for (auto i : sources)
			{
				char theTime[256];
				OS::FormatTime(i.second.loadTime, theTime, 256);
				visitor.ShowString(i.first.c_str(), "%8d bytes      %8.8s", i.second.code->SourceLength(), theTime);
			}
		}

		virtual void Free()
		{
			delete this;
		}

		virtual ISParserTree* GetSource(cstr resourceName)
		{
			auto i = sources.find(resourceName);
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

			installation.LoadResource(resourceName, *fileBuffer, 64_megabytes);

			ISourceCode* src = DuplicateSourceCode(installation.OS(), *unicodeBuffer, spp(), *fileBuffer, resourceName);
			sources[resourceName] = Binding{ nullptr, src, 0 };

			// We have cached the source, so that if tree generation creates an exception, the source codes is still existant

			ISParserTree* tree = spp().CreateTree(*src);
			sources[resourceName] = Binding{ tree, src, OS::UTCTime() };

			return tree;
		}

		virtual void Release(cstr resourceName)
		{
			auto i = sources.find(resourceName);
			if (i != sources.end())
			{
				i->second.code->Release();
				if (i->second.tree)i->second.tree->Release();
				sources.erase(i);
			}
		}
	};

	ISourceCache* CreateSourceCache(IInstallation& installation)
	{
		auto* cache = new SourceCache(installation);
		return cache->GetInterface();
	}

	using namespace Rococo::Compiler;
	using namespace Rococo::VM;
	using namespace Rococo::Visitors;
	using namespace Rococo::Script;

	void PopulateStackTree(IPublicScriptSystem& ss, IDebuggerWindow& debugger, int depth)
	{
		struct : Visitors::ITreePopulator
		{
			IPublicScriptSystem* ss;
			int depth;

			virtual void Populate(Visitors::IUITree& tree)
			{
				auto& vm = ss->PublicProgramObject().VirtualMachine();

				const Rococo::uint8* sf = nullptr;
				const Rococo::uint8* pc = nullptr;
				const IFunction* f = nullptr;

				size_t fnOffset;
				size_t pcOffset;
				if (!GetCallDescription(sf, pc, f, fnOffset, *ss, depth, pcOffset) || !f)
				{
					return;
				}

				char desc[256];
				SafeFormat(desc, sizeof(desc), "%p %s - %s", sf, f->Module().Name(), f->Name());

				auto sfNode = tree.AddRootItem(desc, CheckState_Clear);
				tree.SetId(sfNode, depth + 1);

				using namespace Rococo::Debugger;

				struct : public IVariableEnumeratorCallback
				{
					virtual void OnVariable(size_t index, const VariableDesc& v)
					{
						char desc[256];
						if (v.Value[0] != 0)
						{
							SafeFormat(desc, sizeof(desc), "[%d] %p %s: %s %s = %s", v.Address, v.Address + SF, v.Location, v.Type, v.Name, v.Value);
						}
						else
						{
							SafeFormat(desc, sizeof(desc), "[%d] %p %s: %s %s", v.Address, v.Address + SF, v.Location, v.Type, v.Name);
						}
						auto node = tree->AddChild(sfNode, desc, CheckState_NoCheckBox);
						tree->SetId(node, depth + 1);

						struct MyMemberEnumeratorCallback : MemberEnumeratorCallback
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

							void OnMember(IPublicScriptSystem& ss, cstr childName, const Rococo::Compiler::IMember& member, const uint8* sfItem, int recurseDepth) override
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
										SafeFormat(classDesc, sizeof(classDesc), "[+%d] %s DestructorId: %lld. Refcount: %d", (sfItem - instance), concreteStruct->Name(), header->Desc->DestructorId, header->refCount);
										auto node = tree->AddChild(parentId, classDesc, CheckState_NoCheckBox);

										firstUnkIndex = 3 + concreteStruct->InterfaceCount();

										for (int i = 0; i < concreteStruct->InterfaceCount(); ++i)
										{
											auto& interface = parentStruct->GetInterface(i);
											auto* vTable = concreteStruct->GetVirtualTable(i+1);
											char interfaceDesc[256];
											SafeFormat(interfaceDesc, sizeof(interfaceDesc), "Implements %s. vTable %p", interface.Name(), vTable);

											auto inode = tree->AddChild(node, interfaceDesc, CheckState_NoCheckBox);

											auto* instanceVTable = header->pVTables[i];
											if (instanceVTable != (VirtualTable*) vTable)
											{
												char vTableDesc[256];
												SafeFormat(vTableDesc, sizeof(vTableDesc), "vTable mismatch. Expecting %p, but found %p", vTable, instanceVTable);
												auto vnode = tree->AddChild(inode, vTableDesc, CheckState_NoCheckBox);
											}

										}
										return;
									}
									else if (index < firstUnkIndex)
									{
										return;
									}
								}

								char prefix[256] = { 0 };

								char value[256];

								auto* name = member.UnderlyingType()->Name();
								if (Eq(name, "_Null_Sys_Type_IString"))
								{
									__try
									{
										auto* s = (CClassSysTypeStringBuilder*)sfItem;
										SafeFormat(value, sizeof(value), s->buffer);
									}
									__except (EXCEPTION_EXECUTE_HANDLER)
									{
										SafeFormat(value, sizeof(value), "IString: <Bad pointer>");
									}
								}
								else
								{
									FormatValue(ss, value, sizeof(value), member.UnderlyingType()->VarType(), sfItem);
								}

								char memberDesc[256];
								SafeFormat(memberDesc, sizeof(memberDesc), "[+%d] %s%p %s: %s", (sfItem - instance), prefix, sfItem, member.Name(), value);

								auto node = tree->AddChild(parentId, memberDesc, CheckState_NoCheckBox);
								tree->SetId(node, depth + 1);

								if (member.UnderlyingType()->VarType() == VARTYPE_Derivative)
								{
									MyMemberEnumeratorCallback subMember;
									subMember.instance = instance;
									subMember.parentId = node;
									subMember.tree = tree;
									subMember.depth = depth;

									if (member.UnderlyingType()->InterfaceCount() != 0)
									{
										subMember.parentStruct = member.UnderlyingType();
									}

									GetMembers(ss, *member.UnderlyingType(), member.Name(), sfItem, 0, subMember, 1);
								}
							}
						} addMember;

						addMember.instance = v.instance;
						addMember.parentId = node;
						addMember.tree = tree;
						addMember.depth = depth + 1;

						__try
						{
							auto* instance = v.instance && v.s ? ( IsNullType(*v.s) ? *(const uint8**)v.instance : v.instance ) : nullptr;
							if (v.s) GetMembers(*ss, *v.s, v.parentName, instance, 0, addMember, 1);
						}
						__except (1)
						{

						}
					}

					int32 depth;
					TREE_NODE_ID sfNode;
					IUITree* tree;
					const uint8* SF;
					Script::IPublicScriptSystem* ss;
				} addToTree;

				addToTree.tree = &tree;
				addToTree.SF = sf;
				addToTree.sfNode = sfNode;
				addToTree.depth = depth;
				addToTree.ss = ss;

				__try
				{
					Script::ForeachVariable(*ss, addToTree, depth);
				}
				__except(1)
				{

				}
			}
		} anon;

		anon.ss = &ss;
		anon.depth = depth;
		debugger.PopulateStackView(anon);
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

		size_t i = 0;
		while (i < functionLength)
		{
			SymbolValue symbol = f->Code().GetSymbol(i);

			IDisassembler::Rep rep;
			disassembler.Disassemble(fstart + i, rep);

			bool isHighlight = (fstart + i == pc);
			if (isHighlight)
			{
				*ppExpr = (const ISExpression*)symbol.SourceExpression;
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
			}
			else
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
			}

			debugger.AddDisassembly(RGBAb(0, 0, 0), "\n");

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

			auto& vm = ss->PublicProgramObject().VirtualMachine();
			AutoFree<VM::IDisassembler> disassembler(vm.Core().CreateDisassembler());

			if (refreshAll)
			{
				debugger.BeginStackUpdate();
			}

			for (int depth = 0; depth < 10; depth++)
			{
				const ISExpression* s;
				const uint8* SF;
				auto* f = DisassembleCallStackAndAppendToView(*disassembler, debugger, *ss, vm.Cpu(), depth, &s, &SF, stackDepth);
				if (depth == stackDepth && f != nullptr)
				{
					size_t progOffset = vm.Cpu().PC() - vm.Cpu().ProgramStart;

					CodeSection section;
					f->Code().GetCodeSection(section);
					size_t fnOffset = progOffset - ss->PublicProgramObject().ProgramMemory().GetFunctionAddress(section.Id);

					auto sym = f->Code().GetSymbol(fnOffset);

					if (sym.SourceExpression != nullptr)
					{
						const Sex::ISExpression* sexpr = reinterpret_cast<const Sex::ISExpression*>(sym.SourceExpression);
						auto origin = sexpr->Tree().Source().Origin();
						auto p0 = sexpr->Start() - Vec2i{ 1,0 };
						auto p1 = sexpr->End() - Vec2i{ 1,0 };
						debugger.SetCodeHilight(sexpr->Tree().Source().Name(), p0, p1, "!");
					}
				}

				if (refreshAll)
				{
					PopulateStackTree(*ss, debugger, depth);
				}

				if (SF == nullptr) break;
			}

			if (refreshAll)
			{
				debugger.EndStackUpdate();
			}

			PopulateSourceView(*ss, debugger, stackDepth);
		}
	};

	EXECUTERESULT ExecuteAndCatchIException(IVirtualMachine& vm, Rococo::Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger)
	{
		try
		{
			EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(true, false));
			return result;
		}
		catch (IException& ex)
		{
			UpdateDebugger(ss, debugger, 0, true);
			ss.PublicProgramObject().Log().Write(ex.Message());
			Throw(ex.ErrorCode(), "%s", ex.Message());
			return EXECUTERESULT_THROWN;
		}
	}

	void Preprocess(cr_sex sourceRoot, ISourceCache& sources, Rococo::Script::IPublicScriptSystem& ss)
	{
		bool hasIncludedFiles = false;
		bool hasIncludedNatives = false;

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

						auto includedModule = sources.GetSource(name->Buffer);
						ss.AddTree(*includedModule);
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
			}
		}
	}

	void InitSexyScript(ISParserTree& mainModule, IDebuggerWindow& debugger, Script::IPublicScriptSystem& ss, ISourceCache& sources, IEventCallback<ScriptCompileArgs>& onCompile)
	{
		using namespace Rococo::Script;
		using namespace Rococo::Compiler;

		ScriptCompileArgs args{ ss };
		onCompile.OnEvent(args);

		Preprocess(mainModule.Root(), sources, ss);

		ss.AddTree(mainModule);
		ss.Compile();
	}

	void Execute(VM::IVirtualMachine& vm, Rococo::Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger)
	{
		EXECUTERESULT result = EXECUTERESULT_YIELDED;

		if (!OS::IsDebugging())
		{
			TRY_PROTECTED
			{
				result = ExecuteAndCatchIException(vm, ss, debugger);
			}
				CATCH_PROTECTED
			{
				result = EXECUTERESULT_SEH;
			}
		}
		else
		{
			result = ExecuteAndCatchIException(vm, ss, debugger);
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

	void ExecuteFunction(ID_BYTECODE bytecodeId, IArgEnumerator& args, Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger)
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

		Execute(vm, ss, debugger);

		args.PopOutputs(argStack);
	};

	void ExecuteFunction(cstr name, IArgEnumerator& args, Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger)
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

		Execute(vm, ss, debugger);

		args.PopOutputs(argStack);
	};

	int ExecuteSexyScript(ScriptPerformanceStats& stats, ISParserTree& mainModule, IDebuggerWindow& debugger, Script::IPublicScriptSystem& ss, ISourceCache& sources, int32 param, IEventCallback<ScriptCompileArgs>& onCompile)
	{
		using namespace Rococo::Script;
		using namespace Rococo::Compiler;

		OS::ticks start = OS::CpuTicks();
		InitSexyScript(mainModule, debugger, ss, sources, onCompile);

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

		vm.Push(param);

		stats.compileTime = OS::CpuTicks() - start;

		start = OS::CpuTicks();

		Execute(vm, ss, debugger);

		stats.executeTime = OS::CpuTicks() - start;

		int exitCode = vm.PopInt32();
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

	void DebuggerLoop(Rococo::Script::IPublicScriptSystem &ss, IDebuggerWindow& debugger)
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
}