#pragma once
#include "ImGuiWidget.h"
#include <functional>

class FunctionWidget : public ImGuiWidget {
public:
    FunctionWidget(std::string, std::function<void()>);
    virtual void Frame() override;

private:
    std::function<void()> m_guiFunc;
}; // FunctionWidget