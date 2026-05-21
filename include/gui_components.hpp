#pragma once

#include "gui_theme.hpp"
#include "gui_rendering.hpp"
#include "gui_animations.hpp"
#include <string>
#include <functional>
#include <vector>

namespace dppbot {
namespace components {

using namespace theme;
using namespace rendering;

// Базовый компонент
struct Component {
    float x, y, width, height;
    bool visible = true;
    bool enabled = true;
    std::string id;

    bool Contains(float px, float py) const {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
};

// Кнопка
struct Button : Component {
    std::wstring text;
    bool hovered = false;
    bool pressed = false;
    std::function<void()> onClick;

    void Draw(
        ID2D1RenderTarget* rt,
        ID2D1SolidColorBrush* brush,
        IDWriteTextFormat* textFormat,
        dppbot::animation::TransitionManager& transitions
    ) {
        if (!visible) return;

        // Анимация hover
        float hoverValue = transitions.GetHoverValue(id);
        float pressValue = transitions.GetPressValue(id);

        // Если анимация не активна, используем 1.0 по умолчанию
        if (pressValue < 0.01f) {
            pressValue = 1.0f;
        }

        // Цвет фона с интерполяцией
        Color bgColor = LerpColor(colors::Surface, colors::SurfaceElevated, hoverValue);
        brush->SetColor(bgColor.ToD2D());

        // Применяем press scale
        float scaledWidth = width * pressValue;
        float scaledHeight = height * pressValue;
        float offsetX = (width - scaledWidth) / 2;
        float offsetY = (height - scaledHeight) / 2;

        // Glow эффект при hover
        if (hoverValue > 0.01f) {
            DrawGlow(rt, brush, x + offsetX, y + offsetY, scaledWidth, scaledHeight,
                     sizes::ButtonRadius, 12.0f, 0.3f * hoverValue);
        }

        // Основная кнопка
        DrawRoundedRect(rt, brush, x + offsetX, y + offsetY, scaledWidth, scaledHeight, sizes::ButtonRadius);

        // Граница
        Color borderColor = LerpColor(colors::BorderSubtle, colors::AccentBlue, hoverValue);
        brush->SetColor(borderColor.ToD2D());
        DrawRoundedRectBorder(rt, brush, x + offsetX, y + offsetY, scaledWidth, scaledHeight, sizes::ButtonRadius, 1.0f);

        // Текст
        brush->SetColor(colors::TextPrimary.ToD2D());
        DrawText(rt, textFormat, brush, text.c_str(),
                 x + offsetX, y + offsetY, scaledWidth, scaledHeight,
                 DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    void OnMouseEnter(dppbot::animation::TransitionManager& transitions) {
        hovered = true;
        transitions.StartHover(id);
    }

    void OnMouseLeave(dppbot::animation::TransitionManager& transitions) {
        hovered = false;
        transitions.EndHover(id);
    }

    void OnMouseDown(dppbot::animation::TransitionManager& transitions) {
        pressed = true;
        transitions.StartPress(id);
    }

    void OnMouseUp(dppbot::animation::TransitionManager& transitions) {
        pressed = false;
        transitions.EndPress(id);
        if (hovered && onClick) {
            onClick();
        }
    }
};

// Карточка
struct Card : Component {
    std::wstring title;
    std::vector<std::wstring> content;

    void Draw(
        ID2D1RenderTarget* rt,
        ID2D1SolidColorBrush* brush,
        ID2D1SolidColorBrush* shadowBrush,
        IDWriteTextFormat* titleFormat,
        IDWriteTextFormat* bodyFormat
    ) {
        if (!visible) return;

        // Карточка с тенью
        DrawRoundedRectWithShadow(
            rt, brush, shadowBrush,
            x, y, width, height,
            sizes::CardRadius, elevation::Level1
        );

        // Заголовок
        if (!title.empty()) {
            brush->SetColor(colors::TextPrimary.ToD2D());
            DrawText(rt, titleFormat, brush, title.c_str(),
                     x + sizes::CardPadding, y + sizes::CardPadding,
                     width - sizes::CardPadding * 2, 24.0f,
                     DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        }

        // Контент
        float contentY = y + sizes::CardPadding + (title.empty() ? 0.0f : 32.0f);
        for (const auto& line : content) {
            brush->SetColor(colors::TextSecondary.ToD2D());
            DrawText(rt, bodyFormat, brush, line.c_str(),
                     x + sizes::CardPadding, contentY,
                     width - sizes::CardPadding * 2, 20.0f,
                     DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
            contentY += 24.0f;
        }
    }
};

// Элемент списка
struct ListItem {
    std::wstring text;
    std::wstring secondaryText;
    bool selected = false;
    bool hovered = false;
    int index;
};

// Список
struct List : Component {
    std::vector<ListItem> items;
    int selectedIndex = -1;
    int hoverIndex = -1;
    int scrollOffset = 0;
    std::function<void(int)> onSelectionChanged;

    void Draw(
        ID2D1RenderTarget* rt,
        ID2D1SolidColorBrush* brush,
        IDWriteTextFormat* textFormat,
        IDWriteTextFormat* secondaryFormat,
        dppbot::animation::TransitionManager& transitions
    ) {
        if (!visible) return;

        // Фон списка
        brush->SetColor(colors::Surface.ToD2D());
        DrawRoundedRect(rt, brush, x, y, width, height, sizes::CardRadius);

        // Граница
        brush->SetColor(colors::BorderSubtle.ToD2D());
        DrawRoundedRectBorder(rt, brush, x, y, width, height, sizes::CardRadius, 1.0f);

        // Виртуализация: рендерим только видимые элементы
        int firstVisible = scrollOffset / static_cast<int>(sizes::ListItemHeight);
        int visibleCount = static_cast<int>(height / sizes::ListItemHeight) + 2;
        int lastVisible = std::min(firstVisible + visibleCount, static_cast<int>(items.size()));

        for (int i = firstVisible; i < lastVisible; ++i) {
            const auto& item = items[i];
            float itemY = y + i * sizes::ListItemHeight - scrollOffset;

            // Пропускаем если за пределами видимости
            if (itemY + sizes::ListItemHeight < y || itemY > y + height) {
                continue;
            }

            // Фон элемента
            if (item.selected) {
                brush->SetColor(colors::AccentBlueDim.ToD2D());
                DrawRoundedRect(rt, brush, x + 4, itemY + 2, width - 8, sizes::ListItemHeight - 4, 4.0f);

                // Акцентная полоска слева
                brush->SetColor(colors::AccentBlue.ToD2D());
                DrawRoundedRect(rt, brush, x + 4, itemY + 2, 3, sizes::ListItemHeight - 4, 1.5f);
            } else if (item.hovered) {
                float hoverValue = transitions.GetHoverValue(id + "_item_" + std::to_string(i));
                Color hoverColor = colors::SurfaceElevated;
                hoverColor.a = hoverValue;
                brush->SetColor(hoverColor.ToD2D());
                DrawRoundedRect(rt, brush, x + 4, itemY + 2, width - 8, sizes::ListItemHeight - 4, 4.0f);
            }

            // Основной текст
            brush->SetColor(colors::TextPrimary.ToD2D());
            DrawText(rt, textFormat, brush, item.text.c_str(),
                     x + sizes::ListItemPadding, itemY,
                     width - sizes::ListItemPadding * 2, sizes::ListItemHeight / 2,
                     DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

            // Вторичный текст
            if (!item.secondaryText.empty()) {
                brush->SetColor(colors::TextSecondary.ToD2D());
                DrawText(rt, secondaryFormat, brush, item.secondaryText.c_str(),
                         x + sizes::ListItemPadding, itemY + sizes::ListItemHeight / 2,
                         width - sizes::ListItemPadding * 2, sizes::ListItemHeight / 2,
                         DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            }
        }
    }

    int HitTest(float px, float py) {
        if (!Contains(px, py)) return -1;
        int index = static_cast<int>((py - y + scrollOffset) / sizes::ListItemHeight);
        return (index >= 0 && index < static_cast<int>(items.size())) ? index : -1;
    }

    void OnMouseMove(float px, float py, dppbot::animation::TransitionManager& transitions) {
        int newHoverIndex = HitTest(px, py);
        if (newHoverIndex != hoverIndex) {
            if (hoverIndex >= 0 && hoverIndex < static_cast<int>(items.size())) {
                items[hoverIndex].hovered = false;
                transitions.EndHover(id + "_item_" + std::to_string(hoverIndex));
            }
            hoverIndex = newHoverIndex;
            if (hoverIndex >= 0) {
                items[hoverIndex].hovered = true;
                transitions.StartHover(id + "_item_" + std::to_string(hoverIndex));
            }
        }
    }

    void OnClick(float px, float py) {
        int index = HitTest(px, py);
        if (index >= 0) {
            if (selectedIndex >= 0 && selectedIndex < static_cast<int>(items.size())) {
                items[selectedIndex].selected = false;
            }
            selectedIndex = index;
            items[index].selected = true;
            if (onSelectionChanged) {
                onSelectionChanged(index);
            }
        }
    }
};

// Таб
struct Tab {
    std::wstring text;
    bool active = false;
    bool hovered = false;
};

// Панель табов
struct TabBar : Component {
    std::vector<Tab> tabs;
    int activeIndex = 0;
    std::function<void(int)> onTabChanged;

    void Draw(
        ID2D1RenderTarget* rt,
        ID2D1SolidColorBrush* brush,
        IDWriteTextFormat* textFormat,
        dppbot::animation::TransitionManager& transitions
    ) {
        if (!visible) return;

        // Фон панели табов
        brush->SetColor(colors::Surface.ToD2D());
        rt->FillRectangle(D2D1::RectF(x, y, x + width, y + height), brush);

        // Нижняя граница
        brush->SetColor(colors::BorderSubtle.ToD2D());
        DrawLine(rt, brush, x, y + height - 1, x + width, y + height - 1, 1.0f);

        float tabWidth = width / tabs.size();
        for (size_t i = 0; i < tabs.size(); ++i) {
            const auto& tab = tabs[i];
            float tabX = x + i * tabWidth;

            // Hover эффект
            if (tab.hovered && !tab.active) {
                float hoverValue = transitions.GetHoverValue(id + "_tab_" + std::to_string(i));
                Color hoverColor = colors::SurfaceElevated;
                hoverColor.a = hoverValue * 0.5f;
                brush->SetColor(hoverColor.ToD2D());
                rt->FillRectangle(D2D1::RectF(tabX, y, tabX + tabWidth, y + height - 1), brush);
            }

            // Текст таба
            Color textColor = tab.active ? colors::AccentBlue : colors::TextSecondary;
            brush->SetColor(textColor.ToD2D());
            DrawText(rt, textFormat, brush, tab.text.c_str(),
                     tabX, y, tabWidth, height,
                     DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

            // Индикатор активного таба
            if (tab.active) {
                brush->SetColor(colors::AccentBlue.ToD2D());
                rt->FillRectangle(D2D1::RectF(tabX, y + height - 3, tabX + tabWidth, y + height), brush);
            }
        }
    }

    int HitTest(float px, float py) {
        if (!Contains(px, py)) return -1;
        float tabWidth = width / tabs.size();
        int index = static_cast<int>((px - x) / tabWidth);
        return (index >= 0 && index < static_cast<int>(tabs.size())) ? index : -1;
    }

    void OnMouseMove(float px, float py, dppbot::animation::TransitionManager& transitions) {
        int hoverIndex = HitTest(px, py);
        for (size_t i = 0; i < tabs.size(); ++i) {
            bool wasHovered = tabs[i].hovered;
            tabs[i].hovered = (static_cast<int>(i) == hoverIndex);

            if (tabs[i].hovered && !wasHovered) {
                transitions.StartHover(id + "_tab_" + std::to_string(i));
            } else if (!tabs[i].hovered && wasHovered) {
                transitions.EndHover(id + "_tab_" + std::to_string(i));
            }
        }
    }

    void OnClick(float px, float py, dppbot::animation::TransitionManager& transitions) {
        int index = HitTest(px, py);
        if (index >= 0 && index != activeIndex) {
            tabs[activeIndex].active = false;
            activeIndex = index;
            tabs[index].active = true;

            // Анимация переключения
            transitions.StartFadeOut(id + "_content");

            if (onTabChanged) {
                onTabChanged(index);
            }
        }
    }
};

}  // namespace components
}  // namespace dppbot
