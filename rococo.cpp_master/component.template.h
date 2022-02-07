namespace Rococo::Components::Sys
{
	using namespace Rococo::Components;

	ROCOCOAPI IComponentInterfaceTable
	{
		virtual void Free() = 0;
		virtual IComponentInterface* AddNew(EntityIndex id) = 0;
		virtual IComponentInterface* Find(EntityIndex id) = 0;
		virtual void Deprecate(EntityIndex id) = 0;
		virtual void Flush() = 0;
	};
}

namespace Rococo::Components::Sys::Factories
{
	IComponentInterfaceTable* NewComponentInterfaceTable(IComponentInterfaceFactory& factory);
}