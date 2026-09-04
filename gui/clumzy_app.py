"""Clumzy Qt UI — ZubCut charcoal/teal widgets, clumsy WinDivert engine behind them."""
from __future__ import annotations

import configparser
import ctypes
import os
import sys
import threading
from ctypes import c_char_p, c_float, c_int

from PyQt5.QtCore import QEvent, Qt, QThread, QTimer, pyqtSignal
from PyQt5.QtGui import QColor, QFont, QIcon, QPainter, QPalette, QPixmap
from PyQt5.QtWidgets import (
    QApplication,
    QCheckBox,
    QComboBox,
    QDoubleSpinBox,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QMainWindow,
    QMessageBox,
    QPushButton,
    QSpinBox,
    QVBoxLayout,
    QWidget,
)

UI_BG = '#141414'
UI_BTN = '#2b2b2b'
UI_HOVER = '#383838'
UI_PRESS = '#323232'
UI_BORDER = '#3d3d3d'
UI_TEXT = '#e8eaed'
UI_MUTE = '#9a9a9a'
UI_TEAL = '#316E69'
UI_SAGE = '#5D706E'
UI_SEL = '#f2f2f2'

ARROW_UP = (
    'data:image/svg+xml;base64,'
    'PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSI5IiBoZWlnaHQ9IjYi'
    'PjxwYXRoIGZpbGw9IiM5YTlhOWEiIGQ9Ik0wIDYgTDQuNSAwIEw5IDYgWiIvPjwvc3ZnPg=='
)
ARROW_DOWN = (
    'data:image/svg+xml;base64,'
    'PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSI5IiBoZWlnaHQ9IjYi'
    'PjxwYXRoIGZpbGw9IiM5YTlhOWEiIGQ9Ik0wIDAgTDQuNSA2IEw5IDAgWiIvPjwvc3ZnPg=='
)

NOTICE = (
    "NOTICE: When capturing localhost (loopback) packets, you CAN'T include inbound "
    "criteria.\nFilters like 'udp' need to be 'udp and outbound' to work."
)


def app_dir() -> str:
    if getattr(sys, 'frozen', False):
        return os.path.dirname(sys.executable)
    return os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))


def resource_path(*parts: str) -> str:
    here = app_dir()
    candidates = [
        os.path.join(here, *parts),
        os.path.join(here, 'etc', *parts),
        os.path.join(getattr(sys, '_MEIPASS', here), *parts),
        os.path.join(getattr(sys, '_MEIPASS', here), 'etc', *parts),
    ]
    for path in candidates:
        if os.path.isfile(path):
            return path
    return candidates[0]


ICON_SIZES = (16, 20, 24, 32, 40, 48, 64, 256)


def set_win_app_id() -> None:
    try:
        ctypes.windll.shell32.SetCurrentProcessExplicitAppUserModelID('zubcats.Clumzy')
    except Exception:
        pass


def load_app_icon() -> QIcon:
    icon = QIcon()
    ico_path = resource_path('clumzy-icon.ico')
    if os.path.isfile(ico_path):
        icon.addFile(ico_path)
    png_path = resource_path('clumzy-logo.png')
    pix = QPixmap(png_path) if os.path.isfile(png_path) else QPixmap()
    if pix.isNull():
        return icon
    for size in ICON_SIZES:
        canvas = QPixmap(size, size)
        canvas.fill(QColor('#141414'))
        fitted = pix.scaled(size, size, Qt.KeepAspectRatio, Qt.SmoothTransformation)
        painter = QPainter(canvas)
        painter.setRenderHint(QPainter.SmoothPixmapTransform, True)
        painter.drawPixmap((size - fitted.width()) // 2, (size - fitted.height()) // 2, fitted)
        painter.end()
        icon.addPixmap(canvas)
    return icon


def apply_taskbar_icon(hwnd: int, ico_path: str) -> None:
    if not hwnd or not os.path.isfile(ico_path):
        return
    user32 = ctypes.windll.user32
    IMAGE_ICON = 1
    LR_LOADFROMFILE = 0x0010
    WM_SETICON = 0x0080
    path = os.path.abspath(ico_path)
    user32.LoadImageW.argtypes = [
        ctypes.c_void_p, ctypes.c_wchar_p, ctypes.c_uint,
        ctypes.c_int, ctypes.c_int, ctypes.c_uint,
    ]
    user32.LoadImageW.restype = ctypes.c_void_p
    user32.SendMessageW.argtypes = [
        ctypes.c_void_p, ctypes.c_uint, ctypes.c_void_p, ctypes.c_void_p,
    ]
    small = user32.LoadImageW(None, path, IMAGE_ICON, 16, 16, LR_LOADFROMFILE)
    big = user32.LoadImageW(None, path, IMAGE_ICON, 32, 32, LR_LOADFROMFILE)
    if small:
        user32.SendMessageW(hwnd, WM_SETICON, 0, small)
    if big:
        user32.SendMessageW(hwnd, WM_SETICON, 1, big)


def zubcut_qss() -> str:
    return f"""
QMainWindow, QWidget#clumzyRoot {{
    background-color: {UI_BG};
    color: {UI_TEXT};
}}
QGroupBox {{
    font-weight: normal;
    color: {UI_SAGE};
    border: 1px solid {UI_TEAL};
    border-radius: 4px;
    margin-top: 12px;
    padding-top: 10px;
    background-color: {UI_BG};
}}
QGroupBox::title {{
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 8px;
    padding: 0 4px;
    color: {UI_SAGE};
}}
QLabel {{
    color: {UI_SAGE};
    background-color: transparent;
}}
QLabel#noticeLabel {{
    color: {UI_TEXT};
}}
QLabel#keybindLabel {{
    color: {UI_TEXT};
}}
QPushButton {{
    font-weight: normal;
    background-color: {UI_BTN};
    color: {UI_TEXT};
    border: 1px solid {UI_BORDER};
    border-radius: 4px;
    padding: 6px 12px;
    min-height: 22px;
}}
QPushButton:hover {{
    background-color: {UI_HOVER};
    border: 1px solid {UI_HOVER};
    color: #d0d0d0;
}}
QPushButton:pressed {{
    background-color: {UI_PRESS};
    border: 1px solid {UI_PRESS};
    color: {UI_MUTE};
}}
QPushButton:disabled {{
    background-color: {UI_BTN};
    color: {UI_MUTE};
    border: 1px solid {UI_BORDER};
}}
QCheckBox {{
    color: {UI_SAGE};
    background-color: transparent;
    spacing: 6px;
}}
QCheckBox:hover {{
    color: {UI_TEAL};
}}
QCheckBox::indicator {{
    image: none;
    width: 14px;
    height: 14px;
    border: 1px solid {UI_SAGE};
    background-color: transparent;
    margin: 0px;
}}
QCheckBox::indicator:unchecked {{
    image: none;
    background-color: transparent;
    border: 1px solid {UI_SAGE};
}}
QCheckBox::indicator:hover,
QCheckBox::indicator:unchecked:hover {{
    image: none;
    border: 1px solid {UI_TEAL};
    background-color: transparent;
}}
QCheckBox::indicator:checked {{
    image: none;
    background-color: {UI_TEAL};
    border: 1px solid {UI_SAGE};
}}
QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox {{
    background-color: {UI_BTN};
    color: {UI_TEXT};
    border: 1px solid {UI_SAGE};
    border-radius: 3px;
    padding: 4px 6px;
    min-height: 22px;
    selection-background-color: {UI_TEAL};
    selection-color: {UI_SEL};
}}
QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus {{
    border: 1px solid {UI_SAGE};
}}
QComboBox::drop-down {{
    subcontrol-origin: padding;
    subcontrol-position: top right;
    width: 22px;
    border: none;
    border-left: 1px solid {UI_SAGE};
}}
QComboBox::down-arrow {{
    image: url({ARROW_DOWN});
    width: 9px;
    height: 6px;
}}
QComboBox QAbstractItemView {{
    background-color: {UI_BG};
    color: {UI_TEXT};
    selection-background-color: {UI_TEAL};
    selection-color: {UI_SEL};
    border: 1px solid {UI_SAGE};
    outline: none;
}}
QSpinBox::up-button, QDoubleSpinBox::up-button,
QSpinBox::down-button, QDoubleSpinBox::down-button {{
    background-color: {UI_BTN};
    border: 1px solid {UI_SAGE};
    width: 16px;
}}
QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover,
QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover {{
    background-color: {UI_TEAL};
}}
QSpinBox::up-arrow, QDoubleSpinBox::up-arrow {{
    image: url({ARROW_UP});
    border: none;
    width: 9px;
    height: 6px;
}}
QSpinBox::down-arrow, QDoubleSpinBox::down-arrow {{
    image: url({ARROW_DOWN});
    border: none;
    width: 9px;
    height: 6px;
}}
QScrollBar:vertical {{
    background-color: {UI_BG};
    width: 10px;
    border: none;
}}
QScrollBar::handle:vertical {{
    background-color: #4a4a4a;
    min-height: 24px;
    border-radius: 4px;
}}
QScrollBar::handle:vertical:hover {{
    background-color: {UI_TEAL};
}}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {{
    height: 0;
    border: none;
}}
"""


def apply_fusion_palette(app: QApplication) -> None:
    palette = QPalette()
    window = QColor(UI_BG)
    text = QColor(UI_TEXT)
    button = QColor(UI_BTN)
    palette.setColor(QPalette.Window, window)
    palette.setColor(QPalette.WindowText, text)
    palette.setColor(QPalette.Base, button)
    palette.setColor(QPalette.AlternateBase, QColor('#1a1a1a'))
    palette.setColor(QPalette.Text, text)
    palette.setColor(QPalette.Button, button)
    palette.setColor(QPalette.ButtonText, text)
    palette.setColor(QPalette.Highlight, QColor(UI_TEAL))
    palette.setColor(QPalette.HighlightedText, QColor(UI_SEL))
    palette.setColor(QPalette.ToolTipBase, button)
    palette.setColor(QPalette.ToolTipText, text)
    palette.setColor(QPalette.PlaceholderText, QColor(UI_MUTE))
    app.setPalette(palette)


def apply_dark_titlebar(hwnd: int) -> None:
    try:
        value = ctypes.c_int(1)
        ctypes.windll.dwmapi.DwmSetWindowAttribute(hwnd, 20, ctypes.byref(value), 4)
        ctypes.windll.dwmapi.DwmSetWindowAttribute(hwnd, 19, ctypes.byref(value), 4)
    except Exception:
        pass


def parse_truth(value: str) -> bool:
    return str(value).strip().lower() in ('true', '1', 'on', 'yes')


def parse_config_filters(path: str) -> list[tuple[str, str]]:
    filters: list[tuple[str, str]] = []
    if not os.path.isfile(path):
        return [('loopback packets',
                 'outbound and ip.DstAddr >= 127.0.0.1 and ip.DstAddr <= 127.255.255.255')]
    with open(path, 'r', encoding='utf-8', errors='replace') as handle:
        for raw in handle:
            line = raw.strip()
            if not line or line.startswith('#'):
                continue
            if ':' not in line:
                continue
            name, value = line.split(':', 1)
            filters.append((name.strip(), value.strip()))
    if not filters:
        filters.append(('loopback packets',
                        'outbound and ip.DstAddr >= 127.0.0.1 and ip.DstAddr <= 127.255.255.255'))
    return filters


def settings_path() -> str:
    return os.path.join(app_dir(), 'settings.ini')


def parse_presets(path: str) -> tuple[str, list[dict]]:
    parser = configparser.ConfigParser(interpolation=None)
    parser.optionxform = str
    keybind = '['
    presets: list[dict] = []
    if not os.path.isfile(path):
        return keybind, presets
    parser.read(path, encoding='utf-8')
    if parser.has_section('General'):
        keybind = parser.get('General', 'Keybind', fallback='[') or '['
    for index in range(1, 6):
        section = f'Preset{index}'
        if not parser.has_section(section):
            continue
        presets.append(dict(parser.items(section)))
    return keybind, presets


class Engine:
    def __init__(self, dll_path: str) -> None:
        self.dll = ctypes.CDLL(dll_path)
        self.dll.clumzy_engine_init.restype = c_int
        self.dll.clumzy_is_admin.restype = c_int
        self.dll.clumzy_set_network.argtypes = [c_int]
        self.dll.clumzy_start.argtypes = [c_char_p, c_char_p, c_int]
        self.dll.clumzy_start.restype = c_int
        self.dll.clumzy_stop.restype = None
        self.dll.clumzy_is_running.restype = c_int
        self.dll.clumzy_enable.argtypes = [c_char_p, c_int]
        self.dll.clumzy_lag.argtypes = [c_int, c_int, c_int]
        self.dll.clumzy_drop.argtypes = [c_int, c_int, c_float]
        self.dll.clumzy_disconnect.argtypes = [c_int, c_int]
        self.dll.clumzy_bandwidth.argtypes = [c_int, c_int, c_int, c_int, c_int]
        self.dll.clumzy_throttle.argtypes = [c_int, c_int, c_float, c_int, c_int]
        self.dll.clumzy_duplicate.argtypes = [c_int, c_int, c_float, c_int]
        self.dll.clumzy_ood.argtypes = [c_int, c_int, c_float]
        self.dll.clumzy_tamper.argtypes = [c_int, c_int, c_float, c_int]
        self.dll.clumzy_reset.argtypes = [c_int, c_int, c_float]
        self.dll.clumzy_reset_next.argtypes = []
        self._lock = threading.Lock()
        if self.dll.clumzy_engine_init() == 0:
            raise RuntimeError('engine init failed')

    def start(self, filt: str) -> str | None:
        err = ctypes.create_string_buffer(512)
        with self._lock:
            if self.dll.clumzy_start(filt.encode('utf-8', 'replace'), err, 512) == 0:
                return err.value.decode('utf-8', 'replace') or 'Failed to start filtering.'
        return None

    def stop(self) -> None:
        with self._lock:
            self.dll.clumzy_stop()

    def enable(self, name: str, on: bool) -> None:
        self.dll.clumzy_enable(name.encode('ascii'), 1 if on else 0)


class EngineCallThread(QThread):
    finished_ok = pyqtSignal(str)

    def __init__(self, fn) -> None:
        super().__init__()
        self._fn = fn

    def run(self) -> None:
        try:
            result = self._fn()
            self.finished_ok.emit(result if isinstance(result, str) else '')
        except Exception as exc:
            self.finished_ok.emit(str(exc))


class ModuleRow(QWidget):
    def __init__(self, title: str, extra: QWidget) -> None:
        super().__init__()
        self.enabled = QCheckBox(title)
        layout = QHBoxLayout(self)
        layout.setContentsMargins(2, 2, 2, 2)
        layout.setSpacing(8)
        layout.addWidget(self.enabled)
        layout.addStretch(1)
        layout.addWidget(extra)
        extra.setEnabled(False)
        self.enabled.toggled.connect(extra.setEnabled)


def inbound_outbound() -> tuple[QCheckBox, QCheckBox, QHBoxLayout]:
    inbound = QCheckBox('Inbound')
    outbound = QCheckBox('Outbound')
    inbound.setChecked(True)
    outbound.setChecked(True)
    row = QHBoxLayout()
    row.setContentsMargins(0, 0, 0, 0)
    row.setSpacing(8)
    row.addWidget(inbound)
    row.addWidget(outbound)
    return inbound, outbound, row


def labeled_spin(label: str, box: QWidget) -> QHBoxLayout:
    row = QHBoxLayout()
    row.setContentsMargins(0, 0, 0, 0)
    row.setSpacing(6)
    text = QLabel(label)
    row.addWidget(text)
    row.addWidget(box)
    return row


class ClumzyWindow(QMainWindow):
    def __init__(self, engine: Engine, filters: list[tuple[str, str]],
                 keybind: str, presets: list[dict]) -> None:
        super().__init__()
        self.engine = engine
        self.filters = filters
        self.keybind = keybind[:1] if keybind else '['
        self.presets = presets
        self.running = False
        self._key_down = False
        self._hotkey_armed = False
        self._engine_busy = False
        self._repeat_active = False
        self._stop_requested = False
        self._loading_settings = True
        self._worker = None
        self.setWindowTitle('Clumzy 2.0 | Zub Clan 2026 | Last Updated: 090326')
        self.setObjectName('clumzyMain')
        self.setWindowIcon(load_app_icon())

        root = QWidget()
        root.setObjectName('clumzyRoot')
        self.setCentralWidget(root)
        outer = QVBoxLayout(root)
        outer.setContentsMargins(10, 10, 10, 10)
        outer.setSpacing(8)

        self.filter_edit = QLineEdit(filters[0][1] if filters else 'true')
        self.start_btn = QPushButton('Start')
        self.start_btn.clicked.connect(self.toggle_filter)
        self.network = QComboBox()
        self.network.addItem('(Local) This Device', 1)
        self.network.addItem('(Remote) Shared Devices', 2)
        self.network.setCurrentIndex(1)
        self.filter_presets = QComboBox()
        for name, _value in filters:
            self.filter_presets.addItem(name)
        self.filter_presets.currentIndexChanged.connect(self._on_filter_preset)
        self.filter_edit.textChanged.connect(self._on_filter_typed)
        self.filter_edit.textChanged.connect(self.schedule_save)

        filter_box = QGroupBox('Filtering')
        filter_col = QVBoxLayout(filter_box)
        filter_col.addWidget(self.filter_edit)
        filter_row = QHBoxLayout()
        filter_row.addWidget(self.start_btn)
        filter_row.addWidget(QLabel('Network:'))
        filter_row.addWidget(self.network)
        filter_row.addStretch(1)
        filter_row.addWidget(QLabel('Presets:'))
        filter_row.addWidget(self.filter_presets)
        filter_col.addLayout(filter_row)
        outer.addWidget(filter_box)

        info_box = QGroupBox('Info')
        info_row = QHBoxLayout(info_box)
        info_row.setAlignment(Qt.AlignVCenter)
        self.logo = QLabel()
        self.logo.setAlignment(Qt.AlignCenter)
        self._load_logo()
        info_row.addWidget(self.logo)
        key_label = QLabel(f'Use the key {self.keybind} to toggle on/off')
        key_label.setObjectName('keybindLabel')
        info_row.addWidget(key_label, 1)
        outer.addWidget(info_box)

        extra_box = QGroupBox('Extra Presets')
        extra_row = QHBoxLayout(extra_box)
        extra_row.addWidget(QLabel('Function Presets:'))
        self.func_presets = QComboBox()
        self.func_presets.addItem('Select preset')
        for preset in presets:
            self.func_presets.addItem(preset.get('PresetName', 'Preset'))
        self.func_presets.currentIndexChanged.connect(self._on_func_preset)
        extra_row.addWidget(self.func_presets)
        extra_row.addStretch(1)
        extra_row.addWidget(QLabel('Trigger Mode'))
        self.trigger = QComboBox()
        self.trigger.addItems(['Toggle', 'Timer'])
        self.trigger.currentTextChanged.connect(self._on_trigger)
        self.trigger.currentTextChanged.connect(self.schedule_save)
        extra_row.addWidget(self.trigger)
        self.timer_label = QLabel('On (s):')
        self.timer_secs = QDoubleSpinBox()
        self.timer_secs.setDecimals(2)
        self.timer_secs.setSingleStep(0.1)
        self.timer_secs.setRange(0.05, 120.0)
        self.timer_secs.setValue(1.00)
        self.timer_secs.setKeyboardTracking(False)
        self.timer_secs.valueChanged.connect(self.schedule_save)
        self.repeat_chk = QCheckBox('Repeat')
        self.repeat_chk.toggled.connect(self.schedule_save)
        extra_row.addWidget(self.timer_label)
        extra_row.addWidget(self.timer_secs)
        extra_row.addWidget(self.repeat_chk)
        self.timer_label.hide()
        self.timer_secs.hide()
        self.repeat_chk.hide()
        outer.addWidget(extra_box)

        functions = QGroupBox('Functions')
        func_col = QVBoxLayout(functions)
        self._build_modules(func_col)
        outer.addWidget(functions, 1)

        self.status = QLabel(NOTICE)
        self.status.setObjectName('noticeLabel')
        self.status.setWordWrap(True)
        outer.addWidget(self.status)

        self.auto_stop = QTimer(self)
        self.auto_stop.setSingleShot(True)
        self.auto_stop.timeout.connect(self._on_timer_elapsed)
        self._sync = QTimer(self)
        self._sync.setSingleShot(True)
        self._sync.setInterval(80)
        self._sync.timeout.connect(self.push_engine)
        self._save = QTimer(self)
        self._save.setSingleShot(True)
        self._save.setInterval(250)
        self._save.timeout.connect(self.save_settings)
        self.key_timer = QTimer(self)
        self.key_timer.timeout.connect(self._poll_key)
        self.key_timer.start(30)
        QTimer.singleShot(1000, self._arm_hotkey)

        skip = {self.filter_edit, self.filter_presets, self.func_presets, self.trigger, self.timer_secs}
        for widget in self.findChildren(QCheckBox):
            widget.toggled.connect(self.schedule_push)
        for widget in self.findChildren(QSpinBox) + self.findChildren(QDoubleSpinBox):
            if widget not in skip:
                widget.valueChanged.connect(self.schedule_push)
        for widget in self.findChildren(QComboBox):
            if widget not in skip:
                widget.currentIndexChanged.connect(self.schedule_push)
        self.network.currentIndexChanged.connect(self.schedule_save)
        self.filter_presets.currentIndexChanged.connect(self.schedule_save)
        app = QApplication.instance()
        if app:
            app.installEventFilter(self)
        self.load_settings()
        self._loading_settings = False
        self.schedule_push()

    def _load_logo(self) -> None:
        path = resource_path('clumzy-logo.png')
        if not os.path.isfile(path):
            path = resource_path('etc', 'clumzy-logo.png')
        pix = QPixmap(path)
        if pix.isNull():
            self.logo.setText('Clumzy')
            return
        fitted = pix.scaled(96, 48, Qt.KeepAspectRatio, Qt.SmoothTransformation)
        self.logo.setPixmap(fitted)

    def _build_modules(self, parent: QVBoxLayout) -> None:
        lag_extra = QWidget()
        lag_row = QHBoxLayout(lag_extra)
        lag_row.setContentsMargins(0, 0, 0, 0)
        self.lag_in, self.lag_out, dirs = inbound_outbound()
        lag_row.addLayout(dirs)
        self.lag_ms = QSpinBox()
        self.lag_ms.setRange(0, 15000)
        self.lag_ms.setValue(170)
        lag_row.addLayout(labeled_spin('Delay(ms):', self.lag_ms))
        self.lag_row = ModuleRow('Lag', lag_extra)
        parent.addWidget(self.lag_row)

        drop_extra = QWidget()
        drop_row = QHBoxLayout(drop_extra)
        drop_row.setContentsMargins(0, 0, 0, 0)
        self.drop_in, self.drop_out, dirs = inbound_outbound()
        drop_row.addLayout(dirs)
        self.drop_chance = QDoubleSpinBox()
        self.drop_chance.setRange(0.0, 100.0)
        self.drop_chance.setDecimals(1)
        self.drop_chance.setValue(87.0)
        drop_row.addLayout(labeled_spin('Chance(%):', self.drop_chance))
        self.drop_row = ModuleRow('Drop', drop_extra)
        parent.addWidget(self.drop_row)

        disc_extra = QWidget()
        disc_row = QHBoxLayout(disc_extra)
        disc_row.setContentsMargins(0, 0, 0, 0)
        self.disc_in, self.disc_out, dirs = inbound_outbound()
        disc_row.addLayout(dirs)
        self.disc_row = ModuleRow('Disconnect', disc_extra)
        parent.addWidget(self.disc_row)

        bw_extra = QWidget()
        bw_row = QHBoxLayout(bw_extra)
        bw_row.setContentsMargins(0, 0, 0, 0)
        self.bw_in, self.bw_out, dirs = inbound_outbound()
        bw_row.addLayout(dirs)
        self.bw_queue = QSpinBox()
        self.bw_queue.setRange(0, 99999)
        self.bw_queue.setValue(100)
        self.bw_limit = QSpinBox()
        self.bw_limit.setRange(0, 99999)
        self.bw_limit.setValue(100)
        self.bw_unit = QComboBox()
        self.bw_unit.addItem('KB/s', True)
        self.bw_unit.addItem('MB/s', False)
        bw_row.addLayout(labeled_spin('Queuesize:', self.bw_queue))
        bw_row.addLayout(labeled_spin('Limit:', self.bw_limit))
        bw_row.addWidget(self.bw_unit)
        self.bw_row = ModuleRow('Bandwidth Limiter', bw_extra)
        parent.addWidget(self.bw_row)

        th_extra = QWidget()
        th_row = QHBoxLayout(th_extra)
        th_row.setContentsMargins(0, 0, 0, 0)
        self.th_drop = QCheckBox('Drop Throttled')
        self.th_in, self.th_out, dirs = inbound_outbound()
        self.th_frame = QSpinBox()
        self.th_frame.setRange(0, 1000)
        self.th_frame.setValue(30)
        self.th_chance = QDoubleSpinBox()
        self.th_chance.setRange(0.0, 100.0)
        self.th_chance.setDecimals(1)
        self.th_chance.setValue(10.0)
        th_row.addWidget(self.th_drop)
        th_row.addLayout(labeled_spin('Timeframe(ms):', self.th_frame))
        th_row.addLayout(dirs)
        th_row.addLayout(labeled_spin('Chance(%):', self.th_chance))
        self.th_row = ModuleRow('Throttle', th_extra)
        parent.addWidget(self.th_row)

        dup_extra = QWidget()
        dup_row = QHBoxLayout(dup_extra)
        dup_row.setContentsMargins(0, 0, 0, 0)
        self.dup_count = QSpinBox()
        self.dup_count.setRange(0, 50)
        self.dup_count.setValue(2)
        self.dup_in, self.dup_out, dirs = inbound_outbound()
        self.dup_chance = QDoubleSpinBox()
        self.dup_chance.setRange(0.0, 100.0)
        self.dup_chance.setDecimals(1)
        self.dup_chance.setValue(10.0)
        dup_row.addLayout(labeled_spin('Count:', self.dup_count))
        dup_row.addLayout(dirs)
        dup_row.addLayout(labeled_spin('Chance(%):', self.dup_chance))
        self.dup_row = ModuleRow('Duplicate', dup_extra)
        parent.addWidget(self.dup_row)

        ood_extra = QWidget()
        ood_row = QHBoxLayout(ood_extra)
        ood_row.setContentsMargins(0, 0, 0, 0)
        self.ood_in, self.ood_out, dirs = inbound_outbound()
        self.ood_chance = QDoubleSpinBox()
        self.ood_chance.setRange(0.0, 100.0)
        self.ood_chance.setDecimals(1)
        self.ood_chance.setValue(10.0)
        ood_row.addLayout(dirs)
        ood_row.addLayout(labeled_spin('Chance(%):', self.ood_chance))
        self.ood_row = ModuleRow('Out of order', ood_extra)
        parent.addWidget(self.ood_row)

        tam_extra = QWidget()
        tam_row = QHBoxLayout(tam_extra)
        tam_row.setContentsMargins(0, 0, 0, 0)
        self.tam_sum = QCheckBox('Redo Checksum')
        self.tam_sum.setChecked(True)
        self.tam_in, self.tam_out, dirs = inbound_outbound()
        self.tam_chance = QDoubleSpinBox()
        self.tam_chance.setRange(0.0, 100.0)
        self.tam_chance.setDecimals(1)
        self.tam_chance.setValue(10.0)
        tam_row.addWidget(self.tam_sum)
        tam_row.addLayout(dirs)
        tam_row.addLayout(labeled_spin('Chance(%):', self.tam_chance))
        self.tam_row = ModuleRow('Tamper', tam_extra)
        parent.addWidget(self.tam_row)

        rst_extra = QWidget()
        rst_row = QHBoxLayout(rst_extra)
        rst_row.setContentsMargins(0, 0, 0, 0)
        self.rst_next = QPushButton('RST next packet')
        self.rst_next.clicked.connect(lambda: self.engine.dll.clumzy_reset_next())
        self.rst_in, self.rst_out, dirs = inbound_outbound()
        self.rst_chance = QDoubleSpinBox()
        self.rst_chance.setRange(0.0, 100.0)
        self.rst_chance.setDecimals(1)
        self.rst_chance.setValue(0.0)
        rst_row.addWidget(self.rst_next)
        rst_row.addLayout(dirs)
        rst_row.addLayout(labeled_spin('Chance(%):', self.rst_chance))
        self.rst_row = ModuleRow('Set TCP RST', rst_extra)
        parent.addWidget(self.rst_row)

    def _on_filter_preset(self, index: int) -> None:
        if 0 <= index < len(self.filters):
            self.filter_edit.blockSignals(True)
            self.filter_edit.setText(self.filters[index][1])
            self.filter_edit.blockSignals(False)

    def _on_filter_typed(self) -> None:
        self.filter_presets.blockSignals(True)
        self.filter_presets.setCurrentIndex(-1)
        self.filter_presets.blockSignals(False)

    def _on_trigger(self, text: str) -> None:
        timed = text == 'Timer'
        self.timer_label.setVisible(timed)
        self.timer_secs.setVisible(timed)
        self.repeat_chk.setVisible(timed)

    def _timer_ms(self) -> int:
        return max(50, int(round(self.timer_secs.value() * 1000.0)))

    def _timer_status(self) -> str:
        secs = self.timer_secs.value()
        if self._repeat_active:
            return f'Repeat timer {secs:g}s on — Stop ends the cycle.'
        return f'Timer {secs:g}s, then stop.'

    def _on_func_preset(self, index: int) -> None:
        if index <= 0 or index > len(self.presets):
            return
        preset = self.presets[index - 1]
        self.lag_in.setChecked(parse_truth(preset.get('Lag_Inbound', 'false')))
        self.lag_out.setChecked(parse_truth(preset.get('Lag_Outbound', 'false')))
        self.lag_ms.setValue(int(float(preset.get('Lag_Delay', '0') or 0)))
        self.drop_in.setChecked(parse_truth(preset.get('Drop_Inbound', 'false')))
        self.drop_out.setChecked(parse_truth(preset.get('Drop_Outbound', 'false')))
        self.drop_chance.setValue(float(preset.get('Drop_Chance', '0') or 0))
        self.disc_in.setChecked(parse_truth(preset.get('Disconnect_Inbound', 'false')))
        self.disc_out.setChecked(parse_truth(preset.get('Disconnect_Outbound', 'false')))
        self.bw_in.setChecked(parse_truth(preset.get('BandwidthLimiter_Inbound', 'false')))
        self.bw_out.setChecked(parse_truth(preset.get('BandwidthLimiter_Outbound', 'false')))
        self.bw_queue.setValue(int(float(preset.get('BandwidthLimiter_QueueSize', '0') or 0)))
        self.bw_limit.setValue(int(float(preset.get('BandwidthLimiter_Limit', '0') or 0)))
        unit = (preset.get('BandwidthLimiter_Size', 'kb') or 'kb').lower()
        self.bw_unit.setCurrentIndex(0 if unit == 'kb' else 1)
        self.th_in.setChecked(parse_truth(preset.get('Throttle_Inbound', 'false')))
        self.th_out.setChecked(parse_truth(preset.get('Throttle_Outbound', 'false')))
        self.th_drop.setChecked(parse_truth(preset.get('Throttle_DropThrottled', 'false')))
        self.th_frame.setValue(int(float(preset.get('Throttle_Timeframe', '0') or 0)))
        self.th_chance.setValue(float(preset.get('Throttle_Chance', '0') or 0))
        self.dup_in.setChecked(parse_truth(preset.get('Duplicate_Inbound', 'false')))
        self.dup_out.setChecked(parse_truth(preset.get('Duplicate_Outbound', 'false')))
        self.dup_count.setValue(int(float(preset.get('Duplicate_Count', '0') or 0)))
        self.dup_chance.setValue(float(preset.get('Duplicate_Chance', '0') or 0))
        self.ood_in.setChecked(parse_truth(preset.get('OutOfOrder_Inbound', 'false')))
        self.ood_out.setChecked(parse_truth(preset.get('OutOfOrder_Outbound', 'false')))
        self.ood_chance.setValue(float(preset.get('OutOfOrder_Chance', '0') or 0))
        self.tam_in.setChecked(parse_truth(preset.get('Tamper_Inbound', 'false')))
        self.tam_out.setChecked(parse_truth(preset.get('Tamper_Outbound', 'false')))
        self.tam_sum.setChecked(parse_truth(preset.get('Tamper_RedoChecksum', 'false')))
        self.tam_chance.setValue(float(preset.get('Tamper_Chance', '0') or 0))
        self.rst_in.setChecked(parse_truth(preset.get('SetTCPRST_Inbound', 'false')))
        self.rst_out.setChecked(parse_truth(preset.get('SetTCPRST_Outbound', 'false')))
        self.rst_chance.setValue(float(preset.get('SetTCPRST_Chance', '0') or 0))
        self.schedule_push()

    def schedule_push(self) -> None:
        self._sync.start()
        self.schedule_save()

    def schedule_save(self) -> None:
        if self._loading_settings:
            return
        self._save.start()

    def eventFilter(self, obj, event) -> bool:
        if event.type() in (QEvent.KeyPress, QEvent.ShortcutOverride) and self.keybind:
            text = event.text()
            if text and text[:1].lower() == self.keybind[:1].lower():
                focused = QApplication.focusWidget()
                if isinstance(obj, (QComboBox, QSpinBox, QDoubleSpinBox)) or isinstance(
                    focused, (QComboBox, QSpinBox, QDoubleSpinBox)
                ):
                    return True
        return super().eventFilter(obj, event)

    def load_settings(self) -> None:
        path = settings_path()
        if not os.path.isfile(path):
            return
        cfg = configparser.ConfigParser(interpolation=None)
        cfg.optionxform = str
        try:
            cfg.read(path, encoding='utf-8')
        except Exception:
            return
        if not cfg.has_section('Session'):
            return
        s = cfg['Session']
        self._loading_settings = True
        try:
            mode = s.get('TriggerMode', 'Toggle')
            idx = self.trigger.findText(mode)
            if idx >= 0:
                self.trigger.setCurrentIndex(idx)
            self._on_trigger(self.trigger.currentText())
            try:
                self.timer_secs.setValue(float(s.get('TimerSeconds', '1') or 1))
            except ValueError:
                pass
            self.repeat_chk.setChecked(parse_truth(s.get('Repeat', 'false')))
            try:
                net = int(s.get('Network', '2') or 2)
                pos = self.network.findData(net)
                if pos >= 0:
                    self.network.setCurrentIndex(pos)
            except ValueError:
                pass
            filt = s.get('Filter', '')
            if filt:
                self.filter_edit.blockSignals(True)
                self.filter_edit.setText(filt)
                self.filter_edit.blockSignals(False)
            try:
                fp = int(s.get('FilterPreset', '-1') or -1)
                if 0 <= fp < self.filter_presets.count():
                    self.filter_presets.blockSignals(True)
                    self.filter_presets.setCurrentIndex(fp)
                    self.filter_presets.blockSignals(False)
            except ValueError:
                pass
            try:
                gp = int(s.get('FunctionPreset', '0') or 0)
                if 0 <= gp < self.func_presets.count():
                    self.func_presets.blockSignals(True)
                    self.func_presets.setCurrentIndex(gp)
                    self.func_presets.blockSignals(False)
            except ValueError:
                pass
            self._apply_saved_modules(s)
        finally:
            self._loading_settings = False

    def _apply_saved_modules(self, s: configparser.SectionProxy) -> None:
        def _bool(key: str, default: bool) -> bool:
            if key not in s:
                return default
            return parse_truth(s.get(key, str(default)))

        def _float(key: str, default: float) -> float:
            try:
                return float(s.get(key, str(default)) or default)
            except ValueError:
                return default

        def _int(key: str, default: int) -> int:
            try:
                return int(float(s.get(key, str(default)) or default))
            except ValueError:
                return default

        self.lag_row.enabled.setChecked(_bool('LagEnabled', False))
        self.lag_in.setChecked(_bool('LagInbound', True))
        self.lag_out.setChecked(_bool('LagOutbound', True))
        self.lag_ms.setValue(_int('LagMs', 170))
        self.drop_row.enabled.setChecked(_bool('DropEnabled', False))
        self.drop_in.setChecked(_bool('DropInbound', True))
        self.drop_out.setChecked(_bool('DropOutbound', True))
        self.drop_chance.setValue(_float('DropChance', 87.0))
        self.disc_row.enabled.setChecked(_bool('DisconnectEnabled', False))
        self.disc_in.setChecked(_bool('DisconnectInbound', True))
        self.disc_out.setChecked(_bool('DisconnectOutbound', True))
        self.bw_row.enabled.setChecked(_bool('BandwidthEnabled', False))
        self.bw_in.setChecked(_bool('BandwidthInbound', True))
        self.bw_out.setChecked(_bool('BandwidthOutbound', True))
        self.bw_queue.setValue(_int('BandwidthQueue', 100))
        self.bw_limit.setValue(_int('BandwidthLimit', 100))
        self.bw_unit.setCurrentIndex(0 if _bool('BandwidthKB', True) else 1)
        self.th_row.enabled.setChecked(_bool('ThrottleEnabled', False))
        self.th_drop.setChecked(_bool('ThrottleDrop', False))
        self.th_in.setChecked(_bool('ThrottleInbound', True))
        self.th_out.setChecked(_bool('ThrottleOutbound', True))
        self.th_frame.setValue(_int('ThrottleFrame', 30))
        self.th_chance.setValue(_float('ThrottleChance', 10.0))
        self.dup_row.enabled.setChecked(_bool('DuplicateEnabled', False))
        self.dup_in.setChecked(_bool('DuplicateInbound', True))
        self.dup_out.setChecked(_bool('DuplicateOutbound', True))
        self.dup_count.setValue(_int('DuplicateCount', 2))
        self.dup_chance.setValue(_float('DuplicateChance', 10.0))
        self.ood_row.enabled.setChecked(_bool('OodEnabled', False))
        self.ood_in.setChecked(_bool('OodInbound', True))
        self.ood_out.setChecked(_bool('OodOutbound', True))
        self.ood_chance.setValue(_float('OodChance', 10.0))
        self.tam_row.enabled.setChecked(_bool('TamperEnabled', False))
        self.tam_sum.setChecked(_bool('TamperChecksum', True))
        self.tam_in.setChecked(_bool('TamperInbound', True))
        self.tam_out.setChecked(_bool('TamperOutbound', True))
        self.tam_chance.setValue(_float('TamperChance', 10.0))
        self.rst_row.enabled.setChecked(_bool('ResetEnabled', False))
        self.rst_in.setChecked(_bool('ResetInbound', True))
        self.rst_out.setChecked(_bool('ResetOutbound', True))
        self.rst_chance.setValue(_float('ResetChance', 0.0))

    def save_settings(self) -> None:
        if self._loading_settings:
            return
        cfg = configparser.ConfigParser(interpolation=None)
        cfg.optionxform = str
        cfg['Session'] = {
            'TriggerMode': self.trigger.currentText(),
            'TimerSeconds': f'{self.timer_secs.value():.2f}',
            'Repeat': 'true' if self.repeat_chk.isChecked() else 'false',
            'Network': str(int(self.network.currentData() or 2)),
            'Filter': self.filter_edit.text(),
            'FilterPreset': str(self.filter_presets.currentIndex()),
            'FunctionPreset': str(self.func_presets.currentIndex()),
            'LagEnabled': str(self.lag_row.enabled.isChecked()).lower(),
            'LagInbound': str(self.lag_in.isChecked()).lower(),
            'LagOutbound': str(self.lag_out.isChecked()).lower(),
            'LagMs': str(self.lag_ms.value()),
            'DropEnabled': str(self.drop_row.enabled.isChecked()).lower(),
            'DropInbound': str(self.drop_in.isChecked()).lower(),
            'DropOutbound': str(self.drop_out.isChecked()).lower(),
            'DropChance': str(self.drop_chance.value()),
            'DisconnectEnabled': str(self.disc_row.enabled.isChecked()).lower(),
            'DisconnectInbound': str(self.disc_in.isChecked()).lower(),
            'DisconnectOutbound': str(self.disc_out.isChecked()).lower(),
            'BandwidthEnabled': str(self.bw_row.enabled.isChecked()).lower(),
            'BandwidthInbound': str(self.bw_in.isChecked()).lower(),
            'BandwidthOutbound': str(self.bw_out.isChecked()).lower(),
            'BandwidthQueue': str(self.bw_queue.value()),
            'BandwidthLimit': str(self.bw_limit.value()),
            'BandwidthKB': str(bool(self.bw_unit.currentData())).lower(),
            'ThrottleEnabled': str(self.th_row.enabled.isChecked()).lower(),
            'ThrottleDrop': str(self.th_drop.isChecked()).lower(),
            'ThrottleInbound': str(self.th_in.isChecked()).lower(),
            'ThrottleOutbound': str(self.th_out.isChecked()).lower(),
            'ThrottleFrame': str(self.th_frame.value()),
            'ThrottleChance': str(self.th_chance.value()),
            'DuplicateEnabled': str(self.dup_row.enabled.isChecked()).lower(),
            'DuplicateInbound': str(self.dup_in.isChecked()).lower(),
            'DuplicateOutbound': str(self.dup_out.isChecked()).lower(),
            'DuplicateCount': str(self.dup_count.value()),
            'DuplicateChance': str(self.dup_chance.value()),
            'OodEnabled': str(self.ood_row.enabled.isChecked()).lower(),
            'OodInbound': str(self.ood_in.isChecked()).lower(),
            'OodOutbound': str(self.ood_out.isChecked()).lower(),
            'OodChance': str(self.ood_chance.value()),
            'TamperEnabled': str(self.tam_row.enabled.isChecked()).lower(),
            'TamperChecksum': str(self.tam_sum.isChecked()).lower(),
            'TamperInbound': str(self.tam_in.isChecked()).lower(),
            'TamperOutbound': str(self.tam_out.isChecked()).lower(),
            'TamperChance': str(self.tam_chance.value()),
            'ResetEnabled': str(self.rst_row.enabled.isChecked()).lower(),
            'ResetInbound': str(self.rst_in.isChecked()).lower(),
            'ResetOutbound': str(self.rst_out.isChecked()).lower(),
            'ResetChance': str(self.rst_chance.value()),
        }
        try:
            with open(settings_path(), 'w', encoding='utf-8') as handle:
                cfg.write(handle)
        except OSError:
            pass

    def _arm_hotkey(self) -> None:
        self._hotkey_armed = True

    def push_engine(self) -> None:
        e = self.engine
        if e is None:
            return
        e.dll.clumzy_set_network(int(self.network.currentData() or 2))
        e.enable('lag', self.lag_row.enabled.isChecked())
        e.dll.clumzy_lag(int(self.lag_in.isChecked()), int(self.lag_out.isChecked()), int(self.lag_ms.value()))
        e.enable('drop', self.drop_row.enabled.isChecked())
        e.dll.clumzy_drop(int(self.drop_in.isChecked()), int(self.drop_out.isChecked()), c_float(self.drop_chance.value()))
        e.enable('disconnect', self.disc_row.enabled.isChecked())
        e.dll.clumzy_disconnect(int(self.disc_in.isChecked()), int(self.disc_out.isChecked()))
        e.enable('bandwidth', self.bw_row.enabled.isChecked())
        e.dll.clumzy_bandwidth(
            int(self.bw_in.isChecked()), int(self.bw_out.isChecked()),
            int(self.bw_limit.value()), int(self.bw_queue.value()),
            1 if self.bw_unit.currentData() else 0)
        e.enable('throttle', self.th_row.enabled.isChecked())
        e.dll.clumzy_throttle(
            int(self.th_in.isChecked()), int(self.th_out.isChecked()),
            c_float(self.th_chance.value()), int(self.th_frame.value()),
            int(self.th_drop.isChecked()))
        e.enable('duplicate', self.dup_row.enabled.isChecked())
        e.dll.clumzy_duplicate(
            int(self.dup_in.isChecked()), int(self.dup_out.isChecked()),
            c_float(self.dup_chance.value()), int(self.dup_count.value()))
        e.enable('ood', self.ood_row.enabled.isChecked())
        e.dll.clumzy_ood(int(self.ood_in.isChecked()), int(self.ood_out.isChecked()), c_float(self.ood_chance.value()))
        e.enable('tamper', self.tam_row.enabled.isChecked())
        e.dll.clumzy_tamper(
            int(self.tam_in.isChecked()), int(self.tam_out.isChecked()),
            c_float(self.tam_chance.value()), int(self.tam_sum.isChecked()))
        e.enable('reset', self.rst_row.enabled.isChecked())
        e.dll.clumzy_reset(int(self.rst_in.isChecked()), int(self.rst_out.isChecked()), c_float(self.rst_chance.value()))

    def toggle_filter(self) -> None:
        if self.running or self._repeat_active or self._engine_busy:
            self.stop_filter()
        else:
            self.start_filter()

    def start_filter(self) -> None:
        if self._engine_busy:
            return
        self._repeat_active = (
            self.trigger.currentText() == 'Timer' and self.repeat_chk.isChecked()
        )
        self._stop_requested = False
        self.push_engine()
        filt = self.filter_edit.text().strip() or 'true'
        self._engine_busy = True
        self.start_btn.setEnabled(False)
        self.status.setText('Starting filter…')
        self._worker = EngineCallThread(lambda: self.engine.start(filt) or '')
        self._worker.finished_ok.connect(self._on_started, Qt.QueuedConnection)
        self._worker.start()

    def _on_started(self, error: str) -> None:
        self._engine_busy = False
        self.start_btn.setEnabled(True)
        if self._worker:
            self._worker.deleteLater()
            self._worker = None
        if error:
            self._repeat_active = False
            self._stop_requested = False
            self.status.setText(error)
            return
        self.running = True
        if self._stop_requested:
            self.stop_filter()
            return
        self.start_btn.setText('Stop')
        self.filter_edit.setEnabled(False)
        if self.trigger.currentText() == 'Timer':
            self.auto_stop.start(self._timer_ms())
            self.status.setText(self._timer_status())
        else:
            self.status.setText('Started filtering. Enable functionalities to take effect.')

    def _on_timer_elapsed(self) -> None:
        if self._repeat_active:
            self._cycle_filter()
        else:
            self.stop_filter()

    def _cycle_filter(self) -> None:
        if self._engine_busy or not self._repeat_active:
            return
        filt = self.filter_edit.text().strip() or 'true'
        self._engine_busy = True
        self.status.setText('Cycling off/on…')

        def work() -> str:
            self.engine.stop()
            if not self._repeat_active or self._stop_requested:
                return 'stopped'
            return self.engine.start(filt) or ''

        self._worker = EngineCallThread(work)
        self._worker.finished_ok.connect(self._on_cycled, Qt.QueuedConnection)
        self._worker.start()

    def _on_cycled(self, error: str) -> None:
        self._engine_busy = False
        if self._worker:
            self._worker.deleteLater()
            self._worker = None
        if error == 'stopped' or not self._repeat_active or self._stop_requested:
            try:
                self.engine.stop()
            except Exception:
                pass
            self._finish_stopped()
            return
        if error:
            self._repeat_active = False
            try:
                self.engine.stop()
            except Exception:
                pass
            self._finish_stopped()
            self.status.setText(error)
            return
        self.running = True
        self.start_btn.setText('Stop')
        self.start_btn.setEnabled(True)
        self.filter_edit.setEnabled(False)
        self.auto_stop.start(self._timer_ms())
        self.status.setText(self._timer_status())

    def stop_filter(self) -> None:
        self.auto_stop.stop()
        self._repeat_active = False
        self._stop_requested = True
        if self._engine_busy:
            return
        if not self.running:
            self._finish_stopped()
            return
        self._engine_busy = True
        self.start_btn.setEnabled(False)
        self.status.setText('Stopping…')
        self._worker = EngineCallThread(lambda: self.engine.stop() or '')
        self._worker.finished_ok.connect(self._on_stopped, Qt.QueuedConnection)
        self._worker.start()

    def _on_stopped(self, _unused: str = '') -> None:
        self._engine_busy = False
        if self._worker:
            self._worker.deleteLater()
            self._worker = None
        self._finish_stopped()

    def _finish_stopped(self) -> None:
        self.running = False
        self._repeat_active = False
        self._stop_requested = False
        self.start_btn.setText('Start')
        self.start_btn.setEnabled(True)
        self.filter_edit.setEnabled(True)
        self.status.setText('Stopped. To begin again, edit criteria and click Start.')

    def _poll_key(self) -> None:
        if not self._hotkey_armed or not self.keybind:
            return
        scan = ctypes.windll.user32.VkKeyScanW(ord(self.keybind[0]))
        if scan == -1:
            return
        vk = scan & 0xFF
        down = (ctypes.windll.user32.GetAsyncKeyState(vk) & 0x8000) != 0
        if down and not self._key_down:
            self.toggle_filter()
        self._key_down = down

    def closeEvent(self, event) -> None:
        self.key_timer.stop()
        self.auto_stop.stop()
        self._hotkey_armed = False
        self._repeat_active = False
        self._stop_requested = True
        self.save_settings()
        if self._worker is not None:
            try:
                self._worker.finished_ok.disconnect()
            except TypeError:
                pass
            if self._worker.isRunning():
                self._worker.wait(10000)
        try:
            self.engine.stop()
        except Exception:
            pass
        event.accept()


def already_running() -> bool:
    handle = ctypes.windll.kernel32.CreateEventW(
        None, False, False, 'Global\\CLUMZY_IS_RUNNING_EVENT_NAME')
    return ctypes.windll.kernel32.GetLastError() == 183 and bool(handle)


def main() -> int:
    os.chdir(app_dir())
    set_win_app_id()
    QApplication.setAttribute(Qt.AA_EnableHighDpiScaling, True)
    app = QApplication(sys.argv)
    app.setApplicationName('Clumzy')
    app.setWindowIcon(load_app_icon())
    app.setStyle('Fusion')
    apply_fusion_palette(app)
    font = QFont('Segoe UI', 9)
    app.setFont(font)

    if already_running():
        QMessageBox.critical(None, 'Clumzy', "There's already an instance of Clumzy running.")
        return 1

    dll_path = os.path.join(app_dir(), 'clumzy_engine.dll')
    if hasattr(os, 'add_dll_directory'):
        os.add_dll_directory(app_dir())
    if not os.path.isfile(dll_path):
        QMessageBox.critical(None, 'Clumzy', f'Missing engine DLL:\n{dll_path}')
        return 1
    try:
        engine = Engine(dll_path)
    except OSError as exc:
        QMessageBox.critical(None, 'Clumzy', f'Could not load the packet engine.\n{exc}')
        return 1

    filters = parse_config_filters(os.path.join(app_dir(), 'config.txt'))
    keybind, presets = parse_presets(os.path.join(app_dir(), 'presets.ini'))
    window = ClumzyWindow(engine, filters, keybind, presets)
    window.setStyleSheet(zubcut_qss())
    window.resize(860, 760)
    window.show()
    hwnd = int(window.winId())
    apply_dark_titlebar(hwnd)
    apply_taskbar_icon(hwnd, resource_path('clumzy-icon.ico'))
    return app.exec_()


if __name__ == '__main__':
    sys.exit(main())
