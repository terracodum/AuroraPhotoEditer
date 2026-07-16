# Aurora Photo Editor

Производительный фоторедактор для операционной системы **Aurora OS**, написанный на C++/Qt5 и QML (Sailfish Silica).

## Возможности

- Быстрый просмотр и редактирование фото прямо на устройстве.
- Undo/Redo стек изменений на стороне C++.
- Авто-улучшение фото (CLAHE + баланс белого).
- Масштабирование изображений (downscale/upscale).
- ML-инференс на базе ONNX Runtime для «умных» фильтров.
- Профилирование каждой операции обработки (препроцессинг / инференс / постпроцессинг) с JSON-логами.

## Архитектура

Проект строго разделён на два слоя:

1. **Frontend (QML + Sailfish Silica)** — только отрисовка интерфейса, без тяжёлой логики.
2. **Backend (C++ / Qt 5)** — вся бизнес-логика, обработка изображений и работа с файловой системой.

Ключевые решения:

- **Связь C++ ↔ QML** — через кастомный `QQuickImageProvider` (`PipelineImageProvider`), зарегистрированный по протоколу `image://pipeline/`. Пиксели передаются из C++ в QML напрямую, без лишних копий.
- **Многопоточность** — тяжёлые операции (экспорт, сохранение, ML-инференс) выполняются в фоне через `QThread` (паттерн `Worker`), UI-поток никогда не блокируется.
- **Управление памятью** — кэширование QML для динамических изображений отключено (`cache: false`) во избежание OOM при многократном наложении фильтров; история Undo/Redo хранится в `PipelineManager` на стороне C++.
- **Профилирование ML** — каждая команда обработки (`ImageEditorCommand`) фиксирует тайминги своих фаз через `MLProfiler`; сбор и вывод структурированных JSON-логов централизован в `BackgroundWorker`.

## Стек и зависимости

| Компонент | Назначение |
|---|---|
| Aurora OS SDK (Qt 5.6) | Целевая платформа |
| CMake + Ninja | Сборка |
| Conan | Управление C++ зависимостями |
| Qt 5 (Core, Network, Qml, Gui, Quick, Concurrent) | Frontend/backend фреймворк |
| OpenCV | Обработка изображений |
| ONNX Runtime | ML-инференс |

## Структура проекта

```
AuroraPhotoEditor/
├── src/                     # C++ backend
│   ├── main.cpp                     # точка входа, регистрация типов
│   ├── PipelineManager.*            # контроллер: Undo/Redo, загрузка/сохранение
│   ├── BackgroundWorker.*           # фоновое выполнение в QThread
│   ├── MLProfiler.*                 # сбор таймингов, JSON-логи
│   ├── MLInferenceEngine.*          # работа с ONNX-моделями
│   ├── EnhanceCommand.*             # авто-улучшение фото (CLAHE + WB)
│   ├── ImageEditorCommand.h         # интерфейс команд-фильтров
│   └── PipelineImageProvider.h      # провайдер image://pipeline/
├── qml/                     # UI frontend
│   ├── pages/MainPage.qml           # холст, undo/redo, панель инструментов
│   ├── pages/AboutPage.qml          # страница «О приложении»
│   ├── pages/NoticePage.qml         # системные уведомления
│   └── cover/DefaultCoverPage.qml   # обложка свёрнутого приложения
├── tests/                   # юнит-тесты (QtTest)
├── rpm/                     # конфигурация пакетирования (.spec)
├── translations/            # локализация (.ts)
└── conanfile.py             # зависимости Conan (OpenCV, ONNX Runtime)
```

## Сборка и запуск

1. Открыть проект в **Aurora IDE / SDK**.
2. Настроить таргеты сборки под нужную архитектуру (`armv7hl` / `aarch64` / `i486`).
3. Зависимости (OpenCV, ONNX Runtime) подтягиваются через Conan (`conanfile.py`) и pkg-config — при необходимости выполнить `conan install`.
4. Собрать и запустить (**Build & Run**) в Aurora IDE.

## Лицензия

BSD-3-Clause
