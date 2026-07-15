# Реализация дизайна «Neon Pop» — сводка

Дизайн-хендофф из этой папки встроен в `AuroraPhotoEditor` (Qt5/QML + Sailfish Silica,
C++ бэкенд на OpenCV + ONNX Runtime). Ниже — что сделано и что осталось мокнутым до
готовности соответствующих слоёв бэкенда (см. `architectural_plan_onnx.pdf`).

## Что сделано

### Дизайн-система
- [`NeonTheme.qml`](../AuroraPhotoEditor/qml/NeonTheme.qml) + [`qmldir`](../AuroraPhotoEditor/qml/qmldir) —
  singleton с токенами (цвета/шрифты/отступы/радиусы) из раздела README "Design Tokens".
  Масштабируется через `Theme.pixelRatio` относительно эталона 1.25 (устройство F+ T1100).
  Назван `NeonTheme`, а не `Theme`, чтобы не конфликтовать с `Sailfish.Silica.Theme`.
- [`qml/components/`](../AuroraPhotoEditor/qml/components/):
  - `Icon.qml` — свой набор line-иконок на Canvas (undo, reset, save, about, layers, spark,
    palette, brush, eraser, close, check, chevronDown, warning, back, plus, gallery/image)
    вместо переноса инлайн-SVG из HTML-прототипа.
  - `GradientButton`, `GlyphButton`, `ToolButton`, `SegmentedControl`, `ToastBanner`,
    `BusyOverlay`, `PageHeaderBar`.
  - `BottomSheet.qml` — обёртка над Silica `DockedPanel` (bottom sheet сделан на готовом
    Silica-паттерне, а не кастомным `Rectangle`, как и просили в требованиях).

### Экраны
- [`MainPage.qml`](../AuroraPhotoEditor/qml/pages/MainPage.qml) — полностью переверстан:
  кастомная шапка, пустое/загруженное состояние, докнутая панель инструментов, busy-оверлей
  со спиннером, toast-баннер, decorative pull-down affordance. Undo/Reset/Save/About работают
  как раньше; кнопка «Улучшение» теперь по-настоящему вызывает бэкенд (раньше это был
  `console.log`-стаб — баг, явно отмеченный в `functional-spec-for-design.md`, закрыт по-настоящему).
- `BackgroundSheet.qml`, `StyleSheet.qml`, `HistoryPanel.qml` — bottom sheets для инструментов
  «Фон», «Стиль» и истории изменений.
- `MaskEditPage.qml` — полноэкранная правка маски (кисть/ластик).
- `BatchPage.qml` — новый экран пакетной обработки.
- `AboutPage.qml`, `NoticePage.qml`, `DefaultCoverPage.qml` — перестилизованы под Neon Pop
  (у `NoticePage` теперь есть success/error-вариант с разным цветом иконки).

### C++ backend (реальные доработки, не моки)
- `PipelineManager::applyEnhance()` — впервые подключил кнопку «Улучшение» к уже готовому
  `EnhanceCommand` (CLAHE + Gray World). Раньше backend-логика существовала, но не была
  вызвана из QML вообще.
- `PipelineManager::historySteps` (`Q_PROPERTY QVariantList`) — панель истории показывает
  реальный стек команд (`m_commandStack`), тот же самый, которым уже пользуются Undo/Reset.
- `PipelineManager::loadFailed(reason)` — сигнал для уже существующей защиты от
  decompression bomb (>64 МП) и битых файлов; раньше при отказе ничего не показывалось
  пользователю, теперь UI кидает toast.
- `PipelineManager::operationCanceled()` — сигнал для существующей логики отмены операции
  при переключении инструмента во время `isProcessing`.
- `commandCount()` стал `Q_INVOKABLE`, чтобы QML вообще мог его вызывать (раньше был обычным
  C++-методом, невидимым для QML meta-object системы).

## Сводка того, что замокано

Все моки помечены в коде комментарием `// MOCK (ISSUE-N.N): ...` — легко найти через grep
и заменить одной функцией без переделки UI.

| Место | Что мокнуто | Точка интеграции |
|---|---|---|
| `BackgroundSheet.qml` | Все действия «Фон» (режим Удалить/Размыть/Заменить, пресеты, своё фото из галереи) — локальный таймер 550мс вместо реального ONNX-инференса сегментации | `applyPreset()` / `applyCustomBackground()`, `MOCK (ISSUE-4.1 / 4.3 / 6.1)` |
| `StyleSheet.qml` | Выбор и применение стиля — debounce 450мс реальный, но самого инференса нет; слайдер «Сила эффекта» уже не трогает busy (соответствует ISSUE-6.3) | `runMockInference()`, `MOCK (ISSUE-5.1 / 5.2 / 6.3)` |
| `MaskEditPage.qml` | Кисть/ластик рисуют полупрозрачные мазки на отдельном `Canvas` поверх фото — реальная альфа-маска в C++ не меняется | `onStrokePoint()`, `MOCK (ISSUE-6.2)`, ждёт `pipelineManager.modifyMask(x, y, radius, isEraser)` |
| `HistoryPanel.qml` | Кнопка «Сохранить как проект» — эмитит сигнал и показывает тост, но JSON/SQLite не пишет | `MOCK (ISSUE-6.4)` |
| `BatchPage.qml` | Вся страница: сетка — 8 плейсхолдер-тайлов вместо реальной галереи с мультивыбором (`Sailfish.Pickers`); прогресс — `Timer` на 260мс/фото вместо `BatchWorker` в `QThreadPool` | `runMockBatch()`, `MOCK (ISSUE-6.4)` |
| Шрифты | Unbounded/Manrope не забандлены (`.ttf`/`.otf` нет в репозитории) — используется системный шрифт по этим именам с фоллбэком | `TODO(assets)` в `NeonTheme.qml` |
| `AboutPage.qml` | Версия «0.1» захардкожена, не тянется из сборки | `TODO` в файле |

## Важно

Изменения **не прогнаны через реальную сборку** — в этом окружении недоступен Aurora
SDK/`sfdk`. Код тщательно сверен вручную по QML/Qt5 и Sailfish Silica API, но первым делом
после этой сессии стоит сделать `sfdk build` и посмотреть warnings, в частности:
- регистрацию `NeonTheme` как QML-singleton через `qmldir` (директорийный импорт `import "../"`);
- свойства Silica-компонентов (`Slider`, `BusyIndicatorSize`, `DockedPanel`), использованные
  по памяти без доступа к актуальной документации Aurora SDK.

Также не выполнялось обновление `.ts`-файлов переводов (`lupdate`) — новые `qsTr()`-строки
пока просто не переведены (fallback на исходный текст), это не блокирует сборку.
