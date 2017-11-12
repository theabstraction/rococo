	struct MaterialVertexData
	{
		RGBAb materialColour;
		MaterialId materialIndex;
		float gloss;
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
	struct QuadColours
	{
		RGBAb a;
		RGBAb b;
		RGBAb c;
		RGBAb d;
	};
	struct QuadVertices
	{
		Quad positions;
		GuiRectf uv;
		Quad normals;
		QuadColours colours;
	};
	struct ObjectTriangle
	{
		ObjectVertex a;
		ObjectVertex b;
		ObjectVertex c;
	};
