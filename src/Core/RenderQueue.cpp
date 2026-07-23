#include "Pch.h"
#include "RenderQueue.h"
// STL
#include <algorithm>

RenderQueue::RenderQueue() {
} // RenderQueue

RenderQueue::~RenderQueue() {
} // ~RenderQueue

void RenderQueue::Init() {
    m_commands.reserve(2000);
} // Init

void RenderQueue::Submit(const DrawCommand& cmd) {
    m_commands.push_back(cmd);
} // Submit

void RenderQueue::SortOpaque() {
    std::sort(m_commands.begin(), m_commands.end(), [](const DrawCommand& a, const DrawCommand& b) {
        return a.sortKey < b.sortKey;
    });
} // SortOpaque

void RenderQueue::SortTransparent() {
    std::sort(m_commands.begin(), m_commands.end(), [](const DrawCommand& a, const DrawCommand& b) {
        return a.sortKey > b.sortKey;
    });
} // SortTransparent

void RenderQueue::Execute(ID3D12GraphicsCommandList* cmdList) {
    ID3D12PipelineState* lastPSO = nullptr;

    for (auto& cmd : m_commands) {
        if (cmd.pso != lastPSO && cmd.pso != nullptr) {
            cmdList->SetPipelineState(cmd.pso);
            lastPSO = cmd.pso;
        }

        if (cmd.execute) {
            cmd.execute(cmdList);
        }
    }
} // Execute

void RenderQueue::Clear() {
    m_commands.clear();
} // Clear