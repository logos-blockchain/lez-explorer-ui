#pragma once

#include <QString>

namespace Style {

// --- Color palette ---

namespace Color {
    inline const char* bg()        { return "#1e1e2e"; }
    inline const char* surface()   { return "#2a2a3c"; }
    inline const char* surfaceHover() { return "#33334a"; }
    inline const char* border()    { return "#3b3b52"; }
    inline const char* text()      { return "#cdd6f4"; }
    inline const char* subtext()   { return "#7f849c"; }
    inline const char* accent()    { return "#89b4fa"; }
    inline const char* green()     { return "#a6e3a1"; }
    inline const char* yellow()    { return "#f9e2af"; }
    inline const char* red()       { return "#f38ba8"; }
    inline const char* blue()      { return "#89b4fa"; }
    inline const char* mauve()     { return "#cba6f7"; }
    inline const char* peach()     { return "#fab387"; }
    inline const char* teal()      { return "#94e2d5"; }
}

// --- Base font size applied to the entire app ---

inline int baseFontSize() { return 14; }

// --- Widget styles ---

inline QString appBackground()
{
    return QString("background-color: %1; color: %2; font-size: %3px;")
        .arg(Color::bg(), Color::text())
        .arg(baseFontSize());
}

inline QString cardFrame()
{
    return QString(
        "background: %1; border: 1px solid %2; border-radius: 8px; padding: 18px;"
    ).arg(Color::surface(), Color::border());
}

inline QString cardFrameWithLabels()
{
    return QString(
        "QFrame { background: %1; border: 1px solid %2; border-radius: 8px; padding: 18px; }"
        " QFrame QLabel { color: %3; font-size: %4px; }"
    ).arg(Color::surface(), Color::border(), Color::text())
     .arg(baseFontSize());
}

inline QString clickableRowWithLabels(const QString& selector)
{
    return QString(
        "%1 { background: %2; border: 1px solid %3; border-radius: 8px; padding: 12px 16px; margin: 3px 0; }"
        " %1 QLabel { color: %4; font-size: %5px; }"
    ).arg(selector, Color::surface(), Color::border(), Color::text())
     .arg(baseFontSize());
}

inline QString mutedText()
{
    return QString("color: %1;").arg(Color::subtext());
}

inline QString monoText()
{
    return QString("font-family: 'Menlo', 'Courier New'; font-size: %1px; color: %2;")
        .arg(baseFontSize() - 1)
        .arg(Color::accent());
}

inline QString badge(const QString& bgColor)
{
    return QString("color: %1; background: %2; border-radius: 4px; padding: 3px 12px; font-weight: bold; font-size: %3px;")
        .arg(Color::bg(), bgColor)
        .arg(baseFontSize() - 1);
}

inline QString sectionHeader()
{
    return QString("color: %1;").arg(Color::text());
}

inline QString navButton()
{
    return QString(
        "QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: 6px; padding: 6px 14px; font-size: %7px; }"
        " QPushButton:hover { background: %4; border-color: %5; }"
        " QPushButton:disabled { color: %6; background: %1; border-color: %3; opacity: 0.4; }"
    ).arg(Color::surface(), Color::text(), Color::border(), Color::surfaceHover(), Color::accent(), Color::subtext())
     .arg(baseFontSize() + 2);
}

inline QString searchInput()
{
    return QString(
        "QLineEdit { background: %1; color: %2; border: 1px solid %3; border-radius: 6px; padding: 10px 14px; font-size: %6px; }"
        " QLineEdit:focus { border-color: %4; }"
        " QLineEdit::placeholder { color: %5; }"
    ).arg(Color::surface(), Color::text(), Color::border(), Color::accent(), Color::subtext())
     .arg(baseFontSize());
}

inline QString searchButton()
{
    return QString(
        "QPushButton { background: %1; color: %2; border: none; border-radius: 6px; padding: 10px 20px; font-size: %4px; font-weight: bold; }"
        " QPushButton:hover { background: %3; }"
    ).arg(Color::accent(), Color::bg(), Color::mauve())
     .arg(baseFontSize());
}

inline QString healthLabel()
{
    return QString("color: %1; font-weight: bold; font-size: %2px;")
        .arg(Color::green())
        .arg(baseFontSize());
}

// --- Status colors ---

inline QString statusColor(const QString& status)
{
    if (status == "Finalized") return Color::green();
    if (status == "Safe")      return Color::yellow();
    return Color::subtext(); // Pending
}

// --- Transaction type colors ---

inline QString txTypeColor(const QString& type)
{
    if (type == "Public")              return Color::blue();
    if (type == "Privacy-Preserving")  return Color::mauve();
    return Color::peach(); // Program Deployment
}

} // namespace Style
