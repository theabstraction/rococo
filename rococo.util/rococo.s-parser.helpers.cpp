#include <sexy.types.h>
#include <Sexy.S-Parser.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>
#include <sexy.compiler.public.h>
#include <sexy.debug.types.h>
#include <sexy.script.h>

#include <rococo.api.h>
#include <rococo.io.h>
#include <rococo.visitors.h>

#include <stdarg.h>
#include <malloc.h>
#include <wchar.h>
#include <unordered_map>
#include <excpt.h>

#include <windows.h>

#include <rococo.maths.h>
#include <rococo.strings.h>

namespace Rococo
{
   using namespace Sexy;
   using namespace Sexy::Sex;

   void UpdateDebugger(Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger, Rococo::int32 stackDepth, bool refreshAll);
}

namespace
{
   using namespace Sexy;
   using namespace Sexy::Sex;
   using namespace Sexy::Compiler;
   using namespace Rococo::Visitors;

   const wchar_t* Sanitize(const wchar_t* s)
   {
      if (wcscmp(s, L"_Null_Sys_Type_IString") == 0)
      {
         return L"IString";
      }
      else if (wcscmp(s, L"_Null_Sys_Type_IStringBuilder") == 0)
      {
         return L"IStringBuilder";
      }
      return s;
   }

   void AddArguments(const IArchetype& f, Visitors::IUITree& tree, Visitors::TREE_NODE_ID typeId)
   {
      wchar_t desc[256];

      if (f.NumberOfInputs() > (f.IsVirtualMethod() ? 1 : 0))
      {
         auto inputId = tree.AddChild(typeId, L"[Inputs]", Visitors::CheckState_Clear);
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

               SafeFormat(desc, _TRUNCATE, L"%s %s", Sanitize(arg.Name()), argName);
               tree.AddChild(inputId, desc, Visitors::CheckState_Clear);
            }
         }
      }

      if (f.NumberOfOutputs() > 0)
      {
         auto outputId = tree.AddChild(typeId, L"[Outputs]", Visitors::CheckState_Clear);

         if (f.NumberOfOutputs() == 1 && (f.NumberOfInputs() == 0 || (f.NumberOfInputs() == 1 && f.IsVirtualMethod())))
         {
            auto& arg = f.GetArgument(0);
            auto argName = f.GetArgName(0);

            SafeFormat(desc, _TRUNCATE, L"%s %s - Get Accessor", Sanitize(arg.Name()), argName);
            tree.AddChild(outputId, desc, Visitors::CheckState_Clear);
         }
         else
         {  
            for (int i = 0; i < f.NumberOfOutputs(); ++i)
            {
               auto& arg = f.GetArgument(i);
               auto argName = f.GetArgName(i);

               SafeFormat(desc, _TRUNCATE, L"%s %s", Sanitize(arg.Name()), argName);
               tree.AddChild(outputId, desc, Visitors::CheckState_Clear);
            }
         }
      }
   }

   struct StardardDebugControl : IDebugControl
   {
      Sexy::VM::IVirtualMachine* vm;
      Sexy::Script::IPublicScriptSystem* ss;
      IDebuggerWindow* debugger;

      void Update()
      {
         UpdateDebugger(*ss, *debugger, 0, true);
      }

      void RecurseNamespaces(const Sexy::Compiler::INamespace& ns, IUITree& tree, Visitors::TREE_NODE_ID rootId)
      {
         if (ns.ChildCount() > 0)
         {
            auto childSubspacesId = tree.AddChild(rootId, L"[Subspaces]", CheckState_Clear);

            for (int i = 0; i < ns.ChildCount(); ++i)
            {
               auto& child = ns.GetChild(i);
               auto childId = tree.AddChild(childSubspacesId, child.Name()->Buffer, CheckState_Clear);
               RecurseNamespaces(child, tree, childId);
            }
         }

         struct : public ICallback<const Sexy::Compiler::IStructure, csexstr>
         {
            TREE_NODE_ID childStructuresId;
            TREE_NODE_ID rootId;
            IUITree* tree;
            const Sexy::Compiler::INamespace* ns;
            virtual CALLBACK_CONTROL operator()(const Sexy::Compiler::IStructure& s, csexstr alias)
            {
               if (childStructuresId.value == 0) childStructuresId = tree->AddChild(rootId, L"[Structures]", CheckState_Clear);

               wchar_t desc[256];
               SafeFormat(desc, _TRUNCATE, L"%s.%s", ns->FullName()->Buffer, alias);
               auto typeId = tree->AddChild(childStructuresId, desc, CheckState_Clear);

               SafeFormat(desc, _TRUNCATE, L"%s - %d bytes. Defined in %s", Parse::VarTypeName(s.VarType()), s.SizeOfStruct(), s.Module().Name());
               auto typeDescId = tree->AddChild(typeId, desc, CheckState_Clear);

               if (s.VarType() == VARTYPE_Derivative)
               {
                  for (int32 i = 0; i < s.MemberCount(); ++i)
                  {
                     const wchar_t* fieldType = s.GetMember(i).UnderlyingType() ? s.GetMember(i).UnderlyingType()->Name() : L"Unknown Type";
                     SafeFormat(desc, _TRUNCATE, L"%s %s", fieldType, s.GetMember(i).Name());
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

         struct : public ICallback<const Sexy::Compiler::IFunction, csexstr>
         {
            TREE_NODE_ID childFunctionsId;
            TREE_NODE_ID rootId;
            IUITree* tree;
            const Sexy::Compiler::INamespace* ns;
            virtual CALLBACK_CONTROL operator()(const Sexy::Compiler::IFunction& f, csexstr alias)
            {
               if (childFunctionsId.value == 0) childFunctionsId = tree->AddChild(rootId, L"[Functions]", CheckState_Clear);

               TREE_NODE_ID typeId;
               wchar_t desc[256];

               SafeFormat(desc, _TRUNCATE, L"%s.%s", ns->FullName()->Buffer, alias);
               typeId = tree->AddChild(childFunctionsId, desc, CheckState_Clear);

               SafeFormat(desc, _TRUNCATE, L"Defined in %s", f.Module().Name());
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
            wchar_t desc[256];

            auto interfaceId = tree.AddChild(rootId, L"[Interfaces]", CheckState_Clear);

            for (int i = 0; i < ns.InterfaceCount(); ++i)
            {
               auto& inter = ns.GetInterface(i);
               auto* base = inter.Base();

               if (base)
               {
                  SafeFormat(desc, _TRUNCATE, L"%s.%s extending %s", ns.FullName()->Buffer, inter.Name(), base->Name());
               }
               else
               {
                  SafeFormat(desc, _TRUNCATE, L"%s.%s", ns.FullName()->Buffer, inter.Name());
               }

               auto interId = tree.AddChild(interfaceId, desc, CheckState_Clear);

               for (int j = 0; j < inter.MethodCount(); ++j)
               {
                  auto& method = inter.GetMethod(j);
                  SafeFormat(desc, _TRUNCATE, L"method %s", method.Name());
                  auto methodId = tree.AddChild(interId, desc, CheckState_Clear);
                  AddArguments(method, tree, methodId);
               }
            }
         }

         struct : public ICallback<const Sexy::Compiler::IFactory, csexstr>
         {
            TREE_NODE_ID childFactoryId;
            TREE_NODE_ID rootId;
            IUITree* tree;
            const Sexy::Compiler::INamespace* ns;
            virtual CALLBACK_CONTROL operator()(const Sexy::Compiler::IFactory& f, csexstr alias)
            {
               if (childFactoryId.value == 0) childFactoryId = tree->AddChild(rootId, L"[Factories]", CheckState_Clear);

               TREE_NODE_ID typeId;
               wchar_t desc[256];

               SafeFormat(desc, _TRUNCATE, L"%s.%s - creates objects of type %s", ns->FullName()->Buffer, alias, f.InterfaceType()->Buffer);
               typeId = tree->AddChild(childFactoryId, desc, CheckState_Clear);

               SafeFormat(desc, _TRUNCATE, L"Defined in %s", f.Constructor().Module().Name());
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
         auto nsid = tree.AddRootItem(L"[Namespaces]", CheckState_Clear);
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
	using namespace Sexy;
	using namespace Sexy::Sex;
   using namespace Rococo::Visitors;

	void ThrowSex(cr_sex s, const wchar_t* format, ...)
	{
		va_list args;
		va_start(args, format);

		wchar_t msg[512];
		SafeVFormat(msg, _TRUNCATE, format, args);

		auto start = s.Start();
		auto end = s.End();

		SEXCHAR specimen[64];
		Sexy::Sex::GetSpecimen(specimen, s);

		ParseException ex(start, end, L"ParseException", msg, specimen, &s);

		TripDebugger();

		throw ex;
	}

	void ValidateArgument(cr_sex s, const wchar_t* arg)
	{
		auto txt = s.String();

		if (!IsAtomic(s) || wcscmp(arg, txt->Buffer) != 0)
		{
			if (arg[0] == '\'' && arg[1] == 0)
			{
				ThrowSex(s, L"Expecting quote character");
			}
			else
			{
				ThrowSex(s, L"Expecting atomic argument: '%s'", arg);
			}
		}
	}

	float GetValue(cr_sex s, float minValue, float maxValue, csexstr hint)
	{
		sexstring txt = s.String();

		float value;
		if (!IsAtomic(s) || Parse::PARSERESULT_GOOD != Parse::TryParseFloat(value, txt->Buffer))
		{
			ThrowSex(s, L"%s: Expecting atomic argument float", hint);
		}

		if (value < minValue || value > maxValue)
		{
			ThrowSex(s, L"%s: Value %g must be in domain [%g,%g]", hint, value, minValue, maxValue);
		}

		return value;
	}

	RGBAb GetColourValue(cr_sex s)
	{
		int32 value;
		if (!IsAtomic(s) || Parse::PARSERESULT_GOOD != Parse::TryParseHex(value, s.String()->Buffer))
		{
			ThrowSex(s, L"Cannot parse hex colour value");
		}

		if (value > 0x00FFFFFF)
		{
			ThrowSex(s, L"Expecting hex digits RRGGBB");
		}

		int red = (value >> 16) & 0x000000FF;
		int green = (value >> 8) & 0x000000FF;
		int blue = value & 0x000000FF;

		return RGBAb(red, green, blue);
	}

	Quat GetQuat(cr_sex s)
	{
		if (s.NumberOfElements() != 4) Throw(s, L"Expecting quat (Vx Vy Vz S)");
		float Vx = GetValue(s[0], -1.0e10f, 1e10f, L"Vx component");
		float Vy = GetValue(s[1], -1.0e10f, 1e10f, L"Vy component");
		float Vz = GetValue(s[2], -1.0e10f, 1e10f, L"Vz component");
		float S = GetValue(s[3], -1.0e10f, 1e10f, L"scalar component");

		return Quat{ Vec3{ Vx,Vy,Vz }, S };
	}

	Vec3 GetVec3Value(cr_sex sx, cr_sex sy, cr_sex sz)
	{
		float x = GetValue(sx, -1.0e10f, 1e10f, L"x component");
		float y = GetValue(sy, -1.0e10f, 1e10f, L"y component");
		float z = GetValue(sz, -1.0e10f, 1e10f, L"z component");
		return Vec3{ x, y, z };
	}

	int32 GetValue(cr_sex s, int32 minValue, int32 maxValue, csexstr hint)
	{
		sexstring txt = s.String();

		int32 value;
		if (!IsAtomic(s) || Parse::PARSERESULT_GOOD != Parse::TryParseDecimal(value, txt->Buffer))
		{
			ThrowSex(s, L"%s: Expecting atomic argument int32", hint);
		}

		if (value < minValue || value > maxValue)
		{
			ThrowSex(s, L"%s: Value %d must be in domain [%d,%d]", hint, value, minValue, maxValue);
		}

		return value;
	}

	void ScanExpression(cr_sex s, const wchar_t* hint, const char* format, ...)
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
					ThrowSex(s, L"Too few elements in expression. Format is : %s", hint);
				}

				const ISExpression** ppExpr = va_arg(args, const ISExpression**);
				*ppExpr = &s[elementIndex++];

				cr_sex child = **ppExpr;

				const auto s = child.String();

				if (!IsAtomic(child))
				{
					ThrowSex(child, L"Expecting atomic element in expression. Format is : %s", hint);
				}
			}
			else if (*p == 'c')
			{
				if (elementIndex >= nElements)
				{
					ThrowSex(s, L"Too few elements in expression. Format is : %s", hint);
				}

				const ISExpression** ppExpr = va_arg(args, const ISExpression**);
				*ppExpr = &s[elementIndex++];

				cr_sex child = **ppExpr;

				if (!IsCompound(child))
				{
					ThrowSex(child, L"Expecting compound element in expression. Format is : %s", hint);
				}
			}
			else if (*p == ' ')
			{

			}
			else
			{
				Throw(0, L"Bad format character %c", *p);
			}
		}
	}

	ISourceCode* DuplicateSourceCode(IOS& os, IExpandingBuffer& unicodeBuffer, ISParser& parser, const IBuffer& rawData, const wchar_t* resourcePath)
	{
		const char* utf8data = (const char*)rawData.GetData();
		size_t rawLength = rawData.Length();

		if (rawLength < 2)
		{
			Throw(0, L"Script file '%s' was too small", resourcePath);
		}

		if (rawLength % 2 != 0)
		{
			unicodeBuffer.Resize(2 * rawLength + 2);
			os.UTF8ToUnicode(utf8data, (wchar_t*)unicodeBuffer.GetData(), rawLength, rawLength + 1);
         auto* source = parser.DuplicateSourceBuffer((wchar_t*)unicodeBuffer.GetData(), (int)rawLength, Vec2i{ 1,1 }, resourcePath);
			return source;
		}

		wchar_t bom = *(wchar_t*)utf8data;

		if (bom == 0xFEFF)
		{
			wchar_t* buf = (wchar_t*)(utf8data + 2);
			size_t nChars = rawLength / 2 - 1;
         return parser.DuplicateSourceBuffer(buf, (int)nChars, Vec2i{ 1,1 }, resourcePath);
		}
		else if (bom == 0xFFFE)
		{
			Throw(0, L"Script file '%s' UNICODE was incorrect endian for this OS", resourcePath);
			return nullptr;
		}
		else if ((bom & 0x00FF) == 0)
		{
			wchar_t* buf = (wchar_t*)utf8data;
			size_t nChars = rawLength / 2;
			return parser.DuplicateSourceBuffer(buf, (int)nChars, Vec2i{ 1,1 }, resourcePath);
		}
		else
		{
			unicodeBuffer.Resize(2 * rawLength + 2);
			os.UTF8ToUnicode(utf8data, (wchar_t*)unicodeBuffer.GetData(), rawLength, rawLength + 1);
			auto* source = parser.DuplicateSourceBuffer((wchar_t*)unicodeBuffer.GetData(), (int)rawLength, Vec2i{ 1,1 }, resourcePath);
			return source;
		}
	}

	fstring GetAtomicArg(cr_sex s)
	{
		if (!IsAtomic(s)) ThrowSex(s, L"Expecting atomic argument");
		auto st = s.String();
		return fstring{ st->Buffer, st->Length };
	}

	void PrintExpression(cr_sex s, int &totalOutput, int maxOutput, ILogger& logger)
	{
		switch (s.Type())
		{
		case EXPRESSION_TYPE_ATOMIC:
			totalOutput += logger.Log(SEXTEXT(" %s"), (csexstr)s.String()->Buffer);
			break;
		case EXPRESSION_TYPE_STRING_LITERAL:
			totalOutput += logger.Log(SEXTEXT(" \"%s\""), (csexstr)s.String()->Buffer);
			break;
		case EXPRESSION_TYPE_COMPOUND:

			totalOutput += logger.Log(SEXTEXT(" ("));

			for (int i = 0; i < s.NumberOfElements(); ++i)
			{
				if (totalOutput > maxOutput)
				{
					return;
				}

				cr_sex child = s.GetElement(i);
				PrintExpression(child, totalOutput, maxOutput, logger);
			}

			totalOutput += logger.Log(SEXTEXT(" )"));
			break;
		case EXPRESSION_TYPE_NULL:
			totalOutput += logger.Log(SEXTEXT(" ()"));
			break;
		}
	}

	void LogParseException(ParseException& ex, IDebuggerWindow& debugger)
	{
      Vec2i a = ex.Start();
      Vec2i b = ex.End();

      debugger.AddLogSection(RGBAb(128, 0, 0), L"ParseException\n");
      debugger.AddLogSection(RGBAb(64, 64, 64), L" Name: ");
      debugger.AddLogSection(RGBAb(0, 0, 128), L"%s\n", ex.Name());

      debugger.AddLogSection(RGBAb(64, 64, 64), L" Message: ");
      debugger.AddLogSection(RGBAb(0, 0, 128), L"%s\n", ex.Message());

      debugger.AddLogSection(RGBAb(64, 64, 64), L" Specimen: (");
      debugger.AddLogSection(RGBAb(0, 0, 128), L"%d", a.x);
      debugger.AddLogSection(RGBAb(64, 64, 64), L",");
      debugger.AddLogSection(RGBAb(0, 0, 128), L"%d", a.y);
      debugger.AddLogSection(RGBAb(64, 64, 64), L") to (");
      debugger.AddLogSection(RGBAb(0, 0, 128), L"%d", b.x);
      debugger.AddLogSection(RGBAb(64, 64, 64), L",");
      debugger.AddLogSection(RGBAb(0, 0, 128), L"%d", b.y);
      debugger.AddLogSection(RGBAb(64, 64, 64), L")\n");

      debugger.AddLogSection(RGBAb(0, 127, 0), L"%s\n", ex.Specimen());

      a = a - Vec2i{ 1, 0 };
      b = b - Vec2i{ 1, 0 };

      debugger.SetCodeHilight(ex.Name(), a, b, ex.Message());

		auto s = ex.Source();
		if (s)
		{
			auto t = s->GetTransform();
			if (t)
			{
				int totalOutput = 0;
	//			PrintExpression(*t, totalOutput, 1024, logger);
			}
		}

		for (const ISExpression* s = ex.Source(); s != NULL; s = s->GetOriginal())
		{
			if (s->TransformDepth() > 0)  debugger.Log(SEXTEXT("Macro expansion %d:\n"), s->TransformDepth());

			int totalOutput = 0;
	//		PrintExpression(*s, totalOutput, 1024, logger);

			if (totalOutput > 1024) debugger.Log(SEXTEXT("..."));

         debugger.Log(SEXTEXT("\n"));
		}

      debugger.Log(SEXTEXT("Error parsing script file: scroll down to see the error message"));
	}

	class SourceCache : public ISourceCache
	{
	private:
		struct Binding
		{
			ISParserTree* tree;
			ISourceCode* code;
		};
		std::unordered_map<std::wstring, Binding> sources;
		AutoFree<IExpandingBuffer> fileBuffer;
		AutoFree<IExpandingBuffer> unicodeBuffer;
		Sexy::Sex::CSParserProxy spp;
		IInstallation& installation;

	public:
		SourceCache(IInstallation& _installation) :
			fileBuffer(CreateExpandingBuffer(64_kilobytes)),
			unicodeBuffer(CreateExpandingBuffer(64_kilobytes)),
			installation(_installation)
		{
		}

		~SourceCache()
		{
			for (auto i : sources)
			{
				i.second.code->Release();
				if (i.second.tree) i.second.tree->Release();
			}
		}

		virtual void Free()
		{
			delete this;
		}

		virtual ISParserTree* GetSource(const wchar_t* resourceName)
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
			sources[resourceName] = Binding{ nullptr, src };

			// We have cached the source, so that if tree generation creates an exception, the source codes is still existant

			ISParserTree* tree = spp().CreateTree(*src);
			sources[resourceName] = Binding{ tree, src };

			return tree;
		}

		virtual void Release(const wchar_t* resourceName)
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
		return new SourceCache(installation);
	}

	using namespace Sexy::Compiler;
	using namespace Sexy::VM;
	using namespace Rococo::Visitors;

	void PopulateStackTree(Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger, int depth)
	{
      struct : Visitors::ITreePopulator
      {
         Script::IPublicScriptSystem* ss;
         int depth;

         virtual void Populate(Visitors::IUITree& tree)
         {
            auto& vm = ss->PublicProgramObject().VirtualMachine();

            const Sexy::uint8* sf = nullptr;
            const Sexy::uint8* pc = nullptr;
            const IFunction* f = nullptr;

            size_t fnOffset;
            if (!Sexy::Script::GetCallDescription(sf, pc, f, fnOffset, *ss, depth) || !f)
            {
               return;
            }
            
            wchar_t desc[256];
            SafeFormat(desc, _TRUNCATE, L"%p %s - %s", sf, f->Module().Name(), f->Name());

            auto sfNode = tree.AddRootItem(desc, CheckState_Clear);
            tree.SetId(sfNode, depth + 1);

            using namespace Sexy::Debugger;

            struct : public IVariableEnumeratorCallback
            {
               virtual void OnVariable(size_t index, const VariableDesc& v)
               {
                  wchar_t desc[256];
                  if (v.Value[0] != 0)
                  {
                     SafeFormat(desc, _TRUNCATE, L"%p %S: %S %S = %S", v.Address + SF, v.Location, v.Type, v.Name, v.Value);
                  }
                  else
                  {
                     SafeFormat(desc, _TRUNCATE, L"%p %S: %S %S", v.Address + SF, v.Location, v.Type, v.Name);
                  }
                  auto node = tree->AddChild(sfNode, desc, CheckState_NoCheckBox);
                  tree->SetId(node, depth+1);
               }

               int32 depth;
               TREE_NODE_ID sfNode;
               IUITree* tree;
               const uint8* SF;
            } addToTree;

            addToTree.tree = &tree;
            addToTree.SF = sf;
            addToTree.sfNode = sfNode;
            addToTree.depth = depth;

            Script::ForeachVariable(*ss, addToTree, depth);
         }
      } anon;

      anon.ss = &ss;
      anon.depth = depth;

      debugger.PopulateStackView(anon);
	}

   void PopulateSourceView(Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger, int64 stackDepth)
   {
        const Sexy::uint8* sf = nullptr;
        const Sexy::uint8* pc = nullptr;
        const Sexy::Compiler::IFunction* f;
        
        size_t fnOffset;
        if (Sexy::Script::GetCallDescription(sf, pc, f, fnOffset, ss, stackDepth) && f)
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

		using namespace Sexy::Debugger;

		struct : public IRegisterEnumerationCallback
		{
			int count;
			int maxCount;

			virtual void OnRegister(const char* name, const char* value)
			{
				if (count < maxCount)
				{
					wchar_t wname[128], wvalue[128];
					SafeFormat(wname, _TRUNCATE, L"%S", name);
					SafeFormat(wvalue, _TRUNCATE, L"%S", value);

					const wchar_t* row[] = { wname, wvalue, nullptr };
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

	const IFunction* DisassembleCallStackAndAppendToView(IDisassembler& disassembler, IDebuggerWindow& debugger, Sexy::Script::IPublicScriptSystem& ss, CPU& cpu, size_t callDepth, const ISExpression** ppExpr, const uint8** ppSF, size_t populateAtDepth)
	{
		const Sexy::uint8* sf = nullptr;
		const Sexy::uint8* pc = nullptr;
		const IFunction* f = nullptr;
		*ppExpr = nullptr;
		*ppSF = nullptr;

		size_t fnOffset;
		if (!Sexy::Script::GetCallDescription(sf, pc, f, fnOffset, ss, callDepth) || !f)
		{
			return nullptr;
		}

		*ppSF = sf;

      if (callDepth != populateAtDepth || populateAtDepth == (size_t) -1)
      {
         return f;
      }

		CodeSection section;
		f->Code().GetCodeSection(section);

		IPublicProgramObject& po = ss.PublicProgramObject();
		IVirtualMachine& vm = po.VirtualMachine();

		size_t functionLength = po.ProgramMemory().GetFunctionLength(section.Id); 
      
      debugger.InitDisassembly(section.Id);

		wchar_t metaData[256];
		SafeFormat(metaData, _TRUNCATE, L"%s %s (Id #%d) - %d bytes\n\n", f->Name(), f->Module().Name(), (int32)section.Id, (int32)functionLength);
		debugger.AddDisassembly(RGBAb(128,128,0), metaData);

		int lineCount = 1;

		const Sexy::uint8* fstart = vm.Cpu().ProgramStart + po.ProgramMemory().GetFunctionAddress(section.Id);

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

			wchar_t assemblyLine[256];
			
         if (isHighlight)
         {
            debugger.AddDisassembly(RGBAb(128,0,0), L"*", RGBAb(255, 255, 255), true);
            SafeFormat(assemblyLine, _TRUNCATE, L"%p", fstart + i);
            debugger.AddDisassembly(RGBAb(255, 255, 255), assemblyLine, RGBAb(0, 0, 255));
            SafeFormat(assemblyLine, _TRUNCATE, L" %s %s ", rep.OpcodeText, rep.ArgText);
            debugger.AddDisassembly(RGBAb(255, 255, 255), assemblyLine, RGBAb(0, 0, 255));

            if (symbol.Text[0] != 0)
            {
               SafeFormat(assemblyLine, _TRUNCATE, L"// %s", symbol.Text);
               debugger.AddDisassembly(RGBAb(32, 128, 0), assemblyLine);
            }
         }
         else
         {
            SafeFormat(assemblyLine, _TRUNCATE, L" %p", fstart + i);
            debugger.AddDisassembly(RGBAb(0, 0, 0), assemblyLine);
            SafeFormat(assemblyLine, _TRUNCATE, L" %s %s ", rep.OpcodeText, rep.ArgText);
            debugger.AddDisassembly(RGBAb(128, 0, 0), assemblyLine);

            if (symbol.Text[0] != 0)
            {
               SafeFormat(assemblyLine, _TRUNCATE, L"// %s", symbol.Text);
               debugger.AddDisassembly(RGBAb(0, 128, 0), assemblyLine);
            }
         }

         debugger.AddDisassembly(RGBAb(0, 0, 0), L"\n");

			if (rep.ByteCount == 0)
			{
				debugger.AddDisassembly(RGBAb(128,0,0), L"Bad disassembly");
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
                  auto origin =  sexpr->Tree().Source().Origin();
                  auto p0 = sexpr->Start() - Vec2i{ 1,0 };
                  auto p1 = sexpr->End() - Vec2i{ 1,0 };
                  debugger.SetCodeHilight(sexpr->Tree().Source().Name(), p0, p1, L"!");
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

	EXECUTERESULT ExecuteAndCatchIException(IVirtualMachine& vm, Sexy::Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger)
	{
		try
		{
			EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(true, false));
			return result;
		}
		catch (IException& ex)
		{
			ss.PublicProgramObject().Log().Write(ex.Message());
         UpdateDebugger(ss, debugger, 0, true);
			Throw(ex.ErrorCode(), L"%s", ex.Message());
			return EXECUTERESULT_SEH;
		}
	}

	void Preprocess(cr_sex sourceRoot, ISourceCache& sources, Sexy::Script::IPublicScriptSystem& ss)
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

				if (squot == L"'" && stype == L"#include")
				{
					if (hasIncludedFiles)
					{
						Throw(sincludeExpr, L"An include directive is already been stated. Merge directives.");
					}

					hasIncludedFiles = true;

					for (int j = 2; j < sincludeExpr.NumberOfElements(); j++)
					{
						cr_sex sname = sincludeExpr[j];
						if (!IsStringLiteral(sname))
						{
							Throw(sname, L"expecting string literal in include directive (' #include \"<name1>\" \"<name2 etc>\" ...) ");
						}

						auto name = sname.String();

						auto includedModule = sources.GetSource(name->Buffer);
						ss.AddTree(*includedModule);
					}
				}
				else if (squot == L"'" && stype == L"#natives")
				{
					if (hasIncludedNatives)
					{
						Throw(sincludeExpr, L"A natives directive is already been stated. Merge directives.");
					}

					hasIncludedNatives = true;

					for (int j = 2; j < sincludeExpr.NumberOfElements(); j++)
					{
						cr_sex sname = sincludeExpr[j];
						if (!IsStringLiteral(sname))
						{
							Throw(sname, L"expecting string literal in natives directive (' #natives \"<name1>\" \"<name2 etc>\" ...) ");
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
		using namespace Sexy::Script;
		using namespace Sexy::Compiler;

		ScriptCompileArgs args{ ss };
		onCompile.OnEvent(args);

		Preprocess(mainModule.Root(), sources, ss);

		ss.AddTree(mainModule);
		ss.Compile();
	}

	void Execute(VM::IVirtualMachine& vm, Sexy::Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger)
	{
		EXECUTERESULT result = EXECUTERESULT_YIELDED;

		if (!IsDebugging())
		{
			__try
			{
				result = ExecuteAndCatchIException(vm, ss, debugger);
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
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
			Throw(0, L"Script hit breakpoint");
			break;
		case EXECUTERESULT_ILLEGAL:
         UpdateDebugger(ss, debugger, 0, true);
			Throw(0, L"Script did something bad.");
			break;
		case EXECUTERESULT_NO_ENTRY_POINT:
			Throw(0, L"No entry point");
			break;
		case EXECUTERESULT_NO_PROGRAM:
			Throw(0, L"No program");
			break;
		case EXECUTERESULT_RETURNED:
			Throw(0, L"Unexpected EXECUTERESULT_RETURNED");
			break;
		case EXECUTERESULT_SEH:
         UpdateDebugger(ss, debugger, 0, true);
			Throw(0, L"The script triggered a structured exception handler");
			break;
		case EXECUTERESULT_THROWN:
         UpdateDebugger(ss, debugger, 0, true);
			Throw(0, L"The script triggered a virtual machine exception");
			break;
		case EXECUTERESULT_YIELDED:
         UpdateDebugger(ss, debugger, 0, true);
			Throw(0, L"The script yielded");
			break;
		case EXECUTERESULT_TERMINATED:
			break;
		default:
         Rococo::Throw(0, L"Unexpected EXECUTERESULT %d", result);
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

	void ExecuteFunction(const wchar_t* name, IArgEnumerator& args, Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger)
	{
		auto& module = ss.PublicProgramObject().GetModule(ss.PublicProgramObject().ModuleCount() - 1);
		auto f = module.FindFunction(name);
		if (f == nullptr)
		{
			Throw(0, L"Cannot find function <%s> in <%s>", name, module.Name());
		}

		ss.PublicProgramObject().SetProgramAndEntryPoint(*f);

		auto& vm = ss.PublicProgramObject().VirtualMachine();

		struct: IArgStack, IOutputStack
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

	int32 ExecuteSexyScript(ISParserTree& mainModule, IDebuggerWindow& debugger, Script::IPublicScriptSystem& ss, ISourceCache& sources, int32 param, IEventCallback<ScriptCompileArgs>& onCompile)
	{
		using namespace Sexy::Script;
		using namespace Sexy::Compiler;

		InitSexyScript(mainModule, debugger, ss, sources, onCompile);

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace(SEXTEXT("EntryPoint"));
		if (ns == nullptr)
		{
			Throw(0, L"Cannot find (namespace EntryPoint) in %s", mainModule.Source().Name());
		}

		const IFunction* f = ns->FindFunction(L"Main");
		if (f == nullptr)
		{
			Throw(0, L"Cannot find function EntryPoint.Main in %s", mainModule.Source().Name());
		}

		ss.PublicProgramObject().SetProgramAndEntryPoint(*f);

		auto& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(param);

		Execute(vm, ss, debugger);

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

	void DebuggerLoop(Sexy::Script::IPublicScriptSystem &ss, IDebuggerWindow& debugger)
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