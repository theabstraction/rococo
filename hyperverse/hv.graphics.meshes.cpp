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
      std::unordered_map<std::wstring, ID_SYS_MESH> meshes;
      wchar_t name[Strings::MAX_FQ_NAME_LEN + 1];
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
            Throw(0, L"Call MeshBuilder.End() first");
         }
         Strings::ValidateFQNameIdentifier(fqName);
         SafeCopy(name, fqName, _TRUNCATE);
      }

      virtual void AddTriangle(const Vertex& a, const Vertex& b, const Vertex& c)
      {
         if (*name == 0) Throw(0, L"Call MeshBuilder.Begin() first");
         vertices.push_back(a);
         vertices.push_back(b);
         vertices.push_back(c);
      }

      virtual void End()
      {
         if (vertices.empty())
         {
            Throw(0, L"MeshBuilder::End(): There are no mesh vertices. Empty meshes are forbidden.");
         }

         auto i = meshes.find(name);
         if (i != meshes.end())
         {
            renderer.UpdateMesh(i->second, (const ObjectVertex*)&vertices[0], (uint32) vertices.size());
         }
         else
         {
            auto id = renderer.CreateTriangleMesh((const ObjectVertex*)&vertices[0], (uint32) vertices.size());
            meshes[name] = id;
         }

         Clear();
      }

      virtual bool TryGetByName(const wchar_t* name, ID_SYS_MESH& id)
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