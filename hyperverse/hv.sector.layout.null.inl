namespace HV
{
	struct NullSectorLayout : public ISectorLayout
	{
		int32 CountSquares() override { return 0; }
		boolean32 Exists() override { return false; }
		void GetSquare(int32 sqIndex, AABB2d& sq) override {}
		void CeilingQuad(int32 sqIndex, QuadVertices& q) override {}
		void FloorQuad(int32 sqIndex, QuadVertices& q) override {}
		void Altitude(Vec2& altitudes) override {}
		int32 NumberOfSegments()  override { return 0; }
		int32 NumberOfGaps() override { return 0; }
		void GetSegment(int32 segIndex, HV::WallSegment& segment) override {}
		void GetGap(int32 gapIndex, HV::GapSegment& segment) override {}
		ID_ENTITY AddSceneryAroundObject(const fstring& mesh, ID_ENTITY centrePieceId, const HV::InsertItemSpec& iis, const HV::ObjectCreationSpec& ocs) override { return ID_ENTITY(); }
		ID_ENTITY AddItemToLargestSquare(const fstring& mesh, int32 addItemFlags, const HV::ObjectCreationSpec& ocs) override { return ID_ENTITY(); }
		boolean32 PlaceItemOnUpFacingQuad(ID_ENTITY id) override { return false; }
		void DeleteScenery()  override {}
		void DeleteItemsWithMesh(const fstring& prefix) override {}
		void ClearManagedEntities()  override {}
		void ManageEntity(ID_ENTITY id) override {}
		void UseUpFacingQuads(ID_ENTITY id)  override {}
		boolean32 TryGetAsRectangle(GuiRectf& rect) override { rect = { 0,0,0,0 }; return false; }
	};
}
