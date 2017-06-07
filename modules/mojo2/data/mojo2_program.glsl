
//***** Mojo2 Mega Shader *****

//***** USER OPTIONS ******

//enable less accurate but faster rendering - make sure yer normals are right when enabling this!
#define OPT_FAST 0

//Apply cheesy_hdr to lighting - slower, but nice for colored lights as they saturate to white
#define CHEESY_HDR 1

//enable bump tangents, for correct bump mapping of rotating drawops - can only really handle uniform scaling/rotation, but will probably look OK otherwise...
#define BUMP_TANGENTS 1

//make sure shadows are working!
#define DEBUG_SHADOWS 0

//enable ortho lighting - faster + makes lights easier to position
#define ORTHO_LIGHTING 1

//enable fresnel effect - slower
#define FRESNEL_EFFECT 1

//enable gamma correction - slower. Don't use yet! Messes up with >4 lights...
#define GAMMA_CORRECTION 0


//***** ENABLED VARS *****

#if NUM_LIGHTS && !defined(B3D_CLIPPOSITION)
#define B3D_CLIPPOSITION 1
#endif

#if NUM_LIGHTS && !defined(B3D_VIEWPOSITION)
#define B3D_VIEWPOSITION 1
#endif

#if NUM_LIGHTS && BUMP_TANGENTS && !defined(B3D_VIEWTANGENT)
#define B3D_VIEWTANGENT 1
#endif

#ifdef B3D_TEXCOORD0
varying vec2 b3d_Texcoord0;
#endif

#ifdef B3D_COLOR
varying vec4 b3d_Color;
#endif

#ifdef B3D_VIEWPOSITION
varying vec3 b3d_ViewPosition;
#endif

#ifdef B3D_VIEWNORMAL
varying vec3 b3d_ViewNormal;
#endif

#ifdef B3D_VIEWTANGENT
varying vec3 b3d_ViewTangent;
#endif

#ifdef B3D_CLIPPOSITION
varying vec4 b3d_ClipPosition;
#endif

//@vertex

uniform mat4 ModelViewProjectionMatrix;
uniform mat4 ModelViewMatrix;
uniform vec4 ClipPosScale;	//hack to handle inverted y when rendering to display
uniform vec4 GlobalColor;

attribute vec4 Position;
attribute vec2 Texcoord0;
attribute vec3 Tangent;
attribute vec4 Color;

void main(){

	gl_Position=ModelViewProjectionMatrix * Position;
	
	gl_PointSize=1.0;
	
#ifdef B3D_CLIPPOSITION
	b3d_ClipPosition=gl_Position * ClipPosScale;
#endif

#ifdef B3D_VIEWPOSITION
	b3d_ViewPosition=(ModelViewMatrix * Position).xyz;
#endif
	
#ifdef B3D_VIEWNORMAL
	b3d_ViewNormal=(ModelViewMatrix * vec4( 0.0,0.0,-1.0 )).xyz;
#endif

#ifdef B3D_VIEWTANGENT
#if OPT_FAST
	b3d_ViewTangent=normalize( (ModelViewMatrix * vec4( Tangent,0.0 )).xyz );
#else
	b3d_ViewTangent=(ModelViewMatrix * vec4( Tangent,0.0 )).xyz;
#endif
#endif

#ifdef B3D_TEXCOORD0
	b3d_Texcoord0=Texcoord0;
#endif

#ifdef B3D_COLOR
	b3d_Color=Color * GlobalColor;
#endif
		
}

//@fragment

uniform vec4 FogColor;
uniform vec4 AmbientLight;

#if NUM_LIGHTS
uniform sampler2D ShadowTexture;
uniform vec4 LightColors[NUM_LIGHTS];
uniform vec4 LightVectors[NUM_LIGHTS];
#endif

#define b3d_FragColor gl_FragColor
float b3d_Alpha;
float b3d_Roughness;
vec4 b3d_Ambient;
vec4 b3d_Diffuse;
vec4 b3d_Specular;
vec3 b3d_Normal;

${SHADER}

#if NUM_LIGHTS

float gloss;
float spow;
float fnorm;
float ndotv;
#if ORTHO_LIGHTING
const vec3 eyevec=vec3( 0.0,0.0,-1.0 );
#else
vec3 eyevec;
#endif
vec4 diffuse;
vec4 specular;
mat3 tanMatrix;

void light( in vec4 lightVector,in vec4 lightColor,float shadow ){

#if DEBUG_SHADOWS
	diffuse+=1.0-shadow;
	return;
#endif

	vec3 v=lightVector.xyz-b3d_ViewPosition;
#if BUMP_TANGENTS
	v=tanMatrix * v;
#endif
	
	float falloff=max( 1.0-length( v )/lightVector.w,0.0 );
	
	vec3 lvec=normalize( v );
	vec3 hvec=normalize( lvec + eyevec );
	
	float ndotl=max( dot( b3d_Normal,lvec ),0.0 );
	float ndoth=max( dot( b3d_Normal,hvec ),0.0 );
	
	vec4 icolor=lightColor * ndotl * falloff * shadow;
	
	diffuse+=icolor;
	specular+=icolor * pow( ndoth,spow ) * fnorm;
}

#if CHEESY_HDR
vec4 cheesy_hdr( in vec4 color ){
	vec4 ov=max( color-1.0,0.0 );
	return color+( (ov.r+ov.g+ov.b)/3.0 );
}
#endif

#endif

void main(){

	shader();
	
#ifndef B3D_FRAGCOLOR
	
#if NUM_LIGHTS

#if !ORTHO_LIGHTING
	eyevec=normalize( -b3d_ViewPosition );
#endif

	//specular power	
	gloss=1.0-Roughness;
	spow=pow( 2.0,gloss*12.0 );	//specular power
	fnorm=spow*2.0/8.0;			//energy conservation - sharper highlights are brighter coz they're smaller.
	
#if FRESNEL_EFFECT				//fresnel effect - reflectivity approaches 1 as surface grazing angle approaches 0 for glossy surfaces.
	ndotv=max( dot( b3d_Normal,eyevec ),0.0 );
	b3d_Specular+=(1.0-b3d_Specular) * pow( 1.0-ndotv,5.0 ) * gloss;
#endif

	diffuse=AmbientLight;
	specular=vec4( 0.0 );

#if BUMP_TANGENTS
#if OPT_FAST
	vec3 vtan=b3d_ViewTangent;
#else
	vec3 vtan=normalize( b3d_ViewTangent );
#endif
	tanMatrix=mat3( vec3( vtan.x,-vtan.y,0.0 ),vec3( vtan.y,vtan.x,0.0 ),vec3( 0.0,0.0,1.0 ) );
#endif
	
	vec4 clip=b3d_ClipPosition/b3d_ClipPosition.w;
	vec4 shadow=texture2D( ShadowTexture,clip.xy*0.5+0.5 );
	
	light( LightVectors[0],LightColors[0],shadow.r );
#if NUM_LIGHTS>1
	light( LightVectors[1],LightColors[1],shadow.g );
#if NUM_LIGHTS>2
	light( LightVectors[2],LightColors[2],shadow.b );
#if NUM_LIGHTS>3
	light( LightVectors[3],LightColors[3],shadow.a );
#endif
#endif
#endif

#if CHEESY_HDR
	diffuse=cheesy_hdr( diffuse );
	specular=cheesy_hdr( specular );
#endif

	vec4 color=(b3d_Diffuse * diffuse) + (b3d_Specular * specular) + (b3d_Ambient * AmbientLight.a);

#else

#ifdef B3D_DIFFUSE
	vec4 color=(b3d_Diffuse * AmbientLight) + (b3d_Ambient * AmbientLight.a);
#else
	vec4 color=b3d_Ambient * AmbientLight.a;
#endif	
	
#endif

	color.rgb=mix( color.rgb,FogColor.rgb,FogColor.a * b3d_Alpha );

#if GAMMA_CORRECTION
	gl_FragColor=vec4( pow( color.rgb,vec3( 1.0/2.2 ) ),b3d_Alpha );
#else
	gl_FragColor=vec4( color.rgb,b3d_Alpha );
#endif

#endif	//B3D_FRAGCOLOR

}
