#include "hv.h"
#include "rococo.hashtable.h"
#include "rococo.renderer.h"
#include <rococo.textures.h>
#include <rococo.handles.h>

#include <memory>

using namespace HV;
using namespace Rococo;
using namespace Rococo::Textures;

namespace ANON // This allows the debugger to identify the members from the interface reference
{
	class ObjectPrototype : public IObjectPrototypeSupervisor
	{
		friend class ObjectPrototypeManager;
		InventoryData invData = { 0 };
		ObjectDynamics dynamics = { 0 };
		MeleeData melee = { 0 };
		ArmourData armour = { 0 };
		HString shortName;
		HString desc;
		cstr uniqueName = nullptr;
		BitmapLocation bitmap = { {0,0,0,0}, 0,0 };
		MaterialData material = { 0 };
	public:
		ObjectPrototype()
		{
			invData.maxStackSize = 1;
		}

		const ObjectDynamics& Dynamics() const override
		{
			return dynamics;
		}

		const MaterialData& Mats() const override
		{
			return material;
		}

		const fstring ShortName() const override
		{
			return fstring{ shortName.c_str(), (int32) shortName.length() };
		}

		const InventoryData& InvData() const override
		{
			return invData;
		}

		const ArmourData& Armour() const override
		{
			return armour;
		}

		const MeleeData& Melee() const override
		{
			return melee;
		}

		const fstring Description() const override
		{
			return fstring{ desc, (int32) desc.length() };
		}

		void AppendName(Rococo::IStringPopulator& sb) override
		{
			sb.Populate(shortName);
		}

		bool CanFitSlot(int64 flags) const
		{
			if (invData.legalEquipmentSlotFlags != 0)
			{
				return (invData.legalEquipmentSlotFlags & flags) != 0;
			}
			else
			{
				return true;
			}
		}

		const BitmapLocation& Bitmap() const override
		{
			return bitmap;
		}

		void Free() override
		{
			delete this;
		}
	};

	struct ObjectInstance
	{
		ObjectPrototype* prototype;
		int32 stackSize;
	};

	class ObjectPrototypeManager : public IObjectManager
	{
		IRenderer& renderer;
		stringmap<ObjectPrototype*> nameToPrototype;
		typedef HandleTable<ObjectInstance, 16> TObjectTable;
		typedef TObjectTable::Handle THObjId;
		TObjectTable objects;
		AutoFree<ObjectPrototype> prototype = new ObjectPrototype();
	public:
		ObjectPrototypeManager(IRenderer& refRenderer) :
			renderer(refRenderer), objects("objects", 4096)
		{

		}

		virtual ~ObjectPrototypeManager()
		{
			for (auto i : nameToPrototype)
			{
				i.second->Free();
			}
		}

		ID_OBJECT CreateObject(cstr name, int32 stacksize) override
		{
			auto i = nameToPrototype.find(name);
			if (i == nameToPrototype.end())
			{
				Throw(0, "%s: No such object: %s", __FUNCTION__, name);
			}

			auto pProto = i->second;

			if (stacksize <= 0 || stacksize > pProto->invData.maxStackSize)
			{
				Throw(0, "%s: [stacksize] valid range is [1,%d]", __FUNCTION__, pProto->invData.maxStackSize);
			}

			auto hObject = objects.CreateNew();
			objects.Set(hObject, ObjectInstance{ pProto, stacksize });
			return ID_OBJECT{ hObject.Value() };
		}

		ObjectRef GetObject(ID_OBJECT id) override
		{
			THObjId hObject(id.value);

			ObjectInstance instance;
			if (objects.TryGet(hObject, &instance))
			{
				return ObjectRef{ instance.prototype, instance.stackSize };
			}
			else
			{
				return ObjectRef{ nullptr, 0 };
			}
		}

		void AddIcon(const fstring& pingPath) override
		{
			if (pingPath.length < 2) Throw(0, "Expecting at least two characters in [pingPath]");
			if (!renderer.SpriteBuilder().TryGetBitmapLocation(pingPath, prototype->bitmap))
			{
				Throw(0, "No such bitmap [pingPath='%s']", pingPath.buffer);
			}
		}

		void AddDesc(const fstring& desc) override
		{
			prototype->desc = desc;
		}

		void AddShortName(const fstring& shortname) override
		{
			prototype->shortName = shortname;
		}

		void AddDynamics(const HV::ObjectDynamics& od) override
		{
			if (od.mass < 0) { Throw(0, "[mass] must not be negative"); }
			if (od.airFrictionLinearQuotient < 0) { Throw(0, "[aeroLinearTerm] must not be negative"); }
			if (od.airFrictionQuadraticQuotient < 0) { Throw(0, "[aeroQuadTerm] must not be negative"); }
			if (od.span.x < 0 || od.span.y < 0 || od.span.z < 0) { Throw(0, "[span.xyz] components must not be negative"); }
			prototype->dynamics = od;
		}

		void AddMaterial(const HV::MaterialData& material) override
		{
			if (material.atomicNumber < 0)
			{
				Throw(0, "The [atomicNumber] may not be negative");
			}

			if (material.mohsHardness < 0)
			{
				Throw(0, "The [mohsHardness] may not be negative");
			}

			if (material.toughness < 0)
			{
				Throw(0, "The [toughness] may not be negative");
			}

			prototype->material = material;
		}

		void AddMeleeData(const HV::MeleeData& melee) override
		{
			if (melee.baseDamage < 0.0f || melee.baseDamage > 1000.0f) Throw(0, "[baseDamage] has range [0,1000]");
			if (melee.swingSpeed < 0.0f || melee.swingSpeed > 100.0f)  Throw(0, "[swingSpeed] has range [0,100]");
			prototype->melee = melee;
		}

		void AddArmourData(const HV::ArmourData& armour) override
		{
			if (armour.catchProjectilePercentile < 0.0f || armour.catchProjectilePercentile > 100.0f)
			{
				Throw(0, "[catchProjectilePercentile] has range [0,100]");
			}

			if (armour.thickness < 0.0f || armour.thickness > 0.03f)
			{
				Throw(0, "[thickness] has range [0,0.03]");
			}

			prototype->armour = armour;
		}

		void AddSlot(HV::EquipmentSlot slot) override
		{
			prototype->invData.legalEquipmentSlotFlags |= slot;
		}

		void RemoveSlot(HV::EquipmentSlot slot) override
		{
			prototype->invData.legalEquipmentSlotFlags &= ~slot;
		}

		boolean32 HasSlot(HV::EquipmentSlot slot) override
		{
			return (prototype->invData.legalEquipmentSlotFlags & slot) != 0;
		}

		void MakeStackable(int32 nMaxItems) override
		{
			if (nMaxItems < 0 || nMaxItems > 1'000'000'000)
			{
				Throw(0, "[nMaxItems] has range [0,1 billion]");
			}

			prototype->invData.maxStackSize = nMaxItems;
		}

		void Clear() override
		{
			prototype = new ObjectPrototype();
		}

		void CloneFrom(const fstring& source) override
		{
			if (source.length < 2)
			{
				Throw(0, "The source object-prototype name must have at least 2 characters");
			}

			auto i = nameToPrototype.find(source);
			if (i == nameToPrototype.end())
			{
				Throw(0, "No source object-prototype with name '%s' exists", source.buffer);
			}

			prototype->armour = i->second->armour;
			prototype->dynamics = i->second->dynamics;
			prototype->melee = i->second->melee;
			prototype->invData = i->second->invData;
			prototype->material = i->second->material;
			prototype->shortName = i->second->shortName;
			prototype->desc = i->second->desc;
		}

		void Commit(const fstring& uniqueName) override
		{
			if (uniqueName.length < 2)
			{
				Throw(0, "An object prototype name must have at least 2 characters");
			}

			auto i = nameToPrototype.find(uniqueName);

			if (i != nameToPrototype.end())
			{
				Throw(0, "An object prototype with name '%s' already exists", uniqueName.buffer);
			}

			auto j = nameToPrototype.insert(uniqueName, prototype).first;
			prototype->uniqueName = j->first;
			prototype.Release();
			Clear();
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace HV
{
	IObjectManager* CreateObjectManager(IRenderer& renderer)
	{
		return new ANON::ObjectPrototypeManager(renderer);
	}
}