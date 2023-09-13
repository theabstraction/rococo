#pragma once
#include "rococo.component.base.h"

namespace Rococo::Components
{
	struct IComponentLife;

#pragma pack(push, 1)
	struct RefPointers
	{
		IComponentBase* component;
		IComponentLife* life;
	};
#pragma pack(pop)

	template<class T>
	void AssignRef(RefPointers& ptrs, T& ref)
	{
		static_assert((sizeof T::TInterface) == sizeof IComponentBase, "The interface T must derive from IComponentBase without extra vtables.");
		if (ref)
		{
			T::TInterface* component = &ref.GetComponent();
			ptrs.component = static_cast<IComponentBase*>(component);
			ptrs.life = &ref.Life();
			ptrs.life->AddRef();
		}
		else
		{
			ptrs.component = nullptr;
			ptrs.life = nullptr;
		}
	}
}