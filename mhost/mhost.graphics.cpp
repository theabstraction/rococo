#include "mhost.h"

namespace MHost
{
	using namespace Rococo;

	Vec2 GetTopLeftPos(Vec2 pos, Vec2 span, int32 alignmentFlags)
	{
		Vec2 topLeftPos;

		if (HasFlag(alignmentFlags, AlignmentFlags_Left) && !HasFlag(alignmentFlags, AlignmentFlags_Right))
		{
			topLeftPos.x = pos.x;
		}
		else if (!HasFlag(alignmentFlags, AlignmentFlags_Left) && HasFlag(alignmentFlags, AlignmentFlags_Right))
		{
			topLeftPos.x = pos.x - span.x;
		}
		else // Centred horizontally
		{
			topLeftPos.x = pos.x - 0.5f * span.x;
		}

		if (HasFlag(alignmentFlags, AlignmentFlags_Top) && !HasFlag(alignmentFlags, AlignmentFlags_Bottom))
		{
			topLeftPos.y = pos.y;
		}
		else if (!HasFlag(alignmentFlags, AlignmentFlags_Top) && HasFlag(alignmentFlags, AlignmentFlags_Bottom))
		{
			topLeftPos.y = pos.y - span.y;
		}
		else // Centred vertically
		{
			topLeftPos.y = pos.y - 0.5f * span.y;
		}

		return topLeftPos;
	}
}