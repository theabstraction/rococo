#include "dx11.renderer.h"
#include "dx11helpers.inl"
#include "dx11buffers.inl"

namespace Rococo::DX11
{
	int DrawLightCone(const Light& light, cr_vec3 viewDir, ID3D11DeviceContext& dc, ID3D11Buffer& lightConeBuffer)
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
			return 0;
		}

		p = Normalize(p);

		if (fabsf(light.cutoffCosAngle) < 0.1)
		{
			// Angle too obtuse, do not render
			return 0;
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

		DX11::CopyStructureToBuffer(dc, &lightConeBuffer, v, sizeof(ObjectVertex) * 3);

		UINT vertexCount = 3;
		dc.Draw(vertexCount, 0);

		return vertexCount / 3;
	}
}