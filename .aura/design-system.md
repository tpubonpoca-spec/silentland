# Design System — Бездонно Чёрный Минималистичный

## Цветовая палитра

### Основные цвета
```cpp
// Глубокий чёрный фон
constexpr Color DeepBlack = {0.02f, 0.02f, 0.02f, 1.0f};        // #050505
constexpr Color Surface = {0.04f, 0.04f, 0.04f, 1.0f};          // #0A0A0A
constexpr Color SurfaceElevated = {0.06f, 0.06f, 0.06f, 1.0f};  // #0F0F0F

// Текст
constexpr Color TextPrimary = {0.95f, 0.95f, 0.95f, 1.0f};      // #F2F2F2
constexpr Color TextSecondary = {0.6f, 0.6f, 0.6f, 1.0f};       // #999999
constexpr Color TextDim = {0.35f, 0.35f, 0.35f, 1.0f};          // #595959

// Акценты
constexpr Color AccentBlue = {0.3f, 0.7f, 1.0f, 1.0f};          // #4DB3FF
constexpr Color AccentBlueHover = {0.4f, 0.8f, 1.0f, 1.0f};     // #66CCFF
constexpr Color AccentBlueDim = {0.15f, 0.35f, 0.5f, 0.3f};     // #265980 30%

// Статусы
constexpr Color Success = {0.3f, 0.9f, 0.5f, 1.0f};             // #4DE680
constexpr Color Warning = {1.0f, 0.8f, 0.3f, 1.0f};             // #FFCC4D
constexpr Color Error = {1.0f, 0.4f, 0.4f, 1.0f};               // #FF6666

// Границы
constexpr Color BorderSubtle = {0.12f, 0.12f, 0.12f, 1.0f};     // #1F1F1F
constexpr Color BorderFocus = {0.3f, 0.7f, 1.0f, 0.5f};         // #4DB3FF 50%
```

## Типографика

### Шрифты
- **Основной**: Segoe UI (Windows), SF Pro (macOS fallback)
- **Моноширинный**: Consolas, Cascadia Code

### Размеры
```cpp
constexpr float FontSizeSmall = 11.0f;   // Метки, подсказки
constexpr float FontSizeBody = 13.0f;    // Основной текст
constexpr float FontSizeLarge = 15.0f;   // Заголовки секций
constexpr float FontSizeTitle = 18.0f;   // Заголовок окна
```

### Веса
- Regular (400) — основной текст
- SemiBold (600) — заголовки, акценты
- Bold (700) — критичные элементы

## Spacing System

```cpp
constexpr float Space2 = 2.0f;
constexpr float Space4 = 4.0f;
constexpr float Space8 = 8.0f;
constexpr float Space12 = 12.0f;
constexpr float Space16 = 16.0f;
constexpr float Space24 = 24.0f;
constexpr float Space32 = 32.0f;
constexpr float Space48 = 48.0f;
```

## Компоненты

### Кнопки
```cpp
// Размеры
constexpr float ButtonHeightSmall = 28.0f;
constexpr float ButtonHeightMedium = 36.0f;
constexpr float ButtonHeightLarge = 44.0f;
constexpr float ButtonRadius = 6.0f;

// Состояния
Normal:  Surface + BorderSubtle
Hover:   SurfaceElevated + AccentBlueDim glow
Pressed: AccentBlue + slight scale(0.98)
Disabled: Surface + TextDim (opacity 0.4)
```

### Карточки
```cpp
constexpr float CardRadius = 8.0f;
constexpr float CardPadding = 16.0f;
constexpr float CardElevation = 2.0f; // тень

Background: Surface
Border: BorderSubtle (1px)
Shadow: DeepBlack с opacity 0.3, blur 8px
```

### Поля ввода
```cpp
constexpr float InputHeight = 36.0f;
constexpr float InputRadius = 6.0f;
constexpr float InputPadding = 12.0f;

Normal:  Surface + BorderSubtle
Focus:   Surface + BorderFocus + AccentBlueDim glow
Error:   Surface + Error border
```

### Списки
```cpp
constexpr float ListItemHeight = 40.0f;
constexpr float ListItemPadding = 12.0f;

Normal:  transparent
Hover:   SurfaceElevated
Selected: AccentBlueDim + BorderFocus left accent (3px)
```

## Анимации

### Timing Functions
```cpp
// Easing curves
constexpr auto EaseOut = D2D1::BezierSegment(0.0f, 0.0f, 0.2f, 1.0f);
constexpr auto EaseInOut = D2D1::BezierSegment(0.4f, 0.0f, 0.2f, 1.0f);
constexpr auto EaseIn = D2D1::BezierSegment(0.4f, 0.0f, 1.0f, 1.0f);
```

### Длительности
```cpp
constexpr float DurationFast = 150.0f;    // ms - hover, focus
constexpr float DurationNormal = 250.0f;  // ms - transitions
constexpr float DurationSlow = 400.0f;    // ms - page transitions
```

### Эффекты
- **Hover**: opacity 0.0 → 1.0 (150ms ease-out)
- **Focus**: border + glow (250ms ease-out)
- **Press**: scale 1.0 → 0.98 (150ms ease-in-out)
- **Fade**: opacity transition (250ms ease-out)
- **Slide**: position + opacity (400ms ease-out)

## Layout

### Сетка
```cpp
constexpr float GridGutter = 16.0f;
constexpr float GridMargin = 24.0f;
constexpr int GridColumns = 12;
```

### Breakpoints
```cpp
constexpr int BreakpointSmall = 1024;
constexpr int BreakpointMedium = 1280;
constexpr int BreakpointLarge = 1600;
```

### Структура окна
```
┌─────────────────────────────────────┐
│ Custom Title Bar (40px)             │ ← DeepBlack
├─────────────────────────────────────┤
│                                     │
│  Content Area                       │ ← Surface
│  (padding: 24px)                    │
│                                     │
└─────────────────────────────────────┘
```

## Эффекты глубины

### Elevation Levels
```cpp
Level 0: flat (no shadow)
Level 1: 0 2px 4px rgba(0,0,0,0.2)   // cards
Level 2: 0 4px 8px rgba(0,0,0,0.3)   // elevated cards
Level 3: 0 8px 16px rgba(0,0,0,0.4)  // modals, dropdowns
```

### Glow эффекты
```cpp
// Accent glow (hover, focus)
Color: AccentBlue
Blur: 12px
Opacity: 0.3
Spread: 0px
```

## Accessibility

### Контрастность
- Текст на фоне: минимум 7:1 (AAA)
- Акценты на фоне: минимум 4.5:1 (AA)
- Границы: минимум 3:1

### Focus indicators
- Всегда видимые
- Цвет: BorderFocus
- Толщина: 2px
- Отступ: 2px от элемента

## Иконки

### Стиль
- Линейные (stroke-based)
- Толщина: 1.5px
- Размеры: 16px, 20px, 24px
- Цвет: TextSecondary (normal), TextPrimary (hover)

## Принципы

1. **Минимализм**: Только необходимые элементы
2. **Глубина**: Тонкие тени и elevation для иерархии
3. **Плавность**: Все переходы анимированы
4. **Контраст**: Высокий контраст текста для читаемости
5. **Консистентность**: Единая система spacing и sizing
6. **Фокус**: Чёткие focus states для навигации с клавиатуры
