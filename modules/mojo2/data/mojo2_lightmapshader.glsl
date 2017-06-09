
//shader for drawing lightmaps, ie: light images or layer light masks.

uniform sampler2D ColorTexture;

void shader(){

	b3d_FragColor=vec4( texture2D( ColorTexture,b3d_Texcoord0 ).r );
}
