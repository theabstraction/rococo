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

	3. THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM �AS IS� WITHOUT
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

#include "sexy.types.h"
#include "sexy.strings.h"

#include <stdarg.h>

#include <unordered_map>

#include <sexy.stdstrings.h>

#include "sexy.s-parser.h"
#include "sexy.lib.s-parser.h"
#include "sexy.strings.inl"
#include "sexy.namespaces.inl"

#include "sexy.lib.util.h"


namespace Sexy
{
	bool AreEqual(csexstr s, const sexstring& t)
	{
		return AreEqual(s, t->Buffer);
	}
}

struct TypeDef
{
	Sexy::stdstring sexyType;
	Sexy::stdstring cppType;
};

typedef std::unordered_map<Sexy::stdstring,TypeDef> TTypeMap;

struct ParseContext
{
	Sexy::SEXCHAR scriptInput[_MAX_PATH];
	Sexy::SEXCHAR projectRoot[_MAX_PATH];
	Sexy::SEXCHAR scriptName[_MAX_PATH];
	Sexy::SEXCHAR scriptInputSansExtension[_MAX_PATH];
	Sexy::SEXCHAR cppRoot[_MAX_PATH];
	Sexy::SEXCHAR cppTypes[_MAX_PATH];
	Sexy::SEXCHAR sexyTypes[_MAX_PATH];
	Sexy::SEXCHAR cppException[_MAX_PATH];

	TTypeMap primitives;
	TTypeMap structs;
};

#include "bennyhill.sex.inl"

namespace
{
	typedef std::vector<const Sexy::Sex::ISExpression*> TExpressions;

	Sexy::csexstr StringFrom(Sexy::Sex::cr_sex s)
	{
		if (!IsAtomic(s) && !IsStringLiteral(s)) Throw(s, SEXTEXT("Expecting atomic or string literal"));
		return s.String()->Buffer;
	}

	Sexy::csexstr StringFrom(Sexy::Sex::cr_sex command, int elementIndex)
	{
		if (elementIndex >= command.NumberOfElements()) Throw(command, SEXTEXT("Insufficient elements in expression"));
		return StringFrom(command.GetElement(elementIndex));
	}

	void AppendCppType(FileAppender& appender, cr_sex field, csexstr fieldtype, const ParseContext& pc);


	void GetFQCppStructName(SEXCHAR* compressedStructName, SEXCHAR* cppStructName, size_t capacity, csexstr fqStructName)
	{
		SEXCHAR* p = compressedStructName;
		SEXCHAR* q = cppStructName;
		size_t pos = 0;
		size_t safeZone = capacity - 2;
		const SEXCHAR* pSrc = fqStructName;
		while (true)
		{
			SEXCHAR src = *pSrc++;
			if (src == 0)
			{
				break;
			}

			if (pos >= safeZone)
			{
				OS::OSException ex;
				StringPrint(ex.message, 256, SEXTEXT("Overflow formatting struct name: %.64s"), fqStructName);
				throw ex;
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

	class CppType
	{
	private:
		enum { MAX_TOKEN_LEN = 256 };
		SEXCHAR bennyHillDef[MAX_TOKEN_LEN];
		SEXCHAR compressedName[MAX_TOKEN_LEN];
		SEXCHAR fqName[MAX_TOKEN_LEN];

	public:
		CppType()
		{
			bennyHillDef[0] = 0;
			compressedName[0] = 0;
			fqName[0] = 0;
		}

		void Set(csexstr bennyHillDef)
		{
			CopyString(this->bennyHillDef, MAX_TOKEN_LEN, bennyHillDef);
			GetFQCppStructName(compressedName, fqName, 256, bennyHillDef);
		}

		csexstr CompressedName() const
		{
			return compressedName;
		}

		csexstr FQName() const
		{
			return fqName;
		}

		csexstr SexyName() const
		{
			return bennyHillDef;
		}
	};
}

struct InterfaceContext
{
	enum { MAX_TOKEN_LEN = 256 };
	CppType asCppInterface;
	SEXCHAR asSexyInterface[MAX_TOKEN_LEN];
	SEXCHAR appendSexyFile[_MAX_PATH];
	SEXCHAR appendCppHeaderFile[_MAX_PATH];
	SEXCHAR appendCppImplFile[_MAX_PATH];

	bool isSingleton; // If true then the context comes from the native registration method, else it comes from the factory.
	CppType nceContext;
	bool hasDestructor;
	TExpressions factories;

	InterfaceContext()
	{
		asSexyInterface[0] = 0;
		appendSexyFile[0] = 0;
		appendCppHeaderFile[0] = 0;
		appendCppImplFile[0] = 0;
		hasDestructor = false;
		isSingleton = false;
	}
};

#include "bennyhill.validators.inl"
#include "bennyhill.appender.sexy.inl"
#include "bennyhill.appender.cpp.inl"
#include "bennyhill.config.inl"

using namespace Sexy;
using namespace Sexy::Sex;

void CopyCharToSEXCHAR(SEXCHAR* dest, const char* src, size_t capacity)
{
	for(size_t i = 0; i < capacity; i++)
	{
		dest[i] = src[i];
		dest[i] &= 0x00FF;
	}

	dest[capacity-1] = 0;
}

void GenerateFiles(const ParseContext& pc, const InterfaceContext& ic, cr_sex s, const ISExpression* methods, cr_sex interfaceDef)
{
	FileDeleteOnceOnly(ic.appendSexyFile);
	FileAppender sexyFileAppender(ic.appendSexyFile);		
	DeclareSexyInterface(sexyFileAppender, ic, methods, pc);
	ImplementSexyInterface(sexyFileAppender, ic, methods, s, pc);

	FileDeleteOnceOnly(ic.appendCppHeaderFile);
	FileAppender cppFileAppender(ic.appendCppHeaderFile);
	DeclareCppInterface(cppFileAppender, ic, interfaceDef, methods, pc);

	FileDeleteOnceOnly(ic.appendCppImplFile);
	FileAppender cppFileImplAppender(ic.appendCppImplFile);
	ImplementNativeFunctions(cppFileImplAppender, ic, methods, pc);
}

void GetFileSpec(SEXCHAR filename[_MAX_PATH], csexstr root, csexstr scriptName, csexstr extension)
{
	StringPrint(filename, _MAX_PATH, SEXTEXT("%s%s%s"), root, scriptName, extension);
}

void AppendNativeFunction(cr_sex functionDef, sexstring ns, const ParseContext& pc, FileAppender& outputFile)
{
	CppType nsType;
	nsType.Set(ns->Buffer);

	cr_sex sfname = functionDef.GetElement(0);
	if (!IsAtomic(sfname))
	{
		Throw(sfname, SEXTEXT("Expecting function name atomic argument"));
	}

	sexstring fname = sfname.String();

	outputFile.Append(SEXTEXT("\tvoid Native%s%s(NativeCallEnvironment& _nce)\n"), nsType.CompressedName(), fname->Buffer);
	outputFile.Append(SEXTEXT("\t{\n"));

	outputFile.Append(SEXTEXT("\t\tSexy::uint8* _sf = _nce.cpu.SF();\n"));
	outputFile.Append(SEXTEXT("\t\tptrdiff_t _offset = 2 * sizeof(size_t);\n"));

	bool hasInitializedStringStruct = false;

	TAttributeMap attributes;

	// Write inputs
	int outputStart = GetOutputPosition(functionDef);
	AddNativeInputs(attributes, outputFile, functionDef, 1, outputStart - 2, pc);

	if (functionDef.NumberOfElements() != outputStart + 3 && functionDef.NumberOfElements() != outputStart + 2)
	{
		Throw(functionDef, SEXTEXT("Expecting two elements after the output. The first : and the second the function name to be called in C++"));
	}

	int bodyIndicatorPos = outputStart + 1;

	const ISExpression* outputDef = &functionDef.GetElement(outputStart);
	if (IsAtomic(*outputDef))
	{
		if (AreEqual(outputDef->String(), SEXTEXT(":")))
		{
			bodyIndicatorPos = outputStart;
		}
		else
		{
			Throw(*outputDef, SEXTEXT("Expecting ':' body indicator at this position. Benny Hill functions have only one output. Check spaces between tokens."));
		}
	}

	if (outputDef != nullptr)
	{
		cr_sex bodyIndicator = functionDef.GetElement(outputStart + 1);
		if (!IsAtomic(bodyIndicator))
		{
			Throw(bodyIndicator, SEXTEXT("Expecting ':' body indicator at this position. Benny Hill functions have only one output. Check spaces between tokens."));
		}
	}

	cr_sex sCppFunction = functionDef.GetElement(functionDef.NumberOfElements() - 1);
	if (!IsAtomic(sCppFunction) && !IsStringLiteral(sCppFunction))
	{
		Throw(sCppFunction, SEXTEXT("Expecting function name, atomic or string literal"));
	}

	sexstring cppFunction = sCppFunction.String();

	if (outputDef)
	{
		cr_sex stype = outputDef->GetElement(0);
		cr_sex svalue = outputDef->GetElement(1);

		csexstr type = StringFrom(stype);

		TTypeMap::const_iterator k = pc.primitives.find(type);
		if (k == pc.primitives.end()) Throw(stype, SEXTEXT("Could not find type amongst the primitives"));

		SEXCHAR cppName[256];
		SEXCHAR compressedName[256];
		GetFQCppStructName(compressedName, cppName, 256, k->second.cppType.c_str());

		outputFile.Append(SEXTEXT("\t\t%s %s = "), cppName, StringFrom(svalue));
	}
	else
	{
		outputFile.Append(SEXTEXT("\t\t"));
	}

	outputFile.Append(SEXTEXT("%s("), cppFunction->Buffer);

	int inputCount = 1;

	// Append the input arguments to the method invocation
	for (int i = 1; i < outputStart - 1; ++i)
	{
		cr_sex s = functionDef.GetElement(i);

		cr_sex stype = s.GetElement(0);
		cr_sex svalue = s.GetElement(1);
		csexstr type = StringFrom(stype);

		if (!AreEqual(type, SEXTEXT("#")))
		{
			if (inputCount > 1) outputFile.Append(SEXTEXT(", "));
			inputCount++;

			if (!AreEqual(type, SEXTEXT("IString")) && !AreEqual(type, SEXTEXT("Sys.Type.IString")) && pc.structs.find(type) != pc.structs.end())
			{
				outputFile.Append('*');
			}

			outputFile.Append(SEXTEXT("%s"), StringFrom(svalue));
		}
		else
		{
			// Skip input arguments that are attributes rather than type-name pairs.
		}
	}

	outputFile.Append(SEXTEXT(");\n"));

	if (outputDef)
	{
		cr_sex stype = outputDef->GetElement(0);
		cr_sex svalue = outputDef->GetElement(1);

		outputFile.Append(SEXTEXT("\t\t_offset += sizeof(%s);\n"), StringFrom(svalue));
		outputFile.Append(SEXTEXT("\t\tWriteOutput(%s, _sf, -_offset);\n"), StringFrom(svalue));
	}

	outputFile.Append(SEXTEXT("\t}\n\n"));
}

void AppendNativeRegistration(cr_sex functionDef, const CppType& ns, const ParseContext& pc, FileAppender& outputFile)
{
	cr_sex sfname = functionDef.GetElement(0);
	sexstring fname = sfname.String();
	outputFile.Append(SEXTEXT("ss.AddNativeCall(ns, Native%s%s, nullptr, SEXTEXT(\"%s"), ns.CompressedName(), fname->Buffer, fname->Buffer);

	int outputPos = GetOutputPosition(functionDef);
	for (int i = 1; i < outputPos - 1; i++)
	{
		cr_sex arg = functionDef.GetElement(i);
		AppendInputPair(outputFile, arg, pc);
	}

	outputFile.Append(SEXTEXT(" -> "));
	
	cr_sex outputDef = functionDef.GetElement(outputPos);

	if (IsCompound(outputDef))
	{
		AppendOutputPair(outputFile, outputDef, pc);
	}

	outputFile.Append(SEXTEXT("\"));"));
}

void ParseFunctions(cr_sex functionSetDef, const ParseContext& pc)
{
	if (functionSetDef.NumberOfElements() < 3)
	{
		Throw(functionSetDef, SEXTEXT("Expecting (functions <namespace> <file-prefix> ...)"));
	}

	cr_sex sNS = functionSetDef.GetElement(1);
	if (!IsAtomic(sNS))
	{
		Throw(sNS, SEXTEXT("Expecting atomic namespace expression"));
	}

	ValidateFQSexyInterface(sNS);	
	sexstring ns = sNS.String();
	
	cr_sex sFilePrefix = functionSetDef.GetElement(2);
	if (!IsStringLiteral(sFilePrefix))
	{
		Throw(sNS, SEXTEXT("Expecting string literal for the file prefix"));
	}

	sexstring filePrefix = sFilePrefix.String();

	SEXCHAR sexyFile[_MAX_PATH];
	StringPrint(sexyFile, _MAX_PATH, SEXTEXT("%s%s.inl"), pc.cppRoot, filePrefix->Buffer);

	FileDeleteOnceOnly(sexyFile);

	FileAppender sexyAppender(sexyFile);

	sexyAppender.Append(SEXTEXT("namespace\n{\n"));

	for (int i = 3; i < functionSetDef.NumberOfElements(); ++i)
	{
		cr_sex functionDef = functionSetDef.GetElement(i);
		if (!IsCompound(functionDef))
		{
			Throw(functionDef, SEXTEXT("Expecting function def of form (<sexy-function-name> (input1)...(inputN)->(output): <cpp-function-name>)"));
		}

		AppendNativeFunction(functionDef, ns, pc, sexyAppender);
	}

	sexyAppender.Append(SEXTEXT("}\n\n"));

	SEXCHAR* nsBuffer = (SEXCHAR*)alloca(sizeof(SEXCHAR)* (StringLength(ns->Buffer) + 1));
	CopyString(nsBuffer, StringLength(ns->Buffer) + 1, ns->Buffer);

	SEXCHAR* token = nullptr;
	SEXCHAR* context = nullptr;

#ifndef SEXCHAR_IS_WIDE
# define Tokenize strtok_s
#else
# define Tokenize wcstok_s
#endif
	token = Tokenize(nsBuffer, SEXTEXT("."), &context);

	int namespaceDepth = 0;

	while (token)
	{
		namespaceDepth++;
		sexyAppender.Append(SEXTEXT("namespace %s { "), token);
		token = Tokenize(nullptr, SEXTEXT("."), &context);
	}

	CppType nsType;
	nsType.Set(ns->Buffer);

	sexyAppender.Append(SEXTEXT("\n\tvoid AddNativeCalls_%s(Sexy::Script::IPublicScriptSystem& ss, void* nullContext = nullptr)\n"), nsType.CompressedName());
	sexyAppender.Append(SEXTEXT("\t{\n"));

	sexyAppender.Append(SEXTEXT("\t\tconst INamespace& ns = ss.AddNativeNamespace(SEXTEXT(\"%s\"));\n"), nsType.SexyName());

	for (int i = 3; i < functionSetDef.NumberOfElements(); ++i)
	{
		cr_sex functionDef = functionSetDef.GetElement(i);
		sexyAppender.Append(SEXTEXT("\t\t"));
		AppendNativeRegistration(functionDef, nsType, pc, sexyAppender);
		sexyAppender.Append(SEXTEXT("\n"));
	}

	sexyAppender.Append(SEXTEXT("\t}\n"));

	while (namespaceDepth)
	{
		namespaceDepth--;
		sexyAppender.Append(SEXTEXT("}"));
	}
}

void ParseInterface(cr_sex interfaceDef, const ParseContext& pc)
{
	InterfaceContext ic;
	const ISExpression* methods = NULL;

	bool hasDestructor = false;

	for(int i = 1; i < interfaceDef.NumberOfElements(); ++i)
	{
		cr_sex directive = interfaceDef.GetElement(i);
		if (!IsCompound(directive)) Throw(directive, SEXTEXT("Expecting compound expression in the interface definition"));

		cr_sex directiveName = directive.GetElement(0);
		if (!IsAtomic(directiveName))  Throw(directiveName, SEXTEXT("Expecting atomic expression in the directive name at position #0"));

		const sexstring ssname = directiveName.String();

		if (AreEqual(ssname, SEXTEXT("as.both")))
		{
			if (directive.NumberOfElements() != 3) Throw(directive, SEXTEXT("Expecting (as.both <fully qualified interface name> \"<target-filename-prefix>\")"));
			cr_sex sinterfaceName = directive.GetElement(1);
			ValidateFQSexyInterface(sinterfaceName); // sexy namespaces are more stringent than those of C++. so we use sexy validation for both

			if (ic.asSexyInterface[0] != 0) Throw(directive, SEXTEXT("as.sxy is already defined for this interface"));
			if (ic.asCppInterface.SexyName()[0] != 0) Throw(directive, SEXTEXT("as.cpp is already defined for this interface"));

			cr_sex ssexyFilename = directive.GetElement(2);
			if (!IsStringLiteral(ssexyFilename)) Throw(ssexyFilename, SEXTEXT("Expecting string literal target-prefix. Filenames can potentially have spaces in them."));

			csexstr sexyFilename = ssexyFilename.String()->Buffer;

			StringPrint(ic.appendCppHeaderFile, _MAX_PATH, SEXTEXT("%s%s.sxh.h"), pc.cppRoot, sexyFilename);
			StringPrint(ic.appendCppImplFile, _MAX_PATH, SEXTEXT("%s%s.sxh.inl"), pc.cppRoot, sexyFilename);

			ic.asCppInterface.Set(sinterfaceName.String()->Buffer);
			CopyString(ic.asSexyInterface, InterfaceContext::MAX_TOKEN_LEN, sinterfaceName.String()->Buffer);
			StringPrint(ic.appendSexyFile, _MAX_PATH, SEXTEXT("%s%s.sxh.sxy"), pc.cppRoot, ssexyFilename.String()->Buffer);
		}
		else if (AreEqual(ssname, SEXTEXT("as.sxy")))
		{
			if (directive.NumberOfElements() != 3) Throw(directive, SEXTEXT("Expecting (as.sxy <fully qualified interface name> \"<target-filename>\")"));
			cr_sex sinterfaceName = directive.GetElement(1);
			ValidateFQSexyInterface(sinterfaceName);

			if (ic.asSexyInterface[0] != 0) Throw(directive, SEXTEXT("as.sxy is already defined for this interface"));

			cr_sex ssexyFilename = directive.GetElement(2);
			if (!IsStringLiteral(ssexyFilename)) Throw(ssexyFilename,  SEXTEXT("Expecting string literal"));
			CopyString(ic.asSexyInterface, InterfaceContext::MAX_TOKEN_LEN, sinterfaceName.String()->Buffer);
			StringPrint(ic.appendSexyFile, _MAX_PATH, SEXTEXT("%s%s.sxh.sxy"), pc.cppRoot, ssexyFilename.String()->Buffer);
		}
		else if (AreEqual(ssname, SEXTEXT("as.cpp")))
		{
			if (directive.NumberOfElements() != 3) Throw(directive, SEXTEXT("Expecting (as.cpp <fully qualified interface name> \"<target-filename-sans-extension>\")"));
			cr_sex sstructName = directive.GetElement(1);
			ValidateFQCppStruct(sstructName);

			if (ic.asCppInterface.SexyName()[0] != 0) Throw(directive, SEXTEXT("as.cpp is already defined for this interface"));

			cr_sex ssexyFilename = directive.GetElement(2);
			if (!IsStringLiteral(ssexyFilename)) Throw(ssexyFilename,  SEXTEXT("Expecting string literal"));

			csexstr sexyFilename = ssexyFilename.String()->Buffer;

			StringPrint(ic.appendCppHeaderFile, _MAX_PATH, SEXTEXT("%s%s.sxh.h"), pc.cppRoot, sexyFilename);
			StringPrint(ic.appendCppImplFile, _MAX_PATH, SEXTEXT("%s%s.sxh.inl"), pc.cppRoot, sexyFilename);

			ic.asCppInterface.Set(sstructName.String()->Buffer);
		}
		else if (AreEqual(ssname, SEXTEXT("methods")))
		{
			if (methods != NULL) Throw(directive, SEXTEXT("Duplicate methods directive"));
			methods = &directive;
		}
		else if (AreEqual(ssname, SEXTEXT("~")))
		{
			if (ic.hasDestructor) { Throw(directive, SEXTEXT("Interface already has destructor defined"));	}
			ic.hasDestructor = true;
			if (directive.NumberOfElements() != 1) Throw(directive, SEXTEXT("Destructors take no arguments"));
		}
		else if (AreEqual(ssname, SEXTEXT("factory")))
		{
			cr_sex sfactory = directive;
			if (sfactory.NumberOfElements() < 2 || !IsAtomic(sfactory.GetElement(1)))
			{
				Throw(sfactory, SEXTEXT("Expecting (factory <factory-name> (optional input arg1) ... (optional input argN)"));
			}
			ic.factories.push_back(&sfactory);
		}
		else if (AreEqual(ssname, SEXTEXT("context")))
		{
			if (directive.NumberOfElements() != 3)
			{
				Throw(directive, SEXTEXT("Expecting (context <'factory'|'api'> <context-type>). E.g (context factory Sys.Animal)"));
			}

			cr_sex sroute = directive.GetElement(1);
			cr_sex stype = directive.GetElement(2);
			if (!IsAtomic(sroute)) Throw(stype, SEXTEXT("Expecting atomic value for <'factory'|'api'>"));

			csexstr route = sroute.String()->Buffer;
			if (AreEqual(route, SEXTEXT("factory")))
			{
				ic.isSingleton = false;
			}
			else if (AreEqual(route, SEXTEXT("api")))
			{
				ic.isSingleton = true;
			}
			else
			{
				Throw(sroute, SEXTEXT("Expecting either 'factory' or 'api' sans quotes. 'factory' is used when the factory call determines the underlying C++ object. 'api' is used when the native registration functions determine the underlying C++ object."));
			}

			if (!IsAtomic(stype)) Throw(stype, SEXTEXT("Expecting atomic value for <context-type>"));
			ic.nceContext.Set(stype.String()->Buffer);
		}
		else
		{
			Throw(directiveName, SEXTEXT("Unknown directive in interface definition. Expecting one of (as.sxy, as.cpp, as.both, context, factory, methods)"));
		}		
	}

	if (ic.asCppInterface.SexyName()[0] == 0)
	{
		Throw(interfaceDef, SEXTEXT("Missing as.cpp or as.both"));
	}

	if (ic.asSexyInterface[0] == 0)
	{
		Throw(interfaceDef, SEXTEXT("Missing as.sxy or as.both"));
	}

	if (ic.appendSexyFile[0] == 0) StringPrint(ic.appendSexyFile, _MAX_PATH, SEXTEXT("%s.sxh.sxy"), pc.scriptInputSansExtension);

	if (ic.appendCppHeaderFile[0] == 0) 
	{
		StringPrint(ic.appendCppHeaderFile, _MAX_PATH, SEXTEXT("%s%s.sxh.h"), pc.cppRoot, pc.scriptName);
		StringPrint(ic.appendCppImplFile, _MAX_PATH, SEXTEXT("%s%s.sxh.cpp"), pc.cppRoot, pc.scriptName);
	}

	if (ic.factories.empty())
	{
		Throw(interfaceDef, SEXTEXT("Interface needs to specify at least one factory"));
	}

	if (!ic.nceContext.SexyName()[0])
	{
		Throw(interfaceDef, SEXTEXT("Missing (context <'factory'|'api'> <context-type>)"));
	}
	
	try
	{
		GenerateFiles(pc, ic, interfaceDef, methods, interfaceDef);
	}
	catch(OS::OSException&)
	{
		WriteToStandardOutput(SEXTEXT("Error in interface defintion %s: %d.%d to %d.%d\n"), pc.scriptInput, interfaceDef.Start().X, interfaceDef.Start().Y, interfaceDef.End().X, interfaceDef.End().Y);
		throw;
	}
}

void ParseInterfaceFile(cr_sex root, ParseContext& pc)
{
	bool hasConfig = false;

	for(int i = 0; i < root.NumberOfElements(); ++i)
	{
		cr_sex topLevelItem = root.GetElement(i);

		if (!IsCompound(topLevelItem)) Throw(topLevelItem, SEXTEXT("Expecting compound expression in the root expression"));
		cr_sex command = topLevelItem.GetElement(0);
		if (!IsAtomic(command))  Throw(command, SEXTEXT("Expecting atomic expression in the command at position #0"));

		sexstring cmd = command.String();

		if (AreEqual(SEXTEXT("config"), cmd))
		{
			if (hasConfig) Throw(command, SEXTEXT("Only one config entry permitted"));
			ParseConfigSpec(topLevelItem, pc);
			hasConfig = true;
		}
		else if (AreEqual(SEXTEXT("functions"), cmd))
		{
			if (!hasConfig) Throw(command, SEXTEXT("Must define a (config <config-path>) entry before all functions"));
			ParseFunctions(topLevelItem, pc);
		}
		else if (AreEqual(SEXTEXT("interface"), cmd))
		{
			if (!hasConfig) Throw(command, SEXTEXT("Must define a (config <config-path>) entry before all interfaces"));
			ParseInterface(topLevelItem, pc);
		}
		else
		{
			Throw(command, SEXTEXT("Expecting 'interface or config or functions' in the command at position #0"));
		}		
	}
}

void StripToFilenameSansExtension(csexstr name, SEXCHAR target[_MAX_PATH], SEXCHAR nameSansExtension[_MAX_PATH])
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

	if (scriptModifiedAt < touchModifiedAt)
	{
		printf("%s was last updated before %s\n", scriptInput, touchFile);
		return 0;
	}

	ParseContext pc = {0};
	CopyCharToSEXCHAR(pc.projectRoot, projectRoot, _MAX_PATH);
	CopyCharToSEXCHAR(pc.scriptInput, scriptInput, _MAX_PATH);
	
	StripToFilenameSansExtension(pc.scriptInput, pc.scriptName, pc.scriptInputSansExtension);

	if (*pc.scriptName == 0)
	{
		WriteToStandardOutput(SEXTEXT("Unexpected error. Could not derive script name from script-input-file '%S'. Expecting [...filename.sxy]"), scriptInput);
		return -1;
	}
		
	CSParserProxy spp;
	
	try
	{
		Auto<ISourceCode> src = spp->LoadSource(pc.scriptInput, SourcePos(1,1));
		Auto<ISParserTree> tree = spp->CreateTree(src());

		if (tree->Root().NumberOfElements() == 0)
		{
			PrintUsage();
			Throw(tree->Root(), SEXTEXT("The source code is blank"));
		}

		ParseInterfaceFile(tree->Root(), pc);
	}
	catch(ParseException& ex)
	{
		WriteToStandardOutput(SEXTEXT("%s. %s\nSpecimen: %s.\nPosition: %d.%d to %d.%d\n"), pc.scriptInput, ex.Message(), ex.Specimen(), ex.Start().X, ex.Start().Y, ex.End().X, ex.End().Y);

		if (ex.ErrorCode() != 0)
		{
			WriteStandardErrorCode(ex.ErrorCode());
		}
		return -1;
	}
	catch(IException& iex)
	{
		WriteToStandardOutput(SEXTEXT("Error with bennyhill: %s"), iex.Message());

		if (iex.ErrorCode() != 0)
		{
			WriteStandardErrorCode(iex.ErrorCode());
		}
		return -1;
	}

	return 0;
}