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

   struct MeshBinding
   {
	   ID_SYS_MESH id; // Mesh id as managed by the renderer
	   AABB bounds; // Bounding box surrounding mesh
	   ObjectVertex* pVertexArray;
	   size_t nVertices;
   };

   struct MeshBuilder : public Rococo::Graphics::IMeshBuilderSupervisor, IMathsVenue
   {
      std::unordered_map<std::string, MeshBinding> meshes;
	  rchar name[MAX_FQ_NAME_LEN + 1] = { 0 };
      std::vector<ObjectVertex> vertices;
      IRenderer& renderer;

      MeshBuilder(IRenderer& _renderer) : renderer(_renderer)
      {
      }

	  ~MeshBuilder()
	  {
		  for (auto& i : meshes)
		  {
			  auto& mesh = i.second;
			  renderer.DeleteMesh(mesh.id);
			  delete[] mesh.pVertexArray;
		  }

		  meshes.clear();
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
            auto& mesh = i->second;
            renderer.DeleteMesh(mesh.id);
			delete[] mesh.pVertexArray;
            meshes.erase(i);
         }
      }

	  struct NameBinding
	  {
		  std::pair<std::string, MeshBinding> item;

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
			  renderer.GetMeshDesc(desc, m.item.second.id);

			  auto& b = m.item.second.bounds;
			  visitor.ShowString(m.item.first.c_str(), "%5llu %s", m.item.second.id.value, desc);
		//	  visitor.ShowString(" -> bounds", "(%f, %f, %f) to (%f, %f, %f)", b.minXYZ.x, b.minXYZ.y, b.minXYZ.z, b.maxXYZ.x, b.maxXYZ.y, b.maxXYZ.z);
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

	  void AddTriangleEx(const VertexTriangle& t)
	  {
		  if (*name == 0) Throw(0, "Call MeshBuilder.Begin() first");
		  vertices.push_back(t.a);
		  vertices.push_back(t.b);
		  vertices.push_back(t.c);
	  }

      void End(boolean32 preserveMesh) override
      {
         const ObjectVertex* v = vertices.empty() ? nullptr : (const ObjectVertex*)&vertices[0];

		 AABB boundingBox;

		 for (auto& v : vertices)
		 {
			 boundingBox << v.position;
		 }

		 ObjectVertex* backup = nullptr;

		 if (preserveMesh)
		 {
			 backup = new ObjectVertex[vertices.size()];
			 memcpy(backup, &vertices[0], vertices.size() * sizeof(ObjectVertex));
		 }

         auto i = meshes.find(name);
         if (i != meshes.end())
         {
			i->second.bounds = boundingBox;
            renderer.UpdateMesh(i->second.id, v, (uint32)vertices.size());

			if (preserveMesh)
			{
				i->second.nVertices = vertices.size();
				delete[] i->second.pVertexArray;
				i->second.pVertexArray = backup;
			}
         }
         else
         {
            auto id = renderer.CreateTriangleMesh(v, (uint32)vertices.size());
			meshes[name] = MeshBinding { id, boundingBox, backup, vertices.size() };
         }

         Clear();
      }

      virtual bool TryGetByName(cstr name, ID_SYS_MESH& id, AABB& bounds)
      {
         auto i = meshes.find(name);
         if (i == meshes.end())
         {
            id = ID_SYS_MESH::Invalid();
			bounds = AABB();
            return false;
         }
         else
         {
            id = i->second.id;
			bounds = i->second.bounds;
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