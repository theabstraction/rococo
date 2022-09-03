#include "hv.h"
#include <rococo.strings.h>

using namespace Rococo::Strings;

namespace HV
{
	void SaveLevelAsFunction(ISectors& sectors, StringBuilder& sb)
	{
		sb.AppendFormat("(using HV)\n");
		sb.AppendFormat("(using Rococo.Graphics)\n\n");

		sb.AppendFormat("(function AddSectorsToLevel -> :\n");
		sb.AppendFormat("\t(ISectors sectors (SectorBuilder))\n\n");
		sb.AppendFormat("\t(sectors.Clear)\n");
		sb.AppendFormat("\t(sectors.DisableMeshGeneration)\n\n");

		uint32 index = 0;
		for (auto s : sectors)
		{
			sb.AppendFormat("\t(AddSector%u sectors)%s\n", index++, index == 0 ? " // entrance" : "");
		}

		sb.AppendFormat("\n\t(sectors.EnableMeshGeneration)\n");
		sb.AppendFormat("\t(sectors.GenerateMeshes)\n");

		sb.AppendFormat(")\n\n");

		index = 0;
		for (auto s : sectors)
		{
			sb.AppendFormat("(function AddSector%u (ISectors sectors) -> :\n", index++);

			int z0 = (int)(s->Z0() * 100);
			int z1 = (int)(s->Z1() * 100);

			size_t nVertices;
			auto* v = s->WallVertices(nVertices);

			for (size_t i = 0; i < nVertices; i++)
			{
				sb.AppendFormat("\t(sectors.AddVertex %f %f)\n", v[i].x, v[i].y);
			}

			s->SaveTemplate(sb);

			sb.AppendFormat(")\n\n");
		}
	}

}