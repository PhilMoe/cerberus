
Texture2D shaderTexture;
SamplerState SampleType;

struct PixelShaderInput{

	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
	float4 color : COLOR0;
};

float4 main( PixelShaderInput input ) : SV_TARGET{

	return shaderTexture.Sample( SampleType,input.tex ) * input.color;
}
