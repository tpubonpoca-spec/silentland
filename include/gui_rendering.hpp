#pragma once

#include "gui_theme.hpp"
#include <d2d1.h>
#include <dwrite.h>

namespace dppbot {
namespace rendering {

using namespace theme;

// Рисование прямоугольника с закруглёнными углами
inline void DrawRoundedRect(
    ID2D1RenderTarget* rt,
    ID2D1SolidColorBrush* brush,
    float x, float y, float width, float height,
    float radius = sizes::ButtonRadius
) {
    D2D1_ROUNDED_RECT rect = D2D1::RoundedRect(
        D2D1::RectF(x, y, x + width, y + height),
        radius, radius
    );
    rt->FillRoundedRectangle(rect, brush);
}

// Рисование прямоугольника с тенью
inline void DrawRoundedRectWithShadow(
    ID2D1RenderTarget* rt,
    ID2D1SolidColorBrush* brush,
    ID2D1SolidColorBrush* shadowBrush,
    float x, float y, float width, float height,
    float radius = sizes::CardRadius,
    elevation::Shadow shadow = elevation::Level1
) {
    // Тень
    if (shadow.blur > 0.0f) {
        shadowBrush->SetColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, shadow.opacity));
        DrawRoundedRect(
            rt, shadowBrush,
            x + shadow.offsetX, y + shadow.offsetY,
            width, height, radius
        );
    }

    // Основной прямоугольник
    DrawRoundedRect(rt, brush, x, y, width, height, radius);
}

// Рисование границы
inline void DrawRoundedRectBorder(
    ID2D1RenderTarget* rt,
    ID2D1SolidColorBrush* brush,
    float x, float y, float width, float height,
    float radius = sizes::ButtonRadius,
    float strokeWidth = 1.0f
) {
    D2D1_ROUNDED_RECT rect = D2D1::RoundedRect(
        D2D1::RectF(x, y, x + width, y + height),
        radius, radius
    );
    rt->DrawRoundedRectangle(rect, brush, strokeWidth);
}

// Рисование текста с выравниванием
inline void DrawText(
    ID2D1RenderTarget* rt,
    IDWriteTextFormat* format,
    ID2D1SolidColorBrush* brush,
    const wchar_t* text,
    float x, float y, float width, float height,
    DWRITE_TEXT_ALIGNMENT alignment = DWRITE_TEXT_ALIGNMENT_LEADING,
    DWRITE_PARAGRAPH_ALIGNMENT paragraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER
) {
    format->SetTextAlignment(alignment);
    format->SetParagraphAlignment(paragraphAlignment);

    D2D1_RECT_F rect = D2D1::RectF(x, y, x + width, y + height);
    rt->DrawText(
        text,
        static_cast<UINT32>(wcslen(text)),
        format,
        rect,
        brush,
        D2D1_DRAW_TEXT_OPTIONS_CLIP
    );
}

// Рисование glow эффекта (для hover/focus)
inline void DrawGlow(
    ID2D1RenderTarget* rt,
    ID2D1SolidColorBrush* brush,
    float x, float y, float width, float height,
    float radius = sizes::ButtonRadius,
    float glowSize = 12.0f,
    float opacity = 0.3f
) {
    Color glowColor = colors::AccentBlue;
    glowColor.a = opacity;
    brush->SetColor(glowColor.ToD2D());

    DrawRoundedRect(
        rt, brush,
        x - glowSize / 2, y - glowSize / 2,
        width + glowSize, height + glowSize,
        radius + glowSize / 2
    );
}

// Рисование линии
inline void DrawLine(
    ID2D1RenderTarget* rt,
    ID2D1SolidColorBrush* brush,
    float x1, float y1, float x2, float y2,
    float strokeWidth = 1.0f
) {
    rt->DrawLine(
        D2D1::Point2F(x1, y1),
        D2D1::Point2F(x2, y2),
        brush,
        strokeWidth
    );
}

// Рисование вертикального градиента
inline void DrawVerticalGradient(
    ID2D1RenderTarget* rt,
    ID2D1LinearGradientBrush* gradientBrush,
    float x, float y, float width, float height
) {
    D2D1_RECT_F rect = D2D1::RectF(x, y, x + width, y + height);
    rt->FillRectangle(rect, gradientBrush);
}

// Создание градиентной кисти
inline HRESULT CreateVerticalGradient(
    ID2D1RenderTarget* rt,
    Color topColor,
    Color bottomColor,
    ID2D1LinearGradientBrush** gradientBrush
) {
    ID2D1GradientStopCollection* gradientStops = nullptr;

    D2D1_GRADIENT_STOP stops[2];
    stops[0].position = 0.0f;
    stops[0].color = topColor.ToD2D();
    stops[1].position = 1.0f;
    stops[1].color = bottomColor.ToD2D();

    HRESULT hr = rt->CreateGradientStopCollection(
        stops, 2,
        D2D1_GAMMA_2_2,
        D2D1_EXTEND_MODE_CLAMP,
        &gradientStops
    );

    if (SUCCEEDED(hr)) {
        hr = rt->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(
                D2D1::Point2F(0, 0),
                D2D1::Point2F(0, 100)
            ),
            gradientStops,
            gradientBrush
        );
        gradientStops->Release();
    }

    return hr;
}

// Интерполяция цвета
inline Color LerpColor(Color a, Color b, float t) {
    return {
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t
    };
}

// Интерполяция float
inline float Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

}  // namespace rendering
}  // namespace dppbot
