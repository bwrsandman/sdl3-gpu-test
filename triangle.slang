const static float4 triangle[9] = {
    float4(0.5f, 0.0f, 0.0f, 1.0f),
    float4(0.0f, 1.0f, 0.0f, 1.0f),
    float4(-0.5f, 0.0f, 0.0f, 1.0f),

    float4(1.0f, -1.0f, 0.0f, 1.0f),
    float4(0.5f, 0.0f, 0.0f, 1.0f),
    float4(0.0f, -1.0f, 0.0f, 1.0f),

    float4(-1.0f, -1.0f, 0.0f, 1.0f),
    float4(-0.5f, 0.0f, 0.0f, 1.0f),
    float4(0.0f, -1.0f, 0.0f, 1.0f),
};

[shader("vertex")]
float4 vertexMain(uint vertexID : SV_VertexID) : SV_Position
{
    return triangle[vertexID];
}

[shader("fragment")]
float4 fragmentMain() : SV_Target
{
    return float4(1.0, 1.0, 0.0, 1.0);
}
