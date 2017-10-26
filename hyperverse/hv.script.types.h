namespace HV { 
	struct WallSegment
	{
		Quad quad;
		Vec3 tangent;
		Vec3 vertical;
		Vec3 normal;
		Vec2 span;
		boolean32 leftEdgeIsGap;
		boolean32 rightEdgeIsGap;
	};
}
namespace HV { 
	struct GapSegment
	{
		Quad quad;
		Vec3 tangent;
		Vec3 vertical;
		Vec3 normal;
		boolean32 leadsToCorridor;
		float otherZ0;
		float otherZ1;
	};
}
