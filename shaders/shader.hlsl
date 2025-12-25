struct VertexInput {
    float3 pos : POSITION0;
    float3 color : COLOR1;
};

struct VertexOutput {
    float4 sv_position : SV_Position;
    float3 color : COLOR0;
};

[shader("vertex")]
VertexOutput vertMain(VertexInput vIn) {
    VertexOutput vOut = { float4(vIn.pos, 1.0), vIn.color };
    return vOut;
}

[shader("pixel")]
float4 fragMain(VertexOutput fIn) : SV_Target {
    return float4(fIn.color, 1.0);
}


