#define WIN32_LEAN_AND_MEAN

#include "sexy.types.h"
#include "sexy.strings.h"
#include "sexy.compiler.public.h"

#include <stdarg.h>
#include <rococo.win32.target.win7.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <intrin.h>
#include <string.h>

#include "..\stccore\Sexy.Compiler.h"
#include "sexy.lib.compiler.h"
#include "sexy.lib.util.h"

#include "sexy.vm.h"
#include "Sexy.VM.CPU.h"

using namespace Sexy;
using namespace Sexy::Compiler;
using namespace Sexy::VM;
using namespace Sexy::Parse;


#define VALIDATE(_Expression) if (!(_Expression)) { ShowFailure(#_Expression, __FILE__, __LINE__); Stop(); }

namespace
{
	bool AreApproxEqual(double x, double y)
	{
		if (y != 0)
		{
			double delta = 1 - (x/y);
			return delta > -1.0e-13 && delta < 1.0e-13;
		}

		return x == 0;
	}

	bool AreApproxEqual(float x, float y)
	{
		if (y != 0)
		{
			double delta = 1 - (x/y);
			return delta > -1.0e-6 && delta < 1.0e-6;
		}

		return x == 0;
	}

	struct Logger: public ILog
	{
		virtual void Write(csexstr text) 
		{			
			WriteToStandardOutput(text);
		}

		void OnUnhandledException(int errorCode, csexstr exceptionType, csexstr message, void* exceptionInstance) 
		{
			WriteToStandardOutput(SEXTEXT("%s: code %d\nMessage: %s\n"), exceptionType, errorCode, message);
		}

		void OnJITCompileException(Sex::ParseException& ex)
		{
		}
	} s_logger;

	void Stop()
	{
		if (IsDebuggerPresent())
		{
			__debugbreak();
		}
		else
		{
			exit(-1);
		}
	}

	void ShowFailure(const char* expression, const char* filename, int lineNumber)
	{
		printf("Validation failed in %s[%d]: %s\r\n", filename, lineNumber, expression);
	}

	typedef void (*FN_TEST)(IProgramObject& compiler);

	#define WRAP(x) Wrap(#x, x);

	void Wrap(const char* name, FN_TEST test)
	{
		printf("<<<<< %s\r\n", name);
		ProgramInitParameters pip;
		pip.MaxProgramBytes = 32768;
		IProgramObject* po = CreateProgramObject_1_0_0_0(pip, s_logger);
		VALIDATE(po != NULL);

		try
		{
			test(*po);
		}
		catch (STCException& e)
		{
			WriteToStandardOutput(SEXTEXT("Error %d: %s\r\n"), e.Code(), e.Message());
			po->Free();
			exit(-1);
		}
		catch(std::exception& stdex)
		{
#ifdef SEXCHAR_IS_WIDE
			WriteToStandardOutput(SEXTEXT("Error: %S\r\n"), stdex.what());
#else
			WriteToStandardOutput(SEXTEXT("Error: %s\r\n"), stdex.what());
#endif
			po->Free();
			exit(-1);
		}
		
		po->Free();
		printf("%s >>>>>\r\n\r\n", name);
	}

	IFunctionBuilder& AddStandardTestStuff(IProgramObject& object)
	{
		INamespaceBuilder& sa = object.GetRootNamespace().AddNamespace(SEXTEXT("Sys.Testing"), ADDNAMESPACEFLAGS_CREATE_ROOTS);
		INamespaceBuilder& nsSysType = object.GetRootNamespace().AddNamespace(SEXTEXT("Sys.Type"), ADDNAMESPACEFLAGS_CREATE_ROOTS);
		IModuleBuilder& module = object.AddModule(SEXTEXT("test"));
		nsSysType.Alias(SEXTEXT("Float32"),object.AddIntrinsicStruct(SEXTEXT("Float32"), sizeof(float),  VARTYPE_Float32, NULL));
		nsSysType.Alias(SEXTEXT("Float64"),object.AddIntrinsicStruct(SEXTEXT("Float64"), sizeof(double), VARTYPE_Float64, NULL));
		nsSysType.Alias(SEXTEXT("Int32"),  object.AddIntrinsicStruct(SEXTEXT("Int32"), sizeof(int32),  VARTYPE_Int32, NULL));
		nsSysType.Alias(SEXTEXT("Int64"),  object.AddIntrinsicStruct(SEXTEXT("Int64"), sizeof(int64),  VARTYPE_Int64, NULL));

		VALIDATE(object.ResolveDefinitions());

		object.IntrinsicModule().UsePrefix(SEXTEXT("Sys.Type"));

		IFunctionBuilder& f = module.DeclareFunction(FunctionPrototype(SEXTEXT("Test"), false), NULL);
		sa.Alias(f);
		return f;
	}

	void TestModules(IProgramObject& obj)
	{
		IModule& module = obj.AddModule(SEXTEXT("life.sex"));
		VALIDATE(&module != NULL);
		VALIDATE(AreEqual(module.Name(), SEXTEXT("life.sex")));
		VALIDATE(obj.ModuleCount() == 1);
		VALIDATE(&obj.GetModule(0) == &module);
	}

	void TestNamespaces(IProgramObject& obj)
	{
		INamespaceBuilder& root = obj.GetRootNamespace();
		VALIDATE(&root != NULL);
		VALIDATE(root.Name()->Length == 0);
		VALIDATE(root.ChildCount() == 0);

		INamespaceBuilder& child = root.AddNamespace(SEXTEXT("Fish"), ADDNAMESPACEFLAGS_NORMAL);
		VALIDATE(&child != NULL);
		VALIDATE(AreEqual(child.Name(), SEXTEXT("Fish")));
		VALIDATE(root.ChildCount() == 1);
		VALIDATE(&root.GetChild(0) == &child);

		INamespaceBuilder& child3 = root.AddNamespace(SEXTEXT("Fish"), ADDNAMESPACEFLAGS_NORMAL);
		VALIDATE(&child3 == &child);

		INamespaceBuilder& child2 = root.AddNamespace(SEXTEXT("Eggs"), ADDNAMESPACEFLAGS_NORMAL);
		VALIDATE(&child2 != NULL);
		VALIDATE(AreEqual(child2.Name(), SEXTEXT("Eggs")));
		VALIDATE(root.ChildCount() == 2);
		VALIDATE(&root.GetChild(1) == &child2);

		VALIDATE(&child2 == root.FindSubspace(SEXTEXT("Eggs")));

		INamespace& fnc = child.AddNamespace(SEXTEXT("Chips"), ADDNAMESPACEFLAGS_NORMAL);
		VALIDATE(child.Parent() == &root);
		VALIDATE(root.Parent() == NULL);
		VALIDATE(AreEqual(fnc.FullName(), SEXTEXT("Fish.Chips")));

		VALIDATE(&fnc == root.FindSubspace(SEXTEXT("Fish.Chips")));

		INamespaceBuilder& doctorWho = root.AddNamespace(SEXTEXT("Doctor.Who"), ADDNAMESPACEFLAGS_CREATE_ROOTS);
		VALIDATE(&doctorWho != NULL);
		VALIDATE(AreEqual(doctorWho.Name(), SEXTEXT("Who")));
		VALIDATE(AreEqual(doctorWho.Parent()->Name(), SEXTEXT("Doctor")));
		VALIDATE(AreEqual(doctorWho.Parent()->Parent()->Name(), SEXTEXT("")));
		VALIDATE(doctorWho.Parent()->Parent()->Parent() == NULL);

		try
		{
			root.AddNamespace(SEXTEXT(""), ADDNAMESPACEFLAGS_CREATE_ROOTS);
			VALIDATE(false);
		}
		catch (STCException& e)
		{
			WriteToStandardOutput(SEXTEXT("Expected exception: %s\r\n"), e.Message());
			VALIDATE(e.Code() == ERRORCODE_EMPTY_STRING);
			// The test should throw
		}

		try
		{
			root.AddNamespace(NULL, ADDNAMESPACEFLAGS_CREATE_ROOTS);
			VALIDATE(false);
		}
		catch (STCException& e)
		{
			WriteToStandardOutput(SEXTEXT("Expected exception: %s\r\n"), e.Message());
			VALIDATE(e.Code() == ERRORCODE_NULL_POINTER);
			// The test should throw
		}

		try
		{
			csexstr testString = SEXTEXT("2China.Apples.Yip");
			ValidateNamespaceString(testString, SEXTEXT("ValidateNamespaceString"), __SEXFUNCTION__);
			VALIDATE(false);
		}
		catch (STCException& e)
		{
			WriteToStandardOutput(SEXTEXT("Expected exception: %s\r\n"), e.Message());
			VALIDATE(e.Code() == ERRORCODE_BAD_ARGUMENT);
		}

		try
		{
			csexstr testString = SEXTEXT("china.Apples.Yip");
			ValidateNamespaceString(testString, SEXTEXT("ValidateNamespaceString"), __SEXFUNCTION__);
			VALIDATE(false);
		}
		catch (STCException& e)
		{
			WriteToStandardOutput(SEXTEXT("Expected exception: %s\r\n"), e.Message());
			VALIDATE(e.Code() == ERRORCODE_BAD_ARGUMENT);
		}

		try
		{
			csexstr testString = SEXTEXT("China.Apples.Yip.");
			ValidateNamespaceString(testString, SEXTEXT("ValidateNamespaceString"), __SEXFUNCTION__);
			VALIDATE(false);
		}
		catch (STCException& e)
		{
			WriteToStandardOutput(SEXTEXT("Expected exception: %s\r\n"), e.Message());
			VALIDATE(e.Code() == ERRORCODE_BAD_ARGUMENT);
		}

		try
		{
			csexstr testString = SEXTEXT("China.Apples.Yip");
			ValidateNamespaceString(testString, SEXTEXT("ValidateNamespaceString"), __SEXFUNCTION__);
		}
		catch (IException& e)
		{
			WriteToStandardOutput(SEXTEXT("Expected exception: %s\r\n"), e.Message());
			VALIDATE(FALSE);
		}

		try
		{
			root.AddNamespace(SEXTEXT("!fire fox"), ADDNAMESPACEFLAGS_CREATE_ROOTS);
		}
		catch (STCException& e)
		{
			WriteToStandardOutput(SEXTEXT("Expected exception: %s\r\n"), e.Message());
			VALIDATE(e.Code() == ERRORCODE_BAD_ARGUMENT);
		}

		try
		{
			root.FindSubspace(SEXTEXT("!fire fox"));
		}
		catch (STCException& e)
		{
			WriteToStandardOutput(SEXTEXT("Expected exception: %s\r\n"), e.Message());
			VALIDATE(e.Code() == ERRORCODE_BAD_ARGUMENT);
		}
	}

	void TestFunctions(IProgramObject& obj)
	{
		IModuleBuilder& module = obj.AddModule(SEXTEXT("test"));
		INamespaceBuilder& ns = obj.GetRootNamespace().AddNamespace(SEXTEXT("System.Console"), ADDNAMESPACEFLAGS_CREATE_ROOTS);

		FunctionPrototype prototype(SEXTEXT("Print"), false);

		IFunctionBuilder& fn = module.DeclareFunction(prototype, NULL);	

		ns.Alias(fn);
		VALIDATE(ns.FindFunction(SEXTEXT("Print")) == &fn);

		fn.AddInput(NameString::From(SEXTEXT("format")), TypeString::From(SEXTEXT("Sys.Type.String")), NULL);

		VALIDATE(!obj.ResolveDefinitions());
	}

	void TestStructures(IProgramObject& object)
	{
		IModuleBuilder& module = object.AddModule(SEXTEXT("test"));
		try
		{
			module.UsePrefix(SEXTEXT("Sys.Type.Complex"));
		}
		catch (IException& e)
		{
			WriteToStandardOutput(SEXTEXT("Expected exception: %s\r\n"), e.Message());
		}
		
		INamespaceBuilder& nsSysType = object.GetRootNamespace().AddNamespace(SEXTEXT("Sys.Type"), ADDNAMESPACEFLAGS_CREATE_ROOTS);

		StructurePrototype pt(MEMBERALIGN_4, INSTANCEALIGN_SSE, true, NULL, false);
		IStructureBuilder& s = module.DeclareStructure(SEXTEXT("Vector3"), pt, NULL);
		VALIDATE(AreEqual(s.Name(),SEXTEXT("Vector3")));
		VALIDATE(&s.Module() == &module);
		VALIDATE(&s.Object() == &object);

		VALIDATE(s.MemberCount() == 0);
		s.AddMember(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Float32")));
		s.AddMember(NameString::From(SEXTEXT("y")), TypeString::From(SEXTEXT("Float32")));
		s.AddMember(NameString::From(SEXTEXT("z")), TypeString::From(SEXTEXT("Float32")));

		try
		{
			s.AddMember(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Float32")));
			VALIDATE(false);
		}
		catch (STCException&e)
		{
#ifdef SEXCHAR_IS_WIDE
			printf("Expected exception: %S\r\n", e.Message());
#else
			printf("Expected exception: %s\r\n", e.Message());
#endif
			VALIDATE(e.Code() == ERRORCODE_BAD_ARGUMENT);
		}

		s.Seal();
		VALIDATE(s.MemberCount() == 3);

		try
		{
			s.AddMember(NameString::From(SEXTEXT("W")), TypeString::From(SEXTEXT("Float32")));
			VALIDATE(false);
		}
		catch (STCException&e)
		{
			WriteToStandardOutput(SEXTEXT("Expected exception: %s\r\n"), e.Message());
			VALIDATE(e.Code() == ERRORCODE_SEALED);
		}

		VALIDATE(!object.ResolveDefinitions());
		nsSysType.Alias(SEXTEXT("Float32"), object.AddIntrinsicStruct(SEXTEXT("Float32"), 4, VARTYPE_Float32, NULL));
		VALIDATE(object.ResolveDefinitions());

		IMember& mx = s.GetMember(0);
		VALIDATE(AreEqual(mx.Name(), SEXTEXT("x")));
		VALIDATE(mx.IsResolved());
		
		const IStructure* mxtype = mx.UnderlyingType();
		VALIDATE(mxtype != NULL);
		VALIDATE(mxtype->SizeOfStruct() == 4);
		VALIDATE(mxtype->VarType() == VARTYPE_Float32);
		VALIDATE(s.VarType() == VARTYPE_Derivative);

		VALIDATE(s.SizeOfStruct() == 12);

		StructurePrototype ptm(MEMBERALIGN_4, INSTANCEALIGN_SSE, true, NULL, false);
		IStructureBuilder& sm = module.DeclareStructure(SEXTEXT("Matrix3x3"), ptm, NULL);
		sm.AddMember(NameString::From(SEXTEXT("Row0")), TypeString::From(SEXTEXT("Vector3")));
		sm.AddMember(NameString::From(SEXTEXT("Row1")), TypeString::From(SEXTEXT("Vector3")));
		sm.AddMember(NameString::From(SEXTEXT("Row2")), TypeString::From(SEXTEXT("Vector3")));
		sm.Seal();

		StructurePrototype ptms(MEMBERALIGN_4, INSTANCEALIGN_SSE, true, NULL, false);
		IStructureBuilder& sms = module.DeclareStructure(SEXTEXT("Matrix3x3Stack"), ptms, NULL);
		sms.AddMember(NameString::From(SEXTEXT("Item0")), TypeString::From(SEXTEXT("Matrix3x3")));
		sms.AddMember(NameString::From(SEXTEXT("Item1")), TypeString::From(SEXTEXT("Matrix3x3")));

		try
		{
			sms.AddMember(NameString::From(SEXTEXT("Item1")), TypeString::From(SEXTEXT("Matrix3x3")));
			VALIDATE(false);
		}
		catch (STCException& e)
		{
			WriteToStandardOutput(SEXTEXT("Expected exception: %s\r\n"), e.Message());
			VALIDATE(e.Code() == ERRORCODE_BAD_ARGUMENT);
		}

		sms.AddMember(NameString::From(SEXTEXT("Item2")), TypeString::From(SEXTEXT("Matrix3x3")));
		
		sms.Seal();

		object.ResolveDefinitions();

		VALIDATE(sm.SizeOfStruct() == 36);
	}

	void TestResolvedStructureAndFunction(IProgramObject& object)
	{
		IModuleBuilder& module = object.AddModule(SEXTEXT("test"));
		INamespaceBuilder& nsSysType = object.GetRootNamespace().AddNamespace(SEXTEXT("Sys.Type"), ADDNAMESPACEFLAGS_CREATE_ROOTS);
		IFunctionBuilder& f = module.DeclareFunction(FunctionPrototype(SEXTEXT("increment"), false), NULL);
		nsSysType.Alias(SEXTEXT("Int32"), object.AddIntrinsicStruct(SEXTEXT("Int32"), 4, VARTYPE_Int32, NULL));
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Int32")), NULL);
		object.IntrinsicModule().UsePrefix(SEXTEXT("Sys.Type"));
		VALIDATE(object.ResolveDefinitions() == true);
	}

	void TestParseringDecimal32(IProgramObject& object)
	{
		int32 value;
		VALIDATE(TryParseDecimal(OUT value, SEXTEXT("42")) == PARSERESULT_GOOD);
		VALIDATE(value == 42);

		VALIDATE(TryParseDecimal(OUT value, SEXTEXT("300000000000")) == PARSERESULT_OVERFLOW);
		VALIDATE(TryParseDecimal(OUT value, SEXTEXT("-300000000000")) == PARSERESULT_OVERFLOW);

		VALIDATE(TryParseDecimal(OUT value, SEXTEXT("+42")) == PARSERESULT_GOOD);
		VALIDATE(value == 42);

		VALIDATE(TryParseDecimal(OUT value, SEXTEXT("-42")) == PARSERESULT_GOOD);
		VALIDATE(value == -42);

		#pragma warning(disable: 4146) // Bad compiler warning in VS2010 for valid numeric input
			VALIDATE(TryParseDecimal(OUT value, SEXTEXT("-2147483648")) == PARSERESULT_GOOD);
			VALIDATE(value == -2147483648); // N.B picked up by some compilers (such as MS-VC) as an error, but it is -2^31, which translates to the -2147483648 decimal string for a signed 32-bit number

			VALIDATE(TryParseDecimal(OUT value, SEXTEXT("2147483648")) == PARSERESULT_OVERFLOW);

			VALIDATE(TryParseDecimal(OUT value, SEXTEXT("-2147483647")) == PARSERESULT_GOOD);
			VALIDATE(value == -2147483647);

			VALIDATE(TryParseDecimal(OUT value, SEXTEXT("2147483647")) == PARSERESULT_GOOD);
			VALIDATE(value == 2147483647);
		#pragma warning(default: 4146)

		VALIDATE(TryParseDecimal(OUT value, SEXTEXT("-2147483649")) == PARSERESULT_OVERFLOW);
		VALIDATE(TryParseDecimal(OUT value, SEXTEXT("2147483649")) == PARSERESULT_OVERFLOW);
	}

	void TestParseringDecimal64(IProgramObject& object)
	{
		int64 value;
		VALIDATE(TryParseDecimal(OUT value, SEXTEXT("42")) == PARSERESULT_GOOD);
		VALIDATE(value == 42);

		VALIDATE(TryParseDecimal(OUT value, SEXTEXT("cheese")) == PARSERESULT_BAD_DECIMAL_DIGIT);

		VALIDATE(TryParseDecimal(OUT value, SEXTEXT("9223372036854775812")) == PARSERESULT_OVERFLOW);
		VALIDATE(TryParseDecimal(OUT value, SEXTEXT("-9223372036854775812")) == PARSERESULT_OVERFLOW);

		VALIDATE(TryParseDecimal(OUT value, SEXTEXT("+42")) == PARSERESULT_GOOD);
		VALIDATE(value == 42);

		VALIDATE(TryParseDecimal(OUT value, SEXTEXT("-42")) == PARSERESULT_GOOD);
		VALIDATE(value == -42);

		#pragma warning(disable: 4146) // Bad compiler warning in VS2010 for valid numeric input
			VALIDATE(TryParseDecimal(OUT value, SEXTEXT("-9223372036854775808")) == PARSERESULT_GOOD);
			VALIDATE(value == -9223372036854775808); // N.B picked up by some compilers (such as MS-VC) as an error, but it -2^63, which translates to the -9223372036854775808 decimal string for a signed 32-bit number
		#pragma warning(default: 4146) // Bad compiler warning in VS2010 for valid numeric input

		VALIDATE(TryParseDecimal(OUT value, SEXTEXT("9223372036854775808")) == PARSERESULT_OVERFLOW);

		VALIDATE(TryParseDecimal(OUT value, SEXTEXT("-9223372036854775807")) == PARSERESULT_GOOD);
		VALIDATE(value == -9223372036854775807);

		VALIDATE(TryParseDecimal(OUT value, SEXTEXT("9223372036854775807")) == PARSERESULT_GOOD);
		VALIDATE(value == 9223372036854775807);

		VALIDATE(TryParseDecimal(OUT value, SEXTEXT("-9223372036854775809")) == PARSERESULT_OVERFLOW);
		VALIDATE(TryParseDecimal(OUT value, SEXTEXT("9223372036854775809")) == PARSERESULT_OVERFLOW);
	}

	void TestParseringHex32(IProgramObject& object)
	{
		int32 value;
		VALIDATE(TryParseHex(value, SEXTEXT("ABCDEF10")) == PARSERESULT_GOOD);
		VALIDATE(value == 0xABCDEF10);
		VALIDATE(TryParseHex(value, SEXTEXT("chav")) == PARSERESULT_HEXADECIMAL_BAD_CHARACTER);
		VALIDATE(TryParseHex(value, SEXTEXT("")) == PARSERESULT_HEXADECIMAL_INCORRECT_NUMBER_OF_DIGITS);
		VALIDATE(TryParseHex(value, SEXTEXT("ABCDEF10C")) == PARSERESULT_HEXADECIMAL_INCORRECT_NUMBER_OF_DIGITS);
		VALIDATE(TryParseHex(value, SEXTEXT("ABCD")) == PARSERESULT_GOOD);
		VALIDATE(value == 0xABCD);
	}

	void TestParseringHex64(IProgramObject& object)
	{
		int64 value;
		VALIDATE(TryParseHex(value, SEXTEXT("ABCDEF10")) == PARSERESULT_GOOD);
		VALIDATE(value == 0xABCDEF10);
		VALIDATE(TryParseHex(value, SEXTEXT("chav")) == PARSERESULT_HEXADECIMAL_BAD_CHARACTER);
		VALIDATE(TryParseHex(value, SEXTEXT("")) == PARSERESULT_HEXADECIMAL_INCORRECT_NUMBER_OF_DIGITS);
		VALIDATE(TryParseHex(value, SEXTEXT("ABCDEF10123456789")) == PARSERESULT_HEXADECIMAL_INCORRECT_NUMBER_OF_DIGITS);
		VALIDATE(TryParseHex(value, SEXTEXT("ABCD")) == PARSERESULT_GOOD);
		VALIDATE(value == 0xABCD);
	}

	void TestParseVariant(IProgramObject& object)
	{
		VariantValue value;
		VALIDATE(PARSERESULT_GOOD == TryParse(OUT value, VARTYPE_Int32, SEXTEXT("0xFABC")));
		VALIDATE(value.int32Value == 0xFABC);

		VALIDATE(PARSERESULT_GOOD == TryParse(OUT value, VARTYPE_Int64, SEXTEXT("0xFABCDE")));
		VALIDATE(value.int64Value == 0xFABCDE);

		VALIDATE(PARSERESULT_GOOD == TryParse(OUT value, VARTYPE_Int32, SEXTEXT("1756")));
		VALIDATE(value.int32Value == 1756);

		VALIDATE(PARSERESULT_GOOD == TryParse(OUT value, VARTYPE_Int64, SEXTEXT("-768")));
		VALIDATE(value.int64Value == -768);

		VALIDATE(PARSERESULT_HEX_FOR_FLOAT == TryParse(OUT value, VARTYPE_Float32, SEXTEXT("0xAABB")));
		VALIDATE(PARSERESULT_HEX_FOR_FLOAT == TryParse(OUT value, VARTYPE_Float64, SEXTEXT("0xAABB")));

		VALIDATE(TryParse(OUT value, VARTYPE_Float32, SEXTEXT("1.65")) == PARSERESULT_GOOD);

		VALIDATE(value.floatValue == 1.65f);

		VALIDATE(TryParse(OUT value, VARTYPE_Float64, SEXTEXT("1.6532")) == PARSERESULT_GOOD);
		VALIDATE(value.doubleValue == 1.6532);

		VALIDATE(GetLiteralType(SEXTEXT("0xFFEE")) == VARTYPE_Int32);
		VALIDATE(GetLiteralType(SEXTEXT("0xFFEX")) == VARTYPE_Bad);
		VALIDATE(GetLiteralType(SEXTEXT("0.055")) == VARTYPE_Float32);
		VALIDATE(GetLiteralType(SEXTEXT("-0-26")) == VARTYPE_Bad);
		VALIDATE(GetLiteralType(SEXTEXT("apple")) == VARTYPE_Bad);
		VALIDATE(GetLiteralType(SEXTEXT("-1E-2"))== VARTYPE_Float32);
	}

	void TestParseFloat32(IProgramObject& object)
	{
		float32 value;
		VALIDATE(PARSERESULT_GOOD == TryParseFloat(OUT value, SEXTEXT("0.0")));
		VALIDATE(value == 0);

		VALIDATE(PARSERESULT_GOOD == TryParseFloat(OUT value, SEXTEXT("-0.5")));
		VALIDATE(value == -0.5f);

		VALIDATE(PARSERESULT_GOOD == TryParseFloat(OUT value, SEXTEXT("1.5e-14")));
		VALIDATE(AreApproxEqual(value, 1.5e-14f));

		VALIDATE(PARSERESULT_GOOD == TryParseFloat(OUT value, SEXTEXT("-1.5e14")));
		VALIDATE(AreApproxEqual(value, -1.5e14f));

		VALIDATE(TryParseFloat(OUT value, SEXTEXT("-1.5e40")) == PARSERESULT_OVERFLOW);
	}

	void TestParseFloat64(IProgramObject& object)
	{
		float64 value;
		VALIDATE(PARSERESULT_GOOD == TryParseFloat(OUT value, SEXTEXT("0.0")));
		VALIDATE(value == 0);

		VALIDATE(PARSERESULT_GOOD == TryParseFloat(OUT value, SEXTEXT("-0.5")));
		VALIDATE(value == -0.5);

		VALIDATE(PARSERESULT_GOOD == TryParseFloat(OUT value, SEXTEXT("1.5e-14")));
		VALIDATE(AreApproxEqual(value, 1.5e-14));

		VALIDATE(PARSERESULT_GOOD == TryParseFloat(OUT value, SEXTEXT("-1.5e14")));
		VALIDATE(AreApproxEqual(value, -1.5e14));

		VALIDATE(TryParseFloat(OUT value, SEXTEXT("-1.5e40")) == PARSERESULT_GOOD);

		VALIDATE(TryParseFloat(OUT value, SEXTEXT("-1.5e340")) == PARSERESULT_OVERFLOW);
	}

	bool SetProgramAndEntryPoint(IProgramObject& object, INamespace& ns, csexstr fname)
	{
		const IFunction* f = ns.FindFunction(fname);
		if (f == NULL)
		{
			return false;
		}
		else
		{
			object.SetProgramAndEntryPoint(*f);
			return true;
		}
	}


	void TestCompileAssignLiteral(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignLiteral(NameString::From(SEXTEXT("result")), SEXTEXT("13.5"));
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		float inputValueX = 1.0f;
		object.VirtualMachine().Push(inputValueX); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push((float) 17.25f); // the next thing to do is to allocate space for the outputs by pushing onto the stack
		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		float outputValue = object.VirtualMachine().PopFloat32();
		VALIDATE(outputValue == 13.5f);
	}

	void TestCompileAssignVariableToVariable(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignLiteral(NameString::From(SEXTEXT("x")), SEXTEXT("13.45"));
		builder.AssignVariableToVariable(SEXTEXT("x"), SEXTEXT("result"));
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		float inputValueX = 1.0f;
		object.VirtualMachine().Push(inputValueX); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push((float) 17.25f); // the next thing to do is to allocate space for the outputs by pushing onto the stack
		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		float outputValue = object.VirtualMachine().PopFloat32();
		VALIDATE(outputValue == 13.45f);
	}

	void TestCompileNegateInt32(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From( SEXTEXT("x") ), TypeString::From( SEXTEXT("Sys.Type.Int32") ), NULL);
		f.AddOutput(NameString::From( SEXTEXT("result") ), TypeString::From( SEXTEXT("Sys.Type.Int32") ), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignVariableToTemp(SEXTEXT("x"), 0);
		builder.Negate(0, VARTYPE_Int32);
		builder.AssignTempToVariable(1, SEXTEXT("result"));
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		int32 inputValueX = 132;
		object.VirtualMachine().Push(inputValueX); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push((int32) 17); // the next thing to do is to allocate space for the outputs by pushing onto the stack
		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		int32 outputValue = object.VirtualMachine().PopInt32();
		VALIDATE(outputValue == -132);
	}

	void TestCompileNegateFloat32(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignVariableToTemp(SEXTEXT("x"), 0);
		builder.Negate(0, VARTYPE_Float32);
		builder.AssignTempToVariable(1, SEXTEXT("result"));
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		float inputValueX = 132.7f;
		object.VirtualMachine().Push(inputValueX); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push((float) 17); // the next thing to do is to allocate space for the outputs by pushing onto the stack
		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		float outputValue = object.VirtualMachine().PopFloat32();
		VALIDATE(outputValue == -132.7f);
	}

	void TestCompileNegateFloat64(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Float64")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Float64")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignVariableToTemp(SEXTEXT("x"), 0);
		builder.Negate(0, VARTYPE_Float64);
		builder.AssignTempToVariable(1, SEXTEXT("result"));
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		float64 inputValueX = 132.5;
		object.VirtualMachine().Push(inputValueX); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push((double) 17); // the next thing to do is to allocate space for the outputs by pushing onto the stack
		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		float64 outputValue = object.VirtualMachine().PopFloat64();
		VALIDATE(outputValue == -132.5);
	}

	void TestCompileNegateInt64(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Int64")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Int64")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignVariableToTemp(SEXTEXT("x"), 0);
		builder.Negate(0, VARTYPE_Int64);
		builder.AssignTempToVariable(1, SEXTEXT("result"));
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		int64 inputValueX = -1320;
		object.VirtualMachine().Push(inputValueX); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push((int64) 17); // the next thing to do is to allocate space for the outputs by pushing onto the stack
		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		int64 outputValue = object.VirtualMachine().PopInt64();
		VALIDATE(outputValue == 1320);
	}


	void TestCompileAssignVariableToVariable64(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Float64")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Float64")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignLiteral(NameString::From(SEXTEXT("x")), SEXTEXT("11.43"));
		builder.AssignVariableToVariable(SEXTEXT("x"), SEXTEXT("result"));
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		double inputValueX = 1.0;
		object.VirtualMachine().Push(inputValueX); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push((double) 17.25); // the next thing to do is to allocate space for the outputs by pushing onto the stack
		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		double outputValue = object.VirtualMachine().PopFloat64();
		VALIDATE(outputValue == 11.43);
	}

	void TestCompileAddFloats(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		f.AddInput(NameString::From(SEXTEXT("y")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignVariableToTemp(SEXTEXT("x"), 1);
		builder.AssignVariableToTemp(SEXTEXT("y"), 2);
		builder.BinaryOperatorAdd(1,2,VARTYPE_Float32);
		builder.AssignTempToVariable(0, SEXTEXT("result"));
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		float inputValueX = 12.0f;
		float inputValueY = 13.0f;
		object.VirtualMachine().Push(inputValueX); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push(inputValueY); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push(17.25f); // the next thing to do is to allocate space for the outputs by pushing onto the stack
		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		float outputValue = object.VirtualMachine().PopFloat32();
		VALIDATE(outputValue == 25.0f);
	}

	void TestCompileAddDoubles(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Float64")), NULL);
		f.AddInput(NameString::From(SEXTEXT("y")), TypeString::From(SEXTEXT("Sys.Type.Float64")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Float64")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignVariableToTemp(SEXTEXT("x"), 1);
		builder.AssignVariableToTemp(SEXTEXT("y"), 2);
		builder.BinaryOperatorAdd(1,2,VARTYPE_Float64);
		builder.AssignTempToVariable(0, SEXTEXT("result"));
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		double inputValueX = 13.0;
		double inputValueY = 13.0;
		object.VirtualMachine().Push(inputValueX); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push(inputValueY); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push(17.25); // the next thing to do is to allocate space for the outputs by pushing onto the stack
		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		double outputValue = object.VirtualMachine().PopFloat64();
		VALIDATE(outputValue == 26.0);
	}

	void TestCompileAddInt32s(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Int32")), NULL);
		f.AddInput(NameString::From(SEXTEXT("y")), TypeString::From(SEXTEXT("Sys.Type.Int32")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Int32")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignVariableToTemp(SEXTEXT("x"), 1);
		builder.AssignVariableToTemp(SEXTEXT("y"), 2);
		builder.BinaryOperatorAdd(1,2,VARTYPE_Int32);
		builder.AssignTempToVariable(0, SEXTEXT("result"));
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		int32 inputValueX = 13;
		int32 inputValueY = 14;
		object.VirtualMachine().Push(inputValueX); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push(inputValueY); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push(19); // the next thing to do is to allocate space for the outputs by pushing onto the stack
		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		int32 outputValue = object.VirtualMachine().PopInt32();
		VALIDATE(outputValue == 27);
	}


	void TestCompileAddInt64s(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Int64")), NULL);
		f.AddInput(NameString::From(SEXTEXT("y")), TypeString::From(SEXTEXT("Sys.Type.Int64")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Int64")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignVariableToTemp(SEXTEXT("x"), 1);
		builder.AssignVariableToTemp(SEXTEXT("y"), 2);
		builder.BinaryOperatorAdd(1,2,VARTYPE_Int64);
		builder.AssignTempToVariable(0, SEXTEXT("result"));
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		int64 inputValueX = 12;
		int64 inputValueY = 16;
		object.VirtualMachine().Push(inputValueX); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push(inputValueY); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push((int64)0); // the next thing to do is to allocate space for the outputs by pushing onto the stack
		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		int64 outputValue = object.VirtualMachine().PopInt64();
		VALIDATE(outputValue == 28L);
	}

	void TestCompileSubtractFloats(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		f.AddInput(NameString::From(SEXTEXT("y")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignVariableToTemp(SEXTEXT("x"), 1);
		builder.AssignVariableToTemp(SEXTEXT("y"), 2);
		builder.BinaryOperatorSubtract(1,2,VARTYPE_Float32);
		builder.AssignTempToVariable(0, SEXTEXT("result"));
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		float inputValueX = 12.0f;
		float inputValueY = 13.0f;
		object.VirtualMachine().Push(inputValueX); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push(inputValueY); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push(17.25f); // the next thing to do is to allocate space for the outputs by pushing onto the stack
		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		float outputValue = object.VirtualMachine().PopFloat32();
		VALIDATE(outputValue == -1.0f);
	}

	void TestCompileSubtractDoubles(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Float64")), NULL);
		f.AddInput(NameString::From(SEXTEXT("y")), TypeString::From(SEXTEXT("Sys.Type.Float64")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Float64")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignVariableToTemp(SEXTEXT("x"), 1);
		builder.AssignVariableToTemp(SEXTEXT("y"), 2);
		builder.BinaryOperatorSubtract(1,2,VARTYPE_Float64);
		builder.AssignTempToVariable(0, SEXTEXT("result"));
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		double inputValueX = 13.0;
		double inputValueY = 19.0;
		object.VirtualMachine().Push(inputValueX); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push(inputValueY); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push(17.25); // the next thing to do is to allocate space for the outputs by pushing onto the stack
		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		double outputValue = object.VirtualMachine().PopFloat64();
		VALIDATE(outputValue == -6.0);
	}

	void TestCompileSubtractInt32s(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Int32")), NULL);
		f.AddInput(NameString::From(SEXTEXT("y")), TypeString::From(SEXTEXT("Sys.Type.Int32")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Int32")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignVariableToTemp(SEXTEXT("x"), 1);
		builder.AssignVariableToTemp(SEXTEXT("y"), 2);
		builder.BinaryOperatorSubtract(1,2,VARTYPE_Int32);
		builder.AssignTempToVariable(0, SEXTEXT("result"));
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		int32 inputValueX = 15;
		int32 inputValueY = 8;
		object.VirtualMachine().Push(inputValueX); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push(inputValueY); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push(19); // the next thing to do is to allocate space for the outputs by pushing onto the stack
		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		int32 outputValue = object.VirtualMachine().PopInt32();
		VALIDATE(outputValue == 7);
	}


	void TestCompileSubtractInt64s(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Int64")), NULL);
		f.AddInput(NameString::From(SEXTEXT("y")), TypeString::From(SEXTEXT("Sys.Type.Int64")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Int64")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignVariableToTemp(SEXTEXT("x"), 1);
		builder.AssignVariableToTemp(SEXTEXT("y"), 2);
		builder.BinaryOperatorSubtract(1,2,VARTYPE_Int64);
		builder.AssignTempToVariable(0, SEXTEXT("result"));
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		int64 inputValueX = 12;
		int64 inputValueY = 16;
		object.VirtualMachine().Push(inputValueX); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push(inputValueY); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push((int64)0); // the next thing to do is to allocate space for the outputs by pushing onto the stack
		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		int64 outputValue = object.VirtualMachine().PopInt64();
		VALIDATE(outputValue == -4);
	}

	void TestCompileMultiplyFloats(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		f.AddInput(NameString::From(SEXTEXT("y")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignVariableToTemp(SEXTEXT("x"), 1);
		builder.AssignVariableToTemp(SEXTEXT("y"), 2);
		builder.BinaryOperatorMultiply(1,2,VARTYPE_Float32);
		builder.AssignTempToVariable(0, SEXTEXT("result"));
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		float inputValueX = 12.0f;
		float inputValueY = 13.0f;
		object.VirtualMachine().Push(inputValueX); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push(inputValueY); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push(17.25f); // the next thing to do is to allocate space for the outputs by pushing onto the stack
		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		float outputValue = object.VirtualMachine().PopFloat32();
		VALIDATE(outputValue == 156.0f);
	}

	void TestCompileMultiplyDoubles(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Float64")), NULL);
		f.AddInput(NameString::From(SEXTEXT("y")), TypeString::From(SEXTEXT("Sys.Type.Float64")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Float64")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignVariableToTemp(SEXTEXT("x"), 1);
		builder.AssignVariableToTemp(SEXTEXT("y"), 2);
		builder.BinaryOperatorMultiply(1,2,VARTYPE_Float64);
		builder.AssignTempToVariable(0, SEXTEXT("result"));
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		double inputValueX =  -3.0;
		double inputValueY = -17.0;
		object.VirtualMachine().Push(inputValueX); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push(inputValueY); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push(17.25); // the next thing to do is to allocate space for the outputs by pushing onto the stack
		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		double outputValue = object.VirtualMachine().PopFloat64();
		VALIDATE(outputValue == 51.0);
	}

	void TestCompileMultiplyInt32s(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Int32")), NULL);
		f.AddInput(NameString::From(SEXTEXT("y")), TypeString::From(SEXTEXT("Sys.Type.Int32")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Int32")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignVariableToTemp(SEXTEXT("x"), 1);
		builder.AssignVariableToTemp(SEXTEXT("y"), 2);
		builder.BinaryOperatorMultiply(1,2,VARTYPE_Int32);
		builder.AssignTempToVariable(0, SEXTEXT("result"));
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		int32 inputValueX = 7;
		int32 inputValueY = 8;
		object.VirtualMachine().Push(inputValueX); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push(inputValueY); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push(19); // the next thing to do is to allocate space for the outputs by pushing onto the stack
		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		int32 outputValue = object.VirtualMachine().PopInt32();
		VALIDATE(outputValue == 56);
	}


	void TestCompileMultiplyInt64s(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Int64")), NULL);
		f.AddInput(NameString::From(SEXTEXT("y")), TypeString::From(SEXTEXT("Sys.Type.Int64")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Int64")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignVariableToTemp(SEXTEXT("x"), 1);
		builder.AssignVariableToTemp(SEXTEXT("y"), 2);
		builder.BinaryOperatorMultiply(1,2,VARTYPE_Int64);
		builder.AssignTempToVariable(0, SEXTEXT("result"));
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		int64 inputValueX = -4;
		int64 inputValueY = 16;
		object.VirtualMachine().Push(inputValueX); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push(inputValueY); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push((int64)0); // the next thing to do is to allocate space for the outputs by pushing onto the stack
		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		int64 outputValue = object.VirtualMachine().PopInt64();
		VALIDATE(outputValue == -64);
	}

	void TestCompileDivideFloats(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		f.AddInput(NameString::From(SEXTEXT("y")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignVariableToTemp(SEXTEXT("x"), 1);
		builder.AssignVariableToTemp(SEXTEXT("y"), 2);
		builder.BinaryOperatorDivide(1,2,VARTYPE_Float32);
		builder.AssignTempToVariable(0, SEXTEXT("result"));
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		float inputValueX = 16.0f;
		float inputValueY = -4.0f;
		object.VirtualMachine().Push(inputValueX); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push(inputValueY); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push(17.25f); // the next thing to do is to allocate space for the outputs by pushing onto the stack
		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		float outputValue = object.VirtualMachine().PopFloat32();
		VALIDATE(outputValue == -4.0f);
	}

	void TestCompileDivideDoubles(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Float64")), NULL);
		f.AddInput(NameString::From(SEXTEXT("y")), TypeString::From(SEXTEXT("Sys.Type.Float64")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Float64")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignVariableToTemp(SEXTEXT("x"), 1);
		builder.AssignVariableToTemp(SEXTEXT("y"), 2);
		builder.BinaryOperatorDivide(1,2,VARTYPE_Float64);
		builder.AssignTempToVariable(0, SEXTEXT("result"));
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		double inputValueX =  -8.0;
		double inputValueY = -4.0;
		object.VirtualMachine().Push(inputValueX); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push(inputValueY); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push(17.25); // the next thing to do is to allocate space for the outputs by pushing onto the stack
		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		double outputValue = object.VirtualMachine().PopFloat64();
		VALIDATE(outputValue == 2.0);
	}

	void TestCompileDivideInt32s(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Int32")), NULL);
		f.AddInput(NameString::From(SEXTEXT("y")), TypeString::From(SEXTEXT("Sys.Type.Int32")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Int32")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignVariableToTemp(SEXTEXT("x"), 1);
		builder.AssignVariableToTemp(SEXTEXT("y"), 2);
		builder.BinaryOperatorDivide(1,2,VARTYPE_Int32);
		builder.AssignTempToVariable(0, SEXTEXT("result"));
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		int32 inputValueX = 15;
		int32 inputValueY = 4;
		object.VirtualMachine().Push(inputValueX); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push(inputValueY); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push(19); // the next thing to do is to allocate space for the outputs by pushing onto the stack
		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		int32 outputValue = object.VirtualMachine().PopInt32();
		VALIDATE(outputValue == 3);
	}


	void TestCompileDivideInt64s(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Int64")), NULL);
		f.AddInput(NameString::From(SEXTEXT("y")), TypeString::From(SEXTEXT("Sys.Type.Int64")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Int64")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignVariableToTemp(SEXTEXT("x"), 1);
		builder.AssignVariableToTemp(SEXTEXT("y"), 2);
		builder.BinaryOperatorDivide(1,2,VARTYPE_Int64);
		builder.AssignTempToVariable(0, SEXTEXT("result"));
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		int64 inputValueX = 80;
		int64 inputValueY = 7;
		object.VirtualMachine().Push(inputValueX); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push(inputValueY); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push((int64)0); // the next thing to do is to allocate space for the outputs by pushing onto the stack
		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		int64 outputValue = object.VirtualMachine().PopInt64();
		VALIDATE(outputValue == 11);
	}


	void TestCompileTestAndBranch1(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignVariableToTemp(SEXTEXT("x"), 1);

		struct ANON1: public ICompileSection
		{
			virtual void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* nullData)
			{
				VariantValue v;
				v.floatValue = 16.0f;
				builder.Assembler().Append_SetRegisterImmediate(REGISTER_D6, v, BITCOUNT_32);
				builder.Assembler().Append_FloatSubtract(REGISTER_D5, REGISTER_D6, FLOATSPEC_SINGLE);
			}
		} sectionIfTrue;

		struct ANON2: public ICompileSection
		{
			virtual void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* nullData)
			{
			}
		} sectionElseFalse;

		builder.Assembler().Append_Test(REGISTER_D5, BITCOUNT_32);
		builder.AppendConditional(CONDITION_IF_GREATER_THAN, sectionIfTrue, sectionElseFalse);
		builder.AssignTempToVariable(0, SEXTEXT("result"));
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		float inputValueX = 132.7f;
		object.VirtualMachine().Push(inputValueX); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push((float) 17); // the next thing to do is to allocate space for the outputs by pushing onto the stack
		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		float outputValue = object.VirtualMachine().PopFloat32();
		VALIDATE(outputValue == 116.7f);
	}

	void Disassemble(IAssembler& a)
	{
		IDisassembler* dis = a.Core().CreateDisassembler();

		size_t programLength;
		const uint8* code = a.Program(OUT programLength);
		size_t i = 0;
		while(i < programLength)
		{
			IDisassembler::Rep rep;
			dis->Disassemble(code + i, OUT rep);

			WriteToStandardOutput(SEXTEXT("%s %s\r\n"), rep.OpcodeText, rep.ArgText);

			i += rep.ByteCount;
		}

		dis->Free();
	}

	void TestCompileTestAndBranch2(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignVariableToTemp(SEXTEXT("x"), 1);

		struct ANON1: public ICompileSection
		{
			virtual void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* nullData)
			{
				VariantValue v;
				v.floatValue = 16.0f;
				builder.Assembler().Append_SetRegisterImmediate(REGISTER_D6, v, BITCOUNT_32);
				builder.Assembler().Append_FloatSubtract(REGISTER_D5, REGISTER_D6, FLOATSPEC_SINGLE);
			}
		} sectionIfTrue;

		struct ANON2: public ICompileSection
		{
			virtual void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* nullData)
			{
				VariantValue v;
				v.floatValue = 32.0f;
				builder.Assembler().Append_SetRegisterImmediate(REGISTER_D6, v, BITCOUNT_32);
				builder.Assembler().Append_FloatSubtract(REGISTER_D5, REGISTER_D6, FLOATSPEC_SINGLE);
			}
		} sectionElseFalse;

		builder.Assembler().Append_Test(REGISTER_D5, BITCOUNT_32);
		builder.AppendConditional(CONDITION_IF_LESS_OR_EQUAL, sectionIfTrue, sectionElseFalse);
		builder.AssignTempToVariable(0, SEXTEXT("result"));
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		float inputValueX = 132.7f;
		object.VirtualMachine().Push(inputValueX); // push our inputs on the stack in order of their definition
		object.VirtualMachine().Push((float) 17); // the next thing to do is to allocate space for the outputs by pushing onto the stack
		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		float outputValue = object.VirtualMachine().PopFloat32();
		VALIDATE(outputValue == 100.7f);
	}

	void TestCompileWhileDo(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		f.AddInput(NameString::From(SEXTEXT("count")), TypeString::From(SEXTEXT("Sys.Type.Int32")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignVariableToTemp(SEXTEXT("count"), 1);
		builder.AssignVariableToTemp(SEXTEXT("x"), 3);
		builder.AddVariable(NameString::From(SEXTEXT("sum")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		builder.AssignVariableToTemp(SEXTEXT("sum"), 4);

		VariantValue zero;
		zero.floatValue = 0;
		builder.Assembler().Append_SetRegisterImmediate(REGISTER_D8, zero, BITCOUNT_32);

		struct ANON1: public ICompileSection
		{
			virtual void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* nullData)
			{
				builder.Assembler().Append_Test(REGISTER_D5, BITCOUNT_32);
			}
		} loopCriterion;

		struct ANON2: public ICompileSection
		{
			virtual void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* nullData)
			{
				VariantValue one;
				one.int32Value = 1;
				builder.Assembler().Append_SubtractImmediate(REGISTER_D5, BITCOUNT_32, REGISTER_D5, one);
				builder.Assembler().Append_FloatAdd(REGISTER_D7, REGISTER_D8, FLOATSPEC_SINGLE);
				builder.Assembler().Append_MoveRegister(REGISTER_D6, REGISTER_D8, BITCOUNT_32);
			}
		} loopBody;
		builder.AppendWhileDo(loopCriterion, CONDITION_IF_GREATER_THAN, loopBody);
		builder.AssignTempToVariable(4, SEXTEXT("result"));
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		object.VirtualMachine().Push(0.25f); 
		object.VirtualMachine().Push((int32) 2);
		object.VirtualMachine().Push(0.0f);

		// Disassemble(builder.Assembler());

		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		float outputValue = object.VirtualMachine().PopFloat32();
		VALIDATE(outputValue == 0.5f);
	}

	void TestCompileWhileDo_Break(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		f.AddInput(NameString::From(SEXTEXT("count")), TypeString::From(SEXTEXT("Sys.Type.Int32")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignVariableToTemp(SEXTEXT("count"), 1);
		builder.AssignVariableToTemp(SEXTEXT("x"), 3);
		builder.AddVariable(NameString::From(SEXTEXT("sum")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		builder.AssignVariableToTemp(SEXTEXT("sum"), 4);

		VariantValue zero;
		zero.floatValue = 0;
		builder.Assembler().Append_SetRegisterImmediate(REGISTER_D8, zero, BITCOUNT_32);

		VariantValue v;
		v.floatValue = 0.25f;
		builder.Assembler().Append_SetRegisterImmediate(REGISTER_D9, v, BITCOUNT_32);

		struct ANON1: public ICompileSection
		{
			virtual void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* nullData)
			{
				builder.Assembler().Append_Test(REGISTER_D5, BITCOUNT_32);
			}
		} loopCriterion;

		struct ANON2: public ICompileSection
		{
			virtual void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* cfd)
			{
				VariantValue one;
				one.int32Value = 1;
				builder.Assembler().Append_SubtractImmediate(REGISTER_D5, BITCOUNT_32, REGISTER_D5, one);
				builder.Assembler().Append_FloatAdd(REGISTER_D7, REGISTER_D8, FLOATSPEC_SINGLE);
				builder.Assembler().Append_MoveRegister(REGISTER_D6, REGISTER_D8, BITCOUNT_32);
				builder.Assembler().Append_FloatSubtract(REGISTER_D7, REGISTER_D9, FLOATSPEC_SINGLE);
				builder.Assembler().Append_Test(REGISTER_D6, BITCOUNT_32);
				int32 toBreak = ((int32) cfd->BreakPosition) - ((int32) builder.Assembler().WritePosition());
				builder.Assembler().Append_BranchIf(CONDITION_IF_EQUAL, toBreak);
			}
		} loopBody;
		builder.AppendWhileDo(loopCriterion, CONDITION_IF_GREATER_THAN, loopBody);
		builder.AssignTempToVariable(4, SEXTEXT("result"));
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		object.VirtualMachine().Push(0.25f); 
		object.VirtualMachine().Push((int32) 12);
		object.VirtualMachine().Push(0.0f);

		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		float outputValue = object.VirtualMachine().PopFloat32();
		VALIDATE(outputValue == 0.25f);
	}

	void TestCompileWhileDo_Continue(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		f.AddInput(NameString::From(SEXTEXT("count")), TypeString::From(SEXTEXT("Sys.Type.Int32")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignVariableToTemp(SEXTEXT("count"), 1); // D5 loaded with count
		builder.AssignVariableToTemp(SEXTEXT("x"), 2); // D6 loaded with x

		VariantValue zero;
		zero.floatValue = 0;
		builder.Assembler().Append_SetRegisterImmediate(REGISTER_D10, zero, BITCOUNT_32); // D10, sum, set to 0.0

		struct ANON1: public ICompileSection
		{
			virtual void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* nullData)
			{
				builder.Assembler().Append_Test(REGISTER_D5, BITCOUNT_32); // Test count against zero
			}
		} loopCriterion;

		struct ANON2: public ICompileSection
		{
			virtual void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* cfd)
			{
				VariantValue one;
				one.int32Value = 1;
				builder.Assembler().Append_SubtractImmediate(REGISTER_D5, BITCOUNT_32, REGISTER_D5, one); // count -= 1

				VariantValue two;
				two.int32Value = 2;
				builder.Assembler().Append_SetRegisterImmediate(REGISTER_D12, two, BITCOUNT_32); // D12 = 2
				builder.Assembler().Append_IntSubtract(REGISTER_D12, BITCOUNT_32, REGISTER_D5);  // D11 = 2-count
				builder.Assembler().Append_Test(REGISTER_D11, BITCOUNT_32); // 
				builder.Assembler().Append_BranchIf(CONDITION_IF_EQUAL, (int32) (cfd->ContinuePosition) - (int32) (builder.Assembler().WritePosition()));

				builder.Assembler().Append_FloatAdd(REGISTER_D10, REGISTER_D6, FLOATSPEC_SINGLE);			// D9 = sum + x 
				builder.Assembler().Append_MoveRegister(REGISTER_D9, REGISTER_D10, BITCOUNT_32); // Move D9 to sum
			}
		} loopBody;
		builder.AppendWhileDo(loopCriterion, CONDITION_IF_GREATER_THAN, loopBody);
		builder.AssignTempToVariable(6, SEXTEXT("result")); // save result in call stack
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		object.VirtualMachine().Push(0.25f); 
		object.VirtualMachine().Push((int32) 4);
		object.VirtualMachine().Push(0.0f);

		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		float outputValue = object.VirtualMachine().PopFloat32();
		VALIDATE(outputValue == 0.75f);
	}

	void TestCompileDoWhile(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		f.AddInput(NameString::From(SEXTEXT("count")), TypeString::From(SEXTEXT("Sys.Type.Int32")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignVariableToTemp(SEXTEXT("count"), 0); // D4
		builder.AssignVariableToTemp(SEXTEXT("x"), 1);	    // D5

		VariantValue zero;
		zero.floatValue = 0;
		builder.Assembler().Append_SetRegisterImmediate(REGISTER_D10, zero, BITCOUNT_32); // sum

		struct ANON2: public ICompileSection
		{
			virtual void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* nullData)
			{
				VariantValue one;
				one.int32Value = 1;
				builder.Assembler().Append_SubtractImmediate(REGISTER_D4, BITCOUNT_32, REGISTER_D4, one); // D4 -= 1
				builder.Assembler().Append_FloatAdd(REGISTER_D10, REGISTER_D5, FLOATSPEC_SINGLE); // D9 = D10 + x
				builder.Assembler().Append_MoveRegister(REGISTER_D9, REGISTER_D10, BITCOUNT_32); // sum = D9
			}
		} loopBody;

		struct ANON1: public ICompileSection
		{
			virtual void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* nullData)
			{
				builder.Assembler().Append_Test(REGISTER_D4, BITCOUNT_32);
			}
		} loopCriterion;

		builder.AppendDoWhile(loopBody, loopCriterion, CONDITION_IF_GREATER_THAN);
		builder.AssignTempToVariable(6, SEXTEXT("result")); // result = sum
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		object.VirtualMachine().Push(0.25f); 
		object.VirtualMachine().Push((int32) 12);
		object.VirtualMachine().Push(0.0f);

		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		float outputValue = object.VirtualMachine().PopFloat32();
		VALIDATE(outputValue == 3.0f);
	}

	void TestCompileDoWhile_Break(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		f.AddInput(NameString::From(SEXTEXT("count")), TypeString::From(SEXTEXT("Sys.Type.Int32")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignVariableToTemp(SEXTEXT("count"), 0); // D4 = count
		builder.AssignVariableToTemp(SEXTEXT("x"), 1); // D5 = x

		VariantValue zero;
		zero.floatValue = 0;
		builder.Assembler().Append_SetRegisterImmediate(REGISTER_D10, zero, BITCOUNT_32); // D10 = sum = 0

		struct ANON2: public ICompileSection
		{
			virtual void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* cfd)
			{
				VariantValue one;
				one.int32Value = 1;
				builder.Assembler().Append_SubtractImmediate(REGISTER_D4, BITCOUNT_32, REGISTER_D4, one); // D4 -= 1
				builder.Assembler().Append_FloatAdd(REGISTER_D10, REGISTER_D5, FLOATSPEC_SINGLE); // D9 = sum + x
				builder.Assembler().Append_MoveRegister(REGISTER_D9, REGISTER_D10, BITCOUNT_32); // D10 = sum

				VariantValue six;
				six.int32Value = 6;
				builder.Assembler().Append_SubtractImmediate(REGISTER_D4, BITCOUNT_32, REGISTER_D8, six); // D8 = count - 6
				builder.Assembler().Append_Test(REGISTER_D8, BITCOUNT_32);
				size_t breakCondPos = builder.Assembler().WritePosition();
				builder.Assembler().Append_BranchIf(CONDITION_IF_EQUAL, (int32) cfd->BreakPosition - (int32) breakCondPos);
			}
		} loopBody;

		struct ANON1: public ICompileSection
		{
			virtual void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* nullData)
			{
				builder.Assembler().Append_Test(REGISTER_D4, BITCOUNT_32); // test count against zero
			}
		} loopCriterion;

		builder.AppendDoWhile(loopBody, loopCriterion, CONDITION_IF_GREATER_THAN);
		builder.AssignTempToVariable(6, SEXTEXT("result")); // Copy D10 to result
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		object.VirtualMachine().Push(0.25f); 
		object.VirtualMachine().Push((int32) 12);
		object.VirtualMachine().Push(0.0f);

		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		float outputValue = object.VirtualMachine().PopFloat32();
		VALIDATE(outputValue == 1.5f);
	}

	void TestCompileDoWhile_Continue(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddInput(NameString::From(SEXTEXT("x")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		f.AddInput(NameString::From(SEXTEXT("count")), TypeString::From(SEXTEXT("Sys.Type.Int32")), NULL);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Float32")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();
		builder.AssignVariableToTemp(SEXTEXT("count"), 0); // D4 = count
		builder.AssignVariableToTemp(SEXTEXT("x"), 1); // D5 = x

		VariantValue zero;
		zero.floatValue = 0;
		builder.Assembler().Append_SetRegisterImmediate(REGISTER_D10, zero, BITCOUNT_32); // D10 = sum = 0

		struct ANON2: public ICompileSection
		{
			virtual void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* cfd)
			{
				VariantValue one;
				one.int32Value = 1;
				builder.Assembler().Append_SubtractImmediate(REGISTER_D4, BITCOUNT_32, REGISTER_D4, one); // D4 -= 1
				builder.Assembler().Append_FloatAdd(REGISTER_D10, REGISTER_D5, FLOATSPEC_SINGLE); // D9 = D10 + D5 = sum + x
				
				VariantValue six;
				six.int32Value = 6;
				builder.Assembler().Append_SubtractImmediate(REGISTER_D4, BITCOUNT_32, REGISTER_D8, six); // D8 = D4 - 6
				builder.Assembler().Append_Test(REGISTER_D8, BITCOUNT_32);
				size_t contCondPos = builder.Assembler().WritePosition();
				builder.Assembler().Append_BranchIf(CONDITION_IF_EQUAL, (int32) cfd->ContinuePosition - (int32) contCondPos);
				
				builder.Assembler().Append_MoveRegister(REGISTER_D9, REGISTER_D10, BITCOUNT_32); // D10 = D9
			}
		} loopBody;

		struct ANON1: public ICompileSection
		{
			virtual void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* nullData)
			{
				builder.Assembler().Append_Test(REGISTER_D4, BITCOUNT_32);
			}
		} loopCriterion;

		builder.AppendDoWhile(loopBody, loopCriterion, CONDITION_IF_GREATER_THAN);
		builder.AssignTempToVariable(6, SEXTEXT("result")); // D10 = sum -> result
		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		object.VirtualMachine().Push(0.25f); 
		object.VirtualMachine().Push((int32) 12);
		object.VirtualMachine().Push(0.0f);

		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		float outputValue = object.VirtualMachine().PopFloat32();
		VALIDATE(outputValue == 2.75f);
	}

	void TestCompile_ExpressionTree(IProgramObject& object)
	{
		IFunctionBuilder& f = AddStandardTestStuff(object);
		f.AddOutput(NameString::From(SEXTEXT("result")), TypeString::From(SEXTEXT("Sys.Type.Int32")), NULL);
		VALIDATE(object.ResolveDefinitions());

		ICodeBuilder& builder = f.Builder();
		builder.Begin();

		struct ANON: public IBinaryExpression
		{
			struct ANON1: public IBinaryExpression
			{
				IBinaryExpression* GetLeft()
				{
					return NULL;
				}

				IBinaryExpression* GetRight()
				{
					return NULL;
				}

				void EvaluateBranch(ICodeBuilder& builder, IProgramObject& object, int target)
				{
					VariantValue v;
					v.int32Value = 24;
					builder.Assembler().Append_SetRegisterImmediate(target, v, BITCOUNT_32);
				}
			} lhs;

			struct ANON2: public IBinaryExpression
			{
				IBinaryExpression* GetLeft()
				{
					return NULL;
				}

				IBinaryExpression* GetRight()
				{
					return NULL;
				}

				void EvaluateBranch(ICodeBuilder& builder, IProgramObject& object, int target)
				{
					VariantValue v;
					v.int32Value = 16;
					builder.Assembler().Append_SetRegisterImmediate(target, v, BITCOUNT_32);
				}
			} rhs;

			IBinaryExpression* GetLeft()
			{
				return &lhs;
			}

			IBinaryExpression* GetRight()
			{
				return &rhs;
			}

			void EvaluateBranch(ICodeBuilder& builder, IProgramObject& object, int target)
			{
				builder.Assembler().Append_IntAdd(target+1, BITCOUNT_32, target+2);
			}
		} tree;

		builder.AddExpression(tree);

		builder.AssignTempToVariable(0, SEXTEXT("result"));

		builder.End();

		INamespace* nsSysTesting = object.GetRootNamespace().FindSubspace(SEXTEXT("Sys.Testing"));
		VALIDATE(nsSysTesting != NULL);
		VALIDATE(SetProgramAndEntryPoint(object, *nsSysTesting, SEXTEXT("Test")));

		object.VirtualMachine().Push(0);

		VALIDATE(object.VirtualMachine().Execute(VM::ExecutionFlags(false, true)) == EXECUTERESULT_TERMINATED);
		int outputValue = object.VirtualMachine().PopInt32();
		VALIDATE(outputValue == 40);
	}

	void PresentTests()
	{
		WRAP(TestNamespaces);
		WRAP(TestModules);
		WRAP(TestFunctions);
		WRAP(TestStructures);
		WRAP(TestResolvedStructureAndFunction);
		WRAP(TestParseringDecimal32);
		WRAP(TestParseringDecimal64);
		WRAP(TestParseringHex32);
		WRAP(TestParseringHex64);
		WRAP(TestParseVariant);
		WRAP(TestParseFloat32);
		WRAP(TestParseFloat64);
		WRAP(TestCompileAssignLiteral);
		WRAP(TestCompileAssignVariableToVariable);
		WRAP(TestCompileAssignVariableToVariable64);

		WRAP(TestCompileAddFloats);
		WRAP(TestCompileAddDoubles);
		WRAP(TestCompileAddInt32s);
		WRAP(TestCompileAddInt64s);

		WRAP(TestCompileSubtractFloats);
		WRAP(TestCompileSubtractDoubles);
		WRAP(TestCompileSubtractInt32s);
		WRAP(TestCompileSubtractInt64s);

		WRAP(TestCompileMultiplyFloats);
		WRAP(TestCompileMultiplyDoubles);
		WRAP(TestCompileMultiplyInt32s);
		WRAP(TestCompileMultiplyInt64s);

		WRAP(TestCompileDivideFloats);
		WRAP(TestCompileDivideDoubles);
		WRAP(TestCompileDivideInt32s);
		WRAP(TestCompileDivideInt64s);

		WRAP(TestCompileNegateInt32);
		WRAP(TestCompileNegateInt64);
		WRAP(TestCompileNegateFloat32);
		WRAP(TestCompileNegateFloat64);

		WRAP(TestCompileTestAndBranch1);
		WRAP(TestCompileTestAndBranch2);

		WRAP(TestCompileWhileDo);
		WRAP(TestCompileWhileDo_Break);
		WRAP(TestCompileWhileDo_Continue);

		WRAP(TestCompileDoWhile);
		WRAP(TestCompileDoWhile_Break);
		WRAP(TestCompileDoWhile_Continue);

		WRAP(TestCompile_ExpressionTree);
	}
}

int main(int argc, char* argv)
{
	printf("Sexy Tree Compiler Test Suite\r\n");
	PresentTests();
	printf("All tests complete\r\n");
}