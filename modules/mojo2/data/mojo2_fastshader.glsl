
//shader for simple unlit sprites.

uniform sampler2D ColorTexture;

void shader(){

	b3d_Ambient=texture2D( ColorTexture,b3d_Texcoord0 );
	
#if GAMMA_CORRECTION
	b3d_Ambient.rgb=pow( b3d_Ambient.rgb,vec3( 2.2 ) );
#endif

	b3d_Ambient*=b3d_Color;	//apply vertex coloring

	b3d_Alpha=b3d_Ambient.a;	//extract alpha
}
