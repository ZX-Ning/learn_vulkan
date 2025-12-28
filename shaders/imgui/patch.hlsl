struct VSInput {
    float2 pos : POSITION0;
    float2 uv : TEXCOORD1;
    float4 color : COLOR2;
};

struct VSOutput {
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD1;
};

struct uPushConstant {
    float2 uScale;
    float2 uTranslate;
};

[[vk::push_constant]] uPushConstant pc;

[shader("vertex")]
VSOutput vsMain(VSInput input) {
    VSOutput output;
    output.pos = float4(pc.uScale * input.pos + pc.uTranslate, 0.0f, 1.0f);
    output.color = input.color;
    output.uv = input.uv;
    return output;
}

Texture2D sTexture : register(t0, space0);
SamplerState sSampler : register(s0, space0);

float3 sRGBtoLinear(float3 srgb) {
    return pow(srgb, 2.2f);
}

[shader("pixel")]
float4 fsMain(VSOutput input) : SV_Target {
    float4 color = input.color * sTexture.Sample(sSampler, input.uv);
    return float4(sRGBtoLinear(color.rgb), color.w);
}


