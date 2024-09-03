[[vk::binding(0, 2)]]
Texture2DArray texture;
[[vk::binding(1, 2)]]
SamplerState sampler;

struct vertexOut
{
    float4 position : SV_Position;
    float2 uv;
};

const static vertexOut triangle[9] = {
    {float4(-1.0f, -1.0f, 0.0f, 1.0f), float2(0.0, 1.0)},
    {float4(-1.0f, 1.0f, 0.0f, 1.0f), float2(0.0, 0.0)},
    {float4(1.0f, 1.0f, 0.0f, 1.0f), float2(1.0, 0.0)},
    {float4(-1.0f, -1.0f, 0.0f, 1.0f), float2(0.0, 1.0)},
    {float4(1.0f, -1.0f, 0.0f, 1.0f), float2(1.0, 1.0)},
    {float4(1.0f, 1.0f, 0.0f, 1.0f), float2(1.0, 0.0)},
};


[shader("vertex")]
vertexOut vertexMain(uint vertexID : SV_VertexID)
{
    return triangle[vertexID];
}

[shader("fragment")]
float4 fragmentMain(vertexOut in) : SV_Target
{
    return texture.Sample(sampler, float3(in.uv, in.uv.x * 13)).bgra;
}
