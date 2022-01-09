#version 150
 
uniform vec4 viewport; /* width, height, 1/width, 1/height */
 
in vec4 texture;
in float bitmapSelect;
in vec2 position;
in vec4 vertexColour;
 
out vec4 texelColour;
out vec4 texelSpec;
out float bitmapIndex;

void main()
{
    texelColour = vertexColour;
	
	texelSpec.x = texture.x;
	texelSpec.y = texture.y;
	texelSpec.zw = texture.zw;
	
	vec2 reposition = position.xy; // + vec2(0.5f,0.5f);
	
	/* x = A.X + B, when X = width, x = 1 and when X = 0, x = -1, thus B = -1 and A = 2 / width */
    gl_Position.x = 2.0f * reposition.x * viewport.z - 1.0f;
	
	/* y = C.Y + D, when Y = height, y = -1 and when Y = 0, y = 1, thus D = 1 and C = -2/ width */
	gl_Position.y = -2.0f * reposition.y * viewport.w + 1.0f;
	
	bitmapIndex = bitmapSelect;
	
	gl_Position.z = 0.0f;
	gl_Position.w = 1.0f;
}