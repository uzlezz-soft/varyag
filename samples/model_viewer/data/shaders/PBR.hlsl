#include "Common.hlsli"

struct CameraData
{
    float4x4 viewProjection;
    float4 forward;
    float2 jitter;
};

struct BindData
{
    float4x4 worldMatrix;
    float4 normalMatrix0;
    float4 normalMatrix1;
    float4 normalMatrix2;
    uint cameraData;
    uint material;
};
PushConstants(BindData, bindData);

struct MeshVertex
{
    float3 position : ATTRIBUTE0;
    float3 normal : ATTRIBUTE1;
    float3 tangent : ATTRIBUTE2;
    float2 uv0 : ATTRIBUTE3;
};

struct VSOut
{
    float4 positionCS : SV_Position;
    float3 positionWS : POSITION0;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
    float2 uv0 : TEXCOORD0;
};

[RootSignature(RS)]
VSOut Vertex(MeshVertex vertex RHI_VERTEX_DATA)
{
    ConstantBuffer<CameraData> cameraData = ResourceDescriptorHeap[bindData.cameraData];
    float4 positionWS = mul(bindData.worldMatrix, float4(vertex.position, 1.0));

    float3x3 normalMatrix = float3x3(bindData.normalMatrix0.xyz, bindData.normalMatrix1.xyz, bindData.normalMatrix2.xyz);

    VSOut output;
    output.positionCS = mul(cameraData.viewProjection, positionWS) + float4(cameraData.jitter, 0, 0);
    output.positionWS = positionWS.xyz;
    output.normal = mul(normalMatrix, vertex.normal);
    output.tangent = mul(normalMatrix, vertex.tangent);
    output.uv0 = vertex.uv0;
    return output;
}

[RootSignature(RS)]
float4 Pixel(VSOut input) : SV_Target0
{
    float3 N = normalize(input.normal);

    ConstantBuffer<CameraData> cameraData = ResourceDescriptorHeap[bindData.cameraData];
    float3 L = cameraData.forward;

    float3 diffuse = max(dot(N, -L), 0.4);

    float4 color = 1.xxxx;
    if (bindData.material != -1)
    {
        Texture2D<float4> baseColor = ResourceDescriptorHeap[bindData.material];
        color = baseColor.SampleLevel(linearWrap, input.uv0, 0);
        clip(color.a - 0.5);
    }

    return float4(diffuse * color.rgb, 1);
}