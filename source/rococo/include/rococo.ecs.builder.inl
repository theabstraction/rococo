#pragma once

#include <rococo.ecs.h>
#include <vector>
#include <rococo.allocators.h>
#include <rococo.functional.h>
#include <rococo.debugging.h>

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

		bool Deprecate() override
		{
			if (!isDeprecated)
			{
				isDeprecated = true;
				table.Deprecate(id);
				return true;
			}

			return false;
		}

		void DeprecateWithoutNotify()
		{
			isDeprecated = true;
		}

		bool IsDeprecated() const override
		{
			return isDeprecated;
		}
	};

	template<class COMPONENT>
	struct ComponentTable : IComponentTableSupervisor
	{
		using FACTORY = IComponentFactory<COMPONENT>;

		IECSSupervisor* ecs = nullptr;
		int enumLock = 0;
		cstr friendlyName;
		FACTORY& componentFactory;
		AutoFree<IRoidMap> rows;
		std::vector<COMPONENT*> deathRow;
		std::vector<COMPONENT*> deathRowSurvivors;
		std::vector<ROID> deprecatedList;
		std::vector<ROID> stubbornList;
		AutoFree<Memory::IFreeListAllocatorSupervisor> componentAllocator;
		size_t componentSize;
		uint32 tableIndex = (uint32) - 1;

		ComponentTable(cstr _friendlyName, FACTORY& _componentFactory) :
			friendlyName(_friendlyName),
			componentFactory(_componentFactory),
			rows(CreateRoidMap(_friendlyName, 1024)),
			componentSize(_componentFactory.SizeOfConstructedObject())
		{
			componentAllocator = Memory::CreateFreeListAllocator(componentSize + sizeof ComponentLife<COMPONENT>);
		}

		void BindTableIndex(uint32 tableIndex)
		{
			if (this->tableIndex != (uint32)-1)
			{
				if (this->tableIndex != tableIndex)
				{
					Throw(0, "%s: error, an attempt was made to change the table index", __FUNCTION__);
				}
			}
			else
			{
				this->tableIndex = tableIndex;
			}
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
					allocator->ReclaimBuffer(component);
				}
			} onDelete;
			onDelete.factory = &componentFactory;
			onDelete.allocator = componentAllocator;

			rows->Delete(id, onDelete);
		}

		virtual ~ComponentTable()
		{
			ecs = nullptr;

			struct ANON : IComponentCallback<IComponentBase>
			{
				ComponentTable<COMPONENT>* This = nullptr;
				EFlowLogic OnComponent(ROID, IComponentBase& base) override
				{
					auto* component = static_cast<COMPONENT*>(&base);
					component->~COMPONENT();
					This->componentAllocator->FreeBuffer(component);
					return EFlowLogic::CONTINUE;
				}
			} deleteIt;
			deleteIt.This = this;
			rows->ForEachComponent(deleteIt);

			CollectGarbage();

			if (deathRow.size() > 0)
			{
				Rococo::Debugging::Log("~ComponentTable<%s>: %llu on death row could not be deleted, assume their are own references.\n", friendlyName, deathRow.size());

				for (auto* c : deathRow)
				{
					auto& life = GetLife(*c);
					ROID id = life.id;
					Rococo::Debugging::Log(" %s[%u, salt %u]%s", friendlyName, id.index, id.salt.cycle, id.salt.isDeprecated ? "" : "(deprecated)\n");
				}
			}
		}

		Ref<COMPONENT> AddNew(ROID id)
		{
			if (tableIndex == (uint32)-1)
			{
				Throw(0, "%s: TableIndex not set", friendlyName);
			}

			if (!ECS().IsActive(id))
			{
				return Ref<COMPONENT>();
			}

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

			ECS().OnNotifyComponentAttachedToROID(id, tableIndex);

			return Ref<COMPONENT>(*baseFactory.component, GetLife(*baseFactory.component));
		}

		IECSSupervisor& ECS()
		{
			if (ecs == nullptr)
			{
				Throw(0, "%s: ECS was not linked at this time.", friendlyName);
			}

			return *ecs;
		}

		void Link(IECSSupervisor* ecs)
		{
			if (ecs != nullptr && this->ecs != nullptr && ecs != this->ecs)
			{
				Throw(0, "%s: An attempt was made to link to a competing ECS system. ConfigurationComponents support only one ecs system.", friendlyName);
			}

			this->ecs = ecs;

			ecs->LinkComponentTable(*this);
		}

		IComponentTable& Table()
		{
			return *this;
		}

		void CollectGarbage()
		{
			if (enumLock > 0)
			{
				Throw(0, "%s: An attempt was made to collect garbage during an enumeration lock.", friendlyName);
			}

			while (!deprecatedList.empty())
			{
				ROID id = deprecatedList.back();
				deprecatedList.pop_back();

				NotifyOfDeath(id);
			}

			deathRowSurvivors.clear();

			for (auto* c : deathRow)
			{
				auto& life = GetLife(*c);
				if (life.referenceCount == 0)
				{
					componentFactory.Destruct(c);
					componentAllocator->ReclaimBuffer(c);
				}
				else
				{
					deathRowSurvivors.push_back(c);
				}
			}

			deathRow.clear();
			std::swap(deathRow, deathRowSurvivors);
		}

		void OnNotifyThatTheECSHasDeprecatedARoid(ROID id) override
		{
			DeprecateAndDoNotNotifyDependents(id);
		}

		void DeprecateAndDoNotNotifyDependents(ROID id)
		{
			struct ANON : IEventCallback<IComponentBase*>
			{
				ComponentTable<COMPONENT>* container = nullptr;
				void OnEvent(IComponentBase*& base)
				{
					auto* component = static_cast<COMPONENT*>(base);
					auto& life = container->GetLife(*component);
					life.DeprecateWithoutNotify();
					container->deathRow.push_back(component);
				}
			} onDelete;

			onDelete.container = this;

			rows->Delete(id, onDelete);
		}

		void Deprecate(ROID id) override
		{
			DeprecateAndDoNotNotifyDependents(id);
			ecs->OnNotifyAComponentHasDeprecatedARoid(id, tableIndex);
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
			if (ECS().IsActive(id))
			{
				auto* component = static_cast<COMPONENT*>(rows->Find(id));
				if (component)
				{
					return Ref<COMPONENT>(*component, GetLife(*component));
				}
			}
			return Ref<COMPONENT>();
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

		Ref<COMPONENT> GetComponent(ROID id)
		{
			return table.Find(id);
		}
	};

	template<class COMPONENT>
	struct FactoryBuilder
	{
		IComponentFactory<COMPONENT>* Create() { static_assert(false); }

		// Override this in your specialization and ensure the returned string and pointer are invariant for the lifetime of the ecs system and all its component DLLs.
		cstr Name() const { static_assert(false); }
	};
}

// Stick this in your component .cpp file to declare a singleton component table referenced by Rococo::Components::SINGLETON
// Needs an implementtion of IComponentFactory<ICOMPONENT>* CreateComponentFactory() in the component's API namespace;
// This is used when DefaultFactory<ICOMPONENT, IMPLEMENTATION> is inappropriate for creating object of type IMPLEMENTATION
#define DEFINE_FACTORY_SINGLETON(ICOMPONENT)									\
namespace Module::For##ICOMPONENT												\
{																				\
	using namespace Rococo::Components;											\
	ComponentTable<ICOMPONENT>* theSingleton = 0;								\
	IComponentFactory<ICOMPONENT>* CreateComponentFactory();					\
	IComponentFactory<ICOMPONENT>* theFactory = nullptr;						\
}																				
																																
// Assumes DefaultFactory<ICOMPONENT, IMPLEMENTATION> can create objects of type IMPLEMENTATION that implement ICOMPONENT
// Stick this in your component .cpp file to declare a singleton component table referenced by 'theSingleton'
// If your compiler has trouble interpreting ICOMPONENT, ensure you have set out a using directive with the containing namespace ahead of the macro invocation
#define DEFINE_FACTORY_SINGLETON_WITH_DEFAULT_FACTORY(ICOMPONENT,IMPLEMENTATION)						\
namespace Module::For##ICOMPONENT																		\
{																										\
	using namespace Rococo::Components;																	\
	ComponentTable<ICOMPONENT>* theSingleton = 0;														\
	IComponentFactory<ICOMPONENT>* theFactory = nullptr;												\
	IComponentFactory<ICOMPONENT>* CreateComponentFactory()												\
	{																									\
		return new DefaultFactory<ICOMPONENT, IMPLEMENTATION>();										\
	}																									\
}																										\

// Requires an alias to SINGLETON, typically retrieved from DEFINE_FACTORY_SINGLETON
#define EXPORT_SINGLETON_METHODS(COMPONENT_API,ICOMPONENT)											\
namespace Rococo::Components::API::For##ICOMPONENT													\
{																									\
	COMPONENT_API Ref<ICOMPONENT> Add(ROID id)														\
	{																								\
		return Module::For##ICOMPONENT::theSingleton->AddNew(id);									\
	}																								\
																									\
	COMPONENT_API Ref<ICOMPONENT> Get(ROID id)														\
	{																								\
		return Module::For##ICOMPONENT::theSingleton->Find(id);										\
	}																								\
																									\
	COMPONENT_API void ForEach(Function<EFlowLogic(ROID roid, ICOMPONENT&)> functor)				\
	{																								\
		Module::For##ICOMPONENT::theSingleton->ForEachComponent(functor);							\
	}																								\
}																									\
namespace Rococo::Components::ECS																	\
{																									\
	COMPONENT_API void LINK_NAME(ICOMPONENT, Table)(IECSSupervisor& ecs)							\
	{																								\
		using namespace Module::For##ICOMPONENT;													\
		if (theSingleton) return;																	\
		theFactory =  CreateComponentFactory();														\
		theSingleton = new ComponentTable<ICOMPONENT>(#ICOMPONENT, *theFactory);					\
		Module::For##ICOMPONENT::theSingleton->Link(&ecs);											\
	}																								\
}

#define EXPORT_SINGLETON_METHODS_WITH_LINKARG(COMPONENT_API, ICOMPONENT, LINKARG)					\
namespace Rococo::Components::API::For##ICOMPONENT													\
{																									\
	COMPONENT_API Ref<ICOMPONENT> Add(ROID id)														\
	{																								\
		return Module::For##ICOMPONENT::theSingleton->AddNew(id);									\
	}																								\
																									\
	COMPONENT_API Ref<ICOMPONENT> Get(ROID id)														\
	{																								\
		return Module::For##ICOMPONENT::theSingleton->Find(id);										\
	}																								\
																									\
	COMPONENT_API void ForEach(Function<EFlowLogic(ROID roid, ICOMPONENT&)> functor)				\
	{																								\
		Module::For##ICOMPONENT::theSingleton->ForEachComponent(functor);							\
	}																								\
}																									\
																									\
namespace Rococo::Components::ECS																	\
{																									\
	COMPONENT_API void LINK_NAME(ICOMPONENT, Table)(IECSSupervisor& ecs, LINKARG& args)				\
	{																								\
		using namespace Module::For##ICOMPONENT;													\
		if (theSingleton) return;																	\
		theFactory =  CreateComponentFactory(args);													\
		theSingleton = new ComponentTable<ICOMPONENT>(#ICOMPONENT, *theFactory);					\
		theSingleton->Link(&ecs);																	\
	}																								\
}

#define SINGLETON_MANAGER(COMPONENT_API,ICOMPONENT)													\
namespace Rococo::Components::ECS																	\
{																									\
	COMPONENT_API void ReleaseTablesFor##ICOMPONENT()												\
	{																								\
		if (Module::For##ICOMPONENT::theSingleton)													\
		{																							\
			delete Module::For##ICOMPONENT::theSingleton;											\
			Module::For##ICOMPONENT::theSingleton = nullptr;										\
			Module::For##ICOMPONENT::theFactory->Free();											\
			Module::For##ICOMPONENT::theFactory = nullptr;											\
		}																							\
	}																								\
}

#define DEFINE_AND_EXPORT_SINGLETON_METHODS(COMPONENT_API, ICOMPONENT)										\
DEFINE_FACTORY_SINGLETON(ICOMPONENT)																		\
EXPORT_SINGLETON_METHODS(COMPONENT_API, ICOMPONENT)															

#define DEFINE_AND_EXPORT_SINGLETON_METHODS_WITH_LINKARG(COMPONENT_API, ICOMPONENT, LINK_ARG)				\
DEFINE_FACTORY_SINGLETON(ICOMPONENT)																		\
EXPORT_SINGLETON_METHODS_WITH_LINKARG(COMPONENT_API, ICOMPONENT, LINK_ARG)									\
SINGLETON_MANAGER(COMPONENT_API, ICOMPONENT)

#define DEFINE_AND_EXPORT_SINGLETON_METHODS_WITH_DEFAULT_FACTORY(COMPONENT_API, ICOMPONENT, IMPLEMENTATION)	\
DEFINE_FACTORY_SINGLETON_WITH_DEFAULT_FACTORY(ICOMPONENT, IMPLEMENTATION)									\
EXPORT_SINGLETON_METHODS(COMPONENT_API, ICOMPONENT)															\
SINGLETON_MANAGER(COMPONENT_API, ICOMPONENT)