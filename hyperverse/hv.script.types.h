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
namespace HV { 
	struct InsertItemSpec
	{
		int32 insertFlags;
		Metres minDistance;
		Metres maxDistance;
		boolean32 pointTowardsCentrePiece;
	};
}
namespace HV { 
	struct ObjectCreationSpec
	{
		boolean32 unused;
	};
}
namespace HV { 
	struct MeleeData
	{
		float swingSpeed;
		float baseDamage;
	};
}
namespace HV { 
	struct ArmourData
	{
		float catchProjectilePercentile;
		Metres thickness;
	};
}
namespace HV { 
	struct ObjectDynamics
	{
		Kilograms mass;
		float airFrictionLinearQuotient;
		float airFrictionQuadraticQuotient;
		Vec3 span;
	};
}
namespace HV { 
	struct InventoryData
	{
		int64 legalEquipmentSlotFlags;
		int32 maxStackSize;
		ID_SPRITE icon;
	};
}
namespace HV { 
	struct MaterialData
	{
		int32 mohsHardness;
		int32 toughness;
		int32 atomicNumber;
	};
}
