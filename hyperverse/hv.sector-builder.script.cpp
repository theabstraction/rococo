#include "hv.h"
#include "rococo.mplat.h"
#include <rococo.maths.h>
#include <vector>
#include <algorithm>
#include <rococo.hashtable.h>
#include <rococo.clock.h>
#include <rococo.sexy.api.h>

using namespace HV;
using namespace Rococo::Graphics;

class SectorBuilder: public ISectorBuilderSupervisor
{
private:
	std::vector<Vec2> vertices;
	bool generateMesh = true;
	ISectorBuildAPI& api;

	std::string door_scriptName;
	bool door_useScript = false;

	std::string wall_scriptName;
	bool wall_useScript = false;

	std::string floor_scriptName;
	bool floor_useScript = false;

	stringmap<Material*> nameToMaterials;
	stringmap<float> doorVars;
	stringmap<float> floorVars;
	stringmap<float> wallVars;

public:
	SectorBuilder(ISectorBuildAPI& _api) :
		api(_api)
	{

	}

	virtual ~SectorBuilder()
	{
		for (auto i : nameToMaterials)
		{
			delete i.second;
		}
	}

	void Free() override
	{
		delete this;
	}

	void AddVertex(float x, float y) override
	{
		vertices.push_back(Vec2{ x, y });
	}

	void Clear() override
	{
		vertices.clear();
		api.ClearSectors();
	}

	void DisableMeshGeneration() override
	{
		generateMesh = false;
	}

	void EnableMeshGeneration() override
	{
		generateMesh = true;
	}

	void GenerateMeshes() override
	{
		api.GenerateMeshes();
	}

	bool IsMeshGenerationEnabled() const override
	{
		return generateMesh;
	}

	int32 CreateFromTemplate(int32 altitude, int32 height) override
	{
		struct Anon: MatEnumerator
		{
			SectorBuilder& sb;
			void Enumerate(IEventCallback<MaterialArgs>& cb) override
			{
				for (auto i : sb.nameToMaterials)
				{
					MaterialArgs args{ i.second, i.first };
					cb.OnEvent(args);
				}
			}

			Anon(SectorBuilder& _sb) : sb(_sb) {}
		} mats(*this);

		ISector* s = api.CreateSector();

		s->SetTemplate(mats);

		try
		{
			s->Build(&vertices[0], vertices.size(), 0.01f * altitude, 0.01f * (altitude + height));
			api.Attach(s);
		}
		catch (IException&)
		{
			s->Free();
			vertices.clear();
			throw;
		}

		vertices.clear();

		wallVars.clear();
		floorVars.clear();
		doorVars.clear();

		api.UpdateProgress(s->Id());

		return s->Id();
	}

	void SetTemplateWallScript(boolean32 useScript, const fstring& scriptName) override
	{
		wall_useScript = useScript;
		wall_scriptName = scriptName;
	}

	void SetTemplateDoorScript(boolean32 hasDoor, const fstring& scriptName) override
	{
		door_useScript = hasDoor;
		door_scriptName = scriptName;
	}

	void SetTemplateFloorScript(boolean32 useScript, const fstring& scriptName) override
	{
		floor_useScript = useScript;
		floor_scriptName = scriptName;
	}
	
	void SetTemplateMaterial(const fstring& bodyClass, MaterialCategory cat, RGBAb colour, const fstring& persistentId) override
	{
		auto i = nameToMaterials.find((cstr)bodyClass);
		if (i == nameToMaterials.end())
		{
			i = nameToMaterials.insert(bodyClass, new Material).first;
		}

		i->second->category = cat;
		i->second->mvd.colour = colour;
		SafeFormat(i->second->persistentName, IO::MAX_PATHLEN, "%s", (cstr)persistentId);
		i->second->mvd.materialId = api.GetMaterialId(i->second->persistentName);
		if (i->second->mvd.materialId < 0)
		{
			i->second->mvd.materialId = api.GetRandomMaterialId(cat);
		}
	}

	void SetWallScriptF32(const fstring& name, float value) override
	{
		wallVars[(cstr)name] = value;
	}

	void SetFloorScriptF32(const fstring& name, float value) override
	{
		floorVars[(cstr)name] = value;
	}

	void SetCorridorScriptF32(const fstring& name, float value)override
	{
		doorVars[(cstr)name] = value;
	}

	void EnumerateDoorVars(IEventCallback<VariableCallbackData>& cb) override
	{
		for (auto& v : doorVars)
		{
			cb.OnEvent(VariableCallbackData{ v.first, v.second });
		}
	}

	void EnumerateWallVars(IEventCallback<VariableCallbackData>& cb) override
	{
		for (auto& v : wallVars)
		{
			cb.OnEvent(VariableCallbackData{ v.first, v.second });
		}
	}

	void EnumerateFloorVars(IEventCallback<VariableCallbackData>& cb) override
	{
		for (auto& v : floorVars)
		{
			cb.OnEvent(VariableCallbackData{ v.first, v.second });
		}
	}

	cstr GetTemplateFloorScript(bool& usesScript) const override
	{
		usesScript = floor_useScript;
		return floor_scriptName.empty() ? "" : floor_scriptName.c_str();
	}

	cstr GetTemplateDoorScript(bool& hasDoor) const override
	{
		hasDoor = door_useScript;
		return door_scriptName.empty() ? "" : door_scriptName.c_str();
	}

	cstr GetTemplateWallScript(bool& usesScript) const override
	{
		usesScript = wall_useScript;
		return wall_scriptName.empty() ? "" : wall_scriptName.c_str();
	}
};

namespace HV
{
	ISectorBuilderSupervisor* CreateSectorBuilder(ISectorBuildAPI& api)
	{
		return new SectorBuilder(api);
	}
}