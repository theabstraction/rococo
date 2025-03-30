#include <rococo.sexml.h>
#include <rococo.reflector.h>
#include <rococo.functional.h>
#include <rococo.strings.h>

using namespace Rococo::Reflection;

namespace Rococo::SEXML::Impl
{
	struct SXMLBuilderVisitor : IReflectionVisitor
	{
		Rococo::Sex::SEXML::ISEXMLBuilder& builder;

		SXMLBuilderVisitor(Rococo::Sex::SEXML::ISEXMLBuilder& _builder) : builder(_builder)
		{
			builder.AddDirective("Root");
		}

		void CancelVisit(IReflectionVisitation& visitation) override
		{
			// Unexpected -> SXMLBuilderVisitor visitors do not use visitations 
			UNUSED(visitation);
		}

		// We cannot stick this in the destructor as it throws on failure, which destructors cannot handle.
		void Finalize()
		{
			builder.CloseDirective();

			if (containerCount > 0)
			{
				Throw(0, "Outstanding containers");
			}

			if (elementCount > 0)
			{
				Throw(0, "Outstanding elements");
			}

			if (sectionCount > 0)
			{
				Throw(0, "Outstanding sections");
			}
		}

		~SXMLBuilderVisitor()
		{
			
		}

		EReflectionDirection Direction() const override
		{
			return EReflectionDirection::READ_ONLY;
		}

		int containerCount = 0;

		void EnterContainer(cstr name) override
		{
			containerCount++;
			builder.AddDirective("Container").AddStringLiteral("Container.Name", name);
		}

		void LeaveContainer()  override
		{
			if (containerCount == 0)
			{
				Throw(0, "Too many LeaveContainer(s)");
			}
			containerCount--;

			builder.CloseDirective();
		}

		int elementCount = 0;

		void EnterElement(cstr elementKey) override
		{
			elementCount++;
			builder.AddDirective("Element").AddStringLiteral("Key", elementKey);
		}

		void LeaveElement() override
		{
			if (elementCount == 0)
			{
				Throw(0, "Too many LeaveElement(s)");
			}
			elementCount--;
			builder.CloseDirective();
		}

		void Reflect(cstr name, IReflectionTarget& subTarget, ReflectionMetaData& metaData) override
		{
			UNUSED(metaData);
			UNUSED(subTarget);
			UNUSED(name);
			Throw(0, "SexML subtargets - Not implemented");
		}

		void Reflect(cstr name, int32& value, ReflectionMetaData& metaData) override
		{
			UNUSED(metaData);
			builder.AddAtomicAttribute(name, value);
		}

		void Reflect(cstr name, int64& value, ReflectionMetaData& metaData) override
		{
			UNUSED(metaData);
			builder.AddAtomicAttribute(name, value);
		}

		void Reflect(cstr name, uint64& value, ReflectionMetaData& metaData) override
		{
			UNUSED(metaData);
			builder.AddAtomicAttribute(name, value);
		}

		void Reflect(cstr name, float& value, ReflectionMetaData& metaData) override
		{
			UNUSED(metaData);
			builder.AddAtomicAttribute(name, value);
		}

		void Reflect(cstr name, double& value, ReflectionMetaData& metaData) override
		{
			UNUSED(metaData);
			builder.AddAtomicAttribute(name, value);
		}

		void Reflect(cstr name, bool& value, ReflectionMetaData& metaData) override
		{
			UNUSED(metaData);
			builder.AddAtomicAttribute(name, value);
		}

		void Reflect(cstr name, char* stringBuffer, size_t capacity, ReflectionMetaData& metaData) override
		{
			UNUSED(capacity);
			UNUSED(metaData);
			builder.AddStringLiteral(name, stringBuffer);
		}

		void Reflect(cstr name, Strings::HString& stringRef, ReflectionMetaData& metaData) override
		{
			UNUSED(metaData);
			builder.AddStringLiteral(name, stringRef);
		}

		int sectionCount = 0;

		void EnterSection(cstr sectionName) override
		{
			sectionCount++;
			builder.AddDirective("Section").AddStringLiteral("Section.Name", sectionName);
		}

		void LeaveSection()
		{
			if (sectionCount == 0)
			{
				Throw(0, "Too many LeaveSection(s)");
			}

			sectionCount--;
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
				visitor.Finalize();
			}
		);
	}
}