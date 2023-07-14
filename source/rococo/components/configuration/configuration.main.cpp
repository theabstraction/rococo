#define ROCOCO_COMPONENTS_CONFIG_API __declspec(dllexport)

#include <components/rococo.components.configuration.h>
#include <rococo.allocators.h>
#include <rococo.functional.h>
#include <unordered_map>

namespace Rococo::ECS
{
	using namespace Rococo::Memory;
#pragma warning( disable : 5205)
	IComponentFactory<IConfigurationComponent>* CreateConfigurationFactory();
#pragma warning( default : 5205 )
	struct ConfigurationComponents_Implementation;

	struct ConfigurationComponentLife : IComponentLife
	{
		int64 referenceCount = 0;
		ROID id;
		ConfigurationComponents_Implementation& table;
		bool isDeprecated = false;

		ConfigurationComponentLife(ROID roid, ConfigurationComponents_Implementation& refTable):
			id(roid), table(refTable)
		{

		}

		int64 AddRef() override
		{
			return ++referenceCount;
		}

		int64 GetRefCount() const override
		{
			return referenceCount;
		}

		ROID GetRoid() const override
		{
			return id;
		}

		int64 ReleaseRef() override;

		// Marks the component as deprecated and returns true if this is the first call that marked it so
		bool Deprecate() override;

		bool IsDeprecated() const override
		{
			return isDeprecated;
		}
	};

	struct ConfigurationComponents_Implementation: IComponentTable
	{
		IECS* ecs = nullptr;
		int enumLock = 0;

		struct ComponentDesc
		{
			Components::IConfigurationComponent* interfacePointer = nullptr;
		};

		IComponentFactory<IConfigurationComponent>& componentFactory;
		std::unordered_map<ROID, ComponentDesc, STDROID, STDROID> rows;
		std::vector<ROID> deprecatedList;
		std::vector<ROID> stubbornList;
		AutoFree<IFreeListAllocatorSupervisor> componentAllocator;
		size_t componentSize;

		ConfigurationComponents_Implementation(IComponentFactory<IConfigurationComponent>& _componentFactory):
			componentFactory(_componentFactory), rows(1024), componentSize(_componentFactory.SizeOfConstructedObject())
		{

		}

		void NotifyOfDeath(ROID id)
		{
			if (enumLock > 0)
			{
				// Cannot delete at this time, wait for the next garbage collection
				deprecatedList.push_back(id);
				return;
			}
			// A component has told us that it is deprecated and a reference to it was released and its reference count is now zero
			auto it = rows.find(id);
			if (it != rows.end())
			{
				auto& component = it->second;

				componentFactory.Destruct(component.interfacePointer);
				componentAllocator->FreeBuffer(component.interfacePointer);
				rows.erase(it);
			}
		}

		virtual ~ConfigurationComponents_Implementation()
		{

		}

		Ref<IConfigurationComponent> AddNew(ROID id)
		{
			if (enumLock > 0)
			{
				Throw(0, "%s failed: the components were locked for enumeration", __func__);
			}

			std::pair<ROID, ComponentDesc> nullItem(id, ComponentDesc());
			auto insertion = rows.insert(nullItem);
			if (!insertion.second)
			{
				Throw(0, "%s: a component with the given id 0x%8.8X already exists", __FUNCTION__, id.index);
			}

			auto i = insertion.first;

			try
			{
				void* pComponentMemory = componentAllocator->AllocateBuffer();
				IConfigurationComponent* component = componentFactory.ConstructInPlace(pComponentMemory);
				if (component == nullptr)
				{
					Throw(0, "%s: factory.ConstructInPlace returned null", __FUNCTION__);
				}
				i->second.interfacePointer = component;

				uint8* byteBuffer = (uint8*)pComponentMemory;
				auto* lifeSupport = (ConfigurationComponentLife*)(byteBuffer + componentSize);
				new (lifeSupport) ConfigurationComponentLife(id, *this);
				return Ref<IConfigurationComponent>(*component, GetLife(*component));
			}
			catch (...)
			{
				rows.erase(i);
				throw;
			}
		}

		IECS& ECS()
		{
			if (ecs == nullptr)
			{
				Throw(0, "%s: ECS was not linked at this time.", __FUNCTION__);
			}

			return *ecs;
		}

		void Link(IECS* ecs)
		{
			if (ecs != nullptr && this->ecs != nullptr && ecs != this->ecs)
			{
				Throw(0, "%s: An attempt was made to link to a competing ECS system. ConfigurationComponents support only one ecs system.", __FUNCTION__);
			}

			this->ecs = ecs;
		}

		IComponentTable& Table()
		{
			return *this;
		}

		void CollectGarbage()
		{
			if (enumLock > 0)
			{
				Throw(0, "%s: An attempt was made to collect garbage during an enumeration lock.", __FUNCTION__);
			}

			while (!deprecatedList.empty())
			{
				ROID id = deprecatedList.back();
				deprecatedList.pop_back();

				NotifyOfDeath(id);
			}
		}

		void Deprecate(ROID id)
		{
			auto i = rows.find(id);
			if (i != rows.end())
			{
				auto& component = i->second;
				auto& life = GetLife(*component.interfacePointer);
				life.Deprecate();
			}
		}

		void ForEachAnimationComponent(IComponentCallback<IConfigurationComponent>& cb)
		{
			enumLock++;
			try
			{
				for (auto& row : rows)
				{
					if (cb.OnComponent(row.first, *row.second.interfacePointer) == EFlowLogic::BREAK)
					{
						break;
					}
				}
			}
			catch (...)
			{
				enumLock--;
				throw;
			}
			enumLock--;
		}

		void ForEachAnimationComponent(Rococo::Function<EFlowLogic(ROID roid, IConfigurationComponent&)> functor)
		{
			enumLock++;
			try
			{
				for (auto& row : rows)
				{
					if (functor.Invoke(row.first, *row.second.interfacePointer) == EFlowLogic::BREAK)
					{
						break;
					}
				}
			}
			catch (...)
			{
				enumLock--;
				throw;
			}
			enumLock--;
		}

		Ref<IConfigurationComponent> Find(ROID id)
		{
			auto i = rows.find(id);
			auto& c = i->second;
			return i != rows.end() ? Ref<IConfigurationComponent>(*c.interfacePointer, GetLife(*c.interfacePointer)) : Ref<IConfigurationComponent>();
		}

		size_t GetConfigurationComponentIDs(ROID* roidOutput, size_t nElementsInOutput)
		{
			if (roidOutput != nullptr)
			{
				size_t nElements = min(nElementsInOutput, rows.size());

				auto* roid = roidOutput;
				auto* rend = roid + nElements;

				for (auto row : rows)
				{
					if (roid == rend) break;
					*roid++ = row.first;
				}

				return nElements;
			}

			return rows.size();
		}


		ConfigurationComponentLife& GetLife(IConfigurationComponent& i)
		{
			uint8* objectBuffer = (uint8*)&i;
			return *(ConfigurationComponentLife*)(objectBuffer + componentSize);
		}
	} *s_ConfigurationComponentsImplementation = nullptr;

	int64 ConfigurationComponentLife::ReleaseRef()
	{
		int64 rc = --referenceCount;
		if (rc <= 0 && isDeprecated)
		{
			table.NotifyOfDeath(id);
		}
		return rc;
	}

	bool ConfigurationComponentLife::Deprecate()
	{
		if (!isDeprecated)
		{
			isDeprecated = true;

			if (referenceCount == 0)
			{
				table.NotifyOfDeath(id);
			}

			return true;
		}

		return false;
	}


	static bool s_ConfigurationComponentsPhoenixGuard = false;
	static IComponentFactory<IConfigurationComponent>* s_ConfigurationComponentsFactory = nullptr;

	void InitConfigurationComponents()
	{
		if (!s_ConfigurationComponentsImplementation)
		{
			if (s_ConfigurationComponentsPhoenixGuard)
			{
				return;
			}

			s_ConfigurationComponentsFactory = CreateConfigurationFactory();
			s_ConfigurationComponentsImplementation = new ConfigurationComponents_Implementation(*s_ConfigurationComponentsFactory);

			struct ANON
			{
				static void FreeConfigurationComponentsSingleton()
				{
					if (s_ConfigurationComponentsImplementation)
					{
						delete s_ConfigurationComponentsImplementation;
						s_ConfigurationComponentsFactory->Free();
						s_ConfigurationComponentsFactory = nullptr;
						s_ConfigurationComponentsImplementation = nullptr;
						s_ConfigurationComponentsPhoenixGuard = true;
					}
				}
			};
			atexit(ANON::FreeConfigurationComponentsSingleton);
		}
		else
		{
			Throw(0, "%s: ConfigurationComponents has already been initialized", __FUNCTION__);
		}
	}

	ConfigurationComponents_Implementation& GetConfigurationComponents()
	{
		if (!s_ConfigurationComponentsImplementation)
		{
			InitConfigurationComponents();
		}
		return *s_ConfigurationComponentsImplementation;
	}
}

namespace Rococo::ECS
{
	ROCOCO_COMPONENTS_CONFIG_API Ref<IConfigurationComponent> AddConfigurationComponent(ROID id)
	{
		struct ANON : IECS_ROID_LockedSection
		{
			Ref<IConfigurationComponent> newRef;

			ANON()
			{

			}

			void OnLock(ROID roid, IECS& ecs) override
			{
				UNUSED(ecs);
				newRef = GetConfigurationComponents().AddNew(roid);
			}
		} locked_section;

		GetConfigurationComponents().ECS().TryLockedOperation(id, locked_section);

		// I am generated by code, and I know what I am doing
		return locked_section.newRef;
	}

	ROCOCO_COMPONENTS_CONFIG_API void ConfigurationComponent_LinkToECS(IECS* ecs)
	{
		GetConfigurationComponents().Link(ecs);
	}
}