#include "pack_library.hpp"
#include "gui_theme.hpp"
#include "gui_rendering.hpp"
#include "gui_animations.hpp"
#include "gui_components.hpp"

#include <windows.h>
#include <windowsx.h>
#include <d2d1.h>
#include <dwrite.h>
#include <dwmapi.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dwmapi.lib")

#include <memory>
#include <vector>
#include <string>
#include <optional>

namespace dppbot {

using namespace theme;
using namespace rendering;
using namespace components;

struct AppState {
    HWND window = nullptr;

    // Direct2D
    ID2D1Factory* d2dFactory = nullptr;
    ID2D1HwndRenderTarget* renderTarget = nullptr;
    IDWriteFactory* writeFactory = nullptr;

    // Шрифты
    IDWriteTextFormat* fontBody = nullptr;
    IDWriteTextFormat* fontTitle = nullptr;
    IDWriteTextFormat* fontSmall = nullptr;

    // Кисти
    ID2D1SolidColorBrush* brush = nullptr;
    ID2D1SolidColorBrush* shadowBrush = nullptr;

    // Анимации
    animation::TransitionManager transitions;

    // UI компоненты
    TabBar tabBar;
    std::vector<Button> buttons;
    std::vector<List> lists;
    std::vector<Card> cards;

    // Данные
    PackLibrary library;
    std::vector<std::filesystem::path> loadedPacks;
    std::vector<std::string> availableHeroes;
    std::optional<MultiPackHeroReport> currentReport;

    // Состояние
    int currentTab = 0;
    bool needsRedraw = true;
    POINT lastMousePos = {};
};

std::wstring ToWide(const std::string& str) {
    if (str.empty()) return {};
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring result(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, result.data(), size);
    return result;
}

std::string ToUtf8(const std::wstring& str) {
    if (str.empty()) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, result.data(), size, nullptr, nullptr);
    return result;
}

HRESULT CreateDeviceResources(AppState* state) {
    if (state->renderTarget) return S_OK;

    RECT rc;
    GetClientRect(state->window, &rc);
    D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

    HRESULT hr = state->d2dFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(state->window, size),
        &state->renderTarget
    );

    if (SUCCEEDED(hr)) {
        state->renderTarget->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1), &state->brush);
        state->renderTarget->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0), &state->shadowBrush);
    }

    return hr;
}

void DiscardDeviceResources(AppState* state) {
    if (state->brush) { state->brush->Release(); state->brush = nullptr; }
    if (state->shadowBrush) { state->shadowBrush->Release(); state->shadowBrush = nullptr; }
    if (state->renderTarget) { state->renderTarget->Release(); state->renderTarget = nullptr; }
}

void InitializeUI(AppState* state, float width, float height) {
    // Tab bar
    state->tabBar.id = "main_tabs";
    state->tabBar.x = 0;
    state->tabBar.y = sizes::TitleBarHeight;
    state->tabBar.width = width;
    state->tabBar.height = sizes::TabBarHeight;
    state->tabBar.tabs = {
        {L"Паки", true, false},
        {L"Анализ", false, false},
        {L"Логи", false, false}
    };
    state->tabBar.onTabChanged = [state](int index) {
        state->currentTab = index;
        state->needsRedraw = true;
    };

    // Кнопка загрузки
    Button loadBtn;
    loadBtn.id = "btn_load";
    loadBtn.x = spacing::Space24;
    loadBtn.y = sizes::TitleBarHeight + sizes::TabBarHeight + spacing::Space24;
    loadBtn.width = 200;
    loadBtn.height = sizes::ButtonHeightMedium;
    loadBtn.text = L"Загрузить библиотеку";
    loadBtn.onClick = [state]() {
        // TODO: Открыть диалог выбора папки
        state->needsRedraw = true;
    };
    state->buttons.push_back(loadBtn);

    // Список паков
    List packList;
    packList.id = "list_packs";
    packList.x = spacing::Space24;
    packList.y = loadBtn.y + loadBtn.height + spacing::Space16;
    packList.width = 300;
    packList.height = 400;
    packList.onSelectionChanged = [state](int index) {
        state->needsRedraw = true;
    };
    state->lists.push_back(packList);

    // Список героев
    List heroList;
    heroList.id = "list_heroes";
    heroList.x = packList.x + packList.width + spacing::Space24;
    heroList.y = packList.y;
    heroList.width = 300;
    heroList.height = 400;
    heroList.onSelectionChanged = [state](int index) {
        if (index >= 0 && index < static_cast<int>(state->availableHeroes.size())) {
            // TODO: Анализировать героя
            state->needsRedraw = true;
        }
    };
    state->lists.push_back(heroList);

    // Карточка анализа
    Card analysisCard;
    analysisCard.id = "card_analysis";
    analysisCard.x = heroList.x + heroList.width + spacing::Space24;
    analysisCard.y = packList.y;
    analysisCard.width = width - analysisCard.x - spacing::Space24;
    analysisCard.height = 400;
    analysisCard.title = L"Анализ";
    analysisCard.content = {L"Выберите паки и героя для анализа"};
    state->cards.push_back(analysisCard);
}

void DrawTitleBar(AppState* state, float width) {
    auto* rt = state->renderTarget;
    auto* brush = state->brush;

    // Фон title bar
    brush->SetColor(colors::DeepBlack.ToD2D());
    rt->FillRectangle(D2D1::RectF(0, 0, width, sizes::TitleBarHeight), brush);

    // Заголовок
    brush->SetColor(colors::TextPrimary.ToD2D());
    DrawText(rt, state->fontTitle, brush, L"dppbotcpp Студия",
             spacing::Space16, 0, 300, sizes::TitleBarHeight,
             DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Кнопки управления окном
    float btnWidth = 46.0f;
    float btnY = 0;
    float btnHeight = sizes::TitleBarHeight;

    // Minimize
    brush->SetColor(colors::TextDim.ToD2D());
    DrawText(rt, state->fontBody, brush, L"─",
             width - btnWidth * 3, btnY, btnWidth, btnHeight,
             DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Maximize
    DrawText(rt, state->fontBody, brush, L"□",
             width - btnWidth * 2, btnY, btnWidth, btnHeight,
             DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Close
    brush->SetColor(colors::Error.ToD2D());
    DrawText(rt, state->fontTitle, brush, L"×",
             width - btnWidth, btnY, btnWidth, btnHeight,
             DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Нижняя граница
    brush->SetColor(colors::BorderSubtle.ToD2D());
    DrawLine(rt, brush, 0, sizes::TitleBarHeight - 1, width, sizes::TitleBarHeight - 1, 1.0f);
}

void DrawStatusBar(AppState* state, float width, float height) {
    auto* rt = state->renderTarget;
    auto* brush = state->brush;

    float y = height - sizes::StatusBarHeight;

    // Фон
    brush->SetColor(colors::Surface.ToD2D());
    rt->FillRectangle(D2D1::RectF(0, y, width, height), brush);

    // Верхняя граница
    brush->SetColor(colors::BorderSubtle.ToD2D());
    DrawLine(rt, brush, 0, y, width, y, 1.0f);

    // Статус
    std::wstring status = L"⚡ Готово";
    if (!state->loadedPacks.empty()) {
        status += L" | " + std::to_wstring(state->loadedPacks.size()) + L" паков загружено";
    }

    brush->SetColor(colors::TextSecondary.ToD2D());
    DrawText(rt, state->fontSmall, brush, status.c_str(),
             spacing::Space16, y, width - spacing::Space32, sizes::StatusBarHeight,
             DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

void OnRender(AppState* state) {
    HRESULT hr = CreateDeviceResources(state);
    if (FAILED(hr)) return;

    auto* rt = state->renderTarget;

    rt->BeginDraw();
    rt->SetTransform(D2D1::Matrix3x2F::Identity());

    // Фон
    rt->Clear(colors::DeepBlack.ToD2D());

    RECT rc;
    GetClientRect(state->window, &rc);
    float width = static_cast<float>(rc.right);
    float height = static_cast<float>(rc.bottom);

    // Title bar
    DrawTitleBar(state, width);

    // Tab bar
    state->tabBar.Draw(rt, state->brush, state->fontBody, state->transitions);

    // Content area
    float contentY = sizes::TitleBarHeight + sizes::TabBarHeight;
    float contentHeight = height - contentY - sizes::StatusBarHeight;

    state->brush->SetColor(colors::Surface.ToD2D());
    rt->FillRectangle(D2D1::RectF(0, contentY, width, contentY + contentHeight), state->brush);

    // Компоненты в зависимости от таба
    if (state->currentTab == 0) {
        // Паки
        for (auto& btn : state->buttons) {
            btn.Draw(rt, state->brush, state->fontBody, state->transitions);
        }
        for (auto& list : state->lists) {
            list.Draw(rt, state->brush, state->fontBody, state->fontSmall, state->transitions);
        }
        for (auto& card : state->cards) {
            card.Draw(rt, state->brush, state->shadowBrush, state->fontTitle, state->fontBody);
        }
    }

    // Status bar
    DrawStatusBar(state, width, height);

    hr = rt->EndDraw();

    if (hr == D2DERR_RECREATE_TARGET) {
        DiscardDeviceResources(state);
    }

    state->needsRedraw = false;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    AppState* state = reinterpret_cast<AppState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (message) {
    case WM_CREATE: {
        LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
        state = reinterpret_cast<AppState*>(pcs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));

        RECT rc;
        GetClientRect(hwnd, &rc);
        InitializeUI(state, static_cast<float>(rc.right), static_cast<float>(rc.bottom));

        // Таймер для анимаций (60 FPS)
        SetTimer(hwnd, 1, 16, nullptr);
        return 0;
    }

    case WM_TIMER:
        if (state && state->transitions.HasActiveAnimations()) {
            state->needsRedraw = true;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_PAINT:
    case WM_DISPLAYCHANGE:
        if (state) {
            OnRender(state);
            ValidateRect(hwnd, nullptr);
        }
        return 0;

    case WM_SIZE:
        if (state && state->renderTarget) {
            RECT rc;
            GetClientRect(hwnd, &rc);
            D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);
            state->renderTarget->Resize(size);
            InitializeUI(state, static_cast<float>(rc.right), static_cast<float>(rc.bottom));
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_MOUSEMOVE: {
        if (!state) break;

        POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        float x = static_cast<float>(pt.x);
        float y = static_cast<float>(pt.y);

        // Tab bar
        state->tabBar.OnMouseMove(x, y, state->transitions);

        // Buttons
        for (auto& btn : state->buttons) {
            bool wasHovered = btn.hovered;
            bool nowHovered = btn.Contains(x, y);

            if (nowHovered && !wasHovered) {
                btn.OnMouseEnter(state->transitions);
                state->needsRedraw = true;
            } else if (!nowHovered && wasHovered) {
                btn.OnMouseLeave(state->transitions);
                state->needsRedraw = true;
            }
        }

        // Lists
        for (auto& list : state->lists) {
            list.OnMouseMove(x, y, state->transitions);
        }

        state->lastMousePos = pt;
        if (state->needsRedraw) {
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONDOWN: {
        if (!state) break;

        float x = static_cast<float>(GET_X_LPARAM(lParam));
        float y = static_cast<float>(GET_Y_LPARAM(lParam));

        // Tab bar
        state->tabBar.OnClick(x, y, state->transitions);

        // Buttons
        for (auto& btn : state->buttons) {
            if (btn.Contains(x, y)) {
                btn.OnMouseDown(state->transitions);
                state->needsRedraw = true;
            }
        }

        // Lists
        for (auto& list : state->lists) {
            list.OnClick(x, y);
        }

        if (state->needsRedraw) {
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONUP: {
        if (!state) break;

        for (auto& btn : state->buttons) {
            if (btn.pressed) {
                btn.OnMouseUp(state->transitions);
                state->needsRedraw = true;
            }
        }

        if (state->needsRedraw) {
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_NCHITTEST: {
        LRESULT hit = DefWindowProcW(hwnd, message, wParam, lParam);
        if (hit == HTCLIENT) {
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ScreenToClient(hwnd, &pt);

            if (pt.y < sizes::TitleBarHeight) {
                RECT rc;
                GetClientRect(hwnd, &rc);
                float width = static_cast<float>(rc.right);

                if (pt.x > width - 46) return HTCLOSE;
                if (pt.x > width - 92) return HTMAXBUTTON;
                if (pt.x > width - 138) return HTMINBUTTON;
                return HTCAPTION;
            }
        }
        return hit;
    }

    case WM_DESTROY:
        if (state) {
            KillTimer(hwnd, 1);
            DiscardDeviceResources(state);
            if (state->fontBody) state->fontBody->Release();
            if (state->fontTitle) state->fontTitle->Release();
            if (state->fontSmall) state->fontSmall->Release();
            if (state->writeFactory) state->writeFactory->Release();
            if (state->d2dFactory) state->d2dFactory->Release();
            delete state;
        }
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

}  // namespace dppbot

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    auto* state = new dppbot::AppState();

    // Direct2D
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &state->d2dFactory);
    if (FAILED(hr)) return -1;

    // DirectWrite
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                             reinterpret_cast<IUnknown**>(&state->writeFactory));
    if (FAILED(hr)) return -1;

    // Шрифты
    state->writeFactory->CreateTextFormat(
        dppbot::typography::FontFamily, nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        dppbot::typography::FontSizeBody, L"ru-RU", &state->fontBody
    );

    state->writeFactory->CreateTextFormat(
        dppbot::typography::FontFamily, nullptr,
        DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        dppbot::typography::FontSizeTitle, L"ru-RU", &state->fontTitle
    );

    state->writeFactory->CreateTextFormat(
        dppbot::typography::FontFamily, nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        dppbot::typography::FontSizeSmall, L"ru-RU", &state->fontSmall
    );

    // Окно без системного title bar
    const wchar_t className[] = L"DppbotModernWindow";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = dppbot::WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = className;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    RegisterClassW(&wc);

    // WS_POPUP убирает системный title bar
    state->window = CreateWindowExW(
        WS_EX_APPWINDOW,
        className,
        L"dppbotcpp Студия",
        WS_POPUP | WS_VISIBLE | WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1400, 800,
        nullptr, nullptr, instance, state
    );

    if (!state->window) return -1;

    // Включаем тень окна (DWM)
    MARGINS margins = {0, 0, 0, 1};
    DwmExtendFrameIntoClientArea(state->window, &margins);

    ShowWindow(state->window, showCommand);
    UpdateWindow(state->window);

    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    CoUninitialize();
    return static_cast<int>(msg.wParam);
}
