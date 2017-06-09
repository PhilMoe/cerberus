
//shader for simple matte lighting.

uniform sampler2D ColorTexture;
uniform sampler2D SpecularTexture;
uniform sampler2D NormalTexture;

uniform vec4 AmbientColor;
uniform float Roughness;

void shader(){

	vec4 color=texture2D( ColorTexture,b3d_Texcoord0 );
	
#if GAMMA_CORRECTION
	color.rgb=pow( color.rgb,vec3( 2.2 ) );
#endif

	color*=b3d_Color;

	b3d_Alpha=color.a;

	b3d_Ambient=color * AmbientColor;

	b3d_Diffuse=color * (1.0-AmbientColor);
	
	b3d_Specular=vec4( 0.0 );
	
	b3d_Normal=vec3( 0.0,0.0,-1.0 );

	b3d_Roughness=Roughness;
}
