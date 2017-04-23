#include "hv.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <rococo.strings.h>

namespace
{
   using namespace HV;
   using namespace HV::Graphics;

   struct MeshBuilder : public HV::Graphics::IMeshBuilderSupervisor
   {
      std::unordered_map<std::string, ID_SYS_MESH> meshes;
      rchar name[Strings::MAX_FQ_NAME_LEN + 1];
      std::vector<Vertex> vertices;
      IRenderer& renderer;

      MeshBuilder(IRenderer& _renderer): renderer(_renderer)
      {
      }

      virtual void Clear()
      {
         name[0] = 0;
         vertices.clear();
      }

      virtual void Free()
      {
         delete this;
      }

      virtual void Begin(const fstring& fqName)
      {
         if (*name != 0)
         {
            Throw(0, "Call MeshBuilder.End() first");
         }
         Strings::ValidateFQNameIdentifier(fqName);
         SafeCopy(name, fqName, _TRUNCATE);
      }

      virtual void AddTriangle(const Vertex& a, const Vertex& b, const Vertex& c)
      {
         if (*name == 0) Throw(0, "Call MeshBuilder.Begin() first");
         vertices.push_back(a);
         vertices.push_back(b);
         vertices.push_back(c);
      }

      virtual void End()
      {
         static_assert(sizeof(ObjectVertex) == sizeof(Vertex), "Packing error");
         
         const ObjectVertex* v = vertices.empty() ? nullptr : (const ObjectVertex*)&vertices[0];

         auto i = meshes.find(name);
         if (i != meshes.end())
         {     
            renderer.UpdateMesh(i->second, v, (uint32) vertices.size());
         }
         else
         {
            
            auto id = renderer.CreateTriangleMesh(v, (uint32) vertices.size());
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

namespace HV
{
   namespace Graphics
   {
      IMeshBuilderSupervisor* CreateMeshBuilder(IRenderer& renderer)
      {
         return new MeshBuilder(renderer);
      }
   }
}