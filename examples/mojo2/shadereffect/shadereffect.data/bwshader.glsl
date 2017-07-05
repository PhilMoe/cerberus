
//Simple black/white shader effect

uniform sampler2D ColorTexture;

uniform float EffectLevel;

void shader(){

	//convert clip position to valid tex coords
	vec2 texcoords=(b3d_ClipPosition.st/b3d_ClipPosition.w)*0.5+0.5;
	
	//read source color
	vec3 color=texture2D( ColorTexture,texcoords ).rgb;
	
	//calculate b/w color
	vec3 result=vec3( (color.r+color.g+color.b)/3.0 );
	
	//mix based on effect level
	color=mix( color,result,EffectLevel );
	
	//write output
	b3d_FragColor=vec4( color,1.0 );
}
