// dxc -T cs_6_6 -E Compute -Fo DrawIdEmulationCopyCS.dxil DrawIdEmulationCopyCS.hlsl

#include "Common.hlsli"

struct BindData
{
    uint outputBuffer;
    uint sourceBuffer;
    uint countBuffer;
    uint stride;
    uint numCommands;
    uint mode;
};
PushConstants(BindData, bindData);

struct DrawIndirectCommand
{
    uint vertexCount;
    uint instanceCount;
    uint firstVertex;
    uint firstInstance;
};

struct DrawIndirectIndexedCommand
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    uint vertexOffset;
    uint firstInstance;
};

struct DrawIndirectCommandDrawId
{
    uint drawId;
    uint vertexCount;
    uint instanceCount;
    uint firstVertex;
    uint firstInstance;
};

struct DrawIndexedIndirectCommandDrawId
{
    uint drawId;
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    uint vertexOffset;
    uint firstInstance;
};

[RootSignature(RS)]
[numthreads(64, 1, 1)]
void Compute(uint3 id : SV_DispatchThreadID)
{
    uint numCommands = 0;
    if (bindData.countBuffer != -1)
    {
        ByteAddressBuffer countBuffer = ResourceDescriptorHeap[bindData.countBuffer];
        numCommands = countBuffer.Load(0);
    }
    else
        numCommands = bindData.numCommands;
    if (id.x > numCommands)
        return;
    
    ByteAddressBuffer sourceBuffer = ResourceDescriptorHeap[bindData.sourceBuffer];
    if (bindData.mode == 0)
    {
        uint4 commandData = sourceBuffer.Load4(id.x * bindData.stride);
        DrawIndirectCommandDrawId command;
        command.drawId = id.x;
        command.vertexCount = commandData[0];
        command.instanceCount = commandData[1];
        command.firstVertex = commandData[2];
        command.firstInstance = commandData[3];
        
        RWStructuredBuffer<DrawIndirectCommandDrawId> outputBuffer = ResourceDescriptorHeap[bindData.outputBuffer];
        outputBuffer[id.x] = command;

    }
    else
    {
        uint4 commandData0 = sourceBuffer.Load4(id.x * bindData.stride);
        uint commandData1 = sourceBuffer.Load(id.x * bindData.stride + 4);
        
        DrawIndexedIndirectCommandDrawId command;
        command.drawId = id.x;
        command.indexCount = commandData0[0];
        command.instanceCount = commandData0[1];
        command.firstIndex = commandData0[2];
        command.vertexOffset = commandData0[3];
        command.firstInstance = commandData1;
        
        RWStructuredBuffer<DrawIndexedIndirectCommandDrawId> outputBuffer = ResourceDescriptorHeap[bindData.outputBuffer];
        outputBuffer[id.x] = command;
    }
}