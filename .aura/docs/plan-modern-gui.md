# План: Современный GUI с бездонно чёрным дизайном

## Цель
Создать полнофункциональный современный GUI с Direct2D, плавными анимациями и минималистичным чёрным дизайном, заменяющий текущий gui_modern.cpp.

## Проблемы текущей версии
- ❌ Два title bar (системный + кастомный)
- ❌ Пустой чёрный экран без контента
- ❌ Нет функциональных элементов
- ❌ Плохой шрифт
- ❌ Нет интеграции с pack_library

## Архитектура

### Файловая структура
```
src/
├── gui_modern.cpp          ← Полная переработка
├── gui_components.hpp      ← Новый: UI компоненты
├── gui_animations.hpp      ← Новый: Анимации
└── gui_state.hpp           ← Новый: Управление состоянием
```

### Компоненты (8 файлов)

1. **gui_modern.cpp** — Главный файл
   - WinMain entry point
   - Окно без системного title bar (WS_POPUP)
   - Direct2D инициализация
   - Главный цикл рендеринга

2. **gui_components.hpp** — UI компоненты
   - Button (с hover/press анимациями)
   - Card (elevation + shadow)
   - Input (focus glow)
   - List (виртуализация для больших списков)
   - Tab (плавное переключение)
   - ProgressBar
   - StatusBar

3. **gui_animations.hpp** — Анимация
   - AnimationController
   - Easing functions
   - Transition manager
   - Hover/Focus/Press эффекты

4. **gui_state.hpp** — Состояние приложения
   - PackLibrary интеграция
   - Выбранные паки
   - Текущий герой
   - Логи в реальном времени

5. **gui_layout.hpp** — Layout система
   - Flexbox-like система
   - Responsive grid
   - Auto-layout для списков

6. **gui_theme.hpp** — Тема (из design-system.md)
   - Все цвета
   - Spacing constants
   - Typography

7. **gui_rendering.hpp** — Рендеринг утилиты
   - DrawRoundedRect с тенью
   - DrawText с выравниванием
   - DrawGlow эффект
   - Gradient helpers

8. **gui_events.hpp** — Обработка событий
   - Mouse tracking
   - Keyboard navigation
   - Focus management

## Структура интерфейса

```
┌────────────────────────────────────────────────────┐
│ ⚫ dppbotcpp Студия              [_] [□] [×]       │ ← Custom title bar (40px)
├────────────────────────────────────────────────────┤
│  [Паки] [Анализ] [Логи]                           │ ← Tabs (48px)
├────────────────────────────────────────────────────┤
│                                                    │
│  ┌─────────────────┐  ┌──────────────────────┐   │
│  │ Библиотека      │  │ Герои                │   │
│  │                 │  │                      │   │
│  │ □ pack1.vpk     │  │ ○ shadow_fiend       │   │
│  │ □ pack2.vpk     │  │ ○ pudge              │   │
│  │ □ pack3.vpk     │  │ ○ invoker            │   │
│  │                 │  │                      │   │
│  │ [Загрузить]     │  │                      │   │
│  └─────────────────┘  └──────────────────────┘   │
│                                                    │
│  ┌──────────────────────────────────────────────┐ │
│  │ Анализ                                       │ │
│  │                                              │ │
│  │ Профиль героя: shadow_fiend                 │ │
│  │ Источники: 3 пака                           │ │
│  │ Файлов: 1,247                               │ │
│  │ Конфликтов: 12                              │ │
│  │                                              │ │
│  │ [Экспортировать VPK]                        │ │
│  └──────────────────────────────────────────────┘ │
│                                                    │
└────────────────────────────────────────────────────┘
│ ⚡ Готово | 3 пака загружено | shadow_fiend       │ ← Status bar (32px)
└────────────────────────────────────────────────────┘
```

## Этапы реализации

### Этап 1: Базовая структура (2 файла)
- [ ] gui_theme.hpp — Все константы из design-system.md
- [ ] gui_modern.cpp — Окно без системного title bar, Direct2D setup

### Этап 2: Компоненты (3 файла)
- [ ] gui_rendering.hpp — Утилиты рендеринга
- [ ] gui_components.hpp — Button, Card, List базовые
- [ ] gui_animations.hpp — Hover/Press анимации

### Этап 3: Layout (2 файла)
- [ ] gui_layout.hpp — Layout система
- [ ] gui_state.hpp — AppState с PackLibrary

### Этап 4: Интеграция (1 файл)
- [ ] gui_events.hpp — События + навигация
- [ ] Интеграция с pack_library.hpp
- [ ] Tabs: Паки / Анализ / Логи

### Этап 5: Полировка
- [ ] Плавные переходы между табами
- [ ] Логи в реальном времени
- [ ] Keyboard shortcuts (F12 для логов)
- [ ] Тестирование на разных разрешениях

## Технические детали

### Окно без системного title bar
```cpp
DWORD style = WS_POPUP | WS_VISIBLE;
DWORD exStyle = WS_EX_APPWINDOW | WS_EX_LAYERED;
SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
```

### Анимации через ID2D1RenderTarget
```cpp
// Используем GetTickCount64() для времени
// Интерполяция через easing functions
// Перерисовка через InvalidateRect + WM_PAINT
```

### Виртуализация списков
```cpp
// Рендерим только видимые элементы
// Scroll offset + viewport height
// Переиспользование буферов
```

## Зависимости
- Существующие: pack_library.hpp, vpk_archive.hpp, mod_analyzer.hpp
- Direct2D: d2d1.lib, dwrite.lib
- Без изменений в CMakeLists.txt (уже настроено)

## Критерии успеха
✅ Окно без двойного title bar
✅ Все функции из gui_main.cpp работают
✅ Плавные анимации (60 FPS)
✅ Бездонно чёрный дизайн
✅ Логи в реальном времени
✅ Keyboard navigation
✅ Размер exe ≤ 1 MB

## Риски
- Direct2D производительность на больших списках → виртуализация
- Сложность анимаций → начать с простых transitions
- Интеграция с существующим кодом → использовать те же структуры данных

## Следующий шаг
Начать с Этапа 1: создать gui_theme.hpp и переписать gui_modern.cpp с правильным окном.
