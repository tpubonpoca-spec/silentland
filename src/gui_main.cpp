#include "pack_library.hpp"

#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <objbase.h>

#include <algorithm>
#include <filesystem>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace dppbot {

namespace {

constexpr int kIdSourceEdit = 101;
constexpr int kIdBrowseSource = 102;
constexpr int kIdLoadPacks = 103;
constexpr int kIdPackList = 104;
constexpr int kIdHeroList = 105;
constexpr int kIdOutputEdit = 106;
constexpr int kIdBrowseOutput = 107;
constexpr int kIdExport = 108;
constexpr int kIdLog = 109;
constexpr int kIdOpenFile = 110;
constexpr int kIdDetail = 111;
constexpr int kIdPackName = 112;
constexpr int kIdMoveUp = 113;
constexpr int kIdMoveDown = 114;

struct AppState {
    HWND window = nullptr;
    HWND sourceEdit = nullptr;
    HWND packList = nullptr;
    HWND heroList = nullptr;
    HWND outputEdit = nullptr;
    HWND packNameEdit = nullptr;
    HWND detailEdit = nullptr;
    HWND logEdit = nullptr;
    HFONT uiFont = nullptr;
    HFONT uiFontBold = nullptr;
    std::vector<std::filesystem::path> packs;
    std::vector<PackScanResult> packScans;
    std::vector<std::string> visibleHeroes;
    std::optional<MultiPackHeroReport> currentReport;
};

std::wstring ToWide(const std::string& value) {
    if (value.empty()) {
        return {};
    }
    const int size = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring result(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), result.data(), size);
    return result;
}

std::string ToUtf8(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }
    const int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    std::string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), result.data(), size, nullptr, nullptr);
    return result;
}

std::wstring GetWindowTextString(HWND control) {
    const int length = GetWindowTextLengthW(control);
    std::wstring value(length + 1, L'\0');
    GetWindowTextW(control, value.data(), length + 1);
    value.resize(length);
    return value;
}

void SetWindowTextString(HWND control, const std::wstring& value) {
    SetWindowTextW(control, value.c_str());
}

void AppendLog(AppState* state, const std::wstring& line) {
    const auto current = GetWindowTextString(state->logEdit);
    std::wstring combined = current;
    if (!combined.empty()) {
        combined += L"\r\n";
    }
    combined += line;
    SetWindowTextString(state->logEdit, combined);
}

void ClearListBox(HWND list) {
    SendMessageW(list, LB_RESETCONTENT, 0, 0);
}

int GetSelectedListIndex(HWND list) {
    return static_cast<int>(SendMessageW(list, LB_GETCURSEL, 0, 0));
}

std::vector<int> GetSelectedListIndexes(HWND list) {
    const int count = static_cast<int>(SendMessageW(list, LB_GETSELCOUNT, 0, 0));
    if (count <= 0) {
        const int single = GetSelectedListIndex(list);
        return single >= 0 ? std::vector<int>{single} : std::vector<int>{};
    }
    std::vector<int> items(count);
    SendMessageW(list, LB_GETSELITEMS, count, reinterpret_cast<LPARAM>(items.data()));
    return items;
}

std::filesystem::path PickFolder(HWND owner) {
    wchar_t buffer[MAX_PATH] = {};
    BROWSEINFOW browse = {};
    browse.hwndOwner = owner;
    browse.lpszTitle = L"Choose folder";
    browse.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
    PIDLIST_ABSOLUTE item = SHBrowseForFolderW(&browse);
    if (!item) {
        return {};
    }
    std::filesystem::path path;
    if (SHGetPathFromIDListW(item, buffer)) {
        path = buffer;
    }
    CoTaskMemFree(item);
    return path;
}

std::filesystem::path PickVpkFile(HWND owner) {
    wchar_t fileBuffer[MAX_PATH] = {};
    OPENFILENAMEW dialog = {};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = owner;
    dialog.lpstrFilter = L"VPK files\0*_dir.vpk\0All files\0*.*\0";
    dialog.lpstrFile = fileBuffer;
    dialog.nMaxFile = MAX_PATH;
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameW(&dialog)) {
        return {};
    }
    return fileBuffer;
}

void ApplyFont(AppState* state, HWND control, bool bold = false) {
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(bold ? state->uiFontBold : state->uiFont), TRUE);
}

std::wstring JoinPackNames(const std::vector<std::filesystem::path>& packs) {
    std::wstringstream out;
    for (std::size_t i = 0; i < packs.size(); ++i) {
        if (i != 0) {
            out << L", ";
        }
        out << packs[i].filename().wstring();
    }
    return out.str();
}

std::vector<std::filesystem::path> GetSelectedPackPaths(AppState* state) {
    std::vector<std::filesystem::path> result;
    for (int index : GetSelectedListIndexes(state->packList)) {
        if (index >= 0 && index < static_cast<int>(state->packs.size())) {
            result.push_back(state->packs[index]);
        }
    }
    return result;
}

std::wstring FormatTopCounts(const std::unordered_map<std::string, std::size_t>& counts, std::size_t limit) {
    std::vector<std::pair<std::string, std::size_t>> ranked(counts.begin(), counts.end());
    std::sort(ranked.begin(), ranked.end(), [](const auto& left, const auto& right) {
        if (left.second != right.second) {
            return left.second > right.second;
        }
        return left.first < right.first;
    });

    std::wstringstream out;
    const std::size_t count = std::min(limit, ranked.size());
    for (std::size_t i = 0; i < count; ++i) {
        if (i != 0) {
            out << L", ";
        }
        out << ToWide(ranked[i].first) << L":" << ranked[i].second;
    }
    return out.str();
}

std::wstring FormatReport(const MultiPackHeroReport& report) {
    std::wstringstream out;
    out << L"Hero Profile: " << ToWide(report.hero) << L"\r\n";
    out << L"Sources: " << report.sources.size() << L"\r\n";
    for (const auto& source : report.sources) {
        out << L"  - " << ToWide(source.sourcePackName)
            << L" | files=" << source.includedFiles.size()
            << L" | models=" << source.models.size()
            << L" | materials=" << source.materials.size()
            << L" | effects=" << source.particles.size()
            << L" | ui=" << source.uiAssets.size()
            << L"\r\n";
    }

    out << L"\r\nMerged export summary\r\n";
    out << L"  Total seed files: " << report.totalSeedFiles << L"\r\n";
    out << L"  Total included files: " << report.totalIncludedFiles << L"\r\n";
    out << L"  Unique merged files: " << report.mergedUniqueFiles << L"\r\n";
    out << L"  Root distribution: " << FormatTopCounts(report.filesByRoot, 8) << L"\r\n";
    out << L"  Extension distribution: " << FormatTopCounts(report.filesByExtension, 8) << L"\r\n";

    out << L"\r\nWhat this mod is likely replacing\r\n";
    const std::size_t replacementLimit = std::min<std::size_t>(report.replacementHints.size(), 12);
    for (std::size_t i = 0; i < replacementLimit; ++i) {
        const auto& hint = report.replacementHints[i];
        out << L"  - [" << ToWide(hint.category) << L"] " << ToWide(hint.targetPath) << L"\r\n";
        out << L"    " << ToWide(hint.note) << L"\r\n";
    }
    if (report.replacementHints.empty()) {
        out << L"  - No semantic replacement hints were recognized.\r\n";
    }

    out << L"\r\nPreview / media assets found\r\n";
    const std::size_t previewLimit = std::min<std::size_t>(report.previewAssets.size(), 12);
    for (std::size_t i = 0; i < previewLimit; ++i) {
        const auto& preview = report.previewAssets[i];
        out << L"  - [" << ToWide(preview.kind) << L"] " << ToWide(preview.path) << L"\r\n";
        out << L"    " << ToWide(preview.note) << L"\r\n";
    }
    if (report.previewAssets.empty()) {
        out << L"  - No direct PNG/JPG/WEBM preview media found. Many Dota assets stay compiled in *_c format.\r\n";
    }

    out << L"\r\nConflict report\r\n";
    if (report.conflicts.empty()) {
        out << L"  - No path conflicts across selected packs.\r\n";
    } else {
        const std::size_t conflictLimit = std::min<std::size_t>(report.conflicts.size(), 16);
        for (std::size_t i = 0; i < conflictLimit; ++i) {
            out << L"  - " << ToWide(report.conflicts[i]) << L"\r\n";
        }
        if (report.conflicts.size() > conflictLimit) {
            out << L"  ... and " << (report.conflicts.size() - conflictLimit) << L" more conflicts.\r\n";
        }
    }

    out << L"\r\nIn-game interpretation notes\r\n";
    out << L"  - Models/materials usually change hero body parts, arcanas, or wearable meshes.\r\n";
    out << L"  - Particles usually change spell casts, attacks, ambient glows, or impact visuals.\r\n";
    out << L"  - Panorama/resource assets usually affect icons, portraits, loadout previews, or UI slots.\r\n";
    out << L"  - Export keeps embedded preview media and UI assets when they are present in the source packs.\r\n";
    return out.str();
}

void SetDefaultPackName(AppState* state) {
    const int heroIndex = GetSelectedListIndex(state->heroList);
    if (heroIndex < 0 || heroIndex >= static_cast<int>(state->visibleHeroes.size())) {
        return;
    }
    const auto selectedPacks = GetSelectedPackPaths(state);
    if (selectedPacks.empty()) {
        return;
    }
    const std::string hero = state->visibleHeroes[heroIndex];
    std::wstringstream name;
    name << ToWide(hero) << L"_" << selectedPacks.size() << L"packs_dir.vpk";
    SetWindowTextString(state->packNameEdit, name.str());
}

void RebuildHeroUnion(AppState* state) {
    ClearListBox(state->heroList);
    SetWindowTextString(state->detailEdit, L"");
    state->currentReport.reset();
    state->visibleHeroes.clear();

    std::set<std::string> heroUnion;
    for (int index : GetSelectedListIndexes(state->packList)) {
        if (index < 0 || index >= static_cast<int>(state->packScans.size())) {
            continue;
        }
        if (!state->packScans[index].error.empty()) {
            continue;
        }
        heroUnion.insert(state->packScans[index].heroes.begin(), state->packScans[index].heroes.end());
    }

    for (const auto& hero : heroUnion) {
        state->visibleHeroes.push_back(hero);
        const std::wstring label = ToWide(hero);
        SendMessageW(state->heroList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
    }

    if (!state->visibleHeroes.empty()) {
        SendMessageW(state->heroList, LB_SETCURSEL, 0, 0);
        SetDefaultPackName(state);
    }
}

void LoadPackDirectory(AppState* state) {
    const auto root = std::filesystem::path(GetWindowTextString(state->sourceEdit));
    const LibrarySummary summary = BuildLibrarySummary(root);
    state->packs.clear();
    state->packScans = summary.packs;
    ClearListBox(state->packList);

    if (summary.packs.empty()) {
        AppendLog(state, L"No *_dir.vpk packs found in the selected folder.");
        return;
    }

    for (const auto& scan : summary.packs) {
        state->packs.push_back(scan.packPath);
        std::wstringstream label;
        label << scan.packPath.filename().wstring();
        if (!scan.error.empty()) {
            label << L"  |  scan error";
        } else {
            label << L"  |  " << scan.heroes.size() << L" heroes";
        }
        const std::wstring text = label.str();
        SendMessageW(state->packList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.c_str()));
    }

    for (int i = 0; i < static_cast<int>(state->packs.size()); ++i) {
        SendMessageW(state->packList, LB_SETSEL, TRUE, i);
    }

    RebuildHeroUnion(state);

    std::wstringstream message;
    message << L"Library loaded: " << state->packs.size() << L" packs from " << root.wstring();
    AppendLog(state, message.str());
    if (!summary.heroCoverage.empty()) {
        AppendLog(state, L"Top hero coverage: " + FormatTopCounts(std::unordered_map<std::string, std::size_t>(summary.heroCoverage.begin(), summary.heroCoverage.end()), 10));
    }
}

void LoadSinglePack(AppState* state, const std::filesystem::path& packPath) {
    state->packs = {packPath};
    state->packScans = {ScanPackHeroes(packPath)};
    ClearListBox(state->packList);
    const std::wstring label = packPath.filename().wstring() + L"  |  " + std::to_wstring(state->packScans[0].heroes.size()) + L" heroes";
    SendMessageW(state->packList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
    SendMessageW(state->packList, LB_SETSEL, TRUE, 0);
    RebuildHeroUnion(state);
    AppendLog(state, L"Loaded single pack: " + packPath.wstring());
}

void AnalyzeCurrentHeroSelection(AppState* state) {
    const int heroIndex = GetSelectedListIndex(state->heroList);
    if (heroIndex < 0 || heroIndex >= static_cast<int>(state->visibleHeroes.size())) {
        SetWindowTextString(state->detailEdit, L"Choose one or more packs, then choose a hero.");
        return;
    }

    const auto selectedPacks = GetSelectedPackPaths(state);
    if (selectedPacks.empty()) {
        SetWindowTextString(state->detailEdit, L"Choose at least one pack.");
        return;
    }

    try {
        state->currentReport = AnalyzeHeroAcrossPacks(selectedPacks, state->visibleHeroes[heroIndex]);
        SetWindowTextString(state->detailEdit, FormatReport(*state->currentReport));
        SetDefaultPackName(state);
        AppendLog(state, L"Analyzed hero " + ToWide(state->visibleHeroes[heroIndex]) + L" across " + std::to_wstring(selectedPacks.size()) + L" selected packs.");
    } catch (const std::exception& ex) {
        state->currentReport.reset();
        SetWindowTextString(state->detailEdit, ToWide(ex.what()));
        AppendLog(state, L"Analysis failed: " + ToWide(ex.what()));
    }
}

void MovePackUp(AppState* state) {
    const int index = GetSelectedListIndex(state->packList);
    if (index <= 0 || index >= static_cast<int>(state->packs.size())) {
        return;
    }

    std::swap(state->packs[index], state->packs[index - 1]);
    std::swap(state->packScans[index], state->packScans[index - 1]);

    ClearListBox(state->packList);
    for (const auto& pack : state->packs) {
        SendMessageW(state->packList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(pack.filename().wstring().c_str()));
    }
    SendMessageW(state->packList, LB_SETCURSEL, index - 1, 0);

    AppendLog(state, L"Moved pack up: " + state->packs[index - 1].filename().wstring());
}

void MovePackDown(AppState* state) {
    const int index = GetSelectedListIndex(state->packList);
    if (index < 0 || index >= static_cast<int>(state->packs.size()) - 1) {
        return;
    }

    std::swap(state->packs[index], state->packs[index + 1]);
    std::swap(state->packScans[index], state->packScans[index + 1]);

    ClearListBox(state->packList);
    for (const auto& pack : state->packs) {
        SendMessageW(state->packList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(pack.filename().wstring().c_str()));
    }
    SendMessageW(state->packList, LB_SETCURSEL, index + 1, 0);

    AppendLog(state, L"Moved pack down: " + state->packs[index + 1].filename().wstring());
}

void ExportCurrentSelection(AppState* state) {
    const int heroIndex = GetSelectedListIndex(state->heroList);
    if (heroIndex < 0 || heroIndex >= static_cast<int>(state->visibleHeroes.size())) {
        AppendLog(state, L"Choose a hero before export.");
        return;
    }
    const auto selectedPacks = GetSelectedPackPaths(state);
    if (selectedPacks.empty()) {
        AppendLog(state, L"Choose at least one source pack.");
        return;
    }

    const auto outputRoot = std::filesystem::path(GetWindowTextString(state->outputEdit));
    if (outputRoot.empty()) {
        AppendLog(state, L"Choose an output folder first.");
        return;
    }

    std::filesystem::path outputPath = outputRoot / GetWindowTextString(state->packNameEdit);
    if (outputPath.extension() != ".vpk") {
        outputPath += L".vpk";
    }

    try {
        const std::filesystem::path exported = ExportMergedHeroPack(selectedPacks, state->visibleHeroes[heroIndex], outputPath);
        AppendLog(state, L"Merged VPK exported: " + exported.wstring());
        MessageBoxW(state->window, L"Merged VPK export completed.", L"dppbotcpp", MB_OK | MB_ICONINFORMATION);
    } catch (const std::exception& ex) {
        AppendLog(state, L"Export failed: " + ToWide(ex.what()));
        MessageBoxW(state->window, ToWide(ex.what()).c_str(), L"dppbotcpp", MB_OK | MB_ICONERROR);
    }
}

void BuildUi(HWND hwnd, AppState* state) {
    state->uiFont = CreateFontW(-18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    state->uiFontBold = CreateFontW(-20, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Semibold");

    HWND header = CreateWindowW(L"STATIC", L"dppbotcpp Pack Studio", WS_VISIBLE | WS_CHILD, 20, 16, 400, 28, hwnd, nullptr, nullptr, nullptr);
    ApplyFont(state, header, true);
    HWND subtitle = CreateWindowW(L"STATIC", L"Multi-pack hero analysis, preview-aware merge, and VPK export for Dota 2 mods", WS_VISIBLE | WS_CHILD, 20, 44, 700, 22, hwnd, nullptr, nullptr, nullptr);
    ApplyFont(state, subtitle);

    HWND sourceGroup = CreateWindowW(L"BUTTON", L"Source Library", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 20, 78, 1240, 78, hwnd, nullptr, nullptr, nullptr);
    ApplyFont(state, sourceGroup, true);
    CreateWindowW(L"STATIC", L"Folder with packs", WS_VISIBLE | WS_CHILD, 36, 108, 120, 18, hwnd, nullptr, nullptr, nullptr);
    state->sourceEdit = CreateWindowW(L"EDIT", L"C:\\Users\\PMC\\Desktop\\train", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 36, 128, 720, 26, hwnd, reinterpret_cast<HMENU>(kIdSourceEdit), nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"Browse", WS_VISIBLE | WS_CHILD, 770, 128, 88, 26, hwnd, reinterpret_cast<HMENU>(kIdBrowseSource), nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"Open One VPK", WS_VISIBLE | WS_CHILD, 868, 128, 120, 26, hwnd, reinterpret_cast<HMENU>(kIdOpenFile), nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"Load Library", WS_VISIBLE | WS_CHILD, 998, 128, 120, 26, hwnd, reinterpret_cast<HMENU>(kIdLoadPacks), nullptr, nullptr);
    CreateWindowW(L"STATIC", L"Tip: select multiple packs to build a merged hero VPK. Later packs in the list override earlier ones on path conflicts.", WS_VISIBLE | WS_CHILD, 36, 102, 700, 18, hwnd, nullptr, nullptr, nullptr);

    HWND leftGroup = CreateWindowW(L"BUTTON", L"Pack Selection", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 20, 168, 330, 520, hwnd, nullptr, nullptr, nullptr);
    ApplyFont(state, leftGroup, true);
    CreateWindowW(L"STATIC", L"Multi-select source packs (order = merge priority)", WS_VISIBLE | WS_CHILD, 36, 198, 260, 18, hwnd, nullptr, nullptr, nullptr);
    state->packList = CreateWindowW(L"LISTBOX", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_EXTENDEDSEL, 36, 220, 230, 420, hwnd, reinterpret_cast<HMENU>(kIdPackList), nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"Move Up", WS_VISIBLE | WS_CHILD, 274, 220, 60, 28, hwnd, reinterpret_cast<HMENU>(kIdMoveUp), nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"Move Down", WS_VISIBLE | WS_CHILD, 274, 254, 60, 28, hwnd, reinterpret_cast<HMENU>(kIdMoveDown), nullptr, nullptr);
    CreateWindowW(L"STATIC", L"Later packs override earlier ones", WS_VISIBLE | WS_CHILD, 36, 646, 200, 18, hwnd, nullptr, nullptr, nullptr);

    HWND centerGroup = CreateWindowW(L"BUTTON", L"Hero Profiles", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 364, 168, 260, 520, hwnd, nullptr, nullptr, nullptr);
    ApplyFont(state, centerGroup, true);
    CreateWindowW(L"STATIC", L"Heroes available in selected packs", WS_VISIBLE | WS_CHILD, 380, 198, 180, 18, hwnd, nullptr, nullptr, nullptr);
    state->heroList = CreateWindowW(L"LISTBOX", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_NOTIFY, 380, 220, 228, 450, hwnd, reinterpret_cast<HMENU>(kIdHeroList), nullptr, nullptr);

    HWND rightGroup = CreateWindowW(L"BUTTON", L"Analysis & Build Configurator", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 638, 168, 622, 520, hwnd, nullptr, nullptr, nullptr);
    ApplyFont(state, rightGroup, true);
    CreateWindowW(L"STATIC", L"Output folder", WS_VISIBLE | WS_CHILD, 656, 198, 120, 18, hwnd, nullptr, nullptr, nullptr);
    state->outputEdit = CreateWindowW(L"EDIT", L"C:\\Users\\PMC\\Desktop\\exports", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 656, 218, 450, 26, hwnd, reinterpret_cast<HMENU>(kIdOutputEdit), nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"Browse", WS_VISIBLE | WS_CHILD, 1116, 218, 92, 26, hwnd, reinterpret_cast<HMENU>(kIdBrowseOutput), nullptr, nullptr);
    CreateWindowW(L"STATIC", L"Final VPK filename", WS_VISIBLE | WS_CHILD, 656, 252, 140, 18, hwnd, nullptr, nullptr, nullptr);
    state->packNameEdit = CreateWindowW(L"EDIT", L"hero_build_dir.vpk", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 656, 272, 450, 26, hwnd, reinterpret_cast<HMENU>(kIdPackName), nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"Build Merged VPK", WS_VISIBLE | WS_CHILD, 1116, 268, 120, 34, hwnd, reinterpret_cast<HMENU>(kIdExport), nullptr, nullptr);

    CreateWindowW(L"STATIC", L"Deep analysis of replacements, preview media, asset categories, and pack conflicts", WS_VISIBLE | WS_CHILD, 656, 312, 420, 18, hwnd, nullptr, nullptr, nullptr);
    state->detailEdit = CreateWindowW(L"EDIT", L"Choose packs and a hero to inspect the merged build profile.", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL, 656, 334, 580, 250, hwnd, reinterpret_cast<HMENU>(kIdDetail), nullptr, nullptr);
    CreateWindowW(L"STATIC", L"Activity log", WS_VISIBLE | WS_CHILD, 656, 592, 120, 18, hwnd, nullptr, nullptr, nullptr);
    state->logEdit = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL, 656, 612, 580, 58, hwnd, reinterpret_cast<HMENU>(kIdLog), nullptr, nullptr);

    for (HWND child : {state->sourceEdit, state->packList, state->heroList, state->outputEdit, state->packNameEdit, state->detailEdit, state->logEdit}) {
        ApplyFont(state, child);
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* state = reinterpret_cast<AppState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (message) {
    case WM_CREATE: {
        auto* createdState = new AppState();
        createdState->window = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createdState));
        BuildUi(hwnd, createdState);
        AppendLog(createdState, L"Ready. Load your train library or open a single pack.");
        return 0;
    }
    case WM_COMMAND: {
        if (!state) {
            break;
        }
        const int controlId = LOWORD(wParam);
        const int notifyCode = HIWORD(wParam);

        if (controlId == kIdBrowseSource && notifyCode == BN_CLICKED) {
            const auto path = PickFolder(hwnd);
            if (!path.empty()) {
                SetWindowTextString(state->sourceEdit, path.wstring());
            }
            return 0;
        }
        if (controlId == kIdBrowseOutput && notifyCode == BN_CLICKED) {
            const auto path = PickFolder(hwnd);
            if (!path.empty()) {
                SetWindowTextString(state->outputEdit, path.wstring());
            }
            return 0;
        }
        if (controlId == kIdOpenFile && notifyCode == BN_CLICKED) {
            const auto pack = PickVpkFile(hwnd);
            if (!pack.empty()) {
                LoadSinglePack(state, pack);
                AnalyzeCurrentHeroSelection(state);
            }
            return 0;
        }
        if (controlId == kIdLoadPacks && notifyCode == BN_CLICKED) {
            LoadPackDirectory(state);
            AnalyzeCurrentHeroSelection(state);
            return 0;
        }
        if (controlId == kIdPackList && notifyCode == LBN_SELCHANGE) {
            RebuildHeroUnion(state);
            AnalyzeCurrentHeroSelection(state);
            return 0;
        }
        if (controlId == kIdHeroList && notifyCode == LBN_SELCHANGE) {
            AnalyzeCurrentHeroSelection(state);
            return 0;
        }
        if (controlId == kIdExport && notifyCode == BN_CLICKED) {
            ExportCurrentSelection(state);
            return 0;
        }
        if (controlId == kIdMoveUp && notifyCode == BN_CLICKED) {
            MovePackUp(state);
            RebuildHeroUnion(state);
            AnalyzeCurrentHeroSelection(state);
            return 0;
        }
        if (controlId == kIdMoveDown && notifyCode == BN_CLICKED) {
            MovePackDown(state);
            RebuildHeroUnion(state);
            AnalyzeCurrentHeroSelection(state);
            return 0;
        }
        break;
    }
    case WM_DESTROY:
        if (state) {
            if (state->uiFont) {
                DeleteObject(state->uiFont);
            }
            if (state->uiFontBold) {
                DeleteObject(state->uiFontBold);
            }
            delete state;
        }
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

}  // namespace

}  // namespace dppbot

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    const wchar_t className[] = L"dppbotcpp_main_window";
    WNDCLASSW windowClass = {};
    windowClass.lpfnWndProc = dppbot::WindowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = className;
    windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassW(&windowClass);

    HWND window = CreateWindowExW(
        0,
        className,
        L"dppbotcpp Pack Studio",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1300,
        760,
        nullptr,
        nullptr,
        instance,
        nullptr);

    if (!window) {
        return 1;
    }

    ShowWindow(window, showCommand);
    UpdateWindow(window);

    MSG message = {};
    while (GetMessageW(&message, nullptr, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    CoUninitialize();
    return 0;
}
