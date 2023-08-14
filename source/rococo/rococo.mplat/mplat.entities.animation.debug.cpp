#include "rococo.mplat.h"
#include "rococo.animation.h"
#include "mplat.components.h"

using namespace Rococo;
using namespace Rococo::Entities;
using namespace Rococo::Graphics;

namespace
{
	void AddDirectionArrow(cr_m4x4 model, IRenderContext& rc, RGBAb colour, IRodTesselatorSupervisor& rod)
	{
		MaterialVertexData mat;
		mat.colour = colour;
		mat.gloss = 0;

		rod.SetMaterialTop(mat);
		rod.SetMaterialMiddle(mat);
		rod.SetMaterialBottom(mat);

		rod.Clear();
		rod.AddTube(0.3_metres, 0.01_metres, 0.01_metres, 3);
		rod.AddPyramid(0.05_metres, { -0.01f, 0.01f }, { 0.01f, 0.01f }, { 0.01f, -0.01f }, { -0.01f, -0.01f });
		rod.TransformVertices(model);
		rc.Gui3D().Add3DGuiTriangles(rod.begin(), rod.end());
		rod.Clear();
	}

	void AddDirectionArrow_Up(cr_m4x4 model, IRenderContext& rc, IRodTesselatorSupervisor& rod)
	{
		AddDirectionArrow(model, rc, RGBAb(255, 0, 0, 0), rod);
	}

	void AddDirectionArrow_Right(cr_m4x4 model, IRenderContext& rc, IRodTesselatorSupervisor& rod)
	{
		Matrix4x4 M = model * Matrix4x4::RotateRHAnticlockwiseY(-90.0_degrees);
		AddDirectionArrow(M, rc, RGBAb(0, 255, 0, 0), rod);
	}

	void AddDirectionArrow_Forward(cr_m4x4 model, IRenderContext& rc, IRodTesselatorSupervisor& rod)
	{
		Matrix4x4 M = model * Matrix4x4::RotateRHAnticlockwiseX(-90.0_degrees);
		AddDirectionArrow(M, rc, RGBAb(0, 0, 255, 0), rod);
	}

	void AddOrientationArrows(cr_m4x4 model, IRenderContext& rc, IRodTesselatorSupervisor& rod)
	{
		AddDirectionArrow_Up(model, rc, rod);
		AddDirectionArrow_Right(model, rc, rod);
		AddDirectionArrow_Forward(model, rc, rod);
	}

	void DrawBone(const IBone& bone, cr_m4x4 model, IRenderContext& rc, IRodTesselatorSupervisor& rod)
	{
		if (bone.Length() > 0)
		{
			MaterialVertexData mat;
			mat.colour = RGBAb(255, 255, 255, 255);
			mat.gloss = 0;

			rod.Clear();

			rod.SetMaterialTop(mat);
			rod.SetMaterialMiddle(mat);
			rod.SetMaterialBottom(mat);

			rod.AddTube(bone.Length(), 0.05_metres, 0.05_metres, 8);
			rod.TransformVertices(model);
			rc.Gui3D().Add3DGuiTriangles(rod.begin(), rod.end());
			rod.Clear();
		}
	}

	void AddDebugBone(const IBone& bone, const Matrix4x4& model, IRenderContext& rc, IRodTesselatorSupervisor& rod)
	{
		DrawBone(bone, model, rc, rod);
		AddOrientationArrows(model, rc, rod);

		for (auto& child : bone)
		{
			Matrix4x4 childModelMatrix = model * child->GetMatrix();
			AddDebugBone(*child, childModelMatrix, rc, rod);
		}
	}
}

namespace Rococo::Entities
{
	void AddDebugBones(ID_ENTITY id, IRenderContext& rc, IRodTesselatorSupervisor& rod)
	{
		auto skeletonComponent = Components::API::ForISkeletonComponent::Get(id);
		auto skeleton = skeletonComponent ? skeletonComponent->Skeleton() : nullptr;
		if (skeleton)
		{
			auto* root = skeleton->Root();
			if (root)
			{
				auto body = API::ForIBodyComponent::Get(id);
				if (body)
				{
					const Matrix4x4& m = body->Model();
					AddOrientationArrows(m, rc, rod);
					AddDebugBone(*root, m, rc, rod);
				}
			}
		}
	}
}