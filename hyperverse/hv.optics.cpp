#include "hv.h"
#include <rococo.maths.h>

using namespace Rococo;

namespace HV
{
	Vec3 GetNormal(const Gap& gap)
	{
		Vec3 top = { gap.b.x - gap.a.x, gap.a.y - gap.a.y, 0 };
		return Vec3{ top.y, -top.x, 0 };
	}

	// Determine if we can look through a gap to see the sector beyond
	// If we are in the home sector, then don't worry too much if the gap is slightly behind the near plane
	// As we may be on the edge of the home sector, and our near plane just over the gap into the next sector
	bool CanSeeThrough(const Matrix4x4& camera, cr_vec3 forward, const Gap& gap, Vec2& leftPoint, Vec2& rightPoint, bool fromHome)
	{
		Vec4 qGap[4] =
		{
			{ gap.a.x, gap.a.y, gap.z1, 1 },
			{ gap.b.x, gap.b.y, gap.z1, 1 },
			{ gap.b.x, gap.b.y, gap.z0, 1 },
			{ gap.a.x, gap.a.y, gap.z0, 1 }
		};

		Vec4 qScreen[4];

		for (int i = 0; i < 4; ++i)
		{
			qScreen[i] = camera * qGap[i];
		}

		Vec3 qScreenV[4];
		for (int i = 0; i < 4; ++i)
		{
			qScreenV[i] = ((const Vec3&)qScreen[i]) * (1.0f / qScreen[i].w);
		}

		Vec3 ab1 = qScreenV[1] - qScreenV[0];
		Vec3 bc1 = qScreenV[2] - qScreenV[1];
		float tri1Normal = Cross(Flatten(ab1), Flatten(bc1));

		Vec3 ab2 = qScreenV[3] - qScreenV[2];
		Vec3 bc2 = qScreenV[0] - qScreenV[3];
		float tri2Normal = Cross(Flatten(ab2), Flatten(bc2));

		const Vec3* v = qScreenV;

		int j = 0;

		for (int i = 0; i < 4; i++)
		{
			if (qScreen[i].w < 0)
			{
				j--;
			}
			else if (qScreen[i].w > 0)
			{
				j++;
			}
		}

		if (j == 4 || j == -4)
		{
			if (tri1Normal > 0 && tri2Normal > 0)
			{
				// Edge case -> the eye may be in one sector, and the near plane just over the gap
				// This flips triangles, but we are still looking into the sector.
				// If this is so our view direction opposes the normal  of the gap

				if (fromHome)
				{
					if (Dot(forward, GetNormal(gap)))
					{
						return true;
					}
				}

				return false;
			}

			if (v[0].x <= -1 && v[1].x <= -1 && v[2].x <= -1 && v[3].x <= -1)
			{
				// Everything to left of screen, so gap is not visible
				return false;
			}

			if (v[0].x >= 1 && v[1].x >= 1 && v[2].x >= 1 && v[3].x >= 1)
			{
				// Everything to right of screen, so gap is not visible
				return false;
			}

			if (v[0].y >= 1 && v[1].y >= 1 && v[2].y >= 1 && v[3].y >= 1)
			{
				// Everything to top of screen, so gap is not visible
				return false;
			}

			if (v[0].y <= -1 && v[1].y <= -1 && v[2].y <= -1 && v[3].y <= -1)
			{
				// Everything to bottom of screen, so gap is not visible
				return false;
			}
		}

		return true;
	}

}