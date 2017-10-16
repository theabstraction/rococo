#include <rococo.mplat.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <rococo.strings.h>

namespace
{
   using namespace Rococo;
   using namespace Rococo::Graphics;

   struct MeshBuilder : public Rococo::Graphics::IMeshBuilderSupervisor
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

      void Free() override
      {
         delete this;
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