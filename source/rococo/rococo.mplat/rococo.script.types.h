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
		Vec2i cellSpan;
		Vec2i borders;
		Vec2i topLeft;
		boolean32 rowByRow;
		int32 startIndex;
		int32 endIndex;
	};
}
