#pragma once

#include <QString>

namespace Style {

inline QString cardFrame()
{
    return "background: #f8f9fa; color: #212529; border: 1px solid #dee2e6; border-radius: 6px; padding: 12px;";
}

inline QString cardFrameWithLabels()
{
    return "QFrame { background: #f8f9fa; border: 1px solid #dee2e6; border-radius: 6px; padding: 12px; }"
           " QFrame QLabel { color: #212529; }";
}

inline QString clickableRow()
{
    return "background: #f8f9fa; color: #212529; border: 1px solid #dee2e6; border-radius: 6px; padding: 8px; margin: 2px 0;";
}

inline QString clickableRowWithLabels(const QString& selector)
{
    return QString("%1 { background: #f8f9fa; border: 1px solid #dee2e6; border-radius: 6px; padding: 8px; margin: 2px 0; }"
                   " %1 QLabel { color: #212529; }").arg(selector);
}

inline QString monoText()
{
    return "font-family: 'Menlo', 'Courier New', 'DejaVu Sans Mono'; font-size: 11px;";
}

inline QString mutedText()
{
    return "color: #6c757d;";
}

inline QString badge(const QString& bgColor)
{
    return QString("color: white; background: %1; border-radius: 4px; padding: 2px 8px;").arg(bgColor);
}

} // namespace Style
