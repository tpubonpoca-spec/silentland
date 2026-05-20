# dppbotcpp

Подробный handoff-README для следующей нейросети / разработчика, который будет продолжать работу над программой.

Этот документ описывает не только то, что программа делает сейчас, но и текущее устройство кода, ограничения, реальные проверенные сценарии, спорные места архитектуры и список практических следующих шагов.

## Последние изменения (2026-05-20)

### ✅ Критические улучшения выполнены

1. **Исправлен merged manifest** - теперь генерируется синтетический manifest с полной информацией о merge операции:
   - Список всех source packs
   - Conflict summary (какие файлы были перезаписаны)
   - Union preview assets и replacement hints
   - Статистика по файлам

2. **Добавлена поддержка внешних VPK chunks** - убрано критическое ограничение:
   - Теперь читаются файлы `*_000.vpk`, `*_001.vpk` и т.д.
   - Работает с произвольными VPK архивами, не только с embedded data
   - Значительно расширена совместимость

3. **Configurable merge precedence** - пользователь теперь контролирует приоритет:
   - Кнопки "Move Up" / "Move Down" в GUI
   - Порядок паков в списке = приоритет merge
   - Явная и предсказуемая логика конфликтов

4. **Добавлен .gitignore** - чистый репозиторий без build артефактов

5. **JSON отчет при экспорте** - автоматически создается .json файл рядом с VPK:
   - Полная информация о merge операции
   - Список source packs с приоритетами
   - Статистика и конфликты
   - Machine-readable формат для автоматизации

6. **Multi-pack CLI команды** - добавлены команды для работы с несколькими паками:
   - `analyze` - анализ героя через несколько паков
   - `merge` - объединение контента героя из нескольких паков
   - `--packs` флаг для указания списка VPK файлов
   - `--json` флаг для machine-readable вывода

7. **Начат современный GUI** - базовая версия с Direct2D:
   - Кастомный черный минималистичный дизайн
   - Плавная графика через Direct2D
   - Подготовка к полноценному UI с вкладками и логами

### Следующие шаги

- Завершить современный GUI с вкладками и real-time логами
- Preview rendering в GUI
- Export configurator по категориям (models, materials, particles, etc.)
- Scan cache для ускорения UI

## 1. Что это за проект

`dppbotcpp` — это Windows desktop-программа на C++17 для анализа пользовательских Dota 2 VPK-паков с модами на героев, извлечения связанного содержимого выбранного героя и сборки нового отдельного `.vpk`.

Проект эволюционировал так:

1. Сначала был минимальный консольный парсер `*_dir.vpk`.
2. Потом появился экспорт героя в отдельный пак.
3. Потом появился собственный writer для упаковки результата обратно в `.vpk`.
4. Потом появился GUI на чистом Win32 API.
5. Потом GUI был расширен до multi-pack workflow: анализ нескольких паков, объединение контента выбранного героя, конфликт-репорт и merged export.

Сейчас проект ближе к "Dota 2 mod pack studio", чем к простой консольной утилите.

## 2. Главная цель проекта

Итоговая цель, к которой проект движется:

- анализировать один или несколько Dota 2 VPK-паков с модами;
- находить в них контент конкретного героя;
- максимально полно собирать все связанные ресурсы, чтобы при экспорте ничего не терялось;
- показывать пользователю, что именно будет заменено;
- показывать preview/media-ассеты, если они есть;
- собирать отдельный итоговый `.vpk` под выбранный мод/героя;
- в будущем уметь быть "конфигуратором сборки" для кастомного пака.

## 3. Что реально есть сейчас

### 3.1. Уже реализовано

- Чтение VPK version 1/2 directory-файлов (`*_dir.vpk`).
- Индексация файлов внутри VPK.
- Поиск героев по путям и alias-эвристикам.
- Построение `ScanSummary` для одного героя.
- Рекурсивное подтягивание зависимостей по ASCII-путям, найденным внутри бинарных ресурсов.
- Классификация ассетов по категориям:
  - `models`
  - `materials`
  - `particles`
  - `sounds`
  - `uiAssets`
  - `itemVisuals`
  - `previewAssets`
  - `replacementHints`
- Сборка нового single-file VPK через собственный `VpkWriter`.
- Анализ нескольких паков одновременно.
- Merge логика для одного героя across multiple packs.
- Конфликт-репорт по одинаковым путям.
- GUI на чистом Win32 API.
- CLI для технической проверки и пакетной отладки.
- Best-effort парсинг `ru.dota2changer.com` через WinHTTP.

### 3.2. Проверено вручную

Проверенный сценарий:

- входной пак: `C:\Users\PMC\Desktop\train\pak04_dir.vpk`
- герой: `shadow_fiend`
- результат: `shadow_fiend_dir.vpk`
- файл действительно создаётся
- наш же парсер затем читает этот выходной VPK обратно
- внутри есть игровые ресурсы и `manifest.json`
- минимум один preview asset подтверждён:
  - `panorama/videos/heroes/npc_dota_hero_nevermore.webm`

### 3.3. Что ещё не реализовано

- Нормальный визуальный preview прямо внутри GUI.
- Рендер "как это выглядит в игре" в 3D.
- Точная декомпиляция Valve compiled assets (`*_c`) в полноценный человекочитаемый вид.
- Поддержка внешних чанков вида `pak04_000.vpk`, `pak04_001.vpk` и так далее.
- Настоящий granular build configurator с чекбоксами по категориям ассетов.
- Действительно умный semantic merge с приоритетом по качеству, а не только по порядку.

## 4. Где лежат важные файлы

### 4.1. Исходный код

Основные source-файлы:

- `src/vpk_archive.cpp`
- `src/vpk_writer.cpp`
- `src/mod_analyzer.cpp`
- `src/pack_library.cpp`
- `src/gui_main.cpp`
- `src/main.cpp`
- `src/cli.cpp`
- `src/site_metadata.cpp`

Основные заголовки:

- `include/types.hpp`
- `include/vpk_archive.hpp`
- `include/vpk_writer.hpp`
- `include/mod_analyzer.hpp`
- `include/pack_library.hpp`
- `include/cli.hpp`
- `include/site_metadata.hpp`

### 4.2. Бинарники

В `release/` сейчас лежат несколько поколений бинарников.

Актуальные:

- `release/dppbotcpp_studio.exe`
  - текущая "главная" GUI-версия
- `release/dppbotcpp_cli.exe`
  - текущая CLI-версия

Устаревшие / промежуточные:

- `release/dppbotcpp.exe`
- `release/dppbotcpp_vpk.exe`

Если следующая нейросеть будет наводить порядок, можно позже очистить `release/` и оставить только актуальные бинарники, но сначала нужно убедиться, что пользователь не запускает старые файлы по привычке.

### 4.3. DLL для запуска

GUI/CLI собирались через MSYS2 MinGW UCRT toolchain, поэтому рядом с `.exe` нужны:

- `libstdc++-6.dll`
- `libgcc_s_seh-1.dll`
- `libwinpthread-1.dll`

Они уже скопированы в `release/`.

## 5. Как устроен проект архитектурно

### 5.1. `VpkArchive`

Файл:

- `src/vpk_archive.cpp`

Отвечает за:

- открытие `*_dir.vpk`
- чтение header
- разбор tree
- индексацию `path -> VpkEntry`
- чтение содержимого файла по `offset/length`

Важная деталь:

- Сейчас чтение поддерживает только записи с `archiveIndex == 0x7fff`, то есть данные должны лежать прямо в directory-файле.
- Если запись указывает на внешний архивный chunk, код бросает исключение.

Итог:

- Для пользовательских паков из `train` это часто работает.
- Для произвольных реальных VPK из других источников это может быть серьёзным ограничением.

### 5.2. `VpkWriter`

Файл:

- `src/vpk_writer.cpp`

Отвечает за:

- сборку нового VPK version 2
- запись tree
- запись file data section
- вычисление CRC32 для каждого entry

Особенности текущей реализации:

- Пишет single-file VPK, где все данные лежат внутри одного `*_dir.vpk`.
- `archiveMd5SectionSize = 0`
- `otherMd5SectionSize = 48`
- секция other md5 заполняется нулями
- signature section отсутствует

Это достаточно для нашего собственного чтения и базового hand-crafted VPK, но совместимость с любыми внешними инструментами и движком Dota 2 пока не гарантирована на 100%.

### 5.3. `ModAnalyzer`

Файл:

- `src/mod_analyzer.cpp`

Это ядро логики анализа героя.

Что он делает:

1. Нормализует имя героя.
2. Использует alias map:
   - `nevermore -> shadow_fiend`
   - `windrunner -> windranger`
   - `lanaya -> templar_assassin`
   - и так далее
3. Ищет стартовые файлы по совпадению с alias в путях.
4. Читает бинарные данные найденных файлов.
5. Пытается вытащить из ASCII-кусочков внутри бинаря ссылки на другие ассеты.
6. Если ссылочный путь найден во входном архиве, добавляет его в граф зависимостей.
7. Формирует `ScanSummary`.

Что ещё формируется в `ScanSummary`:

- `filesByRoot`
- `filesByExtension`
- `models`
- `materials`
- `particles`
- `sounds`
- `uiAssets`
- `itemVisuals`
- `previewAssets`
- `replacementHints`
- `notes`

### 5.4. `PackLibrary`

Файл:

- `src/pack_library.cpp`

Это слой orchestration поверх одного или нескольких паков.

Функции:

- `DiscoverVpkPacks(root)`
- `ScanPackHeroes(packPath)`
- `BuildLibrarySummary(root)`
- `AnalyzeHeroAcrossPacks(packs, hero)`
- `ExportMergedHeroPack(packs, hero, outputPath)`

`AnalyzeHeroAcrossPacks`:

- строит union-репорт по одному герою из нескольких паков
- агрегирует preview/replacement hints
- считает распределение по root/ext
- собирает конфликт-репорт

`ExportMergedHeroPack`:

- строит `ScanSummary` для каждого выбранного пака
- получает `BuildPackEntries(summary)`
- мержит entries по path
- более поздняя запись перезаписывает более раннюю
- пишет результат в новый `.vpk`

### 5.5. GUI

Файл:

- `src/gui_main.cpp`

GUI написан на raw Win32 API, без Qt, wxWidgets, WinUI и других UI-frameworks.

Сейчас это single-window приложение с тремя основными колонками:

1. `Pack Selection`
   - мультивыбор исходных паков
2. `Hero Profiles`
   - герои, которые встречаются в выбранных паках
3. `Analysis & Build Configurator`
   - output folder
   - final VPK filename
   - текстовый аналитический отчёт
   - activity log

Важно:

- GUI пока не рисует preview images/video.
- Он только показывает текстовый отчёт о найденных preview/media ассетах.
- UI уже гораздо лучше старого, но по сути всё ещё "raw Win32 form with smarter content", а не polished modern desktop design system.

### 5.6. CLI

Файлы:

- `src/main.cpp`
- `src/cli.cpp`

CLI нужна для:

- быстрой проверки анализатора
- экспорта одного героя из одного VPK
- автоматической/regression-проверки без GUI

Поддерживаемые команды:

- `scan`
- `extract`

Примеры:

```powershell
.\build\dppbotcpp_cli.exe scan --vpk C:\Users\PMC\Desktop\train\pak04_dir.vpk
.\build\dppbotcpp_cli.exe extract --vpk C:\Users\PMC\Desktop\train\pak04_dir.vpk --hero shadow_fiend --output C:\Users\PMC\Desktop\exports_vpk_test2
```

Ограничение CLI:

- Сейчас CLI не умеет multi-pack merge напрямую.
- Multi-pack merge доступен только через GUI backend.
- Если следующая нейросеть хочет довести проект, имеет смысл добавить отдельную CLI-команду вроде `merge`.

### 5.7. Site metadata

Файл:

- `src/site_metadata.cpp`

Реализован WinHTTP-запрос к:

- `https://ru.dota2changer.com/choose_heroes/`

Сейчас это best-effort:

- иногда сайт отвечает `403`
- HTML-парсинг очень хрупкий
- GUI эту функцию пока не использует
- CLI может использовать через `--site-metadata`

Это скорее вспомогательный эксперимент, а не надёжный продуктовый модуль.

## 6. Текущая модель данных

Главные структуры лежат в:

- `include/types.hpp`

Самые важные:

- `VpkEntry`
- `PreviewAsset`
- `ReplacementHint`
- `ScanSummary`
- `PackScanResult`
- `LibrarySummary`
- `MultiPackHeroReport`

Ключевая мысль:

- проект уже не про "список файлов", а про "семантический профиль геройского мода"

Это надо сохранять при дальнейшей разработке.

## 7. Как сейчас работает анализ героя

### 7.1. Источник правды

Основная логика основана на:

- пути файла внутри VPK
- alias-эвристиках
- ASCII-референсах, найденных внутри бинарных ресурсов

### 7.2. Что считается "связанным с героем"

Файл попадает в стартовый набор, если:

- в пути встречается alias героя
- путь выглядит как `hero_<alias>`
- путь лежит в ожидаемых hero/item namespace

### 7.3. Как подтягиваются зависимости

Для каждого уже отобранного файла:

- файл читается как бинарь
- из него вынимаются printable ASCII token sequences
- среди них ищутся строки, похожие на asset paths
- если такой путь существует в исходном VPK, он включается в export

Это умнее, чем просто path filter, но всё ещё эвристика.

### 7.4. Что анализатор умеет интерпретировать

Он умеет выделять:

- hero models
- materials
- particle effects
- sounds / soundevents
- panorama / resource UI assets
- item visuals
- preview media
- вероятные replacement targets

### 7.5. Что анализатор пока НЕ умеет

- полноценно понимать внутреннюю структуру Valve compiled resource format
- извлекать настоящий dependency graph на уровне resource serialization
- точно понимать semantic override rules Dota 2
- гарантировать 100% completeness export для любого произвольного пака

## 8. Как сейчас работает merged export

### 8.1. Общее поведение

Если пользователь выбрал несколько паков и одного героя:

1. Для каждого пака строится `ScanSummary`.
2. Для каждого `ScanSummary` строится список `VpkWriteEntry`.
3. Все `VpkWriteEntry` складываются в `unordered_map<string, VpkWriteEntry>`.
4. Если path повторяется, более поздний entry перезаписывает предыдущий.
5. Итоговая карта пишется в новый `.vpk`.

### 8.2. Реальный приоритет конфликтов

Очень важно:

- приоритет сейчас определяется не порядком кликов пользователя
- а порядком индексов выбранных элементов в listbox
- а listbox заполняется отсортированным списком файлов

То есть фактически конфликтный override сейчас определяется сортировкой путей паков, а не явным UI-приоритетом.

Это один из важных кандидатов на доработку.

### 8.3. Критичный баг текущего merge

`ExportMergedHeroPack()` сейчас мержит все entries, включая `manifest.json`.

Следствие:

- последний `manifest.json` перезаписывает предыдущие
- итоговый merged VPK содержит не настоящий merged manifest, а manifest от последнего обработанного source pack

Это надо исправить.

Правильный вариант на будущее:

- при merge генерировать отдельный synthetic merged manifest
- описывать там:
  - список source packs
  - merge precedence
  - conflict summary
  - union preview assets
  - union replacement hints

## 9. GUI workflow в текущей версии

### 9.1. Пользовательский сценарий

1. Пользователь запускает `release/dppbotcpp_studio.exe`.
2. По умолчанию в поле source folder подставлен:
   - `C:\Users\PMC\Desktop\train`
3. Пользователь нажимает `Load Library`.
4. Программа находит top-level `*_dir.vpk` в этой папке.
5. Все найденные паки автоматически выбираются.
6. Из выбранных паков строится union-список героев.
7. Пользователь выбирает героя.
8. Справа появляется текстовый отчёт:
   - сколько источников
   - какие root/ext категории
   - что, вероятно, заменяется
   - какие preview/media assets есть
   - какие конфликты есть между source packs
9. Пользователь указывает имя итогового `.vpk`.
10. Нажимает `Build Merged VPK`.

### 9.2. Что UI пока не умеет

- drag and drop паков
- thumbnail preview
- video preview
- отдельную таблицу конфликтов
- per-pack enable/disable categories
- настроить приоритет merge через UI
- показывать дифф "какой файл из какого пакета победил"

## 10. Build / toolchain / окружение

### 10.1. Сборка

Проект собирался в локальном окружении через:

- MSYS2 UCRT64
- `cmake`
- `ninja`
- `g++`

Фактически использовались:

- `C:\msys64\ucrt64\bin\cmake.exe`
- `C:\msys64\ucrt64\bin\g++.exe`
- `C:\msys64\ucrt64\bin\ninja.exe`

### 10.2. Команда сборки

```powershell
$env:Path='C:\msys64\ucrt64\bin;' + $env:Path
C:\msys64\ucrt64\bin\cmake.exe -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER='C:/msys64/ucrt64/bin/g++.exe'
C:\msys64\ucrt64\bin\cmake.exe --build build --config Release
```

### 10.3. Что собирается

Сейчас CMake собирает два target:

- `dppbotcpp`
  - GUI
- `dppbotcpp_cli`
  - CLI

### 10.4. Runtime caveat

Если собирать заново и переупаковывать `release/`, не забыть рядом с `.exe` положить:

- `libstdc++-6.dll`
- `libgcc_s_seh-1.dll`
- `libwinpthread-1.dll`

Иначе Windows покажет ошибку вида:

- `libgcc_s_seh-1.dll was not found`
- `libwinpthread-1.dll was not found`

## 11. Реальные данные для локальной проверки

В локальном окружении пользователя уже использовались:

- `C:\Users\PMC\Desktop\pak04_dir.vpk`
- `C:\Users\PMC\Desktop\train\`

В `train` лежит библиотека паков вроде:

- `pak01_dir.vpk`
- `pak02_dir.vpk`
- `pak03_dir.vpk`
- `pak04_dir.vpk`
- `pak05_dir.vpk`
- `pak06_dir.vpk`
- `pak08_dir.vpk`
- `pak09_dir.vpk`
- `pak010_dir.vpk`
- `pak011_dir.vpk`
- `pak016_dir.vpk`

GUI по умолчанию ориентируется именно на `C:\Users\PMC\Desktop\train`.

Если следующий разработчик будет работать в другом окружении, это hardcoded default стоит вынести в config или хотя бы сделать более нейтральным.

## 12. Известные ограничения и проблемы

Ниже список ограничений, которые важно понимать до любых серьёзных изменений.

### 12.1. Поддерживаются не все VPK

Главная проблема:

- нет поддержки внешних архивных частей `*_000.vpk`, `*_001.vpk`

Следствие:

- проект хорошо работает только с теми пакетами, где данные реально лежат внутри самого `*_dir.vpk`

### 12.2. Dependency analysis эвристический

Сейчас dependency graph строится по printable ASCII strings inside binary assets.

Плюсы:

- дёшево
- быстро
- уже полезно

Минусы:

- можно пропустить реальные зависимости
- можно взять лишние зависимости
- это не настоящий resource parser

### 12.3. Preview support пока "metadata-only"

Сейчас программа умеет:

- находить preview media по путям и расширениям
- писать про них в отчёт

Но пока не умеет:

- отображать их как картинки
- проигрывать `.webm`
- строить visual diff

### 12.4. GUI raw Win32

Это значит:

- быстро работает
- не имеет внешних зависимостей

Но:

- развивать UI тяжело
- layout код уже начинает разрастаться
- без собственного layout abstraction поддерживаемость будет ухудшаться

### 12.5. Нет автотестов

Сейчас нет:

- unit tests
- integration tests
- golden VPK tests

Проверка делается вручную через CLI и GUI.

### 12.6. `README` раньше был устаревшим

Этот документ создан именно потому, что прежний `README.md` сильно отстал от кода.

Если следующая нейросеть меняет архитектуру, обязательно нужно обновить `README`, иначе контекст опять потеряется.

## 13. Что точно стоит сделать дальше

Это список задач, которые реально принесут следующую заметную пользу.

### 13.1. Высший приоритет

1. ✅ **ВЫПОЛНЕНО (2026-05-20)**: Исправить merged manifest.
   - Теперь генерируется синтетический manifest с информацией о всех source packs
   - Включает conflict summary, union preview assets и replacement hints
   - Полная прозрачность merge операции
2. ✅ **ВЫПОЛНЕНО (2026-05-20)**: Добавить поддержку внешних VPK chunks.
   - Теперь поддерживаются файлы `*_000.vpk`, `*_001.vpk` и т.д.
   - Убрано критическое ограничение на работу только с embedded data
   - Совместимость с произвольными VPK архивами
3. ✅ **ВЫПОЛНЕНО (2026-05-20)**: Сделать configurable merge precedence.
   - Добавлены кнопки "Move Up" / "Move Down" в GUI
   - Порядок паков в списке теперь определяет приоритет merge
   - Явный и предсказуемый контроль над конфликтами
4. Добавить preview rendering в GUI.
5. Сделать export configurator по категориям:
   - models
   - materials
   - particles
   - sounds
   - UI
   - item visuals
   - preview assets

### 13.2. Средний приоритет

1. Сделать recursive pack discovery в папке library.
2. Сохранить scan cache, чтобы не пересканировать всё при каждом выборе.
3. Добавить multi-pack CLI-команду:
   - `merge`
   - `analyze`
4. Добавить красивую структуру detail panel:
   - табы
   - списки
   - filters
5. Показать per-path winner при конфликте.

### 13.3. Низкий, но полезный приоритет

1. Очистить `release/` от legacy exe.
2. Вынести GUI defaults в config.
3. Улучшить parser для `ru.dota2changer.com`.
4. Добавить экспорт JSON-отчёта рядом с VPK.

## 14. Как безопасно продолжать работу

### 14.1. Если меняешь backend анализа

После изменений обязательно проверить:

```powershell
.\build\dppbotcpp_cli.exe scan --vpk C:\Users\PMC\Desktop\train\pak04_dir.vpk
.\build\dppbotcpp_cli.exe extract --vpk C:\Users\PMC\Desktop\train\pak04_dir.vpk --hero shadow_fiend --output C:\Users\PMC\Desktop\exports_vpk_test
```

И затем проверить, что:

- итоговый `.vpk` создан
- он открывается `VpkArchive`
- количество entries выглядит разумно
- preview/media assets не исчезли

### 14.2. Если меняешь merge logic

Надо проверить:

- один pack selected
- несколько packs selected
- конфликтующие пути
- отсутствие конфликтов
- корректность synthetic manifest

### 14.3. Если меняешь GUI

Минимум проверить:

- `Load Library`
- `Open One VPK`
- multi-select в списке паков
- выбор героя
- обновление detail report
- export merged VPK

### 14.4. Если меняешь VPK writer

Обязательно перепроверить:

- наш собственный парсер может прочитать новый VPK
- offsets корректны
- CRC32 пишется
- directory tree корректна
- fileDataSectionSize корректен

## 15. Неочевидные нюансы

### 15.1. `DiscoverVpkPacks()` не рекурсивен

Сейчас просматривается только верхний уровень выбранной папки.

### 15.2. `ScanPackHeroes()` сканирует только список героев

Он не кеширует полный `ScanSummary`.

Если следующий разработчик хочет ускорить UI, надо сделать persistent in-memory cache:

- pack path
- archive index
- detected heroes
- full hero summaries by hero

### 15.3. GUI detail panel — plain text

Сейчас это удобно для быстрого handoff и отладки, но неудобно для конечного пользователя.

Почти наверняка в будущем стоит перейти на:

- list view
- tree view
- image/video preview controls
- отдельную секцию конфликтов

### 15.4. Current merge uses path dedup only

Если два пакета содержат разные semantic варианты, но по разным путям, они оба остаются в merged VPK.

Это может быть хорошо или плохо.

Нужен следующий уровень логики:

- semantic conflicts
- category-based pruning
- per-pack toggles

## 16. Что важно не сломать

Если следующая нейросеть начнёт крупный рефакторинг, крайне важно не потерять:

- возможность single-pack CLI export
- возможность multi-pack GUI export
- сборку в `.vpk`, а не возврат к папке
- поддержку hardcoded train workflow у пользователя
- release-папку с рабочими DLL

## 17. Рекомендованный roadmap для следующей нейросети

Если продолжать работу прагматично, я бы делал так:

### Фаза 1

- исправить merged manifest
- добавить pack selection priority UI
- добавить cached summaries

### Фаза 2

- добавить preview rendering
- добавить build configurator с чекбоксами по категориям
- показать "what replaces what" в отдельной таблице

### Фаза 3

- добавить поддержку external VPK chunks
- улучшить dependency extraction
- добавить export report JSON

### Фаза 4

- серьёзно переработать UI в более современный desktop UX
- возможно перейти с raw Win32 на более удобный UI layer, если это не противоречит требованиям пользователя

## 18. Короткая сводка для новой нейросети

Если читать только один раздел, пусть это будет этот.

### Проект уже умеет

- читать `*_dir.vpk`
- искать героев
- анализировать связанные ресурсы
- строить новый `.vpk`
- объединять несколько паков по одному герою
- показывать текстовый аналитический отчёт в GUI

### Главные технические ограничения

- нет поддержки external VPK chunks
- dependency analysis эвристический
- preview не рендерится
- merge precedence не настраивается
- merged manifest сейчас неправильный

### Главные файлы, с которых лучше начинать

- `src/mod_analyzer.cpp`
- `src/pack_library.cpp`
- `src/vpk_writer.cpp`
- `src/gui_main.cpp`
- `include/types.hpp`

### Главная текущая GUI-версия

- `release/dppbotcpp_studio.exe`

### Главная CLI-версия

- `release/dppbotcpp_cli.exe`

---

Если ты следующая нейросеть, лучший первый практический шаг после чтения этого README:

1. Собрать проект.
2. Прогнать CLI export на `pak04_dir.vpk`.
3. Запустить `dppbotcpp_studio.exe`.
4. Проверить multi-pack workflow на папке `train`.
5. Только потом трогать merge logic и GUI.
