#include "Pch.h"
#include "FunctionWidget.h"
#include "imgui.h"

FunctionWidget::FunctionWidget(std::string name, std::function<void()> guiFunc)
    : ImGuiWidget(), m_guiFunc(guiFunc) {
    m_name = name;
} // FunctionWidget

void FunctionWidget::Frame() {
    ImGui::Begin(m_name.c_str());
    if (m_guiFunc) m_guiFunc();
    ImGui::End();
} // Frame