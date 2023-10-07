#include <rococo.sexml.h>
#include <rococo.reflector.h>
#include <rococo.functional.h>

using namespace Rococo::Reflection;

namespace Rococo::SEXML::Impl
{
	struct SXMLBuilderVisitor : IReflectionVisitor
	{
		Rococo::Sex::SEXML::ISEXMLBuilder& builder;

		SXMLBuilderVisitor(Rococo::Sex::SEXML::ISEXMLBuilder& _builder) : builder(_builder)
		{
			builder.AddDirective("root");
		}

		~SXMLBuilderVisitor()
		{
			builder.CloseDirective();
		}

		EReflectionDirection Direction() const override
		{
			return EReflectionDirection::READ_ONLY;
		}

		void EnterContainer(cstr name) override
		{
			builder.AddDirective("container").AddStringLiteral("container name", name);
		}

		void LeaveContainer()  override
		{
			builder.CloseDirective();
		}

		void EnterElement(cstr elementKey) override
		{
			builder.AddDirective("element").AddStringLiteral("key", elementKey);
		}

		void LeaveElement() override
		{
			builder.CloseDirective();
		}

		void Reflect(cstr name, IReflectionTarget& subTarget, ReflectionMetaData& metaData) override
		{
			Throw(0, "SexML subtargets - Not implemented");
		}

		void Reflect(cstr name, int32& value, ReflectionMetaData& metaData) override
		{
			builder.AddAtomicAttribute(name, value);
		}

		void Reflect(cstr name, int64& value, ReflectionMetaData& metaData) override
		{
			builder.AddAtomicAttribute(name, value);
		}

		void Reflect(cstr name, uint64& value, ReflectionMetaData& metaData) override
		{
			builder.AddAtomicAttribute(name, value);
		}

		void Reflect(cstr name, float& value, ReflectionMetaData& metaData) override
		{
			builder.AddAtomicAttribute(name, value);
		}

		void Reflect(cstr name, double& value, ReflectionMetaData& metaData) override
		{
			builder.AddAtomicAttribute(name, value);
		}

		void Reflect(cstr name, bool& value, ReflectionMetaData& metaData) override
		{
			builder.AddAtomicAttribute(name, value);
		}

		void Reflect(cstr name, IReflectedString& stringValue, ReflectionMetaData& metaData) override
		{
			builder.AddStringLiteral(name, stringValue.ReadString());
		}

		void EnterSection(cstr sectionName) override
		{
			builder.AddDirective("section").AddStringLiteral("container name", sectionName);
		}

		void LeaveSection()
		{
			builder.CloseDirective();
		}
	};
}

namespace Rococo
{
	void SaveAsSexML(cstr userDocName, Reflection::IReflectionTarget& target)
	{
		Rococo::OS::SaveUserSEXML("", userDocName, [&target](Rococo::Sex::SEXML::ISEXMLBuilder& builder)
			{
				Rococo::SEXML::Impl::SXMLBuilderVisitor visitor(builder);
				target.Visit(visitor);
			}
		);
	}
}