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
#include <shlobj.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shell32.lib")

#include <memory>
#include <vector>
#include <string>
#include <optional>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

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
    std::optional<MultiPackHeroReport> currentReport;

    // Логи
    std::vector<std::wstring> logs;

    // Состояние
    int currentTab = 0;
    bool needsRedraw = true;
    POINT lastMousePos = {};

    void AddLog(const std::wstring& message) {
        logs.push_back(message);
        if (logs.size() > 100) {
            logs.erase(logs.begin());
        }
        needsRedraw = true;
    }
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
    result.resize(size - 1); // Remove null terminator
    return result;
}

std::wstring BrowseForFolder(HWND hwnd) {
    BROWSEINFOW bi = {};
    bi.hwndOwner = hwnd;
    bi.lpszTitle = L"Выберите папку с паками";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (!pidl) return {};

    wchar_t path[MAX_PATH];
    if (!SHGetPathFromIDListW(pidl, path)) {
        CoTaskMemFree(pidl);
        return {};
    }

    CoTaskMemFree(pidl);
    return path;
}

void LoadLibrary(AppState* state, const std::wstring& folderPath) {
    if (folderPath.empty()) return;

    state->AddLog(L"[INFO] Загрузка библиотеки из: " + folderPath);

    std::filesystem::path libPath = folderPath;

    // Найти все VPK паки в директории
    auto packs = DiscoverVpkPacks(libPath);

    state->AddLog(L"[INFO] Найдено VPK файлов: " + std::to_wstring(packs.size()));

    // Добавить паки в библиотеку
    state->library = PackLibrary();
    for (const auto& pack : packs) {
        state->library.AddPack(pack);
    }

    // Обновить список паков
    state->loadedPacks = packs;
    state->lists[0].items.clear();

    for (const auto& pack : packs) {
        ListItem item;
        item.text = ToWide(pack.filename().string());
        item.secondaryText = ToWide(pack.parent_path().filename().string());
        item.index = static_cast<int>(state->lists[0].items.size());
        state->lists[0].items.push_back(item);
    }

    state->AddLog(L"[SUCCESS] Загружено паков: " + std::to_wstring(state->loadedPacks.size()));

    // Обновить список героев
    auto heroesWithCount = state->library.GetAllHeroes();
    state->lists[1].items.clear();

    for (const auto& [hero, count] : heroesWithCount) {
        ListItem item;
        item.text = ToWide(hero);
        item.secondaryText = std::to_wstring(count) + L" паков";
        item.index = static_cast<int>(state->lists[1].items.size());
        state->lists[1].items.push_back(item);
    }

    state->AddLog(L"[INFO] Найдено героев: " + std::to_wstring(heroesWithCount.size()));

    state->needsRedraw = true;
}

void AnalyzeHero(AppState* state, const std::string& heroName) {
    if (heroName.empty() || state->loadedPacks.empty()) return;

    state->AddLog(L"[INFO] Анализ героя: " + ToWide(heroName));

    state->currentReport = state->library.AnalyzeHero(heroName);

    if (!state->currentReport.has_value()) {
        state->cards[0].content = {L"Ошибка анализа"};
        state->AddLog(L"[ERROR] Не удалось проанализировать героя");
        state->needsRedraw = true;
        return;
    }

    auto& report = state->currentReport.value();
    state->cards[0].content.clear();
    state->cards[0].content.push_back(L"Герой: " + ToWide(report.hero));
    state->cards[0].content.push_back(L"Исходных файлов: " + std::to_wstring(report.totalSeedFiles));
    state->cards[0].content.push_back(L"Включённых файлов: " + std::to_wstring(report.totalIncludedFiles));
    state->cards[0].content.push_back(L"Уникальных файлов: " + std::to_wstring(report.mergedUniqueFiles));
    state->cards[0].content.push_back(L"Конфликтов: " + std::to_wstring(report.conflicts.size()));

    state->AddLog(L"[SUCCESS] Анализ завершён. Конфликтов: " + std::to_wstring(report.conflicts.size()));

    if (!report.conflicts.empty()) {
        state->cards[0].content.push_back(L"");
        state->cards[0].content.push_back(L"Конфликты:");
        for (size_t i = 0; i < std::min(size_t(5), report.conflicts.size()); ++i) {
            state->cards[0].content.push_back(L"  " + ToWide(report.conflicts[i]));
        }
        if (report.conflicts.size() > 5) {
            state->cards[0].content.push_back(L"  ... и ещё " + std::to_wstring(report.conflicts.size() - 5));
        }
    }

    state->needsRedraw = true;
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
        std::wstring folder = BrowseForFolder(state->window);
        if (!folder.empty()) {
            LoadLibrary(state, folder);
        }
    };
    state->buttons.push_back(loadBtn);

    // Кнопка анализа
    Button analyzeBtn;
    analyzeBtn.id = "btn_analyze";
    analyzeBtn.x = loadBtn.x + loadBtn.width + spacing::Space16;
    analyzeBtn.y = loadBtn.y;
    analyzeBtn.width = 150;
    analyzeBtn.height = sizes::ButtonHeightMedium;
    analyzeBtn.text = L"Анализировать";
    analyzeBtn.onClick = [state]() {
        if (state->lists[1].selectedIndex >= 0 &&
            state->lists[1].selectedIndex < static_cast<int>(state->lists[1].items.size())) {
            std::string heroName = ToUtf8(state->lists[1].items[state->lists[1].selectedIndex].text);
            AnalyzeHero(state, heroName);
        }
    };
    state->buttons.push_back(analyzeBtn);

    // Кнопка экспорта
    Button exportBtn;
    exportBtn.id = "btn_export";
    exportBtn.x = analyzeBtn.x + analyzeBtn.width + spacing::Space16;
    exportBtn.y = loadBtn.y;
    exportBtn.width = 180;
    exportBtn.height = sizes::ButtonHeightMedium;
    exportBtn.text = L"Экспорт VPK";
    exportBtn.onClick = [state]() {
        // TODO: Диалог сохранения и экспорт
        state->needsRedraw = true;
    };
    state->buttons.push_back(exportBtn);

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
        if (index >= 0 && index < static_cast<int>(state->lists[1].items.size())) {
            std::string heroName = ToUtf8(state->lists[1].items[index].text);
            AnalyzeHero(state, heroName);
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

    // Фон title bar (полностью чёрный)
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

    // Minimize button
    brush->SetColor(colors::TextSecondary.ToD2D());
    DrawText(rt, state->fontBody, brush, L"─",
             width - btnWidth * 3, btnY, btnWidth, btnHeight,
             DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Maximize button
    DrawText(rt, state->fontBody, brush, L"□",
             width - btnWidth * 2, btnY, btnWidth, btnHeight,
             DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Close button
    brush->SetColor(colors::Error.ToD2D());
    DrawText(rt, state->fontTitle, brush, L"×",
             width - btnWidth, btnY, btnWidth, btnHeight,
             DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
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
        // Паки - основная вкладка
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
    else if (state->currentTab == 1) {
        // Анализ - детальная информация
        if (state->currentReport.has_value()) {
            auto& report = state->currentReport.value();

            float cardY = contentY + spacing::Space24;
            float cardWidth = width - spacing::Space48;

            // Карточка общей информации
            state->brush->SetColor(colors::SurfaceElevated.ToD2D());
            DrawRoundedRect(rt, state->brush, spacing::Space24, cardY, cardWidth, 150, sizes::CardRadius);

            state->brush->SetColor(colors::TextPrimary.ToD2D());
            DrawText(rt, state->fontTitle, state->brush, (L"Анализ: " + ToWide(report.hero)).c_str(),
                     spacing::Space24 + spacing::Space16, cardY + spacing::Space16,
                     cardWidth - spacing::Space32, 30,
                     DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

            state->brush->SetColor(colors::TextSecondary.ToD2D());
            float infoY = cardY + 60;
            DrawText(rt, state->fontBody, state->brush,
                     (L"Исходных файлов: " + std::to_wstring(report.totalSeedFiles)).c_str(),
                     spacing::Space24 + spacing::Space16, infoY, cardWidth - spacing::Space32, 20,
                     DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

            DrawText(rt, state->fontBody, state->brush,
                     (L"Включённых файлов: " + std::to_wstring(report.totalIncludedFiles)).c_str(),
                     spacing::Space24 + spacing::Space16, infoY + 25, cardWidth - spacing::Space32, 20,
                     DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

            DrawText(rt, state->fontBody, state->brush,
                     (L"Конфликтов: " + std::to_wstring(report.conflicts.size())).c_str(),
                     spacing::Space24 + spacing::Space16, infoY + 50, cardWidth - spacing::Space32, 20,
                     DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

            // Список конфликтов
            if (!report.conflicts.empty()) {
                float conflictsY = cardY + 180;
                state->brush->SetColor(colors::SurfaceElevated.ToD2D());
                DrawRoundedRect(rt, state->brush, spacing::Space24, conflictsY,
                               cardWidth, contentHeight - 210, sizes::CardRadius);

                state->brush->SetColor(colors::TextPrimary.ToD2D());
                DrawText(rt, state->fontTitle, state->brush, L"Конфликты",
                         spacing::Space24 + spacing::Space16, conflictsY + spacing::Space16,
                         cardWidth - spacing::Space32, 30,
                         DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

                state->brush->SetColor(colors::TextSecondary.ToD2D());
                float listY = conflictsY + 60;
                for (size_t i = 0; i < std::min(size_t(15), report.conflicts.size()); ++i) {
                    DrawText(rt, state->fontSmall, state->brush, ToWide(report.conflicts[i]).c_str(),
                             spacing::Space24 + spacing::Space24, listY + i * 22,
                             cardWidth - spacing::Space48, 20,
                             DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
                }
            }
        } else {
            state->brush->SetColor(colors::TextDim.ToD2D());
            DrawText(rt, state->fontBody, state->brush, L"Выберите героя для анализа",
                     0, contentY + contentHeight / 2 - 10, width, 20,
                     DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }
    else if (state->currentTab == 2) {
        // Логи
        float logY = contentY + spacing::Space24;
        float logWidth = width - spacing::Space48;
        float logHeight = contentHeight - spacing::Space48;

        state->brush->SetColor(colors::SurfaceElevated.ToD2D());
        DrawRoundedRect(rt, state->brush, spacing::Space24, logY, logWidth, logHeight, sizes::CardRadius);

        state->brush->SetColor(colors::TextPrimary.ToD2D());
        DrawText(rt, state->fontTitle, state->brush, L"Логи операций",
                 spacing::Space24 + spacing::Space16, logY + spacing::Space16,
                 logWidth - spacing::Space32, 30,
                 DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

        // Отображение логов
        float lineY = logY + 60;
        int startIdx = std::max(0, static_cast<int>(state->logs.size()) - 20);
        for (int i = startIdx; i < static_cast<int>(state->logs.size()); ++i) {
            Color logColor = colors::TextSecondary;
            if (state->logs[i].find(L"[ERROR]") != std::wstring::npos) {
                logColor = colors::Error;
            } else if (state->logs[i].find(L"[SUCCESS]") != std::wstring::npos) {
                logColor = colors::Success;
            } else if (state->logs[i].find(L"[INFO]") != std::wstring::npos) {
                logColor = colors::TextSecondary;
            }

            state->brush->SetColor(logColor.ToD2D());
            DrawText(rt, state->fontSmall, state->brush, state->logs[i].c_str(),
                     spacing::Space24 + spacing::Space16, lineY,
                     logWidth - spacing::Space32, 18,
                     DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
            lineY += 22;
        }

        if (state->logs.empty()) {
            state->brush->SetColor(colors::TextDim.ToD2D());
            DrawText(rt, state->fontBody, state->brush, L"Логи пусты",
                     spacing::Space24 + spacing::Space16, logY + 60,
                     logWidth - spacing::Space32, 20,
                     DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
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

    // WS_POPUP убирает системный title bar, убираем WS_THICKFRAME для фиксированного размера
    state->window = CreateWindowExW(
        WS_EX_APPWINDOW,
        className,
        L"dppbotcpp Студия",
        WS_POPUP | WS_VISIBLE | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1400, 800,
        nullptr, nullptr, instance, state
    );

    if (!state->window) return -1;

    // Убираем белую полосу - отключаем DWM рамку полностью
    MARGINS margins = {0, 0, 0, 0};
    DwmExtendFrameIntoClientArea(state->window, &margins);

    // Устанавливаем чёрный фон для некlient области
    BOOL dark = TRUE;
    DwmSetWindowAttribute(state->window, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

    // Начальные логи
    state->AddLog(L"[INFO] dppbotcpp Студия запущена");
    state->AddLog(L"[INFO] Версия: 1.0.0");
    state->AddLog(L"[INFO] Готово к работе");

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
