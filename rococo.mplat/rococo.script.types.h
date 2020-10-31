namespace Rococo { 
	struct TriangleScan
	{
		ID_ENTITY id;
		ID_SYS_MESH idMesh;
		Triangle t;
	};
}
namespace Rococo { 
	struct InventoryLayoutRules
	{
		int32 rows;
		int32 columns;
		Vec2 cellSpan;
		Vec2 borders;
		Vec2 topLeft;
		boolean32 rowByRow;
		int32 startIndex;
		int32 endIndex;
	};
}
