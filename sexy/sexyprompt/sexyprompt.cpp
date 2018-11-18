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

// sexyprompt.cpp : Defines the entry point for the console application.

#include "stdafx.h"
#include "sexy.types.h"
#include "sexy.strings.h"

#ifdef char_IS_WIDE
# define _UNICODE
# define UNICODE
#endif

#include "sexy.debug.types.h"
#include "sexy.compiler.public.h"
#include "sexy.script.h"
#include "sexy.s-parser.h"

#include "sexy.vm.h"

#include <stdio.h>
#include <tchar.h>

#include <rococo.win32.target.win7.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <intrin.h>
#include <memory.h>
#include <vector>

#include <string.h>
#include "sexy.stdstrings.h"
#include "sexy.apps.logging.inl"

#include "sexy.lib.util.h"

#include <rococo.api.h>

using namespace Rococo;
using namespace Rococo::Sex;
using namespace Rococo::Script;
using namespace Rococo::Compiler;

void PrintHelp()
{
	WriteToStandardOutput(("Usage: SEXYPROMPT [options] [files]:\r\n\r\n"));
	WriteToStandardOutput(("\tOptions::\r\n\r\n"));
	WriteToStandardOutput(("\t\t-?                 show help\r\n"));
	WriteToStandardOutput(("\t\t-version           show version\r\n"));
	WriteToStandardOutput(("\t\t-banner            show banner\r\n"));
	WriteToStandardOutput(("\t\t-lib:[libfilename] open s-expression of files \r\n"));
	WriteToStandardOutput(("\t\t-verbose           toggle verbosity\r\n"));
	WriteToStandardOutput(("\r\n"));
}

void PrintVersion()
{
	int versionMajor = 1;
	int versionMinor = 0;
	WriteToStandardOutput(("Version: 1.2.0.0\r\n"));
}

void PrintBanner()
{
	WriteToStandardOutput(("SexyPrompt by 19th Century Software. Copyright(c)2012. All rights reserved.\r\n"));
	WriteToStandardOutput(("Masterminded and programmed by Mark Anthony Taylor.\r\n"));
	WriteToStandardOutput(("This software is NOT free!\r\n"));
	WriteToStandardOutput(("If by a pigeon's coo thou art subdued I'll sing thee lullabies.\r\n"));
	WriteToStandardOutput(("Compiled %hs %hs\r\n"), __DATE__, __TIME__);
}

void charToAscii(char* dest, size_t maxLen, cstr source)
{
	char* d = dest;
	cstr s = source;

	for(size_t i = 0; i < maxLen-1; ++i)
	{
		char c = *s++;
		*d++ =	c < 256 ? (c & 0xFF): '?';

		if (c == 0) return;
	}

	dest[maxLen-1] = 0;
}

void UnicodeTochar(char* dest, size_t maxLen, cstr source)
{
	char* d = dest;
	cstr s = source;

	for(size_t i = 0; i < maxLen-1; ++i)
	{
		char c = *s++;
		if (c > 127) c = '?';
		*d++ = (char) c;

		if (c == 0) return;
	}

	dest[maxLen-1] = 0;
}

void AsciiTochar(char* dest, size_t maxLen, const char* source)
{
	char* d = dest;
	const char* s = source;

	for(size_t i = 0; i < maxLen-1; ++i)
	{
		char c = *s++;
		*d++ = c;

		if (c == 0) return;
	}

	dest[maxLen-1] = 0;
}

struct FileData
{
	ISourceCode* SC;
	ISParserTree* Tree;
	stdstring Filename;
};

typedef std::vector<FileData> TFiles;

class CStandardFile
{
private:
	FILE* fp;
	char asciiPath[_MAX_PATH];

public:
	CStandardFile(cstr filename, const char* mode): fp(NULL)
	{
		charToAscii(asciiPath, _MAX_PATH, filename);
		
		int err = fopen_s(&fp, asciiPath, mode);
		if (err != 0) Throw(err, "Cannot open file");
	}

	void Throw(const char* hint)
	{
		Throw(errno, hint);
	}

	void Throw(int errCode, const char* hint)
	{
		char msg[1024];
		char errBuffer[256];
      OS::Format_C_Error(errCode, errBuffer, sizeof(errBuffer));
		sprintf_s(msg, 1024, "Error %d - %s: %s: '%s'\r\n",  errCode, errBuffer, hint, asciiPath);
		throw std::runtime_error(msg);
	}

	long GetLengthAndReset()
	{
		fseek(fp, 0, SEEK_END);
		long len = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		if (len < 0) Throw("ftell(...) failed");

		return len;
	}

	long ReadAll(char* buffer, long bufferLen)
	{
		long startPoint = 0;
		while(bufferLen - startPoint > 0)
		{
			size_t bytesRead = fread(buffer + startPoint, 1, bufferLen - startPoint, fp);
			if (bytesRead == 0) // EOF
			{
				break;
			}
			else if (bytesRead == -1)
			{
				 Throw("fread(...) failed");
			}
			startPoint += (long) bytesRead;
		}

		return startPoint;
	}

	~CStandardFile()
	{
		if (fp == NULL)
		{
			fclose(fp);
		}
	}
};

struct CBuffer
{
	char* data;
	size_t capacity;
	long length;

	CBuffer(size_t len): capacity(len)
	{
		data = new char[len];
	}

	~CBuffer()
	{
		delete[] data;
	}

	bool IsUnicode() const
	{
		return ((data[0] == 0 || data[1] == 0) && length >= 2 && (length % 2) == 0);
	}
};

struct CSexBuffer
{
	char* data;
	size_t capacity;
	long length;

	CSexBuffer(size_t len): capacity(len)
	{
		data = new char[len];
	}

	~CSexBuffer()
	{
		delete[] data;
	}
};

void LoadLibFile(TFiles& files, cstr libFile, IPublicScriptSystem& ss, bool isVerbose)
{
	CStandardFile stdLibFile(libFile, "r");

	long len = stdLibFile.GetLengthAndReset();
		
	if (len > 1024 * 1024) stdLibFile.Throw(E2BIG, "The library file was over a megabyte. Unbelievable!");

	CBuffer rawLibCode(len+2);
	rawLibCode.length = stdLibFile.ReadAll(rawLibCode.data, len);
	rawLibCode.data[rawLibCode.length] = 0;
	rawLibCode.data[rawLibCode.length+1] = 0;

	ISourceCode* sc;
#ifdef char_IS_WIDE
	if (rawLibCode.IsUnicode())
	{
		sc = ss.SParser().ProxySourceBuffer((cstr)rawLibCode.data, len / 2, Vec2i{ 0,0 }, libFile);
	}
	else
	{
		CSexBuffer sexLibData(len+2);
		
		AsciiTochar(sexLibData.data, len+1, rawLibCode.data);
		sexLibData.length = len;

		sc = ss.SParser().ProxySourceBuffer(sexLibData.data, sexLibData.length, Vec2i{ 0,0 }, libFile);
	}
#else
	if (rawLibCode.IsUnicode())
	{
		CSexBuffer sexLibData(len+2);
		UnicodeTochar(sexLibData.data, rawLibCode.length, (cstr) rawLibCode.data);
		sc = ss.SParser().ProxySourceBuffer(sexLibData.data, sexLibData.length, Vec2i{ 0,0 }, libFile);
	}
	else
	{
		sc = ss.SParser().ProxySourceBuffer(rawLibCode.data, rawLibCode.length, Vec2i{ 0,0 }, libFile);
	}
#endif
	
	try
	{
		Auto<ISParserTree> tree(ss.SParser().CreateTree(*sc));

		int nElements = tree().Root().NumberOfElements();
		for(int i = 0; i < nElements; ++i)
		{
			cr_sex s = tree().Root().GetElement(i);
			if (IsStringLiteral(s) || IsAtomic(s))
			{
				cstr filename = s.String()->Buffer;

				FileData data;
				data.Filename = filename;
				data.SC = NULL;
				data.Tree = NULL;
				files.push_back(data);
			}
		}
	}
	catch(ParseException& ex)
	{
		PrintParseException(ex);
		sc->Release();
		exit(-1);
	}

	sc->Release();
}

static bool isVerbose = false;

void PreProcessArgs(TFiles& files, int argc, char* argv[], IPublicScriptSystem& ss)
{
	if (argc == 1)
	{
		PrintBanner();
		PrintVersion();
		PrintHelp();
		exit(0);
	}

	for(int i = 1; i < argc; i++)
	{
		cstr value = argv[i];
		if (AreEqual(value, ("-?"))) 
		{
			PrintHelp();
			continue;
		}

		if (AreEqual(value, ("-version")))
		{
			PrintVersion();
			continue;
		}

		if (AreEqual(value, ("-banner")))
		{
			PrintBanner();
			continue;
		}

		if (AreEqual(value, ("-verbose")))
		{
			isVerbose = !isVerbose;
			continue;
		}

		if (AreEqual(value, ("-lib:"), 5))
		{
			cstr libFile = value + 5;

			if (isVerbose) WriteToStandardOutput(("Loading entries from library file: %s\r\n"), libFile);
			LoadLibFile(files, libFile, ss, isVerbose);

			continue;
		}

		FileData data;
		data.Filename = value;
		data.SC = NULL;
		data.Tree = NULL;

		files.push_back(data);
	}
}

ISourceCode* AddSourcecodeModule(cstr fileName, IPublicScriptSystem& ss)
{
	CStandardFile srcModuleFile(fileName, "rb");

	long len = srcModuleFile.GetLengthAndReset();
		
	if (len > 128 * 1024 * 1024) srcModuleFile.Throw(E2BIG, "The source file was over 128 megabytes. Unbelievable!");

	CBuffer rawLibCode(len+2);
	rawLibCode.length = srcModuleFile.ReadAll(rawLibCode.data, len);
	rawLibCode.data[rawLibCode.length] = 0;
	rawLibCode.data[rawLibCode.length+1] = 0;

	ISourceCode* sc;
#ifdef char_IS_WIDE
	if (rawLibCode.IsUnicode())
	{
		sc = ss.SParser().ProxySourceBuffer((cstr)rawLibCode.data, len / 2, Vec2i{ 0,0 }, fileName);
	}
	else
	{
		CSexBuffer sexLibData(len+2);
		
		AsciiTochar(sexLibData.data, len+1, rawLibCode.data);
		sexLibData.length = len;

		sc = ss.SParser().ProxySourceBuffer(sexLibData.data, sexLibData.length, Vec2i{ 0,0 }, fileName);
	}
#else
	if (rawLibCode.IsUnicode())
	{
		CSexBuffer sexLibData(len+2);
		UnicodeTochar(sexLibData.data, rawLibCode.length, (cstr) rawLibCode.data);
		sc = ss.SParser().DuplicateSourceBuffer(sexLibData.data, sexLibData.length, Vec2i{ 0,0 }, fileName);
	}
	else
	{
		sc = ss.SParser().DuplicateSourceBuffer(rawLibCode.data, rawLibCode.length, Vec2i{ 0,0 }, fileName);
	}
#endif

	return sc;
}

int ProtectedMain(int argc, char* argv[], TFiles& files, IPublicScriptSystem& ss)
{
	PreProcessArgs(OUT files, argc, argv, ss);

	if (isVerbose) WriteToStandardOutput(("Adding source code modules\r\n"));
	for(TFiles::iterator i = files.begin(); i != files.end(); ++i)
	{		
		cstr fileName = i->Filename.c_str();

		if (isVerbose) WriteToStandardOutput(("%s\r\n"), fileName);
		i->SC = AddSourcecodeModule(fileName, ss);
		i->Tree = ss.SParser().CreateTree(*i->SC);

		ss.AddTree(*i->Tree);
	}

	if (isVerbose) WriteToStandardOutput(("Compiling\r\n"));
	ss.Compile();
	if (isVerbose) WriteToStandardOutput(("Compiled\r\n"));

	const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace(("EntryPoint"));
	if (ns == NULL)
	{
		WriteToStandardOutput(("Cannot find namespace 'EntryPoint' in the source code\r\n"));
		return -1;
	}

	const IFunction* f = ns->FindFunction(("Main"));
	if (f == NULL)
	{
		WriteToStandardOutput(("Cannot find function 'Main' aliased in the namespace 'EntryPoint'\r\n"));
		return -1;
	}

	if (f->NumberOfInputs() != 0)
	{
		WriteToStandardOutput(("EntryPoint.Main had inputs. This program cannot be run from sexyprompt\r\n"));
		return -1;
	}

	if (f->NumberOfOutputs() != 1)
	{
		WriteToStandardOutput(("EntryPoint.Main had other than a single output. This program cannot be run from sexyprompt\r\n"));
		return -1;
	}

	if (f->GetArgument(0).VarType() != VARTYPE_Int32)
	{
		WriteToStandardOutput(("EntryPoint.Main had other than a single output of Int32. This program cannot be run from sexyprompt\r\n"));
		return -1;
	}
	
	ss.PublicProgramObject().SetProgramAndEntryPoint(*f);

	ss.PublicProgramObject().VirtualMachine().Push(0); // allow space for int32 return value

	if (isVerbose) WriteToStandardOutput(("Executing\r\n"));
	EXECUTERESULT result = ss.PublicProgramObject().VirtualMachine().Execute(VM::ExecutionFlags(true, true));
	if (result == EXECUTERESULT_TERMINATED)
	{
		int retCode = ss.PublicProgramObject().VirtualMachine().PopInt32();

		if (isVerbose) WriteToStandardOutput(("Sexy program terminated with exit code %d\r\n"), retCode);

		return retCode;
	}
	else
	{
		WriteToStandardOutput(("Virtual machine prematurely terminated\r\n"));
		return -1;
	}
}

int _tmain(int argc, char* argv[])
{
	CLogger logger;
	ProgramInitParameters pip;
	pip.MaxProgramBytes = 32768;
	CScriptSystemProxy ss(pip, logger);
	TFiles files;

	int result;

	try
	{
		result = ProtectedMain(argc, argv, files, ss());
	}
	catch(std::exception& ex)
	{
		printf("Exception: %s\r\n", ex.what());
		result = errno;
	}
	catch(ParseException& ex)
	{
		PrintParseException(ex);
		return -1;
	}

	for(TFiles::iterator i = files.begin(); i != files.end(); ++i)
	{
		if (i->Tree != NULL) i->Tree->Release();
		if (i->SC != NULL) i->SC->Release();
	}	

	return result;
}


