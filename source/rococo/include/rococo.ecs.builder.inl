#pragma once

#include <rococo.ecs.h>
#include <vector>
#include <rococo.allocators.h>
#include <rococo.functional.h>

namespace Rococo::Components
{
	template<class COMPONENT>
	struct ComponentTable;

	template<class COMPONENT>
	struct ComponentLife : IComponentLife
	{
		int64 referenceCount = 0;
		ROID id;
		ComponentTable<COMPONENT>& table;
		bool isDeprecated = false;

		ComponentLife(ROID roid, ComponentTable<COMPONENT>& refTable) :
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

		int64 ReleaseRef()
		{
			int64 rc = --referenceCount;
			if (rc <= 0 && isDeprecated)
			{
				table.NotifyOfDeath(id);
			}
			return rc;
		}

		bool Deprecate()
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

		bool IsDeprecated() const override
		{
			return isDeprecated;
		}
	};

	template<class COMPONENT>
	struct ComponentTable : IComponentTable
	{
		using FACTORY = IComponentFactory<COMPONENT>;

		IECS* ecs = nullptr;
		int enumLock = 0;

		FACTORY& componentFactory;
		AutoFree<IRoidMap> rows;
		std::vector<ROID> deprecatedList;
		std::vector<ROID> stubbornList;
		AutoFree<Memory::IFreeListAllocatorSupervisor> componentAllocator;
		size_t componentSize;

		ComponentTable(cstr friendlyName, FACTORY& _componentFactory) :
			componentFactory(_componentFactory), rows(CreateRoidMap(friendlyName, 1024)), componentSize(_componentFactory.SizeOfConstructedObject())
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

			struct ANON : IEventCallback<IComponentBase*>
			{
				FACTORY* factory = nullptr;
				Memory::IFreeListAllocatorSupervisor* allocator = nullptr;

				void OnEvent(IComponentBase*& base) override
				{
					auto* component = static_cast<COMPONENT*>(base);
					factory->Destruct(component);
					allocator->FreeBuffer(component);
				}
			} onDelete;
			onDelete.factory = &componentFactory;
			onDelete.allocator = componentAllocator;

			rows->Delete(id, onDelete);
		}

		virtual ~ComponentTable()
		{

		}

		Ref<COMPONENT> AddNew(ROID id)
		{
			if (enumLock > 0)
			{
				Throw(0, "%s failed: the components were locked for enumeration", __func__);
			}

			struct ANON : IComponentBaseFactory
			{
				ComponentTable<COMPONENT>* container = nullptr;
				ComponentLife<COMPONENT>* lifeSupport = nullptr;
				COMPONENT* component = nullptr;

				IComponentBase* Create(ROID roid) override
				{
					void* pComponentMemory = nullptr;

					try
					{
						pComponentMemory = container->componentAllocator->AllocateBuffer();
						component = container->componentFactory.ConstructInPlace(pComponentMemory);

						if (component == nullptr)
						{
							Throw(0, "%s: factory.ConstructInPlace returned null", __FUNCTION__);
						}
					}
					catch (...)
					{
						container->componentAllocator->FreeBuffer(pComponentMemory);
						throw;
					}

					uint8* byteBuffer = (uint8*)pComponentMemory;
					lifeSupport = (ComponentLife<COMPONENT>*)(byteBuffer + container->componentSize);
					new (lifeSupport) ComponentLife<COMPONENT>(roid, *container);
					return component;
				}
			} baseFactory;
			baseFactory.container = this;

			rows->Insert(id, baseFactory);
			return Ref<COMPONENT>(*baseFactory.component, GetLife(*baseFactory.component));
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
			struct ANON : IEventCallback<IComponentBase*>
			{
				ConfigurationComponents_Implementation* container = nullptr;
				void OnEvent(IComponentBase*& base)
				{
					auto* component = static_cast<COMPONENT*>(base);
					auto& life = container->GetLife(*component);
					life.Deprecate();
				}
			} onDelete;

			onDelete.container = this;

			rows->Delete(id, onDelete);
		}

		void ForEachComponent(IComponentCallback<COMPONENT>& cb)
		{
			enumLock++;
			try
			{
				auto& baseCB = reinterpret_cast<IComponentCallback<IComponentBase>&>(cb);
				rows->ForEachComponent(baseCB);
			}
			catch (...)
			{
				enumLock--;
				throw;
			}
			enumLock--;
		}

		void ForEachComponent(Rococo::Function<EFlowLogic(ROID roid, COMPONENT&)> functor)
		{
			enumLock++;
			try
			{
				struct ANON : IComponentCallback<IComponentBase>
				{
					Rococo::Function<EFlowLogic(ROID roid, COMPONENT&)>* functor = nullptr;
					EFlowLogic OnComponent(ROID id, IComponentBase& base) override
					{
						auto& component = reinterpret_cast<COMPONENT&>(base);
						return functor->Invoke(id, component);
					}
				} cb;

				cb.functor = &functor;

				rows->ForEachComponent(cb);
			}
			catch (...)
			{
				enumLock--;
				throw;
			}
			enumLock--;
		}

		Ref<COMPONENT> Find(ROID id)
		{
			auto* component = reinterpret_cast<IConfigurationComponent*>(rows->Find(id));
			return component ? Ref<COMPONENT>(*component, GetLife(*component)) : Ref<COMPONENT>();
		}

		size_t GetConfigurationComponentIDs(TRoidEntry<COMPONENT>* roidOutput, size_t nElementsInOutput)
		{
			static_assert(sizeof RoidEntry == sizeof TRoidEntry<COMPONENT>);
			auto* baseOutput = reinterpret_cast<RoidEntry*>(roidOutput);
			rows->ToVector(baseOutput, nElementsInOutput);
		}

		ComponentLife<COMPONENT>& GetLife(COMPONENT& i)
		{
			uint8* objectBuffer = (uint8*)&i;
			return *(ComponentLife<COMPONENT>*)(objectBuffer + componentSize);
		}
	};

	template<class COMPONENT>
	struct FactoryBuilder
	{
		IComponentFactory<COMPONENT>* Create() { static_assert(false); }
		cstr Name() const { static_assert(false); }
	};


	template<class COMPONENT>
	class ComponentFactorySingleton
	{
	private:
		inline static ComponentTable<COMPONENT>* table = nullptr;
		inline static bool phoenixGuard = false;
		inline static IComponentFactory<COMPONENT>* factory = nullptr;
		inline static cstr friendlyName = nullptr;
	public:
		static void InitConfigurationComponents()
		{
			FactoryBuilder<COMPONENT> fb;

			if (!table)
			{
				if (phoenixGuard)
				{
					return;
				}

				factory = fb.Create();

				table = new ComponentTable<COMPONENT>(fb.Name(), *factory);
				
				struct ANON
				{
					static void FreeComponentsSingleton()
					{
						if (table)
						{
							delete table;
							factory->Free();
							factory = nullptr;
							table = nullptr;
							phoenixGuard = true;
						}
					}
				};
				atexit(ANON::FreeComponentsSingleton);
			}
			else
			{
				Throw(0, "%s: %s has already been initialized", __FUNCTION__, fb.Name());
			}
		}

		static ComponentTable<COMPONENT>& GetTable()
		{
			if (!table)
			{
				InitConfigurationComponents();
			}
			return *table;
		}

		static Ref<COMPONENT> AddComponent(ROID id)
		{
			struct ANON : IECS_ROID_LockedSection
			{
				Ref<COMPONENT> newRef;

				ANON()
				{

				}

				void OnLock(ROID roid, IECS& ecs) override
				{
					UNUSED(ecs);
					newRef = GetTable().AddNew(roid);
				}
			} locked_section;


			GetTable().ECS().TryLockedOperation(id, locked_section);

			// I am generated by code, and I know what I am doing
			return locked_section.newRef;
		}
	};
}

// Stick this in your component .cpp file to declare a singleton component table referenced by Rococo::Components::SINGLETON
#define DEFINE_FACTORY_SINGLETON(COMPONENT, FACTORY_INVOKE)						\
namespace Rococo::Components													\
{																				\
	template<>																	\
	struct FactoryBuilder<COMPONENT>											\
	{																			\
		IComponentFactory<COMPONENT>* Create()									\
		{																		\
			return FACTORY_INVOKE();											\
		}																		\
																				\
		const char* Name() const												\
		{																		\
			return #COMPONENT;													\
		}																		\
	};																			\
	using SINGLETON = ComponentFactorySingleton<COMPONENT>;						\
}														
