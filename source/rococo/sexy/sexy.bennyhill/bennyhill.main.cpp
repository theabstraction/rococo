/*
	Sexy Scripting Language - Copright(c)2013. Mark Anthony Taylor. All rights reserved.

	https://github.com/theabstraction/rococo

	Email: mark.anthony.taylor@gmail.com

	The Sexy copyright holder is Mark Anthony Taylor, the English author of 'Lazy Bloke Ghosts of Parliament', 'Lazy Bloke in the 35th Century', 'King of the Republic', 'Origin of the Species' 
	

	1. This software is open-source. It can be freely copied, distributed, compiled and the compilation executed.
	
	1.a Modification of the software is not allowed where such modifcations fail to include this exact comment header in each source file, or alter, reduce or extend the Sexy language.
	The purpose of this limitation is to prevent different incompatible versions of the Sexy language emerging. 

	1.b You are allowed to fix bugs and implement performance improvements providing you inform Sexy's copyright owner via email. Such changes may then be incorporated in
	later versions of Sexy without compromising Sexy's copyright license.
	
	2. You are not permitted to copyright derivative versions of the source code. You are free to compile the code into binary libraries and include the binaries in a commercial application. 

	3. THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM “AS IS” WITHOUT
	WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY
	AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

	4. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MODIFIES AND/OR CONVEYS THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR
	DAMAGES, INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING RENDERED
	INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF
	SUCH DAMAGES.

	5. Any program that distributes this software in source or binary form must include an obvious credit to the the language, its copyright holder, and the language homepage. This must be the case in its
	principal credit screen and its principal readme file.
*/

// bennyhill.cpp : Defines the entry point for the console application.
#include "bennyhill.stdafx.h"

#include <stdarg.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <rococo.api.h>
#include <rococo.io.h>
#include <rococo.os.h>
#include <unordered_set>

namespace Rococo
{ 
	// Adds pragma once only once per header
	void AddPragmaOnce(FileAppender& appender, cstr headerFileName)
	{
		static std::unordered_set<std::string> oncedThings;
		if (oncedThings.emplace(headerFileName).second)
		{
			appender.Append("%s", "#pragma once\n\n");
		}
	}

   void ConvertAndAppendCppType(FileAppender& appender, cstr cppType)
   {
      NamespaceSplitter splitter(cppType);

      cstr nsRoot, tail;
      if (splitter.SplitHead(nsRoot, tail))
      {
         appender.Append(("%s::"), nsRoot);
         return ConvertAndAppendCppType(appender, tail);
      }
      else
      {
         appender.Append(("%s"), cppType);
      }
   }

   void AppendCppType(FileAppender& appender, cr_sex field, cstr sxhfieldtype, const ParseContext& pc)
   {
      cstr cpptypeDef = NULL;
      auto i = pc.primitives.find(sxhfieldtype);
      if (i != pc.primitives.end()) cpptypeDef = i->second.cppType.c_str();
      else
      {
         auto j = pc.structs.find(sxhfieldtype);
         if (j != pc.structs.end()) cpptypeDef = j->second.cppType.c_str();
         else
         {
            auto k = pc.interfaces.find(sxhfieldtype);
            if (k != pc.interfaces.end())
            {
               cpptypeDef = k->second->ic.asCppInterface.FQName();
            }
         }
      }

      if (cpptypeDef == NULL) Throw(field, "Cannot resolve the type. Neither a known struct or primitive");

      ConvertAndAppendCppType(appender, cpptypeDef);
   }

   bool AreEqual(cstr s, const sexstring& t)
   {
      return AreEqual(s, t->Buffer);
   }

	Rococo::cstr StringFrom(Rococo::Sex::cr_sex s)
	{
		if (!IsAtomic(s) && !IsStringLiteral(s)) Throw(s, "Expecting atomic or string literal");
		return s.String()->Buffer;
	}

	Rococo::cstr StringFrom(Rococo::Sex::cr_sex command, int elementIndex)
	{
		if (elementIndex >= command.NumberOfElements()) Throw(command, "Insufficient elements in expression");
		return StringFrom(command.GetElement(elementIndex));
	}

	void GetFQCppStructName(char* compressedStructName, char* cppStructName, size_t capacity, cstr fqStructName)
	{
		char* p = compressedStructName;
		char* q = cppStructName;
		size_t pos = 0;
		size_t safeZone = capacity - 2;
		const char* pSrc = fqStructName;
		while (true)
		{
			char src = *pSrc++;
			if (src == 0)
			{
				break;
			}

			if (pos >= safeZone)
			{
				Throw(0, "Overflow formatting struct name: %.64s", fqStructName);
			}

			if (src == '.')
			{
				*q++ = ':';
				*q++ = ':';
				pos += 2;
			}
			else
			{
				*q++ = src;
				*p++ = src;
				pos++;
			}
		}

		*p = 0;
		*q = 0;
	}

   void CopyCharTochar(char* dest, const char* src, size_t capacity)
   {
      for (size_t i = 0; i < capacity; i++)
      {
         dest[i] = src[i];
         dest[i] &= 0x00FF;
      }

      dest[capacity - 1] = 0;
   }
} // Sexy

#include "bennyhill.validators.inl"
#include "bennyhill.appender.sexy.inl"
#include "bennyhill.appender.cpp.inl"
#include "bennyhill.config.inl"

using namespace Rococo;
using namespace Rococo::Sex;

namespace Rococo
{
	void AddPragmaOnce(FileAppender& appender, cstr headerFileName);
}

void GenerateFiles(const ParseContext& pc, const InterfaceContext& ic, cr_sex s, const ISExpression* methods[], cr_sex interfaceDef)
{
	FileAppender sexyFileAppender(ic.appendSexyFile);	

	auto* mostDerivedMethods = methods; 
	while (*mostDerivedMethods != nullptr)
	{
		mostDerivedMethods++;
	}
	DeclareSexyInterface(sexyFileAppender, ic, mostDerivedMethods[-1], pc);

	if (ic.nceContext.SexyName()[0] != 0)
	{
		ImplementSexyInterface(sexyFileAppender, ic, methods, s, pc);
	}

	FileAppender cppFileAppender(ic.appendCppHeaderFile);
	AddPragmaOnce(cppFileAppender, ic.appendCppHeaderFile);
	DeclareCppInterface(cppFileAppender, ic, interfaceDef, mostDerivedMethods[-1], pc);

	if (ic.nceContext.SexyName()[0] != 0)
	{
		FileAppender cppFileImplAppender(ic.appendCppImplFile);
		ImplementNativeFunctions(cppFileImplAppender, ic, methods, pc);
	}
}

void GenerateFiles(const ParseContext& pc, const EnumContext& ec, cr_sex senumDef)
{
	FileAppender cppFileImplAppender(ec.appendCppImplFile);
	ImplementNativeFunctions(cppFileImplAppender, ec, pc);
}

void GetFileSpec(char filename[_MAX_PATH], cstr root, cstr scriptName, cstr extension)
{
   SafeFormat(filename, _MAX_PATH, "%s%s%s", root, scriptName, extension);
}

struct FunctionDesc
{
	std::string name;
	const Rococo::Sex::ISExpression* functionDef;
};

typedef std::unordered_map<std::string, std::vector<FunctionDesc>> TMapNSToFunctionDefs;

void AppendNativeFunction(cr_sex functionDef, const ParseContext& pc, FileAppender& outputFile, TMapNSToFunctionDefs& defs)
{
	cr_sex sfname = functionDef[0];
	if (!IsAtomic(sfname))
	{
		Throw(sfname, "Expecting function name atomic argument");
	}

	NamespaceSplitter splitter(sfname.String()->Buffer);

	cstr ns, fname;
	if (!splitter.SplitTail(ns, fname))
	{
		Throw(sfname, "Expecting function name to be prefixed with a namespace");
	}

	CppType nsType;
	nsType.Set(ns);

	FunctionDesc desc{ fname, &functionDef };

	auto i = defs.find(ns);
	if (i == defs.end())
	{
		std::vector<FunctionDesc> functions = { desc };
		defs[ns] = functions;
	}
	else
	{
		i->second.push_back(desc);
	}

	outputFile.Append("\tvoid Native%s%s(NativeCallEnvironment& _nce)\n", nsType.CompressedName(), fname);
	outputFile.Append("\t{\n");

	outputFile.Append("\t\tRococo::uint8* _sf = _nce.cpu.SF();\n");
	outputFile.Append("\t\tptrdiff_t _offset = 2 * sizeof(size_t);\n");

	bool hasInitializedStringStruct = false;

	TAttributeMap attributes;

	// Write inputs
	int outputStart = GetOutputPosition(functionDef);
	AddNativeInputs(attributes, outputFile, functionDef, 1, outputStart - 2, pc);

	if (functionDef.NumberOfElements() < outputStart + 3 && functionDef.NumberOfElements() != outputStart + 2)
	{
		Throw(functionDef, "Expecting two or more elements after the output. The first : and the second the function name to be called in C++");
	}

	int j = outputStart;
	while (j < functionDef.NumberOfElements())
	{
		cr_sex bodyIndicator = functionDef[j];

		if (IsAtomic(bodyIndicator) && AreEqual(bodyIndicator.String(), ":"))
		{
			break;
		}

		j++;
	}

	if (j == functionDef.NumberOfElements())
	{
		Throw(functionDef, "Expecting ':' body indicator followed by the C++ function name. Check spaces between tokens.");
	}

	cr_sex sCppFunction = functionDef[functionDef.NumberOfElements() - 1];
	if (!IsAtomic(sCppFunction) && !IsStringLiteral(sCppFunction))
	{
		Throw(sCppFunction, "Expecting function name, atomic or string literal");
	}

	sexstring cppFunction = sCppFunction.String();

	if (j > outputStart)
	{
		cr_sex outputDef = functionDef[outputStart];
		cr_sex stype = outputDef.GetElement(0);
		cr_sex svalue = outputDef.GetElement(1);

		cstr type = StringFrom(stype);

		TTypeMap::const_iterator k = pc.primitives.find(type);
		if (k == pc.primitives.end()) Throw(stype, "Could not find type amongst the primitives");

		char cppName[256];
		char compressedName[256];
		GetFQCppStructName(compressedName, cppName, 256, k->second.cppType.c_str());

		int nOutputs = j - outputStart;

		if (nOutputs < 2)
		{
			outputFile.Append(("\t\t%s %s = "), cppName, StringFrom(svalue));
		}
		else
		{
			outputFile.Append("\t\tauto [");
			for (int l = 0; l < nOutputs; l++)
			{
				if (l > 0)
				{
					outputFile.Append(", ", l);
				}

				outputFile.Append("output_%d", l);
			}
			outputFile.Append("] = ");
		}
	}
	else
	{
		outputFile.Append("\t\t");
	}

	outputFile.Append(("%s("), cppFunction->Buffer);

	int inputCount = 1;

	// Append the input arguments to the method invocation
	for (int k = 1; k < outputStart - 1; ++k)
	{
		cr_sex s = functionDef[k];

		const ISExpression* stype = &s[0];
		const ISExpression* svalue = &s[1];
		cstr type = StringFrom(*stype);

		if (Eq(type, "const"))
		{
			stype = &s[1];
			type = StringFrom(*stype);
			svalue = &s[2];
		}

		if (!AreEqual(type, "#"))
		{
			if (inputCount > 1) outputFile.Append(", ");
			inputCount++;

			if (AreEqual(type, "IStringBuilder") || AreEqual(type, "Sys.Text.IStringBuilder"))
			{
				outputFile.Append("_%sPopulator", StringFrom(*svalue));
			}
			else
			{
				if (!AreEqual(type, "IString") && !AreEqual(type, "Sys.Type.IString") && pc.structs.find(type) != pc.structs.end())
				{
					outputFile.Append('*');
				}

				outputFile.Append("%s", StringFrom(*svalue));
			}
		}
		else
		{
			// Skip input arguments that are attributes rather than type-name pairs.
		}
	}

	outputFile.Append(");\n");

	if (j > outputStart)
	{
		int nOutputs = j - outputStart;

		if (nOutputs == 1)
		{
			cr_sex outputDef = functionDef[outputStart];
			cr_sex stype = outputDef.GetElement(0);
			cr_sex svalue = outputDef.GetElement(1);

			outputFile.Append("\t\t_offset += sizeof(%s);\n", StringFrom(svalue));
			outputFile.Append("\t\tWriteOutput(%s, _sf, -_offset);\n", StringFrom(svalue));
		}
		else
		{
			for (int k = 0; k < nOutputs; ++k)
			{
				cr_sex outputDef = functionDef[outputStart + k];
				cr_sex stype = outputDef.GetElement(0);
				cr_sex svalue = outputDef.GetElement(1);

				TTypeMap::const_iterator l = pc.primitives.find(StringFrom(stype));
				if (l == pc.primitives.end()) Throw(stype, "Could not find type amongst the primitives");

				char cppName[256];
				char compressedName[256];
				GetFQCppStructName(compressedName, cppName, 256, l->second.cppType.c_str());

				outputFile.Append("\t\t_offset += sizeof(%s);\n", cppName);
				outputFile.Append("\t\tWriteOutput(output_%d, _sf, -_offset);\n", k);
			}
		}
	}

	outputFile.Append("\t}\n\n");
}

void AppendNativeRegistration(const FunctionDesc& desc, const CppType& ns, const ParseContext& pc, FileAppender& outputFile)
{
	cr_sex sfname = desc.functionDef->GetElement(0);
	sexstring fname = sfname.String();
	CppType nameType;
	nameType.Set(fname->Buffer);
	outputFile.Append(("ss.AddNativeCall(ns, Native%s, nullptr, (\"%s"), nameType.CompressedName(), desc.name.c_str());

	int outputPos = GetOutputPosition(*desc.functionDef);
	for (int i = 1; i < outputPos - 1; i++)
	{
		cr_sex arg = desc.functionDef->GetElement(i);
		AppendInputPair(outputFile, arg, pc);
	}

	outputFile.Append(" -> ");
	
	cr_sex outputDef = desc.functionDef->GetElement(outputPos);

	if (IsCompound(outputDef))
	{
		AppendOutputPair(outputFile, outputDef, pc);
	}

	outputFile.Append("\"), __FILE__, __LINE__);");
}

void ParseFunctions(cr_sex functionSetDef, const ParseContext& pc)
{
	if (functionSetDef.NumberOfElements() < 2)
	{
		Throw(functionSetDef, "Expecting (functions <file-prefix> ...)");
	}

	cr_sex sFilePrefix = functionSetDef[1];
	if (!IsStringLiteral(sFilePrefix))
	{
		Throw(sFilePrefix, "Expecting string literal for the file prefix");
	}

	sexstring filePrefix = sFilePrefix.String();

	char sexyFile[_MAX_PATH];
	SafeFormat(sexyFile, "%s%s.inl", pc.cppRootDirectory, filePrefix->Buffer);

	char sexyHeaderFile[_MAX_PATH];
	SafeFormat(sexyHeaderFile, "%s%s.h", pc.cppRootDirectory, filePrefix->Buffer);
	FileAppender cppHeaderFileAppender(sexyHeaderFile);

	FileAppender sexyAppender(sexyFile);

	AddPragmaOnce(cppHeaderFileAppender, sexyHeaderFile);

	sexyAppender.Append(("namespace\n{\n\tusing namespace Rococo;\n\tusing namespace Rococo::Sex;\n\tusing namespace Rococo::Script;\n\tusing namespace Rococo::Compiler;\n\n"));

	TMapNSToFunctionDefs nsMap;

	for (int i = 2; i < functionSetDef.NumberOfElements(); ++i)
	{
		cr_sex functionDef = functionSetDef[i];
		if (!IsCompound(functionDef))
		{
			Throw(functionDef, "Expecting function def of form (<sexy-function-name> (input1)...(inputN)->(output): <cpp-function-name>)");
		}

		AppendNativeFunction(functionDef, pc, sexyAppender, nsMap);
	}

	sexyAppender.Append("}\n\n");

	for (auto n : nsMap)
	{
		cstr ns = n.first.c_str();
		char* nsBuffer = (char*)alloca(sizeof(char)* (StringLength(ns) + 1));
		CopyString(nsBuffer, StringLength(ns) + 1, ns);

		char* token = nullptr;
		char* context = nullptr;

#ifndef char_IS_WIDE
# define Tokenize strtok_s
#else
# define Tokenize wcstok_s
#endif
		token = Tokenize(nsBuffer, ".", &context);

		int namespaceDepth = 0;

		sexyAppender.Append("namespace ");
		cppHeaderFileAppender.Append("namespace ");

		while (token)
		{
			namespaceDepth++;
			sexyAppender.Append("%s", token);
			cppHeaderFileAppender.Append("%s", token);
			token = Tokenize(nullptr, ".", &context);
			if (token)
			{
				sexyAppender.Append("::");
				cppHeaderFileAppender.Append("::");
			}
		}

		sexyAppender.Append("\n{\n");
		cppHeaderFileAppender.Append("\n{\n");

		CppType nsType;
		nsType.Set(ns);

		cppHeaderFileAppender.Append("\tvoid AddNativeCalls_%s(Rococo::Script::IPublicScriptSystem& ss);\n", nsType.CompressedName());

		sexyAppender.Append("\n\tvoid AddNativeCalls_%s(Rococo::Script::IPublicScriptSystem& ss)\n", nsType.CompressedName());
		sexyAppender.Append("\t{\n");

		sexyAppender.Append(("\t\tconst INamespace& ns = ss.AddNativeNamespace(\"%s\");\n"), nsType.SexyName());

		for(auto functionDesc: n.second)
		{
			sexyAppender.Append("\t\t");
			AppendNativeRegistration(functionDesc, nsType, pc, sexyAppender);
			sexyAppender.Append("\n");
		}

		sexyAppender.Append("\t}\n}\n\n");
		cppHeaderFileAppender.Append("}\n\n");
	}
}

void ParseEnum(cr_sex senumDef, ParseContext& pc)
{
	EnumDef def;
	def.sdef = &senumDef;
	EnumContext& ec = def.ec;

	if (senumDef.NumberOfElements() < 4)
	{
		Throw(senumDef, "Expecting at least 4 elements in enumeration definition");
	}

	cr_sex sbasicType = senumDef[1];
	if (!IsAtomic(sbasicType))
	{
		Throw(sbasicType, "Expecting underlying type, such as Int32 or Int64");
	}

	auto foundType = pc.primitives.find(sbasicType.String()->Buffer);
	if (foundType == pc.primitives.end())
	{
		Throw(sbasicType, "Cannot find primitive type in the config file");
	}

	ec.underlyingType.Set(foundType->second.cppType.c_str());

	for (int i = 2; i < senumDef.NumberOfElements(); ++i)
	{
		cr_sex sdirective = senumDef.GetElement(i);
		if (!IsCompound(sdirective)) Throw(sdirective, "Expecting compound expression in the enum definition");

		cr_sex scmd = sdirective[0];

		if (IsCompound(scmd))
		{
			for (int j = 0; j < sdirective.NumberOfElements(); j++)
			{
				cr_sex sdef = sdirective[j];
				int nElements = sdef.NumberOfElements();
				auto type = sdef.Type();
				if (nElements != 2 || !IsAtomic(sdef[0]) || !IsAtomic(sdef[1]))
				{
					Throw(sdef, "Expecting compound expression with 2 elements (<enum_name> <enum_value>)");
				}

				cr_sex sName = sdef[0];
				cr_sex sValue = sdef[1];

				if (!IsCapital(sName.String()->Buffer[0]))
				{
					Throw(sName, "Expecting a capital letter in the first position");
				}

				for (cstr p = sName.String()->Buffer; *p != 0; p++)
				{
					if (!IsAlphaNumeric(*p))
					{
						Throw(sName, "Expecting alphanumerics throughout the name definition");
					}
				}

				VariantValue value;
				auto result = Rococo::Parse::TryParse(value, VARTYPE_Int64, sValue.String()->Buffer);
				if (result != Rococo::Parse::PARSERESULT_GOOD)
				{
					Throw(sValue, "Expecting int64 numeric for value");
				}

				ec.values.push_back(std::make_pair(sName.String()->Buffer, value.int64Value));
			}
		}
		else if (scmd == "as.both")
		{
			if (sdirective.NumberOfElements() != 3) Throw(sdirective, "Expecting (as.both <fully qualified enum name> \"<target-filename-prefix>\")");
			cr_sex sinterfaceName = sdirective.GetElement(1);
			ValidateFQSexyInterface(sinterfaceName); // sexy namespaces are more stringent than those of C++. so we use sexy validation for both

			if (ec.asSexyEnum[0] != 0) Throw(sdirective, "as.sxy is already defined for this interface");
			if (ec.asCppEnum.SexyName()[0] != 0) Throw(sdirective, "as.cpp is already defined for this interface");

			cr_sex ssexyFilename = sdirective.GetElement(2);
			if (!IsStringLiteral(ssexyFilename)) Throw(ssexyFilename, "Expecting string literal target-prefix. Filenames can potentially have spaces in them.");

			cstr sexyFilename = ssexyFilename.String()->Buffer;

			SafeFormat(ec.appendCppHeaderFile, "%s%s.sxh.h", pc.cppRootDirectory, sexyFilename);
			SafeFormat(ec.appendCppImplFile, "%s%s.sxh.inl", pc.cppRootDirectory, sexyFilename);

			ec.asCppEnum.Set(sinterfaceName.String()->Buffer);
			CopyString(ec.asSexyEnum, InterfaceContext::MAX_TOKEN_LEN, sinterfaceName.String()->Buffer);

			SafeFormat(ec.appendSexyFile, "%s%s_sxh.sxy", pc.cppRootDirectory, ssexyFilename.String()->Buffer);

			OS::ToSysPath(ec.appendCppHeaderFile);
			OS::ToSysPath(ec.appendCppImplFile);
			OS::ToSysPath(ec.appendSexyFile);
		}
		else if (scmd == "as.sxy")
		{
			if (sdirective.NumberOfElements() != 3) Throw(sdirective, "Expecting (as.sxy <fully qualified enum name> \"<target-filename>\")");
			cr_sex sinterfaceName = sdirective.GetElement(1);
			ValidateFQSexyInterface(sinterfaceName);

			if (ec.asSexyEnum[0] != 0) Throw(sdirective, "as.sxy is already defined for this enum");

			cr_sex ssexyFilename = sdirective.GetElement(2);
			if (!IsStringLiteral(ssexyFilename)) Throw(ssexyFilename, "Expecting string literal");
			CopyString(ec.asSexyEnum, InterfaceContext::MAX_TOKEN_LEN, sinterfaceName.String()->Buffer);

			SafeFormat(ec.appendSexyFile, "%s%s_sxh.sxy", pc.cppRootDirectory, ssexyFilename.String()->Buffer);
			OS::ToSysPath(ec.appendSexyFile);
		}
		else if (scmd == "as.cpp")
		{
			if (sdirective.NumberOfElements() != 3) Throw(sdirective, "Expecting (as.cpp <fully qualified enum name> \"<target-filename-sans-extension>\")");
			cr_sex sstructName = sdirective.GetElement(1);
			ValidateFQCppStruct(sstructName);

			if (ec.asCppEnum.SexyName()[0] != 0) Throw(sdirective, "as.cpp is already defined for this enum");

			cr_sex ssexyFilename = sdirective.GetElement(2);
			if (!IsStringLiteral(ssexyFilename)) Throw(ssexyFilename, "Expecting string literal");

			cstr sexyFilename = ssexyFilename.String()->Buffer;

			SafeFormat(ec.appendCppHeaderFile, "%s%s.sxh.h", pc.cppRootDirectory, sexyFilename);
			SafeFormat(ec.appendCppImplFile, "%s%s.sxh.inl", pc.cppRootDirectory, sexyFilename);

			OS::ToSysPath(ec.appendCppHeaderFile);
			OS::ToSysPath(ec.appendCppImplFile);

			ec.asCppEnum.Set(sstructName.String()->Buffer);
		}
		else
		{
			Throw(scmd, "Expecting one of {as.both, as.sxy, as.cpp} or a compound expression giving a set of enumeration name value pairs");
		}
	}

	if (ec.asCppEnum.SexyName()[0] == 0)
	{
		Throw(senumDef, "Missing as.cpp or as.both");
	}

	if (ec.asSexyEnum[0] == 0)
	{
		Throw(senumDef, "Missing as.sxy or as.both");
	}

	if (ec.appendSexyFile[0] == 0)
	{
		SafeFormat(ec.appendSexyFile, "%s_sxh.sxy", pc.scriptInputSansExtension);
	}

	if (ec.appendCppHeaderFile[0] == 0)
	{
		SafeFormat(ec.appendCppHeaderFile, "%s%s.sxh.h", pc.cppRootDirectory, pc.scriptName);
		SafeFormat(ec.appendCppImplFile, "%s%s.sxh.cpp", pc.cppRootDirectory, pc.scriptName);
	}

	pc.enums.push_back(def);
}

void ParseInterface(cr_sex interfaceDef, ParseContext& pc, std::vector<rstdstring, Memory::SexyAllocator<rstdstring>>& defOrder)
{
	InterfaceContext ic;

	bool hasDestructor = false;

	std::vector<const ISExpression*> methods;

	for(int i = 1; i < interfaceDef.NumberOfElements(); ++i)
	{
		cr_sex directive = interfaceDef.GetElement(i);
		if (!IsCompound(directive)) Throw(directive, "Expecting compound expression in the interface definition");

		cr_sex directiveName = directive.GetElement(0);
		if (!IsAtomic(directiveName))  Throw(directiveName, "Expecting atomic expression in the directive name at position #0");

		const sexstring ssname = directiveName.String();

		if (AreEqual(ssname, "as.both"))
		{
			if (directive.NumberOfElements() != 3) Throw(directive, "Expecting (as.both <fully qualified interface name> \"<target-filename-prefix>\")");
			cr_sex sinterfaceName = directive.GetElement(1);
			ValidateFQSexyInterface(sinterfaceName); // sexy namespaces are more stringent than those of C++. so we use sexy validation for both

			if (ic.asSexyInterface[0] != 0) Throw(directive, "as.sxy is already defined for this interface");
			if (ic.asCppInterface.SexyName()[0] != 0) Throw(directive, "as.cpp is already defined for this interface");

			cr_sex ssexyFilename = directive.GetElement(2);
			if (!IsStringLiteral(ssexyFilename)) Throw(ssexyFilename, "Expecting string literal target-prefix. Filenames can potentially have spaces in them.");

			cstr sexyFilename = ssexyFilename.String()->Buffer;

			SafeFormat(ic.appendCppHeaderFile, "%s%s.sxh.h", pc.cppRootDirectory, sexyFilename);
			SafeFormat(ic.appendCppImplFile, "%s%s.sxh.inl", pc.cppRootDirectory, sexyFilename);

			ic.asCppInterface.Set(sinterfaceName.String()->Buffer);
			CopyString(ic.asSexyInterface, InterfaceContext::MAX_TOKEN_LEN, sinterfaceName.String()->Buffer);
			SafeFormat(ic.appendSexyFile, "%s%s_sxh.sxy", pc.cppRootDirectory, ssexyFilename.String()->Buffer);
			OS::ToSysPath(ic.appendSexyFile);
			OS::ToSysPath(ic.appendCppHeaderFile);
			OS::ToSysPath(ic.appendCppImplFile);
		}
		else if (AreEqual(ssname, "as.sxy"))
		{
			if (directive.NumberOfElements() < 3 || directive.NumberOfElements() > 4) Throw(directive, "Expecting (as.sxy <fully qualified interface name> \"<target-filename>\" [inheritance string])");
			cr_sex sinterfaceName = directive.GetElement(1);
			ValidateFQSexyInterface(sinterfaceName);

			if (ic.asSexyInterface[0] != 0) Throw(directive, "as.sxy is already defined for this interface");

			if (directive.NumberOfElements() == 4)
			{
				if (!IsStringLiteral(directive[3]) && !IsAtomic(directive[3])) Throw(directive[3], "Expecting string literal base class");
				ic.sexyBase = directive[3].String()->Buffer;
				ValidateFQSexyInterface(directive[3]);

				auto inter = pc.interfaces.find(ic.sexyBase);
				if (inter == pc.interfaces.end())
				{
					Throw(directive[3], "Base interface [%s] not found prior to the definition of the derived interface [%s]", ic.sexyBase, sinterfaceName.String()->Buffer);
				}

				for (auto* m : inter->second->methodArray)
				{
					if (m != nullptr) methods.push_back(m);
				}
			}

			cr_sex ssexyFilename = directive.GetElement(2);
			if (!IsStringLiteral(ssexyFilename) && !IsAtomic(ssexyFilename)) Throw(ssexyFilename, "Expecting string literal");
			CopyString(ic.asSexyInterface, InterfaceContext::MAX_TOKEN_LEN, sinterfaceName.String()->Buffer);
			SafeFormat(ic.appendSexyFile, "%s%s_sxh.sxy", pc.contentRoot, ssexyFilename.String()->Buffer);
			OS::ToSysPath(ic.appendSexyFile);
		}
		else if (AreEqual(ssname, "as.cpp"))
		{
			if (directive.NumberOfElements() != 3 && directive.NumberOfElements() != 4) Throw(directive, "Expecting (as.cpp <fully qualified interface name> \"<target-filename-sans-extension>\" <optional-base-class>)");
			cr_sex sstructName = directive[1];
			ValidateFQCppStruct(sstructName);

			if (ic.asCppInterface.SexyName()[0] != 0) Throw(directive, "as.cpp is already defined for this interface");

			cr_sex ssexyFilename = directive[2];
			if (!IsStringLiteral(ssexyFilename)) Throw(ssexyFilename, "Expecting string literal");

			cstr sexyFilename = ssexyFilename.String()->Buffer;

			SafeFormat(ic.appendCppHeaderFile, "%s%s.sxh.h", pc.cppRootDirectory, sexyFilename);
			SafeFormat(ic.appendCppImplFile, "%s%s.sxh.inl", pc.cppRootDirectory, sexyFilename);

			if (directive.NumberOfElements() == 4)
			{
				cr_sex sbaseClass = directive[3];
				if (!IsStringLiteral(sbaseClass) && !IsAtomic(sbaseClass)) Throw(sbaseClass, "Expecting string literal or atomic base class");
				ic.cppBase = sbaseClass.String()->Buffer;
			}

			ic.asCppInterface.Set(sstructName.String()->Buffer);
		}
		else if (AreEqual(ssname, "methods"))
		{
			methods.push_back(&directive);
		}
		else if (AreEqual(ssname, "~"))
		{
			if (ic.hasDestructor) { Throw(directive, "Interface already has destructor defined");	}
			ic.hasDestructor = true;
			if (directive.NumberOfElements() != 1) Throw(directive, "Destructors take no arguments");
		}
		else if (AreEqual(ssname, "factory"))
		{
			cr_sex sfactory = directive;
			if (sfactory.NumberOfElements() < 2 || !IsAtomic(sfactory.GetElement(1)))
			{
				Throw(sfactory, "Expecting (factory <factory-name> (optional input arg1) ... (optional input argN)");
			}
			ic.factories.push_back(&sfactory);
		}
		else if (AreEqual(ssname, "context"))
		{
			if (directive.NumberOfElements() != 3)
			{
				Throw(directive, "Expecting (context <'factory'|'api'> <context-type>). E.g (context factory Sys.Animal)");
			}

			cr_sex sroute = directive.GetElement(1);
			cr_sex stype = directive.GetElement(2);
			if (!IsAtomic(sroute)) Throw(stype, "Expecting atomic value for <'factory'|'api'>");

			cstr route = sroute.String()->Buffer;
			if (AreEqual(route, "factory"))
			{
				ic.isSingleton = false;
			}
			else if (AreEqual(route, "api"))
			{
				ic.isSingleton = true;
			}
			else
			{
				Throw(sroute, "Expecting either 'factory' or 'api' sans quotes. 'factory' is used when the factory call determines the underlying C++ object. 'api' is used when the native registration functions determine the underlying C++ object.");
			}

			if (!IsAtomic(stype)) Throw(stype, "Expecting atomic value for <context-type>");
			ic.nceContext.Set(stype.String()->Buffer);
		}
		else
		{
			Throw(directiveName, "Unknown directive in interface definition. Expecting one of (as.sxy, as.cpp, as.both, context, factory, methods)");
		}		
	}

	if (ic.asCppInterface.SexyName()[0] == 0)
	{
		Throw(interfaceDef, "Missing as.cpp or as.both");
	}

	if (ic.asSexyInterface[0] == 0)
	{
		Throw(interfaceDef, "Missing as.sxy or as.both");
	}

	if (ic.appendSexyFile[0] == 0)
	{
		SafeFormat(ic.appendSexyFile, "%s_sxh.sxy", pc.scriptInputSansExtension);
	}

	if (ic.appendCppHeaderFile[0] == 0) 
	{
      SafeFormat(ic.appendCppHeaderFile, "%s%s.sxh.h", pc.cppRootDirectory, pc.scriptName);
      SafeFormat(ic.appendCppImplFile, "%s%s.sxh.cpp", pc.cppRootDirectory, pc.scriptName);
	}

	if (ic.factories.empty())
	{
		// Disable exception. Interfaces can be grabbed from the output of functions that return them
      // Throw(interfaceDef, "Interface needs to specify at least one factory");
	}

	if (!ic.nceContext.SexyName()[0] && !ic.factories.empty())
	{
		Throw(interfaceDef, "Missing (context <'factory'|'api'> <context-type>)");
	}

   InterfaceDef* def = new InterfaceDef;

   for (auto* m : methods)
   {
	   def->methodArray.push_back(m);
   }

   def->sdef = &interfaceDef;
   def->methodArray.push_back(nullptr);
   def->ic = ic;

   defOrder.push_back(ic.asSexyInterface);

   pc.interfaces.insert(std::make_pair(stdstring(ic.asSexyInterface), def));
	/*
	try
	{
		GenerateFiles(pc, ic, interfaceDef, methods, interfaceDef);
	}
	catch(OS::OSException&)
	{
		WriteToStandardOutput(("Error in interface defintion %s: %d.%d to %d.%d\n"), pc.scriptInput, interfaceDef.Start().x, interfaceDef.Start().y, interfaceDef.End().x, interfaceDef.End().y);
		throw;
	}
   */
}

void ParseInterfaceFile(cr_sex root, ParseContext& pc)
{
	bool hasConfig = false;
	bool hasFunctions = false;

	std::vector<rstdstring, Memory::SexyAllocator<rstdstring>> interfaceDefOrder;

	for (int i = 0; i < root.NumberOfElements(); ++i)
	{
		cr_sex topLevelItem = root.GetElement(i);

		if (!IsCompound(topLevelItem)) Throw(topLevelItem, "Expecting compound expression in the root expression");
		cr_sex command = topLevelItem.GetElement(0);
		if (!IsAtomic(command))  Throw(command, "Expecting atomic expression in the command at position #0");

		sexstring cmd = command.String();

		if (AreEqual("config", cmd))
		{
			if (hasConfig) Throw(command, "Only one config entry permitted");
			ParseConfigSpec(topLevelItem, pc);
			hasConfig = true;
		}
		else if (AreEqual("functions", cmd))
		{
			//if (hasFunctions) Throw(command, "Only one set of functions can be defined in the generator file");
			if (!hasConfig) Throw(command, "Must define a (config <config-path>) entry before all functions");
			ParseFunctions(topLevelItem, pc);
			hasFunctions = true;
		}
		else if (AreEqual("interface", cmd))
		{
			if (!hasConfig) Throw(command, "Must define a (config <config-path>) entry before all interfaces");
			ParseInterface(topLevelItem, pc, interfaceDefOrder);
		}
		else if (AreEqual("enum", cmd))
		{
			if (!hasConfig) Throw(command, "Must define a (config <config-path>) entry before all enumerations");
			ParseEnum(topLevelItem, pc);
		}
		else
		{
			Throw(command, "Expecting 'interface or config or functions' in the command at position #0");
		}
	}

	GenerateDeclarations(pc);

	for (auto& i : pc.enums)
	{
		GenerateFiles(pc, i.ec, *i.sdef);
	}

	for (auto& I : interfaceDefOrder)
	{
		auto i = pc.interfaces.find(I);
		auto& def = *i->second;

		if (def.methodArray.size() < 2) Throw(0, "No methods found for %s", i->first.c_str());
		GenerateFiles(pc, def.ic, *def.sdef, def.methodArray.data(), *def.sdef);
	}

	for (auto& i : pc.interfaces)
	{
		delete i.second;
	}
}

// Takes a path to a file, returns the end file name as target (sans extension), and the end path (sans extensiion)
// So that /a/dog.txt goes to 'dog' and '/a/dog' 
void StripToFilenameSansExtension(cstr name, char target[_MAX_PATH], char nameSansExtension[_MAX_PATH])
{
	int len = StringLength(name);
	if (len == 0)
	{
		Throw(0, "%s: Name was blank", __FUNCTION__);
	}

	Substring sName = Rococo::Strings::ToSubstring(name);

	cstr lastSlash = Rococo::Strings::ReverseFind('\\', sName);
	if (!lastSlash) lastSlash = Rococo::Strings::ReverseFind('/', sName);

	CopyString(target, _MAX_PATH, lastSlash ? lastSlash + 1 : name);

	char* lastDot = const_cast<char*>(ReverseFind('.', sName));
	if (lastDot)
	{
		target[lastDot - name] = 0;
	}
	if (!lastDot) lastDot = const_cast<char*>(sName.finish);
	
	Substring sNameSansExtension{ name, lastDot };
	Rococo::Strings::SubstringToString(nameSansExtension, _MAX_PATH, sNameSansExtension);
}

void PrintUsage()
{
	printf("Usage:\n\tbennyhill.exe <project root> <content-root> <script-input-file>\n");

	printf(
		"Example:\n\n"
		"(config $config.xc) // The <config_path> can be prefixed with $, which if found, is substituted with the project root\n"
		"\n"
		"(interface\n"
		"\t(as.sxy Sys.Animals.ITiger \"ITiger\") // Scripting language: generate Sys.Animals.ITiger in \"ITiger_sxh.sxy\"\n"
		"\t(as.cpp Sys.Animals.ITiger \"ITiger\") // C++: generate Sys::Animals::ITiger in \"ITiger.sxh.inl\" and \"ITiger.sxh.h\"\n"
		"\t(context Sys.Animals.ITiger) // C++ -> 'Sys::Animals::ITiger* context' passed to native registration functions\n"
		"\t(methods\n"
		"\n"
		"\t)\n"
		")\n"
		);
}

void EndWithSlash(char* buffer, size_t capacity)
{
	size_t len = StringLength(buffer);
	if (len == 0 || len >= capacity)
	{
		Throw(0, "%s: bad string length", __FUNCTION__);
	}

	char lastChar = buffer[len - 1];
	if (lastChar == '/' || lastChar == '\\')
	{
		return;
	}

	if (len == capacity - 1)
	{
		Throw(0, "%s: string length too long", __FUNCTION__);
	}

	buffer[len] = '\\';
	buffer[len+1] = 0;
}

int main(int argc, char* argv[])
{
	if (argc != 4)
	{
		PrintUsage();
		return -1;
	}

	const char* projectRoot = argv[1];
	const char* contentRoot = argv[2];
	const char* scriptInput = argv[3];

	U8FilePath u8inputName;
	Format(u8inputName, "%s%s", projectRoot, scriptInput);

	U8FilePath currentDirectory;
	IO::GetCurrentDirectoryPath(currentDirectory);
	printf("CurrentDirectory: %s. Generating C++ and SXY files from %s\n", currentDirectory.buf, u8inputName.buf);

	WideFilePath wProjectDir;
	Assign(wProjectDir, projectRoot);

	WideFilePath wContentDir;
	Assign(wContentDir, contentRoot);

	WideFilePath wProject;
	Assign(wProject, u8inputName);

	if (!Rococo::IO::IsDirectory(wProjectDir))
	{
		printf("\n\tCould not find project root directory: %s", projectRoot);
		return -1;
	}

	if (!Rococo::IO::IsDirectory(wContentDir))
	{
		printf("\n\tCould not find content root directory: %s", contentRoot);
		return -1;
	}

	if (!Rococo::OS::IsFileExistant(wProject))
	{
		printf("\n\tCould not find project root file: %s", u8inputName.buf);
		return -1;
	}

	ParseContext pc = { 0 };
	CopyString(pc.projectRoot, sizeof pc.projectRoot - 1, projectRoot);
	CopyString(pc.contentRoot, sizeof pc.contentRoot - 1, contentRoot);
	CopyString(pc.scriptInput, sizeof pc.scriptInput - 1, scriptInput);	

	EndWithSlash(pc.projectRoot, sizeof pc.projectRoot);
	EndWithSlash(pc.contentRoot, sizeof pc.contentRoot);

	StripToFilenameSansExtension(pc.scriptInput, pc.scriptName, pc.scriptInputSansExtension);

	if (*pc.scriptName == 0)
	{
		WriteToStandardOutput("\n\tUnexpected error. Could not derive script name from script-input-file '%s'. Expecting [...filename.sxh]", scriptInput);
		return -1;
	}

	Rococo::OS::SetBreakPoints(Rococo::OS::BreakFlag_All);

	Auto<ISParser> parser = Sexy_CreateSexParser_2_0(Rococo::Memory::CheckedAllocator());
	Auto<ISourceCode> src;
	Auto<ISParserTree> tree;

	try
	{
		WideFilePath inputName;
		Format(inputName, L"%hs%hs", pc.projectRoot, pc.scriptInput);
		src = parser->LoadSource(inputName, Vec2i{ 1,1 });
		tree = parser->CreateTree(src());

		if (tree->Root().NumberOfElements() == 0)
		{
			PrintUsage();
			Throw(tree->Root(), "\n\tThe source code is blank\n");
		}

		ParseInterfaceFile(tree->Root(), pc);

		printf("\n");

		return 0;
	}
	catch (ParseException& ex)
	{
		WriteToStandardOutput("\n\t%s. %s\nSpecimen: %s.\nPosition: %d.%d to %d.%d\n", pc.scriptInput, ex.Message(), ex.Specimen(), ex.Start().x, ex.Start().y, ex.End().x, ex.End().y);

		if (ex.ErrorCode() != 0)
		{
			WriteStandardErrorCode(ex.ErrorCode());
		}
		return -1;
	}
	catch (IException& iex)
	{
		WriteToStandardOutput("\n\tError with bennyhill: %s\n", iex.Message());

		if (iex.ErrorCode() != 0)
		{
			WriteStandardErrorCode(iex.ErrorCode());
		}
		return -1;
	}

	return 0;
}