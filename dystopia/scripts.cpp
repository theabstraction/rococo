#include <rococo.types.h>
#include <rococo.io.h>
#include <rococo.visitors.h>
#include <sexy.types.h>
#include <sexy.compiler.public.h>
#include <sexy.debug.types.h>
#include <sexy.script.h>
#include <Sexy.S-Parser.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>

#include <stdarg.h>
#include <malloc.h>
#include <wchar.h>
#include <process.h>

#include <windows.h>

#include "dystopia.h"

#include <string>
#include <unordered_map>

using namespace Rococo;
using namespace Sexy;
using namespace Sexy::Sex;

namespace
{
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
			auto* source = parser.DuplicateSourceBuffer((wchar_t*)unicodeBuffer.GetData(), (int)rawLength, SourcePos(1, 1), resourcePath);
			return source;
		}

		wchar_t bom = *(wchar_t*)utf8data;

		if (bom == 0xFEFF)
		{
			wchar_t* buf = (wchar_t*)(utf8data + 2);
			size_t nChars = rawLength / 2 - 1;
			return parser.DuplicateSourceBuffer(buf, (int)nChars, SourcePos(1, 1), resourcePath);
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
			return parser.DuplicateSourceBuffer(buf, (int)nChars, SourcePos(1, 1), resourcePath);
		}
		else
		{
			unicodeBuffer.Resize(2 * rawLength + 2);
			os.UTF8ToUnicode(utf8data, (wchar_t*)unicodeBuffer.GetData(), rawLength, rawLength + 1);
			auto* source = parser.DuplicateSourceBuffer((wchar_t*)unicodeBuffer.GetData(), (int)rawLength, SourcePos(1, 1), resourcePath);
			return source;
		}
	}
}

namespace Dystopia
{
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

	fstring FstringFromPointer(const wchar_t* const buffer)
	{
		return fstring{ buffer, (int32) wcslen(buffer) };
	}

	bool AreAtomicTokensEqual(cr_sex s, const fstring& token)
	{
		if (s.Type() == EXPRESSION_TYPE_ATOMIC)
		{
			auto str = s.String();
			if (token.length == str->Length)
			{
				return wcsncmp(token.buffer, token.buffer, token.length) == 0;
			}
		}

		return false;
	}

	class SourceCache: public ISourceCache
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

		enum { MAX_SOURCE_FILE_LENGTH = 64 * 1048576 };
	public:
		SourceCache(IInstallation& _installation): 
			fileBuffer(CreateExpandingBuffer(64 * 1024)),
			unicodeBuffer(CreateExpandingBuffer(64 * 1024)),
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
			
			installation.LoadResource(resourceName, *fileBuffer, MAX_SOURCE_FILE_LENGTH);

			ISourceCode* src = DuplicateSourceCode(installation.OS(), *unicodeBuffer, spp(), *fileBuffer, resourceName);
			sources[resourceName] = Binding{ nullptr, src };

			// We have cached the source, so that if tree generation creates an exception, the source codes is still existant

			ISParserTree* tree = spp().CreateTree(*src);
			sources[resourceName] = Binding { tree, src };
			
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

	void PopulateStackTree(Script::IPublicScriptSystem& ss, IUITree& tree, int depth)
	{
		auto& vm = ss.PublicProgramObject().VirtualMachine();

		const Sexy::uint8* sf = nullptr;
		const Sexy::uint8* pc = nullptr;
		const IFunction* f = nullptr;

		size_t fnOffset;
		if (!Sexy::Script::GetCallDescription(sf, pc, f, fnOffset, ss, depth) || !f)
		{
			return;
		}

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
				tree->AddRootItem(desc, CheckState_NoCheckBox);
			}

			IUITree* tree;
			const uint8* SF;
		} addToTree;

		addToTree.tree = &tree;
		addToTree.SF = sf;

		Script::ForeachVariable(ss, addToTree, depth);

		tree.AddRootItem(L"", CheckState_NoCheckBox);
	}

	void PopulateRegisterWindow(Script::IPublicScriptSystem& ss, IUIList& registerList)
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

		addToList.uiList = &registerList;
		addToList.count = 0;
		addToList.maxCount = 9;

		registerList.ClearRows();

		Script::EnumerateRegisters(vm.Cpu(), addToList);
	}

	const IFunction* DisassembleCallStackAndAppendToView(IDisassembler& disassembler, Environment& e, Sexy::Script::IPublicScriptSystem& ss, CPU& cpu, size_t callDepth, const ISExpression** ppExpr, const uint8** ppSF)
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

		CodeSection section;
		f->Code().GetCodeSection(section);

		IPublicProgramObject& po = ss.PublicProgramObject();
		IVirtualMachine& vm = po.VirtualMachine();

		size_t functionLength = po.ProgramMemory().GetFunctionLength(section.Id);

		wchar_t metaData[256];
		SafeFormat(metaData, _TRUNCATE, L"%s %s (Id #%d) - %d bytes\n", f->Name(), f->Module().Name(), (int32) section.Id, (int32) functionLength);
		e.debuggerWindow.AddDisassembly(false, metaData);

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
			SafeFormat(assemblyLine, _TRUNCATE, L"%c%p %s %s // %s", isHighlight ? L'*': L' ', fstart + i, rep.OpcodeText, rep.ArgText, symbol.Text);
			e.debuggerWindow.AddDisassembly(false, assemblyLine);

			if (rep.ByteCount == 0)
			{
				e.debuggerWindow.AddDisassembly(false, L"Bad disassembly");
				break;
			}

			i += rep.ByteCount;
			lineCount++;
		}

		e.debuggerWindow.AddDisassembly(false, L"");

		return f;
	}

	void Disassemble(VM::IVirtualMachine& vm, Script::IPublicScriptSystem& ss, Environment& e)
	{
		AutoFree<VM::IDisassembler> disassembler(vm.Core().CreateDisassembler());

		e.debuggerWindow.AddDisassembly(true, nullptr);
		e.debuggerWindow.StackTree().ResetContent();
		
		PopulateRegisterWindow(ss, e.debuggerWindow.RegisterList());

		for (int depth = 0; depth < 10; depth++)
		{
			const ISExpression* s;
			const uint8* SF;
			DisassembleCallStackAndAppendToView(*disassembler, e, ss, vm.Cpu(), depth, &s, &SF);
			PopulateStackTree(ss, e.debuggerWindow.StackTree(), depth);
			
			if (SF == nullptr) break;
		}
	}

	void ExecuteSexyScript(ISParserTree& mainModule, Environment& e, Script::IPublicScriptSystem& ss, int32 param, IEventCallback<ScriptCompileArgs>& onCompile)
	{
		using namespace Sexy::Script;
		using namespace Sexy::Compiler;
		
		ScriptCompileArgs args{ ss };
		onCompile.OnEvent(args);

		// Find include directive
		for (int i = 0; i < mainModule.Root().NumberOfElements(); ++i)
		{
			cr_sex sincludeExpr = mainModule.Root()[i];
			if (IsCompound(sincludeExpr) && sincludeExpr.NumberOfElements() >= 3)
			{
				cr_sex squot = sincludeExpr[0];
				cr_sex stype = sincludeExpr[1];

				static fstring fs_quote = FstringFromPointer(L"'");
				static fstring fs_include = FstringFromPointer(L"#include");

				if (AreAtomicTokensEqual(squot, fs_quote) && AreAtomicTokensEqual(stype, fs_include))
				{
					for (int j = 2; j < sincludeExpr.NumberOfElements(); j++)
					{
						cr_sex sname = sincludeExpr[j];
						if (!IsStringLiteral(sname))
						{
							Throw(sname, L"expecting string literal in include directive (' #include \"<name1>\" \"<name2 etc>\" ...) ");
						}

						auto name = sname.String();

						auto includedModule = e.sourceCache.GetSource(name->Buffer);
						ss.AddTree(*includedModule);
					}

					break;
				}
			}
		}

		ss.AddTree(mainModule);
		ss.Compile();

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

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		switch (result)
		{
		case EXECUTERESULT_BREAKPOINT:
			Disassemble(vm, ss, e);
			Throw(0, L"Script hit breakpoint");
			break;
		case EXECUTERESULT_ILLEGAL:
			Disassemble(vm, ss, e);
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
			Disassemble(vm, ss, e);
			Throw(0, L"The script triggered a structured exception handler");
			break;
		case EXECUTERESULT_THROWN:
			Disassemble(vm, ss, e);
			Throw(0, L"The script triggered a virtual machine exception");
			break;
		case EXECUTERESULT_YIELDED:
			Disassemble(vm, ss, e);
			Throw(0, L"The script yielded");
			break;
		case EXECUTERESULT_TERMINATED:
			break;
		default:
			Throw(0, L"Unexpected EXECUTERESULT %d", result);
			break;
		}

		int exitCode = vm.PopInt32();
	}

	void RouteParseException(ParseException& ex, IDebuggerWindow& debugger)
	{
		auto a = ex.Start();
		auto b = ex.End();
		debugger.Log(L"%s\n%s\nSpecimen: %s\n(%d,%d) to (%d,%d)", ex.Name(), ex.Message(), ex.Specimen(), a.X, a.Y, b.X, b.Y);
	}

	struct ScriptLogger : ILog
	{
		IDebuggerWindow& debugger;
		ScriptLogger(IDebuggerWindow& _debugger) : debugger(_debugger) {}

		virtual void Write(csexstr text)
		{
			debugger.Log(L"%s", text);
		}

		virtual void OnUnhandledException(int errorCode, csexstr exceptionType, csexstr message, void* exceptionInstance)
		{

		}

		virtual void OnJITCompileException(Sex::ParseException& ex)
		{
			RouteParseException(ex, debugger);
		}
	};

	void DebuggerLoop(Sexy::Script::IPublicScriptSystem &ss, Environment& e)
	{
		struct : IDebugControl
		{
			IVirtualMachine* vm;
			Sexy::Script::IPublicScriptSystem* ss;
			Environment* e;

			void Update()
			{
				Disassemble(*vm, *ss, *e);
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
		} dc;

		dc.vm = &ss.PublicProgramObject().VirtualMachine();
		dc.ss = &ss;
		dc.e = &e;

		e.debuggerWindow.ShowWindow(true, &dc);

		MSG msg;
		while (GetMessage(&msg, 0, 0, 0) && e.debuggerWindow.IsVisible())
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	void ExecuteSexyScriptLoop(size_t maxBytes, Environment& e, const wchar_t* resourcePath, int32 param, int32 maxScriptSizeBytes, IEventCallback<ScriptCompileArgs>& onCompile)
	{
		ScriptLogger logger(e.debuggerWindow);

		Auto<ISourceCode> src; 
		Auto<ISParserTree> tree;
		
		while (true)
		{
			Script::CScriptSystemProxy ssp(ProgramInitParameters(maxBytes), logger);
			Script::IPublicScriptSystem& ss = ssp();
			if (&ss == nullptr)
			{
				auto id = ShowContinueBox(e.debuggerWindow.GetDebuggerWindowControl(), L"Failed to create script system");
				Throw(0, L"Failed to create script system");
			}

			try
			{
				ISParserTree* tree = e.sourceCache.GetSource(resourcePath);
				ExecuteSexyScript(*tree, e, ss, param, onCompile);
				return;
			}
			catch (ParseException& ex)
			{
				RouteParseException(ex, e.debuggerWindow);
				auto id = ShowContinueBox(e.debuggerWindow.GetDebuggerWindowControl(), ex.Message());
				if (id == CMD_ID_EXIT) Throw(ex.ErrorCode(), L"%s", ex.Message());
				else if (id == CMD_ID_IGNORE) return;
			}
			catch (IException& ex)
			{
				auto id = ShowContinueBox(e.debuggerWindow.GetDebuggerWindowControl(), ex.Message());
				if (id == CMD_ID_EXIT) Throw(ex.ErrorCode(), L"%s", ex.Message());
				else if (id == CMD_ID_IGNORE) return;
			}

			e.sourceCache.Release(resourcePath);

			DebuggerLoop(ss, e);
		}
	}
}