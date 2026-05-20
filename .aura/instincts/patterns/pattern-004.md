---
id: pattern-004
category: api
language: cpp
score: 50
tags: [api, cpp]
---

## 컨텍스트
파일: gui_modern.cpp (Write 완료)

## 핵심 코드
```cpp
#include "pack_library.hpp"

#include <windows.h>
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
```

## 태그
- api
- cpp