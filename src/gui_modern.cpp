#include "pack_library.hpp"

#include <windows.h>
#include <windowsx.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <commdlg.h>
#include <shlobj.h>
#include <objbase.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <functional>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>

namespace dppbot {

namespace {

template<class Interface>
inline void SafeRelease(Interface** ppInterfaceToRelease) {
    if (*ppInterfaceToRelease != nullptr) {
        (*ppInterfaceToRelease)->Release();
        (*ppInterfaceToRelease) = nullptr;
    }
}

struct Color {
    float r, g, b, a;
    D2D1_COLOR_F ToD2D() const { return D2D1::ColorF(r, g, b, a); }
};

namespace Colors {
    constexpr Color Background = {0.05f, 0.05f, 0.05f, 1.0f};
    constexpr Color Surface = {0.08f, 0.08f, 0.08f, 1.0f};
    constexpr Color SurfaceHover = {0.12f, 0.12f, 0.12f, 1.0f};
    constexpr Color Border = {0.15f, 0.15f, 0.15f, 1.0f};
    constexpr Color Text = {0.9f, 0.9f, 0.9f, 1.0f};
    constexpr Color TextDim = {0.5f, 0.5f, 0.5f, 1.0f};
    constexpr Color Accent = {0.2f, 0.6f, 1.0f, 1.0f};
    constexpr Color AccentHover = {0.3f, 0.7f, 1.0f, 1.0f};
    constexpr Color Success = {0.2f, 0.8f, 0.4f, 1.0f};
    constexpr Color Warning = {1.0f, 0.7f, 0.2f, 1.0f};
    constexpr Color Error = {1.0f, 0.3f, 0.3f, 1.0f};
}

struct Rect {
    float x, y, width, height;
    D2D1_RECT_F ToD2D() const { return D2D1::RectF(x, y, x + width, y + height); }
    bool Contains(float px, float py) const {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
};

enum class ButtonState {
    Normal,
    Hover,
    Pressed
};

struct Button {
    Rect bounds;
    std::wstring text;
    ButtonState state = ButtonState::Normal;
    bool enabled = true;
    std::function<void()> onClick;
};

struct ListBox {
    Rect bounds;
    std::vector<std::wstring> items;
    std::vector<bool> selected;
    int hoverIndex = -1;
    int scrollOffset = 0;
    float itemHeight = 32.0f;
    bool multiSelect = false;
};

struct TextBox {
    Rect bounds;
    std::wstring text;
    bool focused = false;
    bool readOnly = false;
    int cursorPos = 0;
};

struct LogEntry {
    std::wstring text;
    std::chrono::system_clock::time_point timestamp;
    enum class Level { Info, Success, Warning, Error } level;
};

struct AppState {
    HWND window = nullptr;
    ID2D1Factory* d2dFactory = nullptr;
    ID2D1HwndRenderTarget* renderTarget = nullptr;
    IDWriteFactory* writeFactory = nullptr;
    IDWriteTextFormat* textFormat = nullptr;
    IDWriteTextFormat* textFormatBold = nullptr;
    IDWriteTextFormat* textFormatSmall = nullptr;

    ID2D1SolidColorBrush* brushBackground = nullptr;
    ID2D1SolidColorBrush* brushSurface = nullptr;
    ID2D1SolidColorBrush* brushText = nullptr;
    ID2D1SolidColorBrush* brushAccent = nullptr;

    bool isDragging = false;
    POINT dragStart = {};

    std::vector<Button> buttons;
    std::vector<ListBox> listBoxes;
    std::vector<TextBox> textBoxes;
    std::vector<LogEntry> logs;

    std::vector<std::filesystem::path> packs;
    std::vector<PackScanResult> packScans;
    std::vector<std::string> visibleHeroes;
    std::optional<MultiPackHeroReport> currentReport;

    bool showLogsWindow = false;
    HWND logsWindow = nullptr;

    int activeTab = 0;
};

std::wstring ToWide(const std::string& value) {
    if (value.empty()) return {};
    const int size = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring result(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), result.data(), size);
    return result;
}

std::string ToUtf8(const std::wstring& value) {
    if (value.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    std::string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), result.data(), size, nullptr, nullptr);
    return result;
}

void AddLog(AppState* state, const std::wstring& text, LogEntry::Level level = LogEntry::Level::Info) {
    LogEntry entry;
    entry.text = text;
    entry.timestamp = std::chrono::system_clock::now();
    entry.level = level;
    state->logs.push_back(entry);

    if (state->logs.size() > 1000) {
        state->logs.erase(state->logs.begin());
    }
}

HRESULT CreateDeviceResources(AppState* state) {
    HRESULT hr = S_OK;

    if (!state->renderTarget) {
        RECT rc;
        GetClientRect(state->window, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

        hr = state->d2dFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(state->window, size),
            &state->renderTarget
        );

        if (SUCCEEDED(hr)) {
            state->renderTarget->CreateSolidColorBrush(Colors::Background.ToD2D(), &state->brushBackground);
            state->renderTarget->CreateSolidColorBrush(Colors::Surface.ToD2D(), &state->brushSurface);
            state->renderTarget->CreateSolidColorBrush(Colors::Text.ToD2D(), &state->brushText);
            state->renderTarget->CreateSolidColorBrush(Colors::Accent.ToD2D(), &state->brushAccent);
        }
    }

    return hr;
}

void DiscardDeviceResources(AppState* state) {
    SafeRelease(&state->brushBackground);
    SafeRelease(&state->brushSurface);
    SafeRelease(&state->brushText);
    SafeRelease(&state->brushAccent);
    SafeRelease(&state->renderTarget);
}

void DrawRoundedRect(ID2D1RenderTarget* rt, const Rect& rect, ID2D1SolidColorBrush* brush, float radius = 4.0f) {
    D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(rect.ToD2D(), radius, radius);
    rt->FillRoundedRectangle(roundedRect, brush);
}

void DrawButton(AppState* state, const Button& btn) {
    if (!btn.enabled) return;

    Color bgColor = Colors::Surface;
    if (btn.state == ButtonState::Hover) bgColor = Colors::SurfaceHover;
    if (btn.state == ButtonState::Pressed) bgColor = Colors::Accent;

    state->brushSurface->SetColor(bgColor.ToD2D());
    DrawRoundedRect(state->renderTarget, btn.bounds, state->brushSurface, 6.0f);

    state->brushText->SetColor(Colors::Text.ToD2D());
    state->renderTarget->DrawText(
        btn.text.c_str(),
        static_cast<UINT32>(btn.text.length()),
        state->textFormat,
        btn.bounds.ToD2D(),
        state->brushText,
        D2D1_DRAW_TEXT_OPTIONS_CLIP,
        DWRITE_MEASURING_MODE_NATURAL
    );
}

void DrawCustomTitleBar(AppState* state, float width) {
    Rect titleBar = {0, 0, width, 40};
    state->brushSurface->SetColor(Colors::Surface.ToD2D());
    state->renderTarget->FillRectangle(titleBar.ToD2D(), state->brushSurface);

    state->brushText->SetColor(Colors::Text.ToD2D());
    Rect titleRect = {16, 8, width - 200, 32};
    state->renderTarget->DrawText(
        L"dppbotcpp Студия",
        17,
        state->textFormatBold,
        titleRect.ToD2D(),
        state->brushText
    );

    float btnWidth = 46.0f;
    float btnY = 0;
    float btnHeight = 40.0f;

    Rect minimizeBtn = {width - btnWidth * 3, btnY, btnWidth, btnHeight};
    Rect maximizeBtn = {width - btnWidth * 2, btnY, btnWidth, btnHeight};
    Rect closeBtn = {width - btnWidth, btnY, btnWidth, btnHeight};

    state->brushText->SetColor(Colors::TextDim.ToD2D());
    state->renderTarget->DrawText(L"_", 1, state->textFormat, minimizeBtn.ToD2D(), state->brushText);
    state->renderTarget->DrawText(L"□", 1, state->textFormat, maximizeBtn.ToD2D(), state->brushText);

    state->brushText->SetColor(Colors::Error.ToD2D());
    state->renderTarget->DrawText(L"×", 1, state->textFormatBold, closeBtn.ToD2D(), state->brushText);
}

HRESULT OnRender(AppState* state) {
    HRESULT hr = CreateDeviceResources(state);

    if (SUCCEEDED(hr)) {
        state->renderTarget->BeginDraw();
        state->renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
        state->renderTarget->Clear(Colors::Background.ToD2D());

        RECT rc;
        GetClientRect(state->window, &rc);
        float width = static_cast<float>(rc.right - rc.left);
        float height = static_cast<float>(rc.bottom - rc.top);

        DrawCustomTitleBar(state, width);

        for (const auto& btn : state->buttons) {
            DrawButton(state, btn);
        }

        hr = state->renderTarget->EndDraw();

        if (hr == D2DERR_RECREATE_TARGET) {
            DiscardDeviceResources(state);
            hr = S_OK;
        }
    }

    return hr;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    AppState* state = reinterpret_cast<AppState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (message) {
    case WM_CREATE: {
        LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
        state = reinterpret_cast<AppState*>(pcs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
        return 0;
    }
    case WM_PAINT:
    case WM_DISPLAYCHANGE: {
        if (state) {
            OnRender(state);
            ValidateRect(hwnd, nullptr);
        }
        return 0;
    }
    case WM_SIZE: {
        if (state && state->renderTarget) {
            RECT rc;
            GetClientRect(hwnd, &rc);
            D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
            state->renderTarget->Resize(size);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }
    case WM_NCHITTEST: {
        LRESULT hit = DefWindowProcW(hwnd, message, wParam, lParam);
        if (hit == HTCLIENT) {
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ScreenToClient(hwnd, &pt);

            if (pt.y < 40) {
                RECT rc;
                GetClientRect(hwnd, &rc);
                float width = static_cast<float>(rc.right - rc.left);

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
            DiscardDeviceResources(state);
            SafeRelease(&state->textFormat);
            SafeRelease(&state->textFormatBold);
            SafeRelease(&state->textFormatSmall);
            SafeRelease(&state->writeFactory);
            SafeRelease(&state->d2dFactory);
            delete state;
        }
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

}  // namespace

}  // namespace dppbot

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    dppbot::AppState* state = new dppbot::AppState();

    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &state->d2dFactory);
    if (FAILED(hr)) return -1;

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&state->writeFactory));
    if (FAILED(hr)) return -1;

    state->writeFactory->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        14.0f,
        L"en-us",
        &state->textFormat
    );

    state->writeFactory->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_SEMI_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        16.0f,
        L"en-us",
        &state->textFormatBold
    );

    state->textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    state->textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    state->textFormatBold->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    state->textFormatBold->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    const wchar_t className[] = L"DppbotModernWindow";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = dppbot::WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = className;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&wc);

    state->window = CreateWindowExW(
        0,
        className,
        L"dppbotcpp Студия",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1400, 800,
        nullptr,
        nullptr,
        instance,
        state
    );

    if (!state->window) return -1;

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
