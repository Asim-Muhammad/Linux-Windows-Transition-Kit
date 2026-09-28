#!/usr/bin/env python3
"""
Windows Security (Defender) Dashboard & KDE Plasma 6 System Tray Icon for Fedora 44.
Provides:
  - Live System Tray Shield Icon (Green Shield when Real-Time Protection is Active, Red when Stopped)
  - Real-Time Virus & Threat Protection status & toggle (clamd@scan + clamav-clamonacc)
  - Attached USB Drive Auto-Scanner status & toggle (usb-defender-watcher)
  - Quick Scan, Custom Folder Scan, Virus Signature Update (freshclam), and ClamTk launcher
  - Firewall (firewalld) & Exploit Protection (SELinux) live status cards
"""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

from PyQt6.QtCore import Qt, QTimer, QProcess
from PyQt6.QtGui import QAction, QColor, QFont, QIcon, QPainter, QPainterPath, QPen, QPixmap
from PyQt6.QtWidgets import (
    QApplication,
    QFileDialog,
    QFrame,
    QGridLayout,
    QHBoxLayout,
    QLabel,
    QMainWindow,
    QMenu,
    QMessageBox,
    QPushButton,
    QSystemTrayIcon,
    QTextEdit,
    QVBoxLayout,
    QWidget,
)


def create_shield_icon(active: bool = True) -> QIcon:
    """Render a crisp Windows Defender-style Shield with a green checkmark or red alert badge."""
    pix = QPixmap(64, 64)
    pix.fill(Qt.GlobalColor.transparent)
    p = QPainter(pix)
    p.setRenderHint(QPainter.RenderHint.Antialiasing)

    # Outer Shield Path
    shield = QPainterPath()
    shield.moveTo(32, 4)
    shield.lineTo(56, 12)
    shield.cubicTo(56, 36, 46, 50, 32, 60)
    shield.cubicTo(18, 50, 8, 36, 8, 12)
    shield.closeSubpath()

    p.setBrush(QColor("#0078D4"))
    p.setPen(QPen(QColor("#60CDFF"), 2))
    p.drawPath(shield)

    # Status Badge in bottom-right corner
    badge_color = QColor("#107C10") if active else QColor("#D13438")
    p.setBrush(badge_color)
    p.setPen(QPen(QColor("#202020"), 2))
    p.drawEllipse(34, 34, 26, 26)

    p.setPen(QPen(Qt.GlobalColor.white, 3.2, Qt.PenStyle.SolidLine, Qt.PenCapStyle.RoundCap, Qt.PenJoinStyle.RoundJoin))
    if active:
        p.drawLine(41, 47, 46, 52)
        p.drawLine(46, 52, 54, 42)
    else:
        p.drawLine(43, 43, 51, 51)
        p.drawLine(51, 43, 43, 51)

    p.end()
    return QIcon(pix)


def is_service_active(service_name: str) -> bool:
    try:
        res = subprocess.run(
            ["systemctl", "is-active", "--quiet", service_name],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        return res.returncode == 0
    except Exception:
        return False


def get_selinux_status() -> str:
    try:
        val = Path("/sys/fs/selinux/enforce").read_text().strip()
        return "Enforcing (Protected)" if val == "1" else "Permissive"
    except Exception:
        return "Unknown"


class StatusCard(QFrame):
    def __init__(self, title: str, subtitle: str) -> None:
        super().__init__()
        self.setObjectName("statusCard")
        layout = QVBoxLayout(self)
        layout.setContentsMargins(18, 16, 18, 16)
        layout.setSpacing(8)

        self.title_lbl = QLabel(title)
        self.title_lbl.setFont(QFont("Sans Serif", 12, QFont.Weight.Bold))
        self.title_lbl.setStyleSheet("color: #FFFFFF;")

        self.status_lbl = QLabel("Checking...")
        self.status_lbl.setFont(QFont("Sans Serif", 10, QFont.Weight.DemiBold))

        self.sub_lbl = QLabel(subtitle)
        self.sub_lbl.setWordWrap(True)
        self.sub_lbl.setStyleSheet("color: #A8A8A8; font-size: 12px;")

        self.btn_row = QHBoxLayout()
        self.btn_row.setSpacing(8)

        layout.addWidget(self.title_lbl)
        layout.addWidget(self.status_lbl)
        layout.addWidget(self.sub_lbl)
        layout.addStretch()
        layout.addLayout(self.btn_row)

    def set_state(self, ok: bool, text: str) -> None:
        color = "#6CCB5F" if ok else "#FF99A4"
        prefix = "✔ " if ok else "⚠ "
        self.status_lbl.setText(f"{prefix}{text}")
        self.status_lbl.setStyleSheet(f"color: {color};")


class WindowsSecurityWindow(QMainWindow):
    def __init__(self, start_hidden: bool = False) -> None:
        super().__init__()
        self.setWindowTitle("Windows Security")
        self.resize(880, 620)
        self.setWindowIcon(create_shield_icon(True))
        self.scan_proc: QProcess | None = None

        self._apply_fluent_dark_theme()
        self._build_ui()
        self._setup_tray()
        self.refresh_status()

        self.timer = QTimer(self)
        self.timer.timeout.connect(self.refresh_status)
        self.timer.start(4000)

        if not start_hidden:
            self.show()

    def _apply_fluent_dark_theme(self) -> None:
        self.setStyleSheet(
            """
            QMainWindow, QWidget#central {
                background-color: #1C1C1C;
                color: #FFFFFF;
            }
            QFrame#headerBanner {
                background-color: #252525;
                border: 1px solid #333333;
                border-radius: 10px;
            }
            QFrame#statusCard {
                background-color: #272727;
                border: 1px solid #383838;
                border-radius: 10px;
            }
            QPushButton {
                background-color: #333333;
                color: #FFFFFF;
                border: 1px solid #484848;
                border-radius: 6px;
                padding: 7px 14px;
                font-weight: 600;
            }
            QPushButton:hover {
                background-color: #3F3F3F;
                border-color: #60CDFF;
            }
            QPushButton#primaryBtn {
                background-color: #0078D4;
                border-color: #0078D4;
            }
            QPushButton#primaryBtn:hover {
                background-color: #1084DF;
            }
            QTextEdit {
                background-color: #141414;
                color: #D4D4D4;
                border: 1px solid #333333;
                border-radius: 8px;
                font-family: monospace;
                font-size: 12px;
            }
            """
        )

    def _build_ui(self) -> None:
        central = QWidget()
        central.setObjectName("central")
        self.setCentralWidget(central)

        root = QVBoxLayout(central)
        root.setContentsMargins(24, 22, 24, 22)
        root.setSpacing(16)

        # Header Banner
        banner = QFrame()
        banner.setObjectName("headerBanner")
        b_layout = QHBoxLayout(banner)
        b_layout.setContentsMargins(20, 16, 20, 16)

        self.hero_icon = QLabel()
        self.hero_icon.setPixmap(create_shield_icon(True).pixmap(52, 52))
        b_layout.addWidget(self.hero_icon)

        title_col = QVBoxLayout()
        self.hero_title = QLabel("Security at a glance")
        self.hero_title.setFont(QFont("Sans Serif", 17, QFont.Weight.Bold))
        self.hero_sub = QLabel(
            "Real-time ClamAV fanotify protection, automatic USB drive scanning, SELinux, and Firewall status."
        )
        self.hero_sub.setStyleSheet("color: #B8B8B8;")
        title_col.addWidget(self.hero_title)
        title_col.addWidget(self.hero_sub)
        b_layout.addLayout(title_col, 1)

        clamtk_btn = QPushButton("Open ClamTk GUI")
        clamtk_btn.clicked.connect(self.open_clamtk)
        b_layout.addWidget(clamtk_btn)

        root.addWidget(banner)

        # 2x2 Status Grid
        grid = QGridLayout()
        grid.setSpacing(14)

        self.card_rt = StatusCard(
            "🛡️ Virus & threat protection",
            "Kernel fanotify real-time scanning (clamd + clamonacc) for Downloads and /tmp.",
        )
        self.btn_toggle_rt = QPushButton("Toggle Real-Time")
        self.btn_toggle_rt.setObjectName("primaryBtn")
        self.btn_toggle_rt.clicked.connect(self.toggle_realtime)
        self.btn_quick_scan = QPushButton("Quick Scan")
        self.btn_quick_scan.clicked.connect(self.run_quick_scan)
        self.btn_custom_scan = QPushButton("Scan Folder...")
        self.btn_custom_scan.clicked.connect(self.run_custom_scan)
        self.card_rt.btn_row.addWidget(self.btn_toggle_rt)
        self.card_rt.btn_row.addWidget(self.btn_quick_scan)
        self.card_rt.btn_row.addWidget(self.btn_custom_scan)

        self.card_usb = StatusCard(
            "🔌 Attached USB & Drive Protection",
            "Automatically scans removable USB flash drives and external disks mounted at /run/media.",
        )
        self.btn_scan_usb = QPushButton("Scan Attached USBs Now")
        self.btn_scan_usb.clicked.connect(self.scan_attached_media)
        self.btn_update_db = QPushButton("Update Signatures")
        self.btn_update_db.clicked.connect(self.update_signatures)
        self.card_usb.btn_row.addWidget(self.btn_scan_usb)
        self.card_usb.btn_row.addWidget(self.btn_update_db)

        self.card_fw = StatusCard(
            "🔥 Firewall & network protection",
            "Fedora firewalld packet filtering blocks unauthorized incoming network connections.",
        )
        self.card_selinux = StatusCard(
            "🔒 App & exploit control (SELinux)",
            "Mandatory Access Control isolates processes and prevents zero-day privilege escalation.",
        )

        grid.addWidget(self.card_rt, 0, 0)
        grid.addWidget(self.card_usb, 0, 1)
        grid.addWidget(self.card_fw, 1, 0)
        grid.addWidget(self.card_selinux, 1, 1)
        root.addLayout(grid)

        # Live Activity & Scan Log Output
        log_lbl = QLabel("Live Protection & Scan Activity Log:")
        log_lbl.setStyleSheet("color: #BDBDBD; font-weight: 600;")
        root.addWidget(log_lbl)

        self.log_box = QTextEdit()
        self.log_box.setReadOnly(True)
        self.log_box.setPlainText(
            "[READY] Windows Security Dashboard initialized.\n"
            "Real-Time Engine: clamd@scan + clamav-clamonacc + usb-defender-watcher."
        )
        root.addWidget(self.log_box, 1)

    def _setup_tray(self) -> None:
        self.tray = QSystemTrayIcon(create_shield_icon(True), self)
        self.tray.setToolTip("Windows Security — Real-Time Protection")

        menu = QMenu()
        act_open = QAction("Open Windows Security Dashboard", self)
        act_open.triggered.connect(self.show_and_raise)
        act_quick = QAction("Run Quick Scan (/tmp & /run/media)", self)
        act_quick.triggered.connect(self.run_quick_scan)
        act_clamtk = QAction("Open ClamTk Virus Scanner", self)
        act_clamtk.triggered.connect(self.open_clamtk)
        act_quit = QAction("Exit Tray Monitor", self)
        act_quit.triggered.connect(QApplication.instance().quit)

        menu.addAction(act_open)
        menu.addAction(act_quick)
        menu.addAction(act_clamtk)
        menu.addSeparator()
        menu.addAction(act_quit)

        self.tray.setContextMenu(menu)
        self.tray.activated.connect(self._on_tray_activated)
        self.tray.show()

    def _on_tray_activated(self, reason: QSystemTrayIcon.ActivationReason) -> None:
        if reason in (
            QSystemTrayIcon.ActivationReason.Trigger,
            QSystemTrayIcon.ActivationReason.DoubleClick,
        ):
            self.show_and_raise()

    def show_and_raise(self) -> None:
        self.showNormal()
        self.raise_()
        self.activateWindow()

    def closeEvent(self, event) -> None:
        if self.tray.isVisible():
            self.hide()
            event.ignore()
        else:
            event.accept()

    def append_log(self, msg: str) -> None:
        self.log_box.append(msg)

    def refresh_status(self) -> None:
        clamd_ok = is_service_active("clamd@scan.service")
        onacc_ok = is_service_active("clamav-clamonacc.service")
        usb_ok = is_service_active("usb-defender-watcher.service")
        fw_ok = is_service_active("firewalld.service")
        selinux_str = get_selinux_status()
        selinux_ok = "Enforcing" in selinux_str

        rt_active = clamd_ok and onacc_ok
        if rt_active:
            self.card_rt.set_state(True, "Real-Time Protection ON (clamd + clamonacc active)")
            self.btn_toggle_rt.setText("Pause Real-Time")
        elif clamd_ok:
            self.card_rt.set_state(False, "clamd running (clamonacc starting/warming up)")
            self.btn_toggle_rt.setText("Enable Real-Time")
        else:
            self.card_rt.set_state(False, "Real-Time Protection OFF")
            self.btn_toggle_rt.setText("Enable Real-Time")

        if usb_ok:
            self.card_usb.set_state(True, "USB Auto-Scan Active (watching /run/media)")
        else:
            self.card_usb.set_state(False, "USB Watcher Stopped")

        self.card_fw.set_state(fw_ok, "Firewall Active (firewalld)" if fw_ok else "Firewall Inactive")
        self.card_selinux.set_state(selinux_ok, f"SELinux: {selinux_str}")

        icon = create_shield_icon(rt_active)
        self.setWindowIcon(icon)
        self.hero_icon.setPixmap(icon.pixmap(52, 52))
        self.tray.setIcon(icon)
        self.tray.setToolTip(
            "Windows Security: Protected (Real-Time ON)"
            if rt_active
            else "Windows Security: Action Needed (Real-Time OFF)"
        )

    def toggle_realtime(self) -> None:
        rt_active = is_service_active("clamd@scan.service") and is_service_active("clamav-clamonacc.service")
        action = "stop" if rt_active else "start"
        self.append_log(f"[ACTION] Requesting '{action}' for Real-Time Antivirus services...")
        subprocess.Popen(
            [
                "pkexec",
                "systemctl",
                action,
                "clamd@scan.service",
                "clamav-clamonacc.service",
                "usb-defender-watcher.service",
            ]
        )

    def open_clamtk(self) -> None:
        subprocess.Popen(["clamtk"])

    def update_signatures(self) -> None:
        self.append_log("[UPDATE] Updating virus signatures via freshclam...")
        self._start_process(["pkexec", "freshclam"])

    def run_quick_scan(self) -> None:
        self.append_log("[SCAN] Starting Quick Scan of /tmp and /run/media...")
        self._start_process(["clamscan", "-r", "--bell", "-i", "/tmp", "/run/media"])

    def scan_attached_media(self) -> None:
        self.append_log("[USB SCAN] Scanning all attached drives in /run/media and /mnt...")
        self._start_process(["clamscan", "-r", "-i", "/run/media", "/mnt"])

    def run_custom_scan(self) -> None:
        folder = QFileDialog.getExistingDirectory(self, "Select Folder or Drive to Scan", "/run/media")
        if folder:
            self.append_log(f"[CUSTOM SCAN] Scanning {folder} ...")
            self._start_process(["clamscan", "-r", "-i", folder])

    def _start_process(self, cmd: list[str]) -> None:
        if self.scan_proc and self.scan_proc.state() != QProcess.ProcessState.NotRunning:
            QMessageBox.information(self, "Task in Progress", "A scan or update is already running.")
            return
        self.scan_proc = QProcess(self)
        self.scan_proc.setProcessChannelMode(QProcess.ProcessChannelMode.MergedChannels)
        self.scan_proc.readyReadStandardOutput.connect(
            lambda: self.append_log(bytes(self.scan_proc.readAllStandardOutput()).decode("utf-8", errors="replace").rstrip())
        )
        self.scan_proc.finished.connect(
            lambda code, _status: self.append_log(f"[DONE] Process finished (exit code {code}).")
        )
        self.scan_proc.start(cmd[0], cmd[1:])


def main() -> int:
    app = QApplication(sys.argv)
    app.setApplicationName("Windows Security")
    app.setQuitOnLastWindowClosed(False)
    start_hidden = "--tray" in sys.argv
    _win = WindowsSecurityWindow(start_hidden=start_hidden)
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
