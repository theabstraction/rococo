#pragma once

#include <rococo.types.h>
#include <rococo.strings.h>

using namespace Rococo;

// template escape sequence:
// %% -> expands to %
// %N -> expands to namespace
// %S -> expands to short object name, i.e IStringBuilder is the shortname for Sys.Type.Strings.IStringBuilder
// %s -> expands to short hand of the object name, example SB could be used for IStringBuilder
// %T -> expand to some string specific to the template type
// %U -> expand to some string specific to the template type
// %V -> expand to some string specific to the template type
// %W -> expand to some string specific to the template type

// TODO -> load this in as a file and allow BennyHill some logic to link it to the SXH spec
const char* componentTemplate =
R"((class Proxy%S
	(implements %N.%S)
	(attribute not-serialized)
	(Sys.Type.ComponentRef ref)
	(ROID roid)
)

(method Proxy%S.Construct(ROID id)(Sys.Type.ComponentRef ref) :
	(this.roid = id)
	(this.ref.hComponent = ref.hComponent)
	(this.ref.hLife = ref.hLife)
)

(method Proxy%S.Destruct -> :
	(%N.Native.Destruct%s)
)

(factory %N.Add%s %N.%S (ROID id):
	(Sys.Type.ComponentRef ref)
	(%N.Native.Add%s ref id)

	(if (ref.hComponent == 0)
		(return)
	)

	(construct Proxy%S id ref)
)

(factory %N.Get%s %N.%S (ROID id) :
	(Sys.Type.ComponentRef ref)
	(%N.Native.Get%s ref id)

	(if (ref.hComponent == 0)
		(return)
	)

	(construct Proxy%S id ref)
)

)";

const char* componentTemplateCppImpl =
R"(/* Requires that Add and Get are declared ahead of the INL file.
Typically with '#define DECLARE_SINGLETON_METHODS(COMPONENT_API,COMPONENT) from the <rococo.ecs.ex.h> header
namespace %T
{
	Ref<%W> Add(ROID roid);
	Ref<%W> Get(ROID roid);
}
*/

namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	RefPointers* ReadRefAndId(OUT ROID& id, IN uint8* _sf, IN OUT ptrdiff_t& _offset)
	{
		_offset += sizeof(ROID);
		ReadInput(id, _sf, -_offset);

		Rococo::Components::RefPointers* pOutput;
		_offset += sizeof(pOutput);
		ReadInput(pOutput, _sf, -_offset);

		return pOutput;
	}

	void Add_%sComponent(NativeCallEnvironment& _nce)
	{
		uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);

		ROID roid;
		RefPointers* output = ReadRefAndId(OUT roid, IN _sf, IN OUT _offset);

		auto ref = %T::Add(roid);
		Rococo::Components::AssignRef(*output, ref);
	}

	void Get_%sComponent(NativeCallEnvironment& _nce)
	{
		uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);

		ROID roid;
		RefPointers* output = ReadRefAndId(OUT roid, IN _sf, IN OUT _offset);

		auto ref = %T::Get(roid);
		Rococo::Components::AssignRef(*output, ref);
	}

	void Destruct_%sComponent(NativeCallEnvironment& _nce)
	{
		uint8* _sf = _nce.cpu.SF();
		ComponentObject* ip;
		ReadInput(ip, _sf, SF_TO_DESTRUCTED_OBJECT_OFFSET);

		ip->life->ReleaseRef();
	}

	void AddNativeCalls_%V_GetAndAdd(const INamespace& ns, Rococo::Script::IPublicScriptSystem& ss)
	{
		ss.AddNativeCall(ns, Add_%sComponent, nullptr, "Add%s (Sys.Type.ComponentRef outputRef) (ROID id) -> ", __FILE__, __LINE__);
		ss.AddNativeCall(ns, Get_%sComponent, nullptr, "Get%s (Sys.Type.ComponentRef outputRef) (ROID id) -> ", __FILE__, __LINE__);
		ss.AddNativeCall(ns, Destruct_%sComponent, nullptr, "Destruct%s -> ", __FILE__, __LINE__);
	}
}
)";

void TransformTemplate(cstr templateString, cstr nsToExpand, cstr shortObjectName, cstr shortHandForObjectName, cstr extraArg, cstr extraArg2, cstr extraArg3, cstr extraArg4, ITextBuilder& builder)
{
	char simple[2] = { 0,0 };

	for (cstr p = templateString; *p != 0; p++)
	{
		if (*p == '%')
		{
			p++;

			switch (*p)
			{
			case 0:
				return;
			case '%':
				builder.WriteString("%");
				break;
			case 'N':
				builder.WriteString(nsToExpand);
				break;
			case 'S':
				builder.WriteString(shortObjectName);
				break;
			case 's':
				builder.WriteString(shortHandForObjectName);
				break;
			case 'T':
				builder.WriteString(extraArg);
				break;
			case 'U':
				builder.WriteString(extraArg2);
				break;
			case 'V':
				builder.WriteString(extraArg3);
				break;
			case 'W':
				builder.WriteString(extraArg4);
				break;
			default:				
				Throw(0, "Bad escape sequence in template");
			}
		}
		else
		{
			simple[0] = *p;
			builder.WriteString(simple);
		}
	}
}

void TransformSexyComponentTemplate(cstr nsToExpand, cstr shortObjectName, cstr shortHandForObjectName, ITextBuilder& builder)
{
	TransformTemplate(componentTemplate, nsToExpand, shortObjectName, shortHandForObjectName, "<unused", "<unused2>", "<unused3>", "<unused4>", builder);
}

void TransformCppComponentTemplate(cstr nsToExpand, cstr shortObjectName, cstr shortHandForObjectName, cstr APInsName, cstr sxyNamespace, cstr compressedNS, cstr componentName, ITextBuilder& builder)
{
	TransformTemplate(componentTemplateCppImpl, nsToExpand, shortObjectName, shortHandForObjectName, APInsName, sxyNamespace, compressedNS, componentName, builder);
}

void ConvertSexyFQNameToCPPNamespaceAndShortName(char* nsBuffer, size_t sizeofNSBuffer, char* shortNameBuffer, size_t sizeOfShortNameBuffer, cstr fqName)
{
	cstr ns, shortName;
	NamespaceSplitter splitter(fqName);
	if (!splitter.SplitTail(ns, shortName))
	{
		Throw(0, "Could not split %s", fqName);
	}

	StackStringBuilder nsBuilder(nsBuffer, sizeofNSBuffer);

	for (const char* p = ns; *p != 0; p++)
	{
		if (*p != '.')
		{
			nsBuilder.AppendChar(*p);
		}
		else
		{
			nsBuilder.AppendChar(':');
			nsBuilder.AppendChar(':');
		}
	}

	CopyString(shortNameBuffer, sizeOfShortNameBuffer, shortName);
}