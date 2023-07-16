#include <rococo.ecs.h>
#include <unordered_map>
#include <rococo.strings.h>

namespace Rococo::Components
{
	using namespace Rococo::Strings;

	struct RoidMap: IRoidMap
	{
		std::unordered_map<ROID, IComponentBase*, STDROID, STDROID> rows;
		HString friendlyName;

		RoidMap(cstr _friendlyName, size_t reserveRows): friendlyName(_friendlyName)
		{
			rows.reserve(reserveRows);
		}

		virtual ~RoidMap()
		{

		}

		void Free() override
		{
			delete this;
		}

		void Delete(ROID id, IEventCallback<IComponentBase*>& onDelete) override
		{
			auto it = rows.find(id);
			if (it != rows.end())
			{
				auto* component = it->second;
				onDelete.OnEvent(component);
				rows.erase(it);
			}
		}

		IComponentBase* Find(ROID id) const override
		{
			auto i = rows.find(id);
			IComponentBase* base = i != rows.end() ? i->second : nullptr;
			return base;
		}

		void ForEachComponent(IComponentCallback<IComponentBase>& cb) override
		{
			for (auto& row : rows)
			{
				if (cb.OnComponent(row.first, *row.second) == EFlowLogic::BREAK)
				{
					break;
				}
			}
		}

		size_t ToVector(RoidEntry* roidOutput, size_t nElementsInOutput) override
		{
			if (roidOutput != nullptr)
			{
				size_t nElements = min(nElementsInOutput, rows.size());

				auto* roid = roidOutput;
				auto* rend = roid + nElements;

				for (auto row : rows)
				{
					if (roid == rend) break;
					*roid++ = { row.first, row.second };
				}

				return nElements;
			}

			return rows.size();
		}

		// Allocate mapping for a ROID, using the factory to create an associated component if successfully reserved. If the factory returns nullptr mapping is revoked.
		// If the factory throws an exception mapping is revoked and the exception propagated to the caller.
		// If the ROID already exists in the map an exception is thrown
		void Insert(ROID id, IComponentBaseFactory& factory) override
		{
			IComponentBase* nullBase = nullptr;
			std::pair<ROID, IComponentBase*> nullItem(id, nullBase);

			auto insertion = rows.insert(nullItem);
			if (!insertion.second)
			{
				Throw(0, "%s(%s): a component with the given id 0x%8.8X already exists", __FUNCTION__, friendlyName.c_str(), id.index);
			}

			IComponentBase* base = nullptr;

			try
			{
				base = factory.Create(id);
				if (base == nullptr)
				{
					Throw(0, "%s(%s): factory returned a nullptr", __FUNCTION__, friendlyName.c_str());
				}
				insertion.first->second = base;
			}
			catch (...)
			{
				rows.erase(insertion.first);
				throw;
			}

			if (!base)
			{
				rows.erase(insertion.first);
			}
		}
	};

	ROCOCO_ECS_API IRoidMap* CreateRoidMap(cstr friendlyName, size_t reserveRows)
	{
		return new RoidMap(friendlyName, reserveRows);
	}
}