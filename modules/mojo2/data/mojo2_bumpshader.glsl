
//shader for full lighting.

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
	
	b3d_Specular=texture2D( SpecularTexture,b3d_Texcoord0 );

#if GAMMA_CORRECTION
	b3d_Specular.rgb=pow( b3d_Specular.rgb * b3d_Alpha,vec3( 2.2 ) );
#else
	b3d_Specular.rgb*=b3d_Alpha;
#endif
	
#if OPT_FAST
	b3d_Normal=texture2D( NormalTexture,b3d_Texcoord0 ).xyz*2.0-1.0;
#else	
	b3d_Normal=normalize( texture2D( NormalTexture,b3d_Texcoord0 ).xyz*2.0-1.0 );
#endif
	b3d_Normal.yz=-b3d_Normal.yz;

	b3d_Roughness=Roughness;
}
