#include <rococo.mplat.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <rococo.strings.h>

#include <algorithm>

namespace
{
   using namespace Rococo;
   using namespace Rococo::Graphics;

   struct MeshBuilder : public Rococo::Graphics::IMeshBuilderSupervisor, IMathsVenue
   {
      std::unordered_map<std::string, ID_SYS_MESH> meshes;
      rchar name[MAX_FQ_NAME_LEN + 1];
      std::vector<ObjectVertex> vertices;
      IRenderer& renderer;

      MeshBuilder(IRenderer& _renderer) : renderer(_renderer)
      {
      }

      void Clear() override
      {
         name[0] = 0;
         vertices.clear();
      }

      void Delete(const fstring& fqName) override
      {
         auto i = meshes.find((cstr) fqName);
         if (i != meshes.end())
         {
            auto id = i->second;
            renderer.DeleteMesh(id);
            meshes.erase(i);
         }
      }

	  struct NameBinding
	  {
		  std::pair<std::string, ID_SYS_MESH> item;

		  bool operator < (const NameBinding& other) const
		  {
			  return item.first < other.item.first;
		  }

		  bool operator == (const NameBinding& other) const
		  {
			  return item.first == other.item.first;
		  }
	  };

	  std::vector<NameBinding> names;

	  void ShowVenue(IMathsVisitor& visitor)
	  {
		  for (auto& m : meshes)
		  {
			  names.push_back({ m });
		  }

		  std::sort(names.begin(), names.end());

		  for(auto& m: names)
		  {
			  char desc[256];
			  renderer.GetMeshDesc(desc, m.item.second);
			  visitor.ShowString(m.item.first.c_str(), "%5llu %s", m.item.second.value, desc);
		  }

		  names.clear();
	  }

      void Free() override
      {
         delete this;
      }

	  IMathsVenue* Venue()
	  {
		  return this;
	  }

      void Begin(const fstring& fqName) override
      {
         if (*name != 0)
         {
            Throw(0, "Call MeshBuilder.End() first");
         }

         ValidateFQNameIdentifier(fqName);

         StackStringBuilder sb(name, sizeof(name));
         sb << fqName;
      }

      void AddTriangle(const ObjectVertex& a, const ObjectVertex& b, const ObjectVertex& c) override
      {
         if (*name == 0) Throw(0, "Call MeshBuilder.Begin() first");
         vertices.push_back(a);
         vertices.push_back(b);
         vertices.push_back(c);
      }

      void End() override
      {
         const ObjectVertex* v = vertices.empty() ? nullptr : (const ObjectVertex*)&vertices[0];

         auto i = meshes.find(name);
         if (i != meshes.end())
         {
            renderer.UpdateMesh(i->second, v, (uint32)vertices.size());
         }
         else
         {

            auto id = renderer.CreateTriangleMesh(v, (uint32)vertices.size());
            meshes[name] = id;
         }

         Clear();
      }

      virtual bool TryGetByName(cstr name, ID_SYS_MESH& id)
      {
         auto i = meshes.find(name);
         if (i == meshes.end())
         {
            id = ID_SYS_MESH::Invalid();
            return false;
         }
         else
         {
            id = i->second;
            return true;
         }
      }
   };
}

namespace Rococo
{
   namespace Graphics
   {
      IMeshBuilderSupervisor* CreateMeshBuilder(IRenderer& renderer)
      {
         return new MeshBuilder(renderer);
      }
   }
}