#version 150

in vec4 texelSpec; 
in float bitmapIndex;
in vec4 texelColour;

uniform sampler2D fontTexture;
uniform sampler2DArray widgetTextureArray;

const float fontScale = 1 / 1024.0f;

out vec4 fragmentColour;
 
void main()
{
	vec2 fontTextSpec = vec2(texelSpec.x, texelSpec.y);
	vec4 fontTexel = texture(fontTexture, fontTextSpec.xy * fontScale);
	float fontAlpha = fontTexel.w;
	float fontBlend = texelSpec.z;
	float bitmapBlend = texelSpec.w;
	
	vec2 uv = vec2(texelSpec.x, texelSpec.y);
	vec4 bitmapTexel = texture(widgetTextureArray, vec3(uv,bitmapIndex));
	
	float fragmentAlpha = fontAlpha * fontBlend + (1.0f - fontBlend);
	bitmapTexel.w *= texelColour.w;
	
	vec4 fontColour = vec4(texelColour.xyz, fragmentAlpha * texelColour.w);
    fragmentColour = mix(fontColour, bitmapTexel, bitmapBlend);
}
