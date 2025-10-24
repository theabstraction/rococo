#include "sexy.script.stdafx.h"

#include <Sexy.S-Parser.h>
#include <sexy.vm.cpu.h>
#include <sexy.compiler.public.h>
#include <sexy.debug.types.h>
#include <sexy.script.h>

#include <rococo.io.h>
#include <rococo.visitors.h>
#include <rococo.hashtable.h>

#include <stdarg.h>
#include <string>
#include <sexy.vector.h>
#include <rococo.ide.h>
#include <rococo.os.h>
#include <rococo.sexy.api.h>
#include <rococo.package.h>
#include <rococo.debugging.h>
#include <rococo.maths.h>

#include <rococo.try-catch.native.h>

using namespace Rococo;
using namespace Rococo::Compiler;
using namespace Rococo::Strings;
using namespace Rococo::Sex;
using namespace Rococo::Script;
using namespace Rococo::VM;
using namespace Rococo::Visitors;
using namespace Rococo::Debugger;

namespace Rococo::Script
{
	size_t GetAlignmentPadding(int alignment, int objectSize);
	
	SCRIPTEXPORT_API bool IsIString(const IObjectInterface& i)
	{
		if (Eq(i.NullObjectType().Name(), "_Null_Sys_Type_IString"))
		{
			return true;
		}

		auto* base = i.Base();
		if (!base)
		{
			return false;
		}

		return IsIString(*base);
	}

	SCRIPTEXPORT_API bool IsIString(const IStructure& typeDesc)
	{
		if (typeDesc.InterfaceCount() == 1)
		{
			return IsIString(typeDesc.GetInterface(0));
		}

		return false;
	}
}

namespace Rococo
{
	void UpdateDebugger(Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger, Rococo::int32 stackDepth, bool refreshAll);
}

namespace Rococo::Memory
{
	ROCOCO_API [[nodiscard]] IAllocator& CheckedAllocator();
	ROCOCO_API [[nodiscard]] IAllocatorSupervisor* CreateBlockAllocator(size_t kilobytes, size_t maxkilobytes, const char* const name);
}

namespace Rococo
{
	SCRIPTEXPORT_API void ThrowSex(cr_sex s, cstr format, ...);
	
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

		float value = 0;
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
		int32 value = 0;
		if (!IsAtomic(s) || Parse::PARSERESULT_GOOD != Parse::TryParseHex(value, s.c_str()))
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

		int32 value = 0;
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

	void ScanExpression(cr_sex sExpr, cstr hint, cstr format, ...)
	{
		va_list args;
		va_start(args, format);

		int nElements = sExpr.NumberOfElements();

		int elementIndex = 0;
		for (const char* p = format; *p != 0; ++p)
		{
			if (*p == 'a')
			{
				if (elementIndex >= nElements)
				{
					ThrowSex(sExpr, "Too few elements in expression. Format is : %s", hint);
				}

				const ISExpression** ppExpr = va_arg(args, const ISExpression**);
				*ppExpr = &sExpr[elementIndex++];

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
					ThrowSex(sExpr, "Too few elements in expression. Format is : %s", hint);
				}

				const ISExpression** ppExpr = va_arg(args, const ISExpression**);
				*ppExpr = &sExpr[elementIndex++];

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

	fstring GetAtomicArg(cr_sex s)
	{
		if (!IsAtomic(s)) ThrowSex(s, "Expecting atomic argument");
		auto st = s.String();
		return fstring{ st->Buffer, st->Length };
	}

	void PrintExpression(cr_sex s, int& totalOutput, int maxOutput, ILogger& logger)
	{
		switch (s.Type())
		{
		case EXPRESSION_TYPE_ATOMIC:
			totalOutput += logger.Log(" %s", (cstr)s.c_str());
			break;
		case EXPRESSION_TYPE_STRING_LITERAL:
			totalOutput += logger.Log(" \"%s\"", (cstr)s.c_str());
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

	SCRIPTEXPORT_API void LogParseException(ParseException& ex, IDebuggerWindow& debugger, bool jitCompileException)
	{
		Vec2i a = ex.Start();
		Vec2i b = ex.End();

		debugger.AddLogSection(RGBAb(128, 0, 0), "ParseException\n");
		debugger.AddLogSection(RGBAb(64, 64, 64), " Name: ");
		debugger.AddLogSection(RGBAb(0, 0, 128), "%s\n", ex.Name());

		debugger.AddLogSection(RGBAb(64, 64, 64), " Message: ");
		debugger.AddLogSection(RGBAb(0, 0, 128), "%s\n", ex.Message());

		debugger.AddLogSection(RGBAb(64, 64, 64), " Specimen: (Line ");
		debugger.AddLogSection(RGBAb(0, 0, 128), "%d", a.y);
		debugger.AddLogSection(RGBAb(64, 64, 64), ", pos ");
		debugger.AddLogSection(RGBAb(0, 0, 128), "%d", a.x);
		debugger.AddLogSection(RGBAb(64, 64, 64), ") to (Line ");
		debugger.AddLogSection(RGBAb(0, 0, 128), "%d", b.y);
		debugger.AddLogSection(RGBAb(64, 64, 64), ", pos ");
		debugger.AddLogSection(RGBAb(0, 0, 128), "%d", b.x);
		debugger.AddLogSection(RGBAb(64, 64, 64), ")\n");

		debugger.AddLogSection(RGBAb(0, 127, 0), "%s\n", ex.Specimen());

		a = a - Vec2i{ 1, 0 };
		b = b - Vec2i{ 1, 0 };

		debugger.SetCodeHilight(ex.Name(), a, b, ex.Message(), jitCompileException);

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

		int depth = 0;
		for (const ISExpression* s = ex.Source(); s != NULL; s = s->GetOriginal())
		{
			if (depth++ > 0)  debugger.Log("Macro expansion %d:\n", depth);

			int totalOutput = 0;
			PrintExpression(*s, totalOutput, 1024, subLogger);

			debugger.Log("%s", subLogger.buf);

			subLogger.ClearLog();

			debugger.Log("\n");
		}
	}

	SCRIPTEXPORT_API IScriptEnumerator* NoImplicitIncludes()
	{
		struct NONE : IScriptEnumerator
		{
			size_t Count() const override
			{
				return 0;

			};

			cstr ResourceName(size_t index) const override
			{
				return nullptr;
			}
		};

		static NONE none;
		return &none;
	}
}
