// This is the main DLL file.

#include "stdafx.h"
#include <string.h>
#include "sexy.strings.h"
#include <stdio.h>
#include "sexy.debug.types.h"
#include "sexy.compiler.public.h"
#include "sexy.script.h"
#include "sexy.s-parser.h"

#include "sexy.vm.h"
#include "sexy.vm.cpu.h"

#include "sexydotnethost.h"

namespace
{
	int GetAssemblyUnicodeAndUpdatePCOffset(const uint8* code, size_t& pcOffset, IDisassembler& disassembler, cstr symbol, Char* tempUnicodeBuffer256)
	{
		VM::IDisassembler::Rep rep;
		disassembler.Disassemble(code + pcOffset, OUT rep);

		char assemblyLine[256];

		if (symbol[0] == 0)
		{
			Rococo::SafeFormat(assemblyLine, 256, ("%p %s %s"), code + pcOffset, rep.OpcodeText, rep.ArgText);
		}
		else
		{
			Rococo::SafeFormat(assemblyLine, 256, ("%p %s %s // %s"), code + pcOffset, rep.OpcodeText, rep.ArgText, symbol);
		}

		if (rep.ByteCount == 0)
		{
			pcOffset += 1;
			// throw gcnew Exception("Error disassembling instruction");
		}

		int assemblyLength = CopycharToUnicode(tempUnicodeBuffer256, 256, assemblyLine);
		if (assemblyLength <= 0)
		{
			throw gcnew Exception("Error converting assembly to unicode");
		}

		pcOffset += rep.ByteCount;

		return assemblyLength;
	}

	int GetSectionHeader(Char* pCharLine, const IFunction& f)
	{
		CodeSection section;
		f.Code().GetCodeSection(OUT section);

		wchar_t unicodeName[256];
		int functionNameLength = CopycharToUnicode(unicodeName, 256, f.Name());
		if (functionNameLength <= 0)
		{
			throw gcnew Exception("Error converting function name to unicode");
		}

		int nCharsWritten = _snwprintf_s(pCharLine, 256, 256, L"%s(id #%lld)", unicodeName, section.Id);
		return nCharsWritten;
	}

	int GetModuleHeader(const IModule& module, Char* pCharLine)
	{
      wchar_t unicodeModuleName[256];
		int nameLength = CopycharToUnicode(unicodeModuleName, 256, module.Name());
		if (nameLength <= 0)
		{
			throw gcnew Exception("Error converting module name to unicode");
		}

		return _snwprintf_s(pCharLine, 256, 256, L"%s", unicodeModuleName);
	}

	void GetProgramCode(const IFunction& f, const IProgramMemory& pm, OUT size_t& start, OUT size_t& programLength, OUT const uint8*& code)
	{
		CodeSection section;
		f.Code().GetCodeSection(OUT section);

		start = pm.GetFunctionAddress(section.Id);
		programLength = pm.GetFunctionLength(section.Id);
		code = pm.StartOfMemory() + start;
	}
}

namespace SexyDotNet { namespace Host
{
	private ref class SSLTools
	{
	private:
		static void DisassembleFunction(const IFunction& f, IPublicProgramObject& po, Dictionary<IntPtr,SourceFileSegment>^ sourceFileSegments, SourceModule^ sourceModule, VM::IDisassembler& disassembler)
		{
			IntPtr functionHandle((Void*) &f);

			Char temp[256];
			CopycharToUnicode(temp, 256, f.Name());
			String^ functionName = gcnew String(temp);
			List<String^>^ assemblySegments = gcnew List<String^>();

			size_t start, functionLength;
			const uint8* code;
			GetProgramCode(f, po.ProgramMemory(), OUT start, OUT functionLength, OUT code);

			size_t i = 0;
			while(i < functionLength)
			{
				SymbolValue symbol = f.Code().GetSymbol(i);
				
				int lineNumber = assemblySegments->Count;
				sourceFileSegments->default[IntPtr((Void*)(i + code))] = SourceFileSegment(lineNumber, IntPtr((Void*)code), sourceModule, FunctionRef(functionHandle,functionName));
				int lineLength = GetAssemblyUnicodeAndUpdatePCOffset(IN code, REF i, IN disassembler, IN symbol.Text, OUT temp);									
				assemblySegments->Add(gcnew String(temp));					
			}	

			sourceModule->Sections->Add(gcnew AssemblySection(functionName, assemblySegments));
		}

	public:

		static bool DisassembleModule(SexyScriptLanguage^ scriptSystem, Dictionary<IntPtr,SourceFileSegment>^ sourceFileSegments, SourceModule^ sourceModule)
		{
			bool changed = false;

			IScriptSystem& ss = *ToSS(scriptSystem->NativeHandle);
			IPublicProgramObject& po = ss.PublicProgramObject();

			IModule& m = *(IModule*) sourceModule->NativeModuleHandle.ToPointer();

			if (sourceModule->bytecodeVersion != m.GetVersion())
			{
				sourceModule->bytecodeVersion = m.GetVersion();
				sourceModule->Sections->Clear();

				changed = true;

				for(int j = 0; j < m.FunctionCount(); ++j)
				{
					const IFunction& f = m.GetFunction(j);	
					
					DisassembleFunction(f, po, sourceFileSegments, sourceModule, *ToDisassembler(scriptSystem->DisassemblerHandle));
				}
			}

			return changed;
		}
	};

	const IFunction* GetCurrentFunction(IPublicProgramObject& po, size_t& programOffset, size_t& pcOffset);

	bool SexyScriptLanguage::UpdateDisassembly()
	{
		bool changed = false; 
		for each(SourceModule^ sourceModule in sourceModules->Values)
		{
			if (SSLTools::DisassembleModule(this, sourceFileSegments, sourceModule))
			{
				changed = true;
			}
		}

		IScriptSystem& ss = *ToSS(nativeHandle);
		IPublicProgramObject& po = ss.PublicProgramObject();

		size_t programOffset, pcOffset;
		const IFunction* f = GetCurrentFunction(po, OUT programOffset, OUT pcOffset);
		if (f != NULL)
		{
			const IModule& currentModule = f->Module();
			if (currentlyViewedModule.ToPointer() != &currentModule)
			{
				currentlyViewedModule = IntPtr((void*)&currentModule);

				Rococo::Sex::ISParserTree* tree = ss.GetSourceCode(currentModule);
				if (tree != NULL)
				{
					cstr src = tree->Source().SourceStart();
					currentViewedSourceCode = gcnew String(src);
					currentViewedFilename = gcnew String(tree->Source().Name());
					changed = true;
				}
			}

			CodeSection section;
			f->Code().GetCodeSection(section);

			size_t fnOffset = pcOffset - po.ProgramMemory().GetFunctionAddress(section.Id);
				
			const Rococo::Sex::ISExpression* s = (const Rococo::Sex::ISExpression*) f->Code().GetSymbol(fnOffset).SourceExpression;
			if (s != NULL)
			{
				if (currentlyViewedExpression.ToPointer() != s)
				{
					currentlyViewedExpression = IntPtr((void*) s);
				
					const Vec2i& start = s->Start();
					const Vec2i& end = s->End();

					this->start = SourceLocation(start.x, start.y);
					this->end = SourceLocation(end.x, end.y);

					changed = true;
				}
			}
		}

		return changed;
	}

	void SexyScriptLanguage::Disassemble()
	{
		sourceFileSegments = gcnew Dictionary<IntPtr,SourceFileSegment>();
		
		for each(SourceModule^ sourceModule in sourceModules->Values)
		{
			SSLTools::DisassembleModule(this, sourceFileSegments, sourceModule);					
		}
	}
}} // SexyDotNet::Host