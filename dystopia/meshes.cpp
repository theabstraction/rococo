#include "dystopia.h"
#include "meshes.h"

#include <sexy.types.h>
#include <Sexy.S-Parser.h>

#include <unordered_map>


namespace Dystopia
{
	using namespace Rococo;
	using namespace Sexy::Sex;
	void ThrowSex(cr_sex s, const wchar_t* format, ...);
	void ScanExpression(cr_sex s, const wchar_t* hint, const char* format, ...);
	void ValidateArgument(cr_sex s, const wchar_t* arg);
	int32 GetValue(cr_sex s, int32 minValue, int32 maxValue, const wchar_t* hint);
	Vec3 GetVec3Value(cr_sex x, cr_sex y, cr_sex z);
	float GetValue(cr_sex s, float minValue, float maxValue, const wchar_t* hint);
	RGBAb GetColourValue(cr_sex s);
	void ExecuteSexyScript(size_t nMegabytesCapacity);
}

namespace
{
	using namespace Rococo;
	using namespace Dystopia;
	using namespace Sexy;
	using namespace Sexy::Sex;

	typedef std::vector<ObjectVertex> TVertices;

	struct MeshDesc
	{
		ID_SYS_MESH rendererId;
		ID_MESH editorId;
		std::wstring resourceName;
	};

	typedef std::vector<BoundingCube> BoundingCubes;
	typedef std::unordered_map<ID_MESH, MeshDesc, ID_MESH> TMeshes;
	typedef std::unordered_map<ID_MESH, BoundingCubes, ID_MESH> TMeshBounds;

	void ParseVertex(ObjectVertex& v, cr_sex sv)
	{
		if (sv.NumberOfElements() == 10)
		{
			const ISExpression* svx, *svy, *svz, *snx, *sny, *snz, *semissive, *sdiffuse, *stu, *stv;
			ScanExpression(sv, L"(vx vy vz nx ny nz emissive diffuse u v)", "a a a a a a a a a a", &svx, &svy, &svz, &snx, &sny, &snz, &semissive, &sdiffuse, &stu, &stv);

			v.position = GetVec3Value(*svx, *svy, *svz);
			v.normal = GetVec3Value(*snx, *sny, *snz);
			v.emissiveColour = GetColourValue(*semissive);
			v.diffuseColour = GetColourValue(*sdiffuse);
			v.u = (float)_wtof(stu->String()->Buffer);
			v.v = (float)_wtof(stv->String()->Buffer);
		}
		else
		{
			const ISExpression* svx, *svy, *svz, *snx, *sny, *snz, *semissive, *sdiffuse;
			ScanExpression(sv, L"(vx vy vz nx ny nz emissive diffuse u v)", "a a a a a a a a", &svx, &svy, &svz, &snx, &sny, &snz, &semissive, &sdiffuse);

			v.position = GetVec3Value(*svx, *svy, *svz);
			v.normal = GetVec3Value(*snx, *sny, *snz);
			v.emissiveColour = GetColourValue(*semissive);
			v.diffuseColour = GetColourValue(*sdiffuse);
			v.diffuseColour.alpha = 0;
			v.u = 0;
			v.v = 0;
		}
	}

	void GenerateHull(BoundingCube& cube, const ObjectVertex* vertices, size_t nVertices)
	{
		float west = 1.0e24f, east = -1.0e24f, north = -1.0e24f, south = 1.0e24f, top = -1.0e24f, bottom = 1.0e24f;

		for (auto* pV = vertices; pV < vertices + nVertices; ++pV)
		{
			auto& v = *pV;
			west = min(v.position.x, west);
			east = max(v.position.x, east);
			south = min(v.position.y, south);
			north = max(v.position.y, north);
			top = max(v.position.z, top);
			bottom = min(v.position.z, bottom);
		}

		cube.topVertices.v.sw = { west, south, top };
		cube.topVertices.v.se = { east, south, top };
		cube.topVertices.v.nw = { west, north, top };
		cube.topVertices.v.ne = { east, north, top };

		cube.bottomVertices.v.sw = { west, south, bottom };
		cube.bottomVertices.v.se = { east, south, bottom };
		cube.bottomVertices.v.nw = { west, north, bottom };
		cube.bottomVertices.v.ne = { east, north, bottom };

		cube.P.westEest.P0.normal = { -1, 0, 0, 0 };
		cube.P.westEest.P0.pointInPlane = Vec4::FromVec3(0.5f * (cube.topVertices.v.nw + cube.bottomVertices.v.sw), 1.0f);
		cube.P.westEest.P1.normal = {  1, 0, 0, 0 };
		cube.P.westEest.P1.pointInPlane = Vec4::FromVec3(0.5f * (cube.topVertices.v.ne + cube.bottomVertices.v.se), 1.0f);

		cube.P.southNorth.P0.normal = {0, -1, 0, 0 };
		cube.P.southNorth.P0.pointInPlane = Vec4::FromVec3(0.5f * (cube.topVertices.v.sw + cube.bottomVertices.v.se), 1.0f);
		cube.P.southNorth.P1.normal = {0, 1, 0, 0 };
		cube.P.southNorth.P1.pointInPlane = Vec4::FromVec3(0.5f * (cube.topVertices.v.nw + cube.bottomVertices.v.ne), 1.0f);

		cube.P.bottomTop.P0.normal = { 0, 0, -1, 0 };
		cube.P.bottomTop.P0.pointInPlane = Vec4::FromVec3(0.5f * (cube.bottomVertices.v.sw + cube.bottomVertices.v.ne), 1.0f);
		cube.P.bottomTop.P1.normal = { 0, 0, 1, 0 };
		cube.P.bottomTop.P1.pointInPlane = Vec4::FromVec3(0.5f * (cube.topVertices.v.sw + cube.topVertices.v.ne), 1.0f);
	}

	void ParseMeshVertices(TVertices& vertexCache, TMeshBounds& physicsHulls, TMeshes& meshes, ID_MESH id, IRenderer& renderer, cr_sex meshDef, const wchar_t* resourcePath, bool generateHull)
	{
		size_t nVertices = meshDef.NumberOfElements() - 1;
		vertexCache.reserve(nVertices);
		vertexCache.clear();

		if (nVertices % 3 != 0)
		{
			ThrowSex(meshDef, L"Expecting 3 vertices per triangle");
		}

		ObjectVertex v{ Vec3{0,0,0}, Vec3{ 0,0,0 }, RGBAb(0,0,0), RGBAb(0,0,0) };

		for (int i = 0; i < nVertices; ++i)
		{
			cr_sex sv = meshDef[i + 1];
			ParseVertex(v, sv);
			vertexCache.push_back(v);
		}

		if (generateHull)
		{
			BoundingCube cube;
			GenerateHull(cube, &vertexCache[0], vertexCache.size());

			auto h = physicsHulls.find(id);
			if (h != physicsHulls.end())
			{
				h->second.clear();
			}
			else
			{
				BoundingCubes cubes;
				h = physicsHulls.insert(std::make_pair(id, cubes)).first;
			}

			h->second.push_back(cube);
		}

		auto i = meshes.find(id);
		if (i != meshes.end())
		{
			renderer.UpdateMesh(i->second.rendererId, &vertexCache[0], (uint32)vertexCache.size());
			i->second.resourceName = resourcePath;
		}
		else
		{
			auto rendererId = renderer.CreateTriangleMesh(&vertexCache[0], (uint32)vertexCache.size());
			meshes.insert(std::make_pair(id, MeshDesc{ rendererId, id, resourcePath }));
		}
	}

	void ParseMeshScript(TVertices& vertexCache, TMeshBounds& physicsHulls, TMeshes& meshes, ISParserTree& tree, IRenderer& renderer, const wchar_t* resourcePath, ID_MESH editorId)
	{
		cr_sex root = tree.Root();

		if (root.NumberOfElements() < 1)
		{
			ThrowSex(root, L"No elements in the script file");
		}

		cr_sex version = root[0];

		const ISExpression* quote, *category, *filetype;
		ScanExpression(version, L"(' file.type dystopia.mesh)", "a a a", &quote, &category, &filetype);

		ValidateArgument(*quote, L"'");
		ValidateArgument(*category, L"file.type");
		ValidateArgument(*filetype, L"dystopia.mesh");

		bool hasMesh = false;
		bool autoHull = false;

		for (int i = 1; i < root.NumberOfElements(); ++i)
		{
			cr_sex sdirective = root[i];

			if (sdirective.NumberOfElements() < 2)
			{
				ThrowSex(sdirective, L"Number of elements fewer than 2 in directive.");
			}

			cr_sex scmd = sdirective[0];
			if (!IsAtomic(scmd))
			{
				ThrowSex(scmd, L"Expecting command directive, one of {mesh,hull}");
			}

			if (scmd == L"mesh")
			{
				if (hasMesh)
				{
					ThrowSex(scmd, L"Only one mesh definition per mesh file is allowed");
				}

				const ISExpression* meshconfirm;
				ScanExpression(sdirective, L"(mesh (vertices ...))", "a", &meshconfirm);

				ValidateArgument(*meshconfirm, L"mesh");

				ParseMeshVertices(vertexCache, physicsHulls, meshes, editorId, renderer, sdirective, resourcePath, autoHull);

				hasMesh = true;
			}
			else if (scmd == L"physics")
			{
				const ISExpression* cmd2;
				const ISExpression* stype;
				ScanExpression(sdirective, L"(physics <auto|none>)", "a a", &cmd2, &stype);

				if (*stype == L"none")
				{
					autoHull = false;
				}
				else if (*stype == L"auto")
				{
					autoHull = true;
				}
				else
				{
					ThrowSex(*stype, L"Unexpected argument, must be one of {auto,none}");
				}
			}
			else
			{
				ThrowSex(sdirective, L"Unknown directive");
			}
		}
	}

	class MeshLoader : public IMeshLoader
	{
		IInstallation& installation;
		ISourceCache& sourceCache;
		IRenderer& renderer;

		AutoFree<IExpandingBuffer> meshFileImage;
		AutoFree<IExpandingBuffer> unicodeFileImage;
		TMeshes meshes;
		TVertices vertexCache;

		TMeshBounds physicsHulls;

		enum { MAX_MESH_FILE_SIZE = 256 * 1024 };
	public:
		MeshLoader(IInstallation& _installation, IRenderer& _renderer, ISourceCache& _sourceCache):
			installation(_installation),
			renderer(_renderer),
			sourceCache(_sourceCache),
			meshFileImage(CreateExpandingBuffer(MAX_MESH_FILE_SIZE)),
			unicodeFileImage(CreateExpandingBuffer(0))
		{
		}

		virtual void BuildMesh(const ObjectVertex* vertices, size_t vertexCount, ID_MESH id, bool createPhysicsBox)
		{
			if (vertexCount > 0x00000000FFFFFFFF)
			{
				Throw(0, L"BuildMesh failed. Vertex count was too high");
			}

			auto i = meshes.find(id);
			if (i != meshes.end())
			{
				renderer.UpdateMesh(i->second.rendererId, vertices, (uint32)vertexCount);
				i->second.resourceName = L"#generated";
			}
			else
			{
				auto rendererId = renderer.CreateTriangleMesh(vertices, (uint32)vertexCount);
				meshes.insert(std::make_pair(id, MeshDesc{ rendererId, id, L"#generated" }));
			}

			if (createPhysicsBox)
			{
				BoundingCube cube;
				GenerateHull(cube, vertices, vertexCount);

				auto h = physicsHulls.find(id);
				if (h != physicsHulls.end())
				{
					h->second.clear();
				}
				else
				{
					BoundingCubes cubes;
					h = physicsHulls.insert(std::make_pair(id, cubes)).first;
				}

				h->second.push_back(cube);
			}
		}

		virtual size_t ForEachPhysicsHull(ID_MESH id, IEnumerator<BoundingCube>& cb)
		{
			auto i = physicsHulls.find(id);
			if (i == physicsHulls.end())
			{
				return 0;
			}
			else
			{
				for (auto& cube : i->second)
				{
					cb(cube);
				}
				return i->second.size();
			}
		}

		ID_SYS_MESH GetRendererId(ID_MESH editorId)
		{
			auto i = meshes.find(editorId);
			if (i == meshes.end())
			{
				return ID_SYS_MESH();
			}

			return i->second.rendererId;
		}

		virtual void Load(const fstring& resourcePath, ID_MESH editorId)
		{
			Load(resourcePath, editorId, false);
		}

		void Load(const fstring& resourcePath, ID_MESH editorId, bool isReloading)
		{
			using namespace Dystopia;

			while (true)
			{
				try
				{
					ProtectedLoadMesh(resourcePath.buffer, editorId);
					return;
				}
				catch (IException& ex)
				{
					installation.OS().FireUnstable();
					CMD_ID id = ShowContinueBox(renderer.Window(), ex.Message());
					switch (id)
					{
					case CMD_ID_EXIT:
						Throw(ex.ErrorCode(), L"%s", ex.Message());
						break;
					case CMD_ID_RETRY:
						break;
					case CMD_ID_IGNORE:
						return;
					}
				}
			}
		}

		void ProtectedLoadMesh(const wchar_t* resourcePath, ID_MESH editorId)
		{
			try
			{
				auto tree = sourceCache.GetSource(resourcePath);
				ParseMeshScript(vertexCache, physicsHulls, meshes, *tree, renderer, resourcePath, editorId);
			}
			catch (Sexy::Sex::ParseException& pex)
			{
				SourcePos p = pex.Start();
				SourcePos q = pex.End();
				Throw(pex.ErrorCode(), L"Error parsering %s\n%s: %s\n(%d,%d) to (%d,%d)\n%s", resourcePath, pex.Name(), pex.Message(), p.X, p.Y, q.X, q.Y, pex.Specimen());
			}
		}

		virtual void Free()
		{
			delete this;
		}

		virtual void UpdateMesh(const wchar_t* filename)
		{
			for (auto i : meshes)
			{
				auto& mesh = i.second;
				if (DoesModifiedFilenameMatchResourceName(filename, mesh.resourceName.c_str()))
				{
					fstring resource = { mesh.resourceName.c_str(), (int32) mesh.resourceName.length() };
					Load(resource, i.first, true);
					return;
				}
			}
		}
	};
}

namespace Dystopia
{
	IMeshLoader* CreateMeshLoader(IInstallation& _installation, IRenderer& _renderer, ISourceCache& _sourceCache)
	{
		return new MeshLoader(_installation, _renderer, _sourceCache);
	}
}