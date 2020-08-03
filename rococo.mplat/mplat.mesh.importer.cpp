#include <rococo.mplat.h>
#include <sexy.types.h>
#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::Sex;

#include <vector>

struct FileVertex
{
	Vec3 position;
	Vec3 normal;
};

void Check(cr_sex s, cstr checkString)
{
	fstring sanityCheck = GetAtomicArg(s);
	if (!Eq(sanityCheck, checkString)) Throw(s, "Expected string '%s'", checkString);
}

float AsF32(cr_sex s)
{
	if (s.Type() != EXPRESSION_TYPE_ATOMIC)
	{
		Throw(s, "Expecting 32-bit floating point value at this position");
	}

	cstr text = s.String()->Buffer;

	// Microsoft's _atoflt does not create floating point exceptions
	// and could be faster that atof, as it is for 32-bit floats, rather than 64-bit doubles
	_CRT_FLOAT result;
	int code = _atoflt(&result, s.String()->Buffer);
	if (code != 0)
	{
		Throw(s, "Could not interpret string as a 32-bit floating point number");
	}
	return result.f;
}

int32 AsI32(cr_sex s)
{
	if (s.Type() != EXPRESSION_TYPE_ATOMIC)
	{
		Throw(s, "Expecting 32-bit int value at this position");
	}

	cstr text = s.String()->Buffer;
	return atoi(text);
}

int32 ParseMat(cr_sex s)
{
	if (s.Type() != EXPRESSION_TYPE_ATOMIC)
	{
		Throw(s, "Expecting '<material-index>' at this position");
	}

	cstr text = s.String()->Buffer;

	int index = atoi(text);
	return index;
}

void ParsePolyVertex(ObjectVertex& v, cr_sex sPolyVertex, const std::vector<FileVertex>& vertexArray)
{
	if (sPolyVertex.Type() == EXPRESSION_TYPE_ATOMIC)
	{
		int vertexIndex = AsI32(sPolyVertex);
		if (vertexIndex < 0 || vertexIndex >= vertexArray.size())
		{
			Throw(sPolyVertex, "Index out of range. Max size is %llu", vertexArray.size() - 1);
		}

		auto& vertex = vertexArray[vertexIndex];
		v.normal = vertex.normal;
		v.position = vertex.position;
		v.uv = { 0,0 };
	}
	else
	{
		if (sPolyVertex.NumberOfElements() != 3) Throw(sPolyVertex, "Expecting (<index> u v)");

		int vertexIndex = AsI32(sPolyVertex[0]);
		if (vertexIndex < 0 || vertexIndex >= vertexArray.size())
		{
			Throw(sPolyVertex[0], "Index out of range. Max size is %llu", vertexArray.size() - 1);
		}

		auto& vertex = vertexArray[vertexIndex];
		v.normal = vertex.normal;
		v.position = vertex.position;

		float txu = AsF32(sPolyVertex[1]);
		float txv = AsF32(sPolyVertex[2]);

		v.uv = { txu, txv };
	}
}

fstring GetStringLiteral(cr_sex s)
{
	if (s.Type() != EXPRESSION_TYPE_STRING_LITERAL) Throw(s, "Expecting string literal");
	return fstring{ s.String()->Buffer, s.String()->Length };
}

struct LocalMaterial
{
	RGBAb diffuse;
	RGBAb specular;
	size_t NumberOfTextures;

	enum { MAX_TEXTURES = 1 };
	MaterialId sysMatId[MAX_TEXTURES];
};

static uint8 F32_to_UI8(float x)
{
	float X = 255.0f * clamp(x, 0.0f, 1.0f);
	return (uint8)(uint32) X;
}

void ParseColourArguments(cr_sex s, RGBAb& colour)
{
	if (s.NumberOfElements() != 4) Throw(s, "Expected 4 elements in colour definition");
	float red = AsF32(s[1]);
	float green = AsF32(s[2]);
	float blue = AsF32(s[3]);
	colour = RGBAb(F32_to_UI8(red), F32_to_UI8(green), F32_to_UI8(blue), 255);
}

void ParseLocalMaterial(cr_sex s, LocalMaterial& mat, IRenderer& renderer)
{
	mat = { 0 };
	if (s.NumberOfElements() < 4) Throw(s, "Expected at least 2 elements");
	cr_sex sIndex = s[0];
	cr_sex sName = s[1];
	AssertStringLiteral(sName);

	for (int i = 2; i < s.NumberOfElements(); ++i)
	{
		cr_sex sProperty = s[i];
		if (sProperty.NumberOfElements() < 2)
		{
			Throw(sProperty, "Expecting at least 2 elements in material property expression");
		}

		cr_sex sPropId = sProperty[0];
		auto& propId = GetAtomicArg(sPropId);
		if (Eq(propId, "diffuse"))
		{
			ParseColourArguments(sProperty, mat.diffuse);
		}
		else if (Eq(propId, "specular"))
		{
			ParseColourArguments(sProperty, mat.specular);
		}
		else if (Eq(propId, "texture"))
		{
			if (mat.NumberOfTextures == LocalMaterial::MAX_TEXTURES)
			{
				Throw(sPropId, "Local material definition already specified the maximum textures count");
			}

			fstring pingPath = GetAtomicArg(sProperty[1]);
			auto matId = renderer.GetMaterialId(pingPath);
			if (matId < 0) matId = 0; // use a default if it is missing
			mat.sysMatId[mat.NumberOfTextures++] = matId;
		}
	}
}

void LoadMesh(Platform& platform, cr_sex s)
{
	// (mesh <name> (vertices ...)(polygons ...)))

	if (s.NumberOfElements() != 6) Throw(s, "Expected (mesh <name> (vertices ...)(polygons ...)(materials ...)(shaders)");

	Check(s[0], "mesh");

	fstring name = GetStringLiteral(s[1]);

	static std::vector<FileVertex> vertexArray;
	vertexArray.clear();

	cr_sex sVertices = s[2];
	if (sVertices.NumberOfElements() < 2)
	{
		Throw(sVertices, "Expected (vertices ...)");
	}

	Check(sVertices[0], "vertices");

	for (int32 i = 1; i < sVertices.NumberOfElements(); ++i)
	{
		FileVertex v;
		cr_sex sVertex = sVertices[i];
		if (sVertex.NumberOfElements() != 7) Throw(sVertex, "Expected (<index> px py pz nx ny nz)");
		v.position.x = AsF32(sVertex[1]);
		v.position.y = AsF32(sVertex[2]);
		v.position.z = AsF32(sVertex[3]);
		v.normal.x = AsF32(sVertex[4]);
		v.normal.y = AsF32(sVertex[5]);
		v.normal.z = AsF32(sVertex[6]);
		vertexArray.push_back(v);
	}

	// Process materials first, so we build up a list of materials before parsing the dependent polygons.
	cr_sex sMaterials = s[4];
	if (sMaterials.NumberOfElements() < 1)
	{
		Throw(sMaterials, "Expected (materials ...)");
	}

	static std::vector<LocalMaterial> localMaterials;
	localMaterials.clear();
	
	for (int i = 1; i < sMaterials.NumberOfElements(); ++i)
	{
		LocalMaterial mat;
		ParseLocalMaterial(sMaterials[i], mat, platform.renderer);
		localMaterials.push_back(mat);
	}

	cr_sex sPolygons = s[3];
	if (sPolygons.NumberOfElements() < 2)
	{
		Throw(sPolygons, "Expected (polygons ...)");
	}

	Check(sPolygons[0], "polygons");

	platform.meshes.Clear();
	platform.meshes.Begin(name);

	for (int32 i = 1; i < sPolygons.NumberOfElements(); ++i)
	{
		cr_sex sPoly = sPolygons[i];
		// (<index> <matId> v1 v2 v3 [v4]))
		int nElements = sPoly.NumberOfElements();

		if (nElements < 5) 
			Throw(sPoly, "Expecting polygon def (<index> matId vindex1 vindex2 vindex3 [vindex4] [vindex5])");

		int32 localMaterialIndex = ParseMat(sPoly[1]);

		if (localMaterialIndex < 0 || localMaterialIndex >= localMaterials.size())
		{
			Throw(sPoly[1], "Bad material index, out of range of the local indices");
		}

		auto& mat = localMaterials[localMaterialIndex];
		MaterialId matId = mat.sysMatId[0];

		ObjectVertex v[5];
		v[0].material = MaterialVertexData{ mat.diffuse, matId, 0 };
		v[1].material = v[0].material;
		v[2].material = v[0].material;
		v[3].material = v[0].material;
		v[4].material = v[0].material;

		for (int32 j = 0; j < 3; j++)
		{
			ParsePolyVertex(v[j], sPoly[j + 2], vertexArray);
		}

		int32 nVertices = nElements - 2;

		if (nVertices == 3) // Triangle
		{		
			const auto& tri = (VertexTriangle&)v[0];
			platform.meshes.AddTriangleEx(tri);
		}
		else if (nVertices == 4) // Quad
		{	
			ParsePolyVertex(v[3], sPoly[5], vertexArray);

			platform.meshes.AddTriangle(v[2], v[1], v[0]);
			platform.meshes.AddTriangle(v[0], v[3], v[2]);
		}
		else if (nVertices == 5) // Pentagon
		{	
			ParsePolyVertex(v[3], sPoly[5], vertexArray);
			ParsePolyVertex(v[4], sPoly[6], vertexArray);

			platform.meshes.AddTriangle(v[2], v[1], v[0]);
			platform.meshes.AddTriangle(v[0], v[3], v[2]);
			platform.meshes.AddTriangle(v[4], v[3], v[0]);
		}
		else
		{
			Throw(sPoly, "Expecting 3, 4 or 5 vertices. Found %d", nVertices);
		}
	}

	platform.meshes.End(false, false);

	cr_sex sShaders = s[5];
	if (sShaders.NumberOfElements() < 2) Throw(sShaders, "Exepecting 2 arguments in (shaders ...) definition");

	Check(sShaders[0], "shaders");

	for (int i = 1; i < sShaders.NumberOfElements(); ++i)
	{
		cr_sex sShaderSpec = sShaders[i];
		if (sShaderSpec.NumberOfElements() != 2)
		{
			Throw(sShaderSpec, "Expecting 2 elements");
		}

		fstring type = GetAtomicArg(sShaderSpec[0]);
		if (Eq(type, "pixels"))
		{
			const fstring key = GetStringLiteral(sShaderSpec[1]);
			if (Eq(key, "no-texture"))
			{
				auto psSpot = "!object.no-texture.ps"_fstring;
				auto psAmbient = "!ambient.no-texture.ps"_fstring;
				platform.meshes.SetSpecialShader(name, psSpot, psAmbient, false);
			}
			else
			{
				Throw(sShaderSpec[1], "Unknown shader key, must be 'no-texture'");
			}
		}
		else
		{
			Throw(sShaderSpec[0], "Unknown shader type, must be 'pixels'");
		}
	}
}

namespace Rococo
{
	namespace M
	{
		void LoadMeshesFromSExpression(Platform& platform, cr_sex s)
		{
			// (ISExpression meshes = '((mesh ...)(mesh ...)...))
			//           s =          ^^^^^^^^^^^^^^^^^^^^^^^^^^^
			// Ergo s[0] is first mesh definition
			int32 nMeshes = s.NumberOfElements();
			for (int32 i = 0; i < nMeshes; ++i)
			{
				cr_sex smesh = s[i];
				LoadMesh(platform, smesh);
			}
		}
	}
}