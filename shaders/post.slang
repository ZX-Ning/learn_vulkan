struct VertexOutput {
    float4 pos : SV_Position;
    float2 uv;
};

[[vk_push_constant]]
ConstantBuffer<float> gamma;

[[vk_binding(0, 0)]]
Sampler2D sampledTexture;

[shader("vertex")]
VertexOutput vertMain(uint id: SV_VertexID) {
    var coord = float2((id << 1) & 2, id & 2);
    return VertexOutput(
        float4(coord * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0),
        coord
    );
}

[shader("fragment")]
float4 fragMain(VertexOutput fIn) : SV_Target {
    return pow(sampledTexture.Sample(fIn.uv), gamma);
}
