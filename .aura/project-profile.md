# Project Profile — dppbotcpp Studio

## Основная информация
- **Название**: dppbotcpp Studio (silentland)
- **Тип**: Desktop GUI Application
- **Язык**: C++17
- **Платформа**: Windows (Win32 API + Direct2D)
- **Назначение**: VPK Mod Pack Studio для Dota 2

## Технологический стек
- **Язык**: C++17
- **GUI**: Win32 API, Direct2D, DirectWrite
- **Графика**: Direct2D для кастомного рендеринга
- **Сборка**: CMake + Ninja (MSYS2 UCRT64)
- **Компилятор**: MinGW-w64 GCC

## Архитектура
- **Паттерн**: Custom GUI с Direct2D рендерингом
- **Структура**:
  - `src/gui_modern.cpp` — современный GUI с Direct2D (в разработке)
  - `src/gui_main.cpp` — основной GUI с Win32 контролами (рабочий)
  - `src/main.cpp` — CLI версия
  - `include/` — заголовочные файлы
  - `build/` — скомпилированные бинарники
  - `release/` — готовые к распространению файлы

## Текущее состояние
- ✅ Основной GUI полностью функционален (русская локализация)
- ⚠️ Современный GUI (gui_modern.cpp) — базовая структура, требует доработки
- ✅ CLI версия работает
- ✅ Все runtime DLL включены

## Цели дизайна
- **Стиль**: Бездонно чёрный минималистичный
- **Анимации**: Плавные переходы
- **Кастомизация**: Полностью свой дизайн без стандартных Windows элементов
- **Цветовая схема**: Глубокий чёрный фон, тонкие акценты

## Зависимости
- libstdc++-6.dll
- libgcc_s_seh-1.dll
- libwinpthread-1.dll
- d2d1.lib (Direct2D)
- dwrite.lib (DirectWrite)
- windowscodecs.lib

## Команды сборки
```bash
export PATH="/c/msys64/ucrt64/bin:$PATH"
cd build
ninja
```

## Git
- Репозиторий: https://github.com/tpubonpoca-spec/silentland
- Ветка: main
