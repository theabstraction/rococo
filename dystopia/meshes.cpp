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

	struct MeshDesc
	{
		ID_MESH rendererId;
		int32 editorId;
		std::wstring resourceName;
	};

	typedef std::unordered_map<int, MeshDesc> TMeshes;

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

	void ParseMeshVertices(TMeshes& meshes, int32 id, IRenderer& renderer, cr_sex meshDef, const wchar_t* resourcePath)
	{
		size_t nVertices = meshDef.NumberOfElements() - 1;
		std::vector<ObjectVertex> vertices;
		vertices.reserve(nVertices);

		if (nVertices % 3 != 0)
		{
			ThrowSex(meshDef, L"Expecting 3 vertices per triangle");
		}

		ObjectVertex v{ Vec3{0,0,0}, Vec3{ 0,0,0 }, RGBAb(0,0,0), RGBAb(0,0,0) };

		for (int i = 0; i < nVertices; ++i)
		{
			cr_sex sv = meshDef[i + 1];
			ParseVertex(v, sv);
			vertices.push_back(v);
		}

		auto i = meshes.find(id);
		if (i != meshes.end())
		{
			renderer.UpdateMesh(i->second.rendererId, &vertices[0], (uint32)vertices.size());
			i->second.resourceName = resourcePath;
		}
		else
		{
			auto rendererId = renderer.CreateTriangleMesh(&vertices[0], (uint32)vertices.size());
			meshes.insert(std::make_pair(id, MeshDesc{ rendererId, id, resourcePath }));
		}
	}

	void ParseMeshScript(TMeshes& meshes, ISParserTree& tree, IRenderer& renderer, const wchar_t* resourcePath, int32 editorId)
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

		if (root.NumberOfElements() != 2)
		{
			ThrowSex(root, L"Only two elements are allowed in a mesh script - the version expression and the mesh definition");
		}

		cr_sex meshDef = root[1];
		const ISExpression* meshconfirm;
		ScanExpression(meshDef, L"(mesh (vertices ...))", "a", &meshconfirm);

		ValidateArgument(*meshconfirm, L"mesh");

		ParseMeshVertices(meshes, editorId, renderer, meshDef, resourcePath);
	}

	class MeshLoader : public IMeshLoader
	{
		IInstallation& installation;
		ISourceCache& sourceCache;
		IRenderer& renderer;

		AutoFree<IExpandingBuffer> meshFileImage;
		AutoFree<IExpandingBuffer> unicodeFileImage;
		TMeshes meshes;

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

		virtual void EnumeratePhysicsHullTriangles(ID_MESH id, ITriangleEnumerator& cb)
		{

		}

		ID_MESH GetRendererId(int32 editorId)
		{
			auto i = meshes.find(editorId);
			if (i == meshes.end())
			{
				return 0;
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
				ParseMeshScript(meshes, *tree, renderer, resourcePath, editorId);
			}
			catch (Sexy::Sex::ParseException& pex)
			{
				SourcePos p = pex.Start();
				SourcePos q = pex.End();
				Throw(pex.ErrorCode(), L"Error parsering %s\n%s: %s\n(%d,%d) to (%d,%d)", resourcePath, pex.Name(), pex.Message(), p.X, p.Y, q.X, q.Y);
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