#include "bennyhill.test.stdafx.h"

#include "sexy.types.h"
#include "sexy.strings.h"
#include "sexy.stdstrings.h"

#include <stdio.h>
#include <tchar.h>
#include <sstream>
#include <stdarg.h>

#include "sexy.s-parser.h"
#include "sexy.lib.s-parser.h"
#include "sexy.lib.util.h"

#include <rococo.win32.target.win7.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define validate(_Expression) if (!(_Expression)) { ShowFailure(#_Expression, __FILE__, __LINE__); Abort(); }

#define TEST(test) Test(#test, test)

typedef void (*FN_TEST)();

using namespace Sexy;
using namespace Sexy::Sex;

namespace
{
	void ShowFailure(const char* expression, const char* filename, int lineNumber)
	{
		printf("Validation failed in %s[%d]: %s\r\n", filename, lineNumber, expression);
	}

	void Abort()
	{
		if (IsDebuggerPresent())
			__debugbreak();
		else
			exit(-1); 
	}

	struct IExecuteNotifier
	{
		virtual void OnOutput(const char* outputText) = 0;
		virtual void OnError(const char* errorText) = 0;
	};

	class Win32Pipe
	{
	private:
		HANDLE hReader;
		HANDLE hWriter;

	public:
		Win32Pipe()
		{
			SECURITY_ATTRIBUTES secattr; 
			ZeroMemory(&secattr,sizeof(secattr));
			secattr.nLength = sizeof(secattr);
			secattr.bInheritHandle = TRUE;

			validate(CreatePipe(&hReader, &hWriter, &secattr, 0));
		}

		HANDLE Writer() { return hWriter; }
		HANDLE Reader() { return hReader; }

		DWORD GetBytesAvailable()
		{
			DWORD available;
			return PeekNamedPipe(hReader, NULL, 0, NULL, &available, NULL) ? available : 0;
		}

		DWORD Read(char* buffer, DWORD capacity)
		{
			DWORD bytesRead;
			if (!ReadFile(hReader, buffer, capacity, &bytesRead, 0)) return 0;
			return bytesRead;
		}

		~Win32Pipe()
		{
			CloseHandle(hReader);
			CloseHandle(hWriter);
		}
	};

	bool ExecuteProtected(Win32Pipe& procOutput, Win32Pipe& procError, PROCESS_INFORMATION& processInfo, IExecuteNotifier& notifier)
	{
		HANDLE waitHandles[3];
		waitHandles[0] = processInfo.hProcess;
		waitHandles[1] = procOutput.Writer();
		waitHandles[2] = procError.Writer();
			  
		while (true)
		{
			DWORD bytesAvailable;

			DWORD result = WaitForMultipleObjects(3, waitHandles, FALSE, 60000L);

			while((bytesAvailable = procOutput.GetBytesAvailable()) > 0)
			{
				char lineOutput[4096];
				DWORD bytesRead = procOutput.Read(lineOutput, sizeof(lineOutput)-1);
				lineOutput[bytesRead]= 0;

				notifier.OnOutput(lineOutput);
			}
			while((bytesAvailable = procError.GetBytesAvailable()) > 0)
			{
				char lineErr[4096];
				DWORD bytesRead = procError.Read(lineErr, sizeof(lineErr)-1);
				lineErr[bytesRead]= 0;

				notifier.OnError(lineErr);
			}

			// Process is done, or we timed out:
			if(result == WAIT_OBJECT_0 || result == WAIT_TIMEOUT)
			break;
		}        

		DWORD exitCode;
		if (!GetExitCodeProcess(processInfo.hProcess, &exitCode))
		{
			exitCode = GetLastError();
		}

		return exitCode == 0;
	}

	void Execute(const char* commandLine, IExecuteNotifier& notifier)
	{
		Win32Pipe procOutput;
		Win32Pipe procError;

		PROCESS_INFORMATION processInfo;
		STARTUPINFO startup = {0};
		startup.dwFlags = STARTF_USESTDHANDLES;
		startup.cb = sizeof(STARTUPINFO);
		startup.hStdOutput = procOutput.Writer();
		startup.hStdError = procError.Writer();

		if (!CreateProcessA(NULL, (char*) commandLine, NULL, NULL, TRUE, 0, NULL, NULL, (STARTUPINFOA*) &startup, &processInfo))
		{
			int nErr = GetLastError();
			char err[256];
			FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, nErr, 0, err, sizeof(err), NULL);
			printf("Error %d: %s\nCommand line:\n\t%s\n", nErr, err, commandLine);
			Abort();
		}
		
		bool isOK = ExecuteProtected(procOutput, procError, processInfo, notifier);

		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);

		validate(isOK);
	}

	void Test(const char* name, FN_TEST fnTest)
	{
		printf("<<<<<< %s\r\n\n", name);
		
		fnTest();
		
		printf("\n%s >>>>>>\r\n\r\n", name);
	}

	struct ISexCallback
	{
		virtual void OnSex(cr_sex s) = 0;
	};

	void TestExecuteBennyHill(const char* bennyExe, const char* sexyRoot, const char* inputFilename, ISexCallback& cb)
	{
		char project[256];
		sprintf_s(project, 256, "%ssexy.bennyhill.test\\tests\\", sexyRoot);

		char cmd[2048];
		sprintf_s(cmd, 2048, "%s %s %s%s null", bennyExe, project, project, inputFilename);

		struct ANON: public IExecuteNotifier
		{
			virtual void OnOutput(const char* outputText)
			{
				puts(outputText);
			}

			virtual void OnError(const char* errorText)
			{
				printf("Error: %s", errorText);
				validate(false);
			}
		}anon;
		Execute(cmd, anon);
	}

	void Throw(ParseException& ex)
	{	
		if (IsDebuggerPresent())
		{
			__debugbreak();
		}
		throw ex;
	}

	void Throw(cr_sex e, csexstr message)
	{
		SEXCHAR specimen[64];
		GetSpecimen(specimen, e);
		ParseException ex(e.Start(), e.End(), e.Tree().Source().Name(), message, specimen, &e);
		Throw(ex);
	}

	void ValidateEquivalentSExpression(cr_sex target, cr_sex source)
	{
		if (target.Type() != source.Type())
		{
			Throw(target, SEXTEXT("Mismatch between source and target expression type"));
		}

		switch(target.Type())
		{
		case Sex::EXPRESSION_TYPE_COMPOUND:
			if (target.NumberOfElements() != source.NumberOfElements())
			{
				Throw(target, SEXTEXT("Mismatch between source and target element count"));
			}

			for(int i = 0; i < target.NumberOfElements(); ++i)
			{
				cr_sex t = target.GetElement(i);
				cr_sex s = source.GetElement(i);

				ValidateEquivalentSExpression(t, s);
			}
			break;
		case Sex::EXPRESSION_TYPE_ATOMIC:
		case Sex::EXPRESSION_TYPE_STRING_LITERAL:
			if (!AreEqual(target.String()->Buffer, source.String()->Buffer))
			{
				Throw(target, SEXTEXT("Mismatch between source and target atomic token"));
			}
			break;
		default: // Null expression
			break;
		}
	}

	void ValidateEquivalentSExpression(csexstr target, cr_sex s)
	{
		CSParserProxy spp;

		try
		{
			Auto<ISourceCode> src = spp().ProxySourceBuffer(target, -1, SourcePos(0,0), SEXTEXT("target"));
			Auto<ISParserTree> tree = spp().CreateTree(src());

			ValidateEquivalentSExpression(tree().Root(), s);
		}
		catch(ParseException& ex)
		{
			PrintToStandardOutput(SEXTEXT("Error matching s-expression to target: %s, %s"), ex.Message(), ex.Specimen());
			validate(false);
		}
		catch(IException& iex)
		{
			PrintToStandardOutput(SEXTEXT("Error: %s"), iex.Message());
			validate(false);
		}
	}

	char s_bennyExe[256];
	char s_sexyRoot[256];

	void TestParseNullInterface()
	{
		struct ANON: public ISexCallback
		{
			virtual void OnSex(cr_sex s)
			{
				csexstr expected = SEXTEXT("(interface Sys.Animals.ITiger)(class ProxyITiger (implements Sys.Animals.ITiger) (Pointer hObject))");
				ValidateEquivalentSExpression(expected, s);
			}
		} anon;
		TestExecuteBennyHill(s_bennyExe, s_sexyRoot, "nullinterface.sxh", anon);
	}

	void TestParseSimplestMethod()
	{
		struct ANON: public ISexCallback
		{
			virtual void OnSex(cr_sex s)
			{
				csexstr expected = SEXTEXT("(interface Sys.Animals.ITiger(Awaken -> )) (class ProxyITiger (implements Sys.Animals.ITiger) (Pointer hObject)) (method ProxyITiger.Awaken -> : (Sys.Animals.Native.ITigerAwaken this.hObject))");
				ValidateEquivalentSExpression(expected, s);
			}
		} anon;
		TestExecuteBennyHill(s_bennyExe, s_sexyRoot, "simplestmethod.sxh", anon);
	}

	void TestParseSetterMethod()
	{
		struct ANON: public ISexCallback
		{
			virtual void OnSex(cr_sex s)
			{
				csexstr expected = SEXTEXT("(interface Sys.Animals.ITiger (Eat (Int32 beefKilos) -> )) (class ProxyITiger (implements Sys.Animals.ITiger) (Pointer hObject)) (method ProxyITiger.Eat (Int32 beefKilos) -> : (Sys.Animals.Native.ITigerEat this.hObject beefKilos ))");
				ValidateEquivalentSExpression(expected, s);
			}
		} anon;
		TestExecuteBennyHill(s_bennyExe, s_sexyRoot, "settermethod.sxh", anon);
	}	

	void TestParseGetterMethod()
	{
		struct ANON: public ISexCallback
		{
			virtual void OnSex(cr_sex s)
			{
				csexstr expected = SEXTEXT("(interface Sys.Animals.ITiger (GetId  -> (Int32 id))) (class ProxyITiger (implements Sys.Animals.ITiger) (Pointer hObject))	(method ProxyITiger.GetId -> (Int32 id) : (Sys.Animals.Native.ITigerGetId this.hObject -> id ))");
				ValidateEquivalentSExpression(expected, s);
			}
		} anon;
		TestExecuteBennyHill(s_bennyExe, s_sexyRoot, "gettermethod.sxh", anon);
	}	

	void TestParseMethod1v1()
	{
		struct ANON: public ISexCallback
		{
			virtual void OnSex(cr_sex s)
			{
				csexstr expected = SEXTEXT("(interface Sys.Animals.ITiger (Advance (Float32 dt) -> (Int32 id))) (class ProxyITiger (implements Sys.Animals.ITiger) (Pointer hObject)) (method ProxyITiger.Advance (Float32 dt)-> (Int32 id) : (Sys.Animals.Native.ITigerAdvance this.hObject dt -> id ))");
				ValidateEquivalentSExpression(expected, s);
			}
		} anon;
		TestExecuteBennyHill(s_bennyExe, s_sexyRoot, "method1v1.sxh", anon);
	}	

	void TestParseMethod3v3()
	{
		struct ANON: public ISexCallback
		{
			virtual void OnSex(cr_sex s)
			{
				csexstr expected = SEXTEXT("(interface Sys.Animals.ITiger(Grab (Bool x)(Int32 y)(Int64 z) -> (Float32 u)(Float64 v)(Vec3 s)))(class ProxyITiger (implements Sys.Animals.ITiger) (Pointer hObject))(method ProxyITiger.Grab (Bool x)(Int32 y)(Int64 z)-> (Float32 u)(Float64 v)(Vec3 s) : (Sys.Animals.Native.ITigerGrab this.hObject x y z -> u v s ))");
				ValidateEquivalentSExpression(expected, s);
			}
		} anon;
		TestExecuteBennyHill(s_bennyExe, s_sexyRoot, "method3v3.sxh", anon);
	}	

	void TestParseStringMethod1()
	{
		struct ANON: public ISexCallback
		{
			virtual void OnSex(cr_sex s)
			{
				csexstr expected = SEXTEXT("(interface Sys.Animals.ITiger(Write (IString text) -> ) )(class ProxyITiger (implements Sys.Animals.ITiger) (Pointer hObject))(method ProxyITiger.Write (IString text)->  : (Sys.Animals.Native.ITigerWrite this.hObject text))");
				ValidateEquivalentSExpression(expected, s);
			}
		} anon;
		TestExecuteBennyHill(s_bennyExe, s_sexyRoot, "stringmethod1.sxh", anon);
	}	

	void TestParseEnum()
	{
		struct ANON : public ISexCallback
		{
			virtual void OnSex(cr_sex s)
			{
				csexstr expected = SEXTEXT("(interface Sys.Animals.ITiger(Write (IString text) -> ) )(class ProxyITiger (implements Sys.Animals.ITiger) (Pointer hObject))(method ProxyITiger.Write (IString text)->  : (Sys.Animals.Native.ITigerWrite this.hObject text))");
				ValidateEquivalentSExpression(expected, s);
			}
		} anon;
		TestExecuteBennyHill(s_bennyExe, s_sexyRoot, "parseenum.sxh", anon);
	}
}

void TruncateDirectory(char* fullpath)
{
	int32 len = (int32) strlen(fullpath);

	for (int i = len; i >= 0; i--)
	{
		if (fullpath[i] == '\\')
		{
			fullpath[i] = 0;
			break;
		}
	}
}

int main(int argc, char* argv[])
{
	const char* testExe = argv[0];

	strcpy_s(s_bennyExe, testExe);
	strcpy_s(s_sexyRoot, testExe);

	TruncateDirectory(s_bennyExe);
	strcat_s(s_bennyExe, "\\sexy.bennyhill.exe");

	TruncateDirectory(s_sexyRoot);
	TruncateDirectory(s_sexyRoot); // takes us to the bin directory
	TruncateDirectory(s_sexyRoot); // takes us to the sexy directory

	strcat_s(s_sexyRoot, "\\");

	TEST(TestParseNullInterface);
	TEST(TestParseSimplestMethod);
	TEST(TestParseSetterMethod);
	TEST(TestParseGetterMethod);
	TEST(TestParseMethod1v1);
	TEST(TestParseMethod3v3);
	TEST(TestParseStringMethod1); 
	TEST(TestParseEnum);
	
	return 0;
}

