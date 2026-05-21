#pragma once

#include <d2d1.h>

namespace dppbot {
namespace theme {

// Цветовая палитра
struct Color {
    float r, g, b, a;
    D2D1_COLOR_F ToD2D() const { return D2D1::ColorF(r, g, b, a); }
};

// Основные цвета
namespace colors {
    constexpr Color DeepBlack = {0.02f, 0.02f, 0.02f, 1.0f};
    constexpr Color Surface = {0.04f, 0.04f, 0.04f, 1.0f};
    constexpr Color SurfaceElevated = {0.06f, 0.06f, 0.06f, 1.0f};

    constexpr Color TextPrimary = {0.95f, 0.95f, 0.95f, 1.0f};
    constexpr Color TextSecondary = {0.6f, 0.6f, 0.6f, 1.0f};
    constexpr Color TextDim = {0.35f, 0.35f, 0.35f, 1.0f};

    constexpr Color AccentBlue = {0.3f, 0.7f, 1.0f, 1.0f};
    constexpr Color AccentBlueHover = {0.4f, 0.8f, 1.0f, 1.0f};
    constexpr Color AccentBlueDim = {0.15f, 0.35f, 0.5f, 0.3f};

    constexpr Color Success = {0.3f, 0.9f, 0.5f, 1.0f};
    constexpr Color Warning = {1.0f, 0.8f, 0.3f, 1.0f};
    constexpr Color Error = {1.0f, 0.4f, 0.4f, 1.0f};

    constexpr Color BorderSubtle = {0.12f, 0.12f, 0.12f, 1.0f};
    constexpr Color BorderFocus = {0.3f, 0.7f, 1.0f, 0.5f};
}

// Типографика
namespace typography {
    constexpr float FontSizeSmall = 11.0f;
    constexpr float FontSizeBody = 13.0f;
    constexpr float FontSizeLarge = 15.0f;
    constexpr float FontSizeTitle = 18.0f;

    constexpr wchar_t FontFamily[] = L"Segoe UI";
}

// Spacing
namespace spacing {
    constexpr float Space2 = 2.0f;
    constexpr float Space4 = 4.0f;
    constexpr float Space8 = 8.0f;
    constexpr float Space12 = 12.0f;
    constexpr float Space16 = 16.0f;
    constexpr float Space24 = 24.0f;
    constexpr float Space32 = 32.0f;
    constexpr float Space48 = 48.0f;
}

// Размеры компонентов
namespace sizes {
    constexpr float TitleBarHeight = 40.0f;
    constexpr float TabBarHeight = 48.0f;
    constexpr float StatusBarHeight = 32.0f;

    constexpr float ButtonHeightSmall = 28.0f;
    constexpr float ButtonHeightMedium = 36.0f;
    constexpr float ButtonHeightLarge = 44.0f;
    constexpr float ButtonRadius = 6.0f;

    constexpr float CardRadius = 8.0f;
    constexpr float CardPadding = 16.0f;

    constexpr float InputHeight = 36.0f;
    constexpr float InputRadius = 6.0f;
    constexpr float InputPadding = 12.0f;

    constexpr float ListItemHeight = 40.0f;
    constexpr float ListItemPadding = 12.0f;
}

// Анимации
namespace anim {
    constexpr float DurationFast = 150.0f;
    constexpr float DurationNormal = 250.0f;
    constexpr float DurationSlow = 400.0f;

    // Easing functions
    inline float EaseOutQuad(float t) {
        return t * (2.0f - t);
    }

    inline float EaseInOutQuad(float t) {
        return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
    }

    inline float EaseInQuad(float t) {
        return t * t;
    }
}

// Elevation (тени)
namespace elevation {
    struct Shadow {
        float offsetX;
        float offsetY;
        float blur;
        float opacity;
    };

    constexpr Shadow Level0 = {0.0f, 0.0f, 0.0f, 0.0f};
    constexpr Shadow Level1 = {0.0f, 2.0f, 4.0f, 0.2f};
    constexpr Shadow Level2 = {0.0f, 4.0f, 8.0f, 0.3f};
    constexpr Shadow Level3 = {0.0f, 8.0f, 16.0f, 0.4f};
}

}  // namespace theme
}  // namespace dppbot
