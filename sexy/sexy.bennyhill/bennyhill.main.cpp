/*
	Sexy Scripting Language - Copright(c)2013. Mark Anthony Taylor. All rights reserved.

	http://www.sexiestengine.com

	Email: mark.anthony.taylor@gmail.com

	The Sexy copyright holder is Mark Anthony Taylor, the English author of 'Lazy Bloke Ghosts of Parliament', 'Lazy Bloke in the 35th Century', 'King of the Republic', 'Origin of the Species' 
	and 'Society of Demons.'

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

#include "sexy.lib.s-parser.h"
#include "sexy.lib.util.h"

namespace Rococo
{
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

      if (cpptypeDef == NULL) Throw(field, ("Cannot resolve the type. Neither a known struct or primitive"));

      ConvertAndAppendCppType(appender, cpptypeDef);
   }

   bool AreEqual(cstr s, const sexstring& t)
   {
      return AreEqual(s, t->Buffer);
   }

	Rococo::cstr StringFrom(Rococo::Sex::cr_sex s)
	{
		if (!IsAtomic(s) && !IsStringLiteral(s)) Throw(s, ("Expecting atomic or string literal"));
		return s.String()->Buffer;
	}

	Rococo::cstr StringFrom(Rococo::Sex::cr_sex command, int elementIndex)
	{
		if (elementIndex >= command.NumberOfElements()) Throw(command, ("Insufficient elements in expression"));
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
				Throw(0, ("Overflow formatting struct name: %.64s"), fqStructName);
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

void GenerateFiles(const ParseContext& pc, const InterfaceContext& ic, cr_sex s, const ISExpression* methods, cr_sex interfaceDef)
{
	FileAppender sexyFileAppender(ic.appendSexyFile);	
	DeclareSexyInterface(sexyFileAppender, ic, methods, pc);
	ImplementSexyInterface(sexyFileAppender, ic, methods, s, pc);

	FileAppender cppFileAppender(ic.appendCppHeaderFile);
	DeclareCppInterface(cppFileAppender, ic, interfaceDef, methods, pc);

	FileAppender cppFileImplAppender(ic.appendCppImplFile); 
	ImplementNativeFunctions(cppFileImplAppender, ic, methods, pc);
}

void GenerateFiles(const ParseContext& pc, const EnumContext& ec, cr_sex senumDef)
{
	FileAppender cppFileImplAppender(ec.appendCppImplFile);
	ImplementNativeFunctions(cppFileImplAppender, ec, pc);
}

void GetFileSpec(char filename[_MAX_PATH], cstr root, cstr scriptName, cstr extension)
{
   SafeFormat(filename, _MAX_PATH, ("%s%s%s"), root, scriptName, extension);
}

void AppendNativeFunction(cr_sex functionDef, sexstring ns, const ParseContext& pc, FileAppender& outputFile)
{
	CppType nsType;
	nsType.Set(ns->Buffer);

	cr_sex sfname = functionDef.GetElement(0);
	if (!IsAtomic(sfname))
	{
		Throw(sfname, ("Expecting function name atomic argument"));
	}

	sexstring fname = sfname.String();

	outputFile.Append(("\tvoid Native%s%s(NativeCallEnvironment& _nce)\n"), nsType.CompressedName(), fname->Buffer);
	outputFile.Append(("\t{\n"));

	outputFile.Append(("\t\tRococo::uint8* _sf = _nce.cpu.SF();\n"));
	outputFile.Append(("\t\tptrdiff_t _offset = 2 * sizeof(size_t);\n"));

	bool hasInitializedStringStruct = false;

	TAttributeMap attributes;

	// Write inputs
	int outputStart = GetOutputPosition(functionDef);
	AddNativeInputs(attributes, outputFile, functionDef, 1, outputStart - 2, pc);

	if (functionDef.NumberOfElements() != outputStart + 3 && functionDef.NumberOfElements() != outputStart + 2)
	{
		Throw(functionDef, ("Expecting two elements after the output. The first : and the second the function name to be called in C++"));
	}

   int bodyIndicatorPos = outputStart;

   if (functionDef.NumberOfElements() == outputStart + 3)
   {
      bodyIndicatorPos++;
   }
   
   cr_sex bodyIndicator = functionDef[bodyIndicatorPos];
   if (!IsAtomic(bodyIndicator) || !AreEqual(bodyIndicator.String(), (":")))
   {
      Throw(bodyIndicator, ("Expecting ':' body indicator at this position. Benny Hill functions have only one output. Check spaces between tokens."));
   }

   cr_sex sCppFunction = functionDef[functionDef.NumberOfElements() - 1];
   if (!IsAtomic(sCppFunction) && !IsStringLiteral(sCppFunction))
   {
      Throw(sCppFunction, ("Expecting function name, atomic or string literal"));
   }

   sexstring cppFunction = sCppFunction.String();

	if (bodyIndicatorPos > outputStart)
	{
      cr_sex outputDef = functionDef[outputStart];
		cr_sex stype = outputDef.GetElement(0);
		cr_sex svalue = outputDef.GetElement(1);

		cstr type = StringFrom(stype);

		TTypeMap::const_iterator k = pc.primitives.find(type);
		if (k == pc.primitives.end()) Throw(stype, ("Could not find type amongst the primitives"));

		char cppName[256];
		char compressedName[256];
		GetFQCppStructName(compressedName, cppName, 256, k->second.cppType.c_str());

		outputFile.Append(("\t\t%s %s = "), cppName, StringFrom(svalue));
	}
	else
	{
		outputFile.Append(("\t\t"));
	}

	outputFile.Append(("%s("), cppFunction->Buffer);

	int inputCount = 1;

	// Append the input arguments to the method invocation
	for (int i = 1; i < outputStart - 1; ++i)
	{
		cr_sex s = functionDef.GetElement(i);

		cr_sex stype = s.GetElement(0);
		cr_sex svalue = s.GetElement(1);
		cstr type = StringFrom(stype);

		if (!AreEqual(type, ("#")))
		{
			if (inputCount > 1) outputFile.Append((", "));
			inputCount++;

			if (!AreEqual(type, ("IString")) && !AreEqual(type, ("Sys.Type.IString")) && pc.structs.find(type) != pc.structs.end())
			{
				outputFile.Append('*');
			}

			outputFile.Append(("%s"), StringFrom(svalue));
		}
		else
		{
			// Skip input arguments that are attributes rather than type-name pairs.
		}
	}

	outputFile.Append((");\n"));

	if (bodyIndicatorPos > outputStart)
	{
      cr_sex outputDef = functionDef[outputStart];
		cr_sex stype = outputDef.GetElement(0);
		cr_sex svalue = outputDef.GetElement(1);

		outputFile.Append(("\t\t_offset += sizeof(%s);\n"), StringFrom(svalue));
		outputFile.Append(("\t\tWriteOutput(%s, _sf, -_offset);\n"), StringFrom(svalue));
	}

	outputFile.Append(("\t}\n\n"));
}

void AppendNativeRegistration(cr_sex functionDef, const CppType& ns, const ParseContext& pc, FileAppender& outputFile)
{
	cr_sex sfname = functionDef.GetElement(0);
	sexstring fname = sfname.String();
	outputFile.Append(("ss.AddNativeCall(ns, Native%s%s, nullptr, (\"%s"), ns.CompressedName(), fname->Buffer, fname->Buffer);

	int outputPos = GetOutputPosition(functionDef);
	for (int i = 1; i < outputPos - 1; i++)
	{
		cr_sex arg = functionDef.GetElement(i);
		AppendInputPair(outputFile, arg, pc);
	}

	outputFile.Append((" -> "));
	
	cr_sex outputDef = functionDef.GetElement(outputPos);

	if (IsCompound(outputDef))
	{
		AppendOutputPair(outputFile, outputDef, pc);
	}

	outputFile.Append(("\"));"));
}

void ParseFunctions(cr_sex functionSetDef, const ParseContext& pc)
{
	if (functionSetDef.NumberOfElements() < 3)
	{
		Throw(functionSetDef, ("Expecting (functions <namespace> <file-prefix> ...)"));
	}

	cr_sex sNS = functionSetDef.GetElement(1);
	if (!IsAtomic(sNS))
	{
		Throw(sNS, ("Expecting atomic namespace expression"));
	}

	ValidateFQSexyInterface(sNS);	
	sexstring ns = sNS.String();
	
	cr_sex sFilePrefix = functionSetDef.GetElement(2);
	if (!IsStringLiteral(sFilePrefix))
	{
		Throw(sNS, ("Expecting string literal for the file prefix"));
	}

	sexstring filePrefix = sFilePrefix.String();

	char sexyFile[_MAX_PATH];
   SafeFormat(sexyFile, _MAX_PATH, ("%s%s.inl"), pc.cppRootDirectory, filePrefix->Buffer);

	FileAppender sexyAppender(sexyFile);

	sexyAppender.Append(("namespace\n{\n"));

	for (int i = 3; i < functionSetDef.NumberOfElements(); ++i)
	{
		cr_sex functionDef = functionSetDef.GetElement(i);
		if (!IsCompound(functionDef))
		{
			Throw(functionDef, ("Expecting function def of form (<sexy-function-name> (input1)...(inputN)->(output): <cpp-function-name>)"));
		}

		AppendNativeFunction(functionDef, ns, pc, sexyAppender);
	}

	sexyAppender.Append(("}\n\n"));

	char* nsBuffer = (char*)alloca(sizeof(char)* (StringLength(ns->Buffer) + 1));
	CopyString(nsBuffer, StringLength(ns->Buffer) + 1, ns->Buffer);

	char* token = nullptr;
	char* context = nullptr;

#ifndef char_IS_WIDE
# define Tokenize strtok_s
#else
# define Tokenize wcstok_s
#endif
	token = Tokenize(nsBuffer, ("."), &context);

	int namespaceDepth = 0;

	while (token)
	{
		namespaceDepth++;
		sexyAppender.Append(("namespace %s { "), token);
		token = Tokenize(nullptr, ("."), &context);
	}

	CppType nsType;
	nsType.Set(ns->Buffer);

	sexyAppender.Append(("\n\tvoid AddNativeCalls_%s(Rococo::Script::IPublicScriptSystem& ss, void* nullContext = nullptr)\n"), nsType.CompressedName());
	sexyAppender.Append(("\t{\n"));

	sexyAppender.Append(("\t\tconst INamespace& ns = ss.AddNativeNamespace((\"%s\"));\n"), nsType.SexyName());

	for (int i = 3; i < functionSetDef.NumberOfElements(); ++i)
	{
		cr_sex functionDef = functionSetDef.GetElement(i);
		sexyAppender.Append(("\t\t"));
		AppendNativeRegistration(functionDef, nsType, pc, sexyAppender);
		sexyAppender.Append(("\n"));
	}

	sexyAppender.Append(("\t}\n"));

	while (namespaceDepth)
	{
		namespaceDepth--;
		sexyAppender.Append(("}"));
	}
}

void ParseEnum(cr_sex senumDef, ParseContext& pc)
{
   EnumDef def;
   def.sdef = &senumDef;
	EnumContext& ec = def.ec;

	if (senumDef.NumberOfElements() < 4)
	{
		Throw(senumDef, ("Expecting at least 4 elements in enumeration definition"));
	}

	cr_sex sbasicType = senumDef[1];
	if (!IsAtomic(sbasicType))
	{
		Throw(sbasicType, ("Expecting underlying type, such as Int32 or Int64"));
	}

	auto i = pc.primitives.find(sbasicType.String()->Buffer);
	if (i == pc.primitives.end())
	{
		Throw(sbasicType, ("Cannot find primitive type in the config file"));
	}

	ec.underlyingType.Set(i->second.cppType.c_str());

	for (int i = 2; i < senumDef.NumberOfElements(); ++i)
	{
		cr_sex sdirective = senumDef.GetElement(i);
		if (!IsCompound(sdirective)) Throw(sdirective, ("Expecting compound expression in the enum definition"));

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
					Throw(sdef, ("Expecting compound expression with 2 elements (<enum_name> <enum_value>)"));
				}

				cr_sex sName = sdef[0];
				cr_sex sValue = sdef[1];

				if (!IsCapital(sName.String()->Buffer[0]))
				{
					Throw(sName, ("Expecting a capital letter in the first position"));
				}

				for (cstr p = sName.String()->Buffer; *p != 0; p++)
				{
					if (!IsAlphaNumeric(*p))
					{
						Throw(sName, ("Expecting alphanumerics throughout the name definition"));
					}
				}

				VariantValue value;
				auto result = Rococo::Parse::TryParse(value, VARTYPE_Int64, sValue.String()->Buffer);
				if (result != Rococo::Parse::PARSERESULT_GOOD)
				{
					Throw(sValue, ("Expecting int64 numeric for value"));
				}

				ec.values.push_back(std::make_pair(sName.String()->Buffer, value.int64Value));
			}
		}
		else if (scmd == ("as.both"))
		{
			if (sdirective.NumberOfElements() != 3) Throw(sdirective, ("Expecting (as.both <fully qualified enum name> \"<target-filename-prefix>\")"));
			cr_sex sinterfaceName = sdirective.GetElement(1);
			ValidateFQSexyInterface(sinterfaceName); // sexy namespaces are more stringent than those of C++. so we use sexy validation for both

			if (ec.asSexyEnum[0] != 0) Throw(sdirective, ("as.sxy is already defined for this interface"));
			if (ec.asCppEnum.SexyName()[0] != 0) Throw(sdirective, ("as.cpp is already defined for this interface"));

			cr_sex ssexyFilename = sdirective.GetElement(2);
			if (!IsStringLiteral(ssexyFilename)) Throw(ssexyFilename, ("Expecting string literal target-prefix. Filenames can potentially have spaces in them."));

			cstr sexyFilename = ssexyFilename.String()->Buffer;

         SafeFormat(ec.appendCppHeaderFile, _MAX_PATH, ("%s%s.sxh.h"), pc.cppRootDirectory, sexyFilename);
         SafeFormat(ec.appendCppImplFile, _MAX_PATH, ("%s%s.sxh.inl"), pc.cppRootDirectory, sexyFilename);

			ec.asCppEnum.Set(sinterfaceName.String()->Buffer);
			CopyString(ec.asSexyEnum, InterfaceContext::MAX_TOKEN_LEN, sinterfaceName.String()->Buffer);
         SafeFormat(ec.appendSexyFile, _MAX_PATH, ("%s%s.sxh.sxy"), pc.cppRootDirectory, ssexyFilename.String()->Buffer);
		}
		else if (scmd == ("as.sxy"))
		{
			if (sdirective.NumberOfElements() != 3) Throw(sdirective, ("Expecting (as.sxy <fully qualified enum name> \"<target-filename>\")"));
			cr_sex sinterfaceName = sdirective.GetElement(1);
			ValidateFQSexyInterface(sinterfaceName);

			if (ec.asSexyEnum[0] != 0) Throw(sdirective, ("as.sxy is already defined for this enum"));

			cr_sex ssexyFilename = sdirective.GetElement(2);
			if (!IsStringLiteral(ssexyFilename)) Throw(ssexyFilename, ("Expecting string literal"));
			CopyString(ec.asSexyEnum, InterfaceContext::MAX_TOKEN_LEN, sinterfaceName.String()->Buffer);
         SafeFormat(ec.appendSexyFile, _MAX_PATH, ("%s%s.sxh.sxy"), pc.cppRootDirectory, ssexyFilename.String()->Buffer);
		}
		else if (scmd == ("as.cpp"))
		{
			if (sdirective.NumberOfElements() != 3) Throw(sdirective, ("Expecting (as.cpp <fully qualified enum name> \"<target-filename-sans-extension>\")"));
			cr_sex sstructName = sdirective.GetElement(1);
			ValidateFQCppStruct(sstructName);

			if (ec.asCppEnum.SexyName()[0] != 0) Throw(sdirective, ("as.cpp is already defined for this enum"));

			cr_sex ssexyFilename = sdirective.GetElement(2);
			if (!IsStringLiteral(ssexyFilename)) Throw(ssexyFilename, ("Expecting string literal"));

			cstr sexyFilename = ssexyFilename.String()->Buffer;

         SafeFormat(ec.appendCppHeaderFile, _MAX_PATH, ("%s%s.sxh.h"), pc.cppRootDirectory, sexyFilename);
         SafeFormat(ec.appendCppImplFile, _MAX_PATH, ("%s%s.sxh.inl"), pc.cppRootDirectory, sexyFilename);

			ec.asCppEnum.Set(sstructName.String()->Buffer);
		}
		else
		{
			Throw(scmd, ("Expecting one of {as.both, as.sxy, as.cpp} or a compound expression giving a set of enumeration name value pairs"));
		}
	}

	if (ec.asCppEnum.SexyName()[0] == 0)
	{
		Throw(senumDef, ("Missing as.cpp or as.both"));
	}

	if (ec.asSexyEnum[0] == 0)
	{
		Throw(senumDef, ("Missing as.sxy or as.both"));
	}

	if (ec.appendSexyFile[0] == 0) SafeFormat(ec.appendSexyFile, _MAX_PATH, ("%s.sxh.sxy"), pc.scriptInputSansExtension);

	if (ec.appendCppHeaderFile[0] == 0)
	{
      SafeFormat(ec.appendCppHeaderFile, _MAX_PATH, ("%s%s.sxh.h"), pc.cppRootDirectory, pc.scriptName);
      SafeFormat(ec.appendCppImplFile, _MAX_PATH, ("%s%s.sxh.cpp"), pc.cppRootDirectory, pc.scriptName);
	}

   pc.enums.push_back(def);
}

void ParseInterface(cr_sex interfaceDef, ParseContext& pc)
{
	InterfaceContext ic;
	const ISExpression* methods = NULL;

	bool hasDestructor = false;

	for(int i = 1; i < interfaceDef.NumberOfElements(); ++i)
	{
		cr_sex directive = interfaceDef.GetElement(i);
		if (!IsCompound(directive)) Throw(directive, ("Expecting compound expression in the interface definition"));

		cr_sex directiveName = directive.GetElement(0);
		if (!IsAtomic(directiveName))  Throw(directiveName, ("Expecting atomic expression in the directive name at position #0"));

		const sexstring ssname = directiveName.String();

		if (AreEqual(ssname, ("as.both")))
		{
			if (directive.NumberOfElements() != 3) Throw(directive, ("Expecting (as.both <fully qualified interface name> \"<target-filename-prefix>\")"));
			cr_sex sinterfaceName = directive.GetElement(1);
			ValidateFQSexyInterface(sinterfaceName); // sexy namespaces are more stringent than those of C++. so we use sexy validation for both

			if (ic.asSexyInterface[0] != 0) Throw(directive, ("as.sxy is already defined for this interface"));
			if (ic.asCppInterface.SexyName()[0] != 0) Throw(directive, ("as.cpp is already defined for this interface"));

			cr_sex ssexyFilename = directive.GetElement(2);
			if (!IsStringLiteral(ssexyFilename)) Throw(ssexyFilename, ("Expecting string literal target-prefix. Filenames can potentially have spaces in them."));

			cstr sexyFilename = ssexyFilename.String()->Buffer;

         SafeFormat(ic.appendCppHeaderFile, _MAX_PATH, ("%s%s.sxh.h"), pc.cppRootDirectory, sexyFilename);
         SafeFormat(ic.appendCppImplFile, _MAX_PATH, ("%s%s.sxh.inl"), pc.cppRootDirectory, sexyFilename);

			ic.asCppInterface.Set(sinterfaceName.String()->Buffer);
			CopyString(ic.asSexyInterface, InterfaceContext::MAX_TOKEN_LEN, sinterfaceName.String()->Buffer);
         SafeFormat(ic.appendSexyFile, _MAX_PATH, ("%s%s.sxh.sxy"), pc.cppRootDirectory, ssexyFilename.String()->Buffer);
		}
		else if (AreEqual(ssname, ("as.sxy")))
		{
			if (directive.NumberOfElements() < 3 || directive.NumberOfElements() > 4) Throw(directive, ("Expecting (as.sxy <fully qualified interface name> \"<target-filename>\" [inheritance string])"));
			cr_sex sinterfaceName = directive.GetElement(1);
			ValidateFQSexyInterface(sinterfaceName);

			if (ic.asSexyInterface[0] != 0) Throw(directive, ("as.sxy is already defined for this interface"));

         if (directive.NumberOfElements() == 4)
         {
            SafeFormat(ic.inheritanceString, 128, ("%s"), directive[3].String()->Buffer);
         }
         else
         {
            *ic.inheritanceString = 0;
         }

			cr_sex ssexyFilename = directive.GetElement(2);
			if (!IsStringLiteral(ssexyFilename)) Throw(ssexyFilename,  ("Expecting string literal"));
			CopyString(ic.asSexyInterface, InterfaceContext::MAX_TOKEN_LEN, sinterfaceName.String()->Buffer);
         SafeFormat(ic.appendSexyFile, _MAX_PATH, ("%s%s.sxh.sxy"), pc.cppRootDirectory, ssexyFilename.String()->Buffer);
		}
		else if (AreEqual(ssname, ("as.cpp")))
		{
			if (directive.NumberOfElements() != 3) Throw(directive, ("Expecting (as.cpp <fully qualified interface name> \"<target-filename-sans-extension>\")"));
			cr_sex sstructName = directive.GetElement(1);
			ValidateFQCppStruct(sstructName);

			if (ic.asCppInterface.SexyName()[0] != 0) Throw(directive, ("as.cpp is already defined for this interface"));

			cr_sex ssexyFilename = directive.GetElement(2);
			if (!IsStringLiteral(ssexyFilename)) Throw(ssexyFilename,  ("Expecting string literal"));

			cstr sexyFilename = ssexyFilename.String()->Buffer;

         SafeFormat(ic.appendCppHeaderFile, _MAX_PATH, ("%s%s.sxh.h"), pc.cppRootDirectory, sexyFilename);
         SafeFormat(ic.appendCppImplFile, _MAX_PATH, ("%s%s.sxh.inl"), pc.cppRootDirectory, sexyFilename);

			ic.asCppInterface.Set(sstructName.String()->Buffer);
		}
		else if (AreEqual(ssname, ("methods")))
		{
			if (methods != NULL) Throw(directive, ("Duplicate methods directive"));
			methods = &directive;
		}
		else if (AreEqual(ssname, ("~")))
		{
			if (ic.hasDestructor) { Throw(directive, ("Interface already has destructor defined"));	}
			ic.hasDestructor = true;
			if (directive.NumberOfElements() != 1) Throw(directive, ("Destructors take no arguments"));
		}
		else if (AreEqual(ssname, ("factory")))
		{
			cr_sex sfactory = directive;
			if (sfactory.NumberOfElements() < 2 || !IsAtomic(sfactory.GetElement(1)))
			{
				Throw(sfactory, ("Expecting (factory <factory-name> (optional input arg1) ... (optional input argN)"));
			}
			ic.factories.push_back(&sfactory);
		}
		else if (AreEqual(ssname, ("context")))
		{
			if (directive.NumberOfElements() != 3)
			{
				Throw(directive, ("Expecting (context <'factory'|'api'> <context-type>). E.g (context factory Sys.Animal)"));
			}

			cr_sex sroute = directive.GetElement(1);
			cr_sex stype = directive.GetElement(2);
			if (!IsAtomic(sroute)) Throw(stype, ("Expecting atomic value for <'factory'|'api'>"));

			cstr route = sroute.String()->Buffer;
			if (AreEqual(route, ("factory")))
			{
				ic.isSingleton = false;
			}
			else if (AreEqual(route, ("api")))
			{
				ic.isSingleton = true;
			}
			else
			{
				Throw(sroute, ("Expecting either 'factory' or 'api' sans quotes. 'factory' is used when the factory call determines the underlying C++ object. 'api' is used when the native registration functions determine the underlying C++ object."));
			}

			if (!IsAtomic(stype)) Throw(stype, ("Expecting atomic value for <context-type>"));
			ic.nceContext.Set(stype.String()->Buffer);
		}
		else
		{
			Throw(directiveName, ("Unknown directive in interface definition. Expecting one of (as.sxy, as.cpp, as.both, context, factory, methods)"));
		}		
	}

	if (ic.asCppInterface.SexyName()[0] == 0)
	{
		Throw(interfaceDef, ("Missing as.cpp or as.both"));
	}

	if (ic.asSexyInterface[0] == 0)
	{
		Throw(interfaceDef, ("Missing as.sxy or as.both"));
	}

	if (ic.appendSexyFile[0] == 0) SafeFormat(ic.appendSexyFile, _MAX_PATH, ("%s.sxh.sxy"), pc.scriptInputSansExtension);

	if (ic.appendCppHeaderFile[0] == 0) 
	{
      SafeFormat(ic.appendCppHeaderFile, _MAX_PATH, ("%s%s.sxh.h"), pc.cppRootDirectory, pc.scriptName);
      SafeFormat(ic.appendCppImplFile, _MAX_PATH, ("%s%s.sxh.cpp"), pc.cppRootDirectory, pc.scriptName);
	}

	if (ic.factories.empty())
	{
		// Disable exception. Interfaces can be grabbed from the output of functions that return them
      // Throw(interfaceDef, ("Interface needs to specify at least one factory"));
	}

	if (!ic.nceContext.SexyName()[0])
	{
		Throw(interfaceDef, ("Missing (context <'factory'|'api'> <context-type>)"));
	}

   InterfaceDef* def = new InterfaceDef;
   def->methods = methods;
   def->sdef = &interfaceDef;
   def->ic = ic;

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

	for (int i = 0; i < root.NumberOfElements(); ++i)
	{
		cr_sex topLevelItem = root.GetElement(i);

		if (!IsCompound(topLevelItem)) Throw(topLevelItem, ("Expecting compound expression in the root expression"));
		cr_sex command = topLevelItem.GetElement(0);
		if (!IsAtomic(command))  Throw(command, ("Expecting atomic expression in the command at position #0"));

		sexstring cmd = command.String();

		if (AreEqual(("config"), cmd))
		{
			if (hasConfig) Throw(command, ("Only one config entry permitted"));
			ParseConfigSpec(topLevelItem, pc);
			hasConfig = true;
		}
		else if (AreEqual(("functions"), cmd))
		{
			if (hasFunctions) Throw(command, ("Only one set of functions can be defined in the generator file"));
			if (!hasConfig) Throw(command, ("Must define a (config <config-path>) entry before all functions"));
			ParseFunctions(topLevelItem, pc);
			hasFunctions = true;
		}
		else if (AreEqual(("interface"), cmd))
		{
			if (!hasConfig) Throw(command, ("Must define a (config <config-path>) entry before all interfaces"));
			ParseInterface(topLevelItem, pc);
		}
		else if (AreEqual(("enum"), cmd))
		{
			if (!hasConfig) Throw(command, ("Must define a (config <config-path>) entry before all enumerations"));
			ParseEnum(topLevelItem, pc);
		}
		else
		{
			Throw(command, ("Expecting 'interface or config or functions' in the command at position #0"));
		}
	}

	GenerateDeclarations(pc);

	for (auto& i : pc.enums)
	{
		GenerateFiles(pc, i.ec, *i.sdef);
	}

	for (auto& i : pc.interfaces)
	{
		auto& def = *i.second;
		GenerateFiles(pc, def.ic, *def.sdef, def.methods, *def.sdef);
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
	for(int i = len; i > 0; i--)
	{
		if (name[i] == '\\' || name[i] == '/')
		{
			int j, k;
			for(k = 0, j = i + 1; name[j] != '.' && name[j] != 0; ++j, ++k)
			{
				target[k] = name[j];
			}
			
			target[k] = 0;

			for(k = 0; k < j; ++k)
			{
				nameSansExtension[k] = name[k];
			}

			nameSansExtension[k] = 0;			
			return;
		}
	}

	int j, k;
	for (k = 0, j = 0; name[j] != '.' && name[j] != 0; ++j, ++k)
	{
		target[k] = name[j];
	}

	target[k] = 0;

	for (k = 0; k < j; ++k)
	{
		nameSansExtension[k] = name[k];
	}

	nameSansExtension[k] = 0;
}

void PrintUsage()
{
	printf("Usage:\n\tbennyhill.exe <project root> <script-input-file> <touch-file>. If <touch-file> is 'null' then compilation is forced.\n");

	printf(
		"Example:\n\n"
		"(config $config.xc) // The <config_path> can be prefixed with $, which if found, is substituted with the project root\n"
		"\n"
		"(interface\n"
		"\t(as.sxy Sys.Animals.ITiger \"ITiger\") // Scripting language: generate Sys.Animals.ITiger in \"ITiger.sxh.sxy\"\n"
		"\t(as.cpp Sys.Animals.ITiger \"ITiger\") // C++: generate Sys::Animals::ITiger in \"ITiger.sxh.inl\" and \"ITiger.sxh.h\"\n"
		"\t(context Sys.Animals.ITiger) // C++ -> 'Sys::Animals::ITiger* context' passed to native registration functions\n"
		"\t(methods\n"
		"\n"
		"\t)\n"
		")\n"
		);
}

int main(int argc, char* argv[])
{
	if (argc != 4)
	{
		PrintUsage();
		return -1;
	}

	const char* projectRoot = argv[1];
	const char* scriptInput = argv[2];
	const char* touchFile = argv[3];

	printf("ProjectRoot: %s\n", projectRoot);
	printf("ScriptInput: %s\n", scriptInput);
	printf("TouchFile: %s\n", touchFile);

	int64 touchModifiedAt = GetLastModifiedDate(touchFile);
	int64 scriptModifiedAt = GetLastModifiedDate(scriptInput);

	if (touchModifiedAt == 0 && strcmp(touchFile, "null") != 0)
	{
		printf("!!! Warning: touchfile '%s' does not appear to exist !!! \n", touchFile);
	}

	if (scriptModifiedAt == 0)
	{
		printf("!!! Warning: scriptInput '%s' does not appear to exist !!!\n", scriptInput);
	}

	if (scriptModifiedAt < touchModifiedAt)
	{
		printf("%s was last updated before %s.\nTerminating BennyHill gracefully.\n", scriptInput, touchFile);
		return 0;
	}

	ParseContext pc = { 0 };
	CopyCharTochar(pc.projectRoot, projectRoot, _MAX_PATH);
	CopyCharTochar(pc.scriptInput, scriptInput, _MAX_PATH);

	StripToFilenameSansExtension(pc.scriptInput, pc.scriptName, pc.scriptInputSansExtension);

	if (*pc.scriptName == 0)
	{
		WriteToStandardOutput(("Unexpected error. Could not derive script name from script-input-file '%s'. Expecting [...filename.sxh]"), scriptInput);
		return -1;
	}

	Rococo::OS::SetBreakPoints(Rococo::OS::BreakFlag_All);

	CSParserProxy spp;
	Auto<ISourceCode> src;
	Auto<ISParserTree> tree;

	try
	{
		src = spp->LoadSource(pc.scriptInput, Vec2i{ 1,1 });
		tree = spp->CreateTree(src());

		if (tree->Root().NumberOfElements() == 0)
		{
			PrintUsage();
			Throw(tree->Root(), ("The source code is blank"));
		}

		ParseInterfaceFile(tree->Root(), pc);
	}
	catch (ParseException& ex)
	{
		WriteToStandardOutput(("%s. %s\nSpecimen: %s.\nPosition: %d.%d to %d.%d\n"), pc.scriptInput, ex.Message(), ex.Specimen(), ex.Start().x, ex.Start().y, ex.End().x, ex.End().y);

		if (ex.ErrorCode() != 0)
		{
			WriteStandardErrorCode(ex.ErrorCode());
		}
		return -1;
	}
	catch (IException& iex)
	{
		WriteToStandardOutput(("Error with bennyhill: %s"), iex.Message());

		if (iex.ErrorCode() != 0)
		{
			WriteStandardErrorCode(iex.ErrorCode());
		}
		return -1;
	}

	return 0;
}