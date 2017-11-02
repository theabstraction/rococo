	struct MaterialVertexData
	{
		RGBAb materialColour;
		MaterialId materialIndex;
	};
	struct ObjectVertex
	{
		Vec3 position;
		Vec3 normal;
		Vec2 uv;
		MaterialVertexData mat;
	};
	struct LightSpec
	{
		Vec3 position;
		Vec3 direction;
		Degrees fov;
		RGBA diffuse;
		RGBA ambience;
		Degrees cutoffAngle;
		float cutoffPower;
		float attenuation;
	};
	struct QuadVertices
	{
		Quad positions;
		GuiRectf uv;
		Quad normals;
	};
