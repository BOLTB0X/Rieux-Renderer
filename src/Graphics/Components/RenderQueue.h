#pragma once
#include <vector>
#include <functional>
#include <algorithm>
#include <d3d12.h>

struct DrawCommand {
    uint64_t                                        sortKey;
    ID3D12PipelineState*                            pso;
    std::function<void(ID3D12GraphicsCommandList*)> execute;

    DrawCommand() : sortKey(0), pso(nullptr) {

    }
}; // DrawCommand

class RenderQueue {
public:
    RenderQueue();
    RenderQueue(const RenderQueue&) = delete;
    RenderQueue& operator=(const RenderQueue&) = delete;
    ~RenderQueue();

    void Init();
    void Submit(const DrawCommand&);
    void SortOpaque();
    void SortTransparent();
    void Execute(ID3D12GraphicsCommandList*);
    void Clear();

private:
    std::vector<DrawCommand> m_commands;
}; // RenderQueue