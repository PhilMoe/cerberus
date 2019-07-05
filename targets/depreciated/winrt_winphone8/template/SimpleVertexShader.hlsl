
cbuffer VertexShaderParams : register( b0 ){

	matrix projection;
};

struct VertexShaderInput{

	float2 pos : POSITION;
	float2 tex : TEXCOORD0;
	float4 color : COLOR0;
};

struct VertexShaderOutput{

	float4 pos : SV_POSITION;
	float4 color : COLOR0;
};

VertexShaderOutput main( VertexShaderInput input ){

	VertexShaderOutput output;

	float4 pos = float4( input.pos,0.0f,1.0f );

	pos=mul( pos,projection );

	output.pos=pos;

	output.color=input.color;

	return output;
}
