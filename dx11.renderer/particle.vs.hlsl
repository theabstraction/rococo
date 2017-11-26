struct ParticleVertex
{
	float3 position : POSITION;
};

struct BillboardVertex
{
	float3 position : POSITION;
};

BillboardVertex main(ParticleVertex input)
{
	BillboardVertex output;
	output.position = input.position;
	return output;
}
