# README.md

## 🇷🇺 Русская версия

### 1. Общее описание проекта

Данный проект представляет собой десктопное приложение на базе Qt для приема, парсинга и визуализации телеметрических данных, поступающих по последовательному порту (UART) от микроконтроллера (например, Arduino или совместимых устройств).

Основные функции:

* Автоматическое обнаружение доступных serial-портов
* Подключение к выбранному устройству
* Потоковое чтение бинарных данных
* Валидация пакетов (сигнатура, версия, CRC)
* Декодирование структуры телеметрии
* Отображение данных в реальном времени (графики)

---

### 2. Архитектура проекта

Проект разделен на три основных слоя:

#### 2.1 DataSource (Источник данных)

Файлы:

* `serialreader.h / .cpp`
* `telemetry_protocol.h`

Ответственность:

* Работа с последовательным портом
* Буферизация входящих данных
* Парсинг телеметрических пакетов
* Проверка CRC
* Генерация сигналов с готовыми данными

---

#### 2.2 UI (Пользовательский интерфейс)

Файлы:

* `main_window.h / .cpp`
* `telemetry_chart_widget.h / .cpp`

Ответственность:

* Отображение графиков
* Выбор COM-порта
* Обработка входящих пакетов
* Связь с SerialReader

---

#### 2.3 Entry Point

Файл:

* `main.cpp`

Ответственность:

* Инициализация Qt-приложения
* Запуск главного окна

---

### 3. Протокол телеметрии

Определен в `telemetry_protocol.h`.

#### 3.1 Структура пакета

| Поле    | Размер  | Описание                         |
| ------- | ------- | -------------------------------- |
| SOF1    | 1 байт  | 0xAA — старт кадра               |
| SOF2    | 1 байт  | 0x55 — второй байт синхронизации |
| Version | 1 байт  | Версия протокола                 |
| Type    | 1 байт  | Тип пакета                       |
| Length  | 2 байта | Длина payload (little-endian)    |
| Payload | N байт  | Данные (структура DataPacket)    |
| CRC     | 2 байта | CRC16 CCITT                      |

---

#### 3.2 Структура DataPacket

Структура упакована (`#pragma pack(1)`), что гарантирует отсутствие padding.

Содержит:

* Время
* Ускорения (ax, ay, az)
* Скорости (vx, vy, vz)
* Позиции (sx, sy, sz)
* Магнитометр (mx, my, mz)
* Гироскоп (gx, gy, gz)
* Углы (angx, angy, angz)
* Давление, температура, высота
* Напряжение и прочее

Важно:

* Все поля типа `float`
* Порядок байтов — little-endian

---

#### 3.3 CRC

Используется алгоритм:

* CRC16-CCITT
* Полином: `0x1021`
* Начальное значение: `0xFFFF`

CRC считается только по payload.

---

### 4. SerialReader

Класс: `SerialReader`

#### Основные функции:

##### open()

Настраивает и открывает последовательный порт:

* Baudrate: 115200
* 8N1
* Без контроля потока

##### onReadyRead()

Вызывается при поступлении данных:

* Считывает данные
* Добавляет в буфер
* Запускает парсинг

##### tryParse()

Ключевая логика:

* Поиск сигнатуры пакета
* Проверка длины
* Проверка CRC
* Декодирование структуры
* Генерация сигнала `packetReady`

Особенности:

* Устойчив к "мусорным" данным
* Автоматическая ресинхронизация

---

### 5. Обнаружение Arduino

Функции:

* `isArduinoLike()`
* `getArduinoPorts()`

Критерии:

* Vendor ID:

  * 0x2341 (Arduino)
  * 0x1A86 (CH340)
  * 0x10C4 (CP210x)
* Проверка строки описания и производителя

---

### 6. MainWindow

Класс: `MainWindow`

#### Функции:

##### Инициализация UI

* Создаются 3 панели
* Добавляются графики:

  * Acceleration X
  * Acceleration Y
  * Acceleration Z

##### Выбор порта

* Отображается список доступных устройств
* Пользователь выбирает порт
* Открывается соединение

##### onPacket()

* Получает DataPacket
* Извлекает данные
* Обновляет графики

---

### 7. TelemetryChartWidget

Назначение:

* Отображение временных рядов

#### Возможности:

* Скользящее окно по времени
* Ограничение количества точек
* Автоматическое масштабирование оси X

#### Основные методы:

* `appendPoint()` — добавить точку
* `clear()` — очистка
* `setYAxisRange()` — диапазон Y
* `setTimeWindow()` — окно времени
* `setPointLimit()` — лимит точек

---

### 8. Поток данных

1. MCU отправляет бинарный пакет
2. SerialReader читает данные
3. Данные буферизуются
4. tryParse извлекает валидные пакеты
5. Генерируется сигнал `packetReady`
6. MainWindow принимает пакет
7. Данные отображаются на графике

---

### 9. Ограничения текущей версии

* Нет записи данных в файл
* Нет обработки ошибок на уровне UI
* Нет реконнекта
* Нет поддержки разных типов пакетов
* Нет конфигурации через UI

---

### 10. Возможные улучшения

* Добавить логирование в файл
* Добавить настройки пользователя
* Поддержка нескольких устройств
* Добавить фильтрацию данных
* Реализовать replay телеметрии

---

---

## 🇬🇧 English Version

### 1. Project Overview

This project is a Qt-based desktop application for receiving, parsing, and visualizing telemetry data over a serial interface (UART) from a microcontroller (e.g., Arduino-compatible devices).

Core functionality:

* Automatic detection of serial ports
* Device selection
* Streaming binary data reading
* Packet validation (header, version, CRC)
* Telemetry decoding
* Real-time plotting

---

### 2. Architecture

The project consists of three main layers:

#### 2.1 Data Source

Files:

* `serialreader.*`
* `telemetry_protocol.h`

Responsibilities:

* Serial communication
* Buffer management
* Packet parsing
* CRC validation
* Emitting parsed data

---

#### 2.2 UI Layer

Files:

* `main_window.*`
* `telemetry_chart_widget.*`

Responsibilities:

* Visualization
* User interaction
* Port selection
* Data display

---

#### 2.3 Entry Point

File:

* `main.cpp`

Responsibilities:

* Application initialization
* Launching main window

---

### 3. Telemetry Protocol

Defined in `telemetry_protocol.h`.

#### 3.1 Packet Structure

| Field   | Size | Description      |
| ------- | ---- | ---------------- |
| SOF1    | 1B   | 0xAA start byte  |
| SOF2    | 1B   | 0x55 sync byte   |
| Version | 1B   | Protocol version |
| Type    | 1B   | Packet type      |
| Length  | 2B   | Payload size     |
| Payload | N    | Data             |
| CRC     | 2B   | CRC16            |

---

#### 3.2 DataPacket

Packed struct (no padding).

Contains:

* Time
* Acceleration
* Velocity
* Position
* Magnetometer
* Gyroscope
* Orientation
* Pressure, temperature, altitude
* Voltage, etc.

All fields are `float`, little-endian.

---

#### 3.3 CRC

Algorithm:

* CRC16-CCITT
* Polynomial: `0x1021`
* Initial value: `0xFFFF`

Applied only to payload.

---

### 4. SerialReader

Handles:

* Port configuration
* Data reception
* Packet parsing

Key features:

* Robust resynchronization
* Incremental parsing
* Signal emission (`packetReady`)

---

### 5. Arduino Detection

Based on:

* Vendor IDs (Arduino, CH340, CP210x)
* Description string matching

---

### 6. MainWindow

Responsibilities:

* UI layout
* Port selection dialog
* Handling incoming packets
* Updating charts

---

### 7. TelemetryChartWidget

Features:

* Real-time plotting
* Sliding time window
* Point limit
* Axis auto-update

---

### 8. Data Flow

1. MCU sends binary frame
2. SerialReader receives data
3. Buffer accumulates bytes
4. Parser extracts valid frames
5. Signal emitted
6. UI receives data
7. Charts updated

---

### 9. Current Limitations

* No data persistence
* No reconnection logic
* No multi-device support
* No UI configuration
* Only one packet type supported

---

### 10. Future Improvements

* Logging to file
* Configurable UI
* Multi-device support
* Data filtering
* Telemetry replay system

---
