#include <rococo.types.h>

// #define RAL_PIPELINE_API ROCOCO_API_EXPORT

#include <rococo.renderer.types.h>
#include <rococo.renderer.h>
#include <RAL\RAL.h>
#include <RAL\RAL.pipeline.h>

using namespace Rococo;
using namespace Rococo::Graphics;
using namespace Rococo::RAL;

struct RAL_LightCones : IRAL_LightCones
{
	IRAL& ral;
	IRenderStates& renderStates;
	IPipeline& pipeline;
	AutoFree<IRALConstantDataBuffer> instanceBuffer;
	AutoFree<IRALVertexDataBuffer> lightConeBuffer;
	ID_PIXEL_SHADER idLightConePS;
	ID_VERTEX_SHADER idLightConeVS;

	RAL_LightCones(IRAL& _ral, IRenderStates& _renderStates, IPipeline& _pipeline) : ral(_ral), renderStates(_renderStates), pipeline(_pipeline)
	{
		instanceBuffer = ral.CreateConstantBuffer(sizeof ObjectInstance, 1);
		lightConeBuffer = ral.CreateDynamicVertexBuffer(sizeof ObjectVertex, 3);
		idLightConePS = ral.Shaders().CreatePixelShader("!shaders/compiled/light_cone.ps");
		idLightConeVS = ral.Shaders().CreateVertexShader("!shaders/compiled/light_cone.vs", RAL::GetObjectVertexElements());
	}

	void Free() override
	{
		delete this;
	}

	void DrawLightCone(const LightConstantBuffer& light, cr_vec3 viewDir)
	{
		/* our aim is to render a cross section of the light cone as a single alpha blended triangle
				B
				'
			'
			' (cutoffTheta)
		A   -----------------> Light direction D
			'
			'
				'
					C


			A is the position of the light facing D. We need to compute B and C

			The triangle ABC is parallel to the screen (with screem direction F), which means BC.F = 0

			A unit vector p parallel to BC is thus the normalized cross product of F and D.

			p = |D x F|.

			We can now construct B and C by taking basis vectors d and p from A.

			Given we specify a length k of the light cone, along its central axis,
			then the radius R along the vector p to construct B and C is such that
			R / k = tan ( cutoffTheta )

			The component of d of D parallel to the screen is | F x p |

		*/

		const Vec3& D = light.direction;

		Vec3 p = Cross(D, viewDir);

		if (LengthSq(p) < 0.15)
		{
			// Too acute, do not render
			return;
		}

		p = Normalize(p);

		if (fabsf(light.cutoffCosAngle) < 0.1)
		{
			// Angle too obtuse, do not render
			return;
		}

		float cutOffAngle = acosf(light.cutoffCosAngle);
		float tanCutoff = tanf(cutOffAngle);

		auto coneLength = 1.0_metres;
		auto radius = coneLength * tanCutoff;

		Vec3 d = Normalize(Cross(viewDir, p));

		ObjectVertex v[3] = { 0 };
		v[0].position = light.position;
		v[0].material.colour = RGBAb(255, 255, 255, 128);
		v[0].uv.x = 0;
		v[1].uv.y = 0;

		v[1].position = light.position + coneLength * D + radius * p;
		v[1].material.colour = RGBAb(255, 255, 255, 0);
		v[1].uv.x = 1.0f;
		v[1].uv.y = -1.0f;

		v[2].position = light.position + coneLength * D - radius * p;
		v[2].material.colour = RGBAb(255, 255, 255, 0);
		v[2].uv.x = 1.0f;
		v[2].uv.y = -1.0f;

		lightConeBuffer->CopyDataToBuffer(&v, sizeof v);

		ral.Draw(sizeof v / sizeof ObjectVertex, 0);
	}

	void DrawLightCones(IScene& scene) override
	{
		auto lights = scene.GetLights();
		if (lights.lightArray != nullptr)
		{
			ral.ClearBoundVertexBufferArray();
			ral.BindVertexBuffer(lightConeBuffer, sizeof ObjectVertex, 0);
			ral.CommitBoundVertexBuffers();
			renderStates.SetDrawTopology(PrimitiveTopology::TRIANGLELIST);
			renderStates.UseSpriteRasterizer();
			ral.Shaders().UseShaders(idLightConeVS, idLightConePS);
			renderStates.UseAlphaBlend();
			renderStates.DisableWritesOnDepthState();

			pipeline.AssignGlobalStateBufferToShaders();

			ObjectInstance identity;
			identity.orientation = Matrix4x4::Identity();
			identity.highlightColour = { 0 };

			instanceBuffer->CopyDataToBuffer(&identity, sizeof identity);
			instanceBuffer->AssignToVS(CBUFFER_INDEX_INSTANCE_BUFFER);

			Matrix4x4 camera;
			Matrix4x4 world;
			Matrix4x4 proj;
			Vec4 eye;
			Vec4 viewDir;
			scene.GetCamera(camera, world, proj, eye, viewDir);

			for (uint32 i = 0; i < lights.count; ++i)
			{
				if (lights.lightArray[i].hasCone)
				{
					DrawLightCone(lights.lightArray[i], viewDir);
				}
			}

			/*
			dc.IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
			dc.RSSetState(nullptr);
			dc.PSSetConstantBuffers(0, 0, nullptr);
			dc.VSSetConstantBuffers(0, 0, nullptr);
			*/
		}
	}
};

namespace Rococo::RAL
{
	IRAL_LightCones* CreateLightCones(IRAL& ral, IRenderStates& renderStates, IPipeline& pipeline)
	{
		return new RAL_LightCones(ral, renderStates, pipeline);
	}
}