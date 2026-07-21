#include "Pch.h"
#include "ImGuiWidget.h"

ImGuiWidget::ImGuiWidget()
    : m_name("?"),
    m_isVisible(true) {
} // ImGuiWidget

ImGuiWidget::ImGuiWidget(const std::string& name)
    : m_name(name),
    m_isVisible(true) {
} // ImGuiWidget

void ImGuiWidget::SetVisible(bool visible) {
    m_isVisible = visible;
} // SetVisible

bool ImGuiWidget::IsVisible() const {
    return m_isVisible;
} // IsVisible

const std::string& ImGuiWidget::GetName() const {
    return m_name;
} // GetName