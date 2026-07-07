#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
基于STM32的冷链运输环境监测系统 - 上位机监控软件
"""

import sys
import json
import time
from datetime import datetime
from collections import deque

import paho.mqtt.client as mqtt
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QGroupBox, QLabel, QPushButton, QTextEdit, QTableWidget,
    QTableWidgetItem, QHeaderView, QTabWidget, QStatusBar, QSpinBox,
    QFormLayout, QMessageBox
)
from PyQt5.QtCore import Qt, QTimer, pyqtSignal, QThread
from PyQt5.QtGui import QFont, QColor, QPalette

MQTT_CONFIG = {
    "broker": "broker.emqx.io",
    "port": 1883,
    "topic_data": "coldchain/data",
    "topic_alarm": "coldchain/alarm",
    "topic_cmd": "coldchain/cmd",
    "client_id": f"ColdChain_Monitor_{int(time.time())}",
}


class MQTTWorker(QThread):
    data_received = pyqtSignal(dict)
    alarm_received = pyqtSignal(dict)
    connected = pyqtSignal(bool)
    log_message = pyqtSignal(str)

    def __init__(self):
        super().__init__()
        self.client = mqtt.Client(client_id=MQTT_CONFIG["client_id"])
        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message
        self.client.on_disconnect = self._on_disconnect

    def _on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            self.connected.emit(True)
            self.log_message.emit("[MQTT] \u5df2\u8fde\u63a5\u5230 " + MQTT_CONFIG["broker"])
            client.subscribe(MQTT_CONFIG["topic_data"], qos=1)
            client.subscribe(MQTT_CONFIG["topic_alarm"], qos=1)
            self.log_message.emit("[MQTT] \u5df2\u8ba2\u9605 coldchain/data")
            self.log_message.emit("[MQTT] \u5df2\u8ba2\u9605 coldchain/alarm")
        else:
            self.connected.emit(False)

    def _on_message(self, client, userdata, msg):
        try:
            payload = json.loads(msg.payload.decode("utf-8"))
            now_str = datetime.now().strftime("%H:%M:%S")
            if "alarm" in msg.topic:
                payload["_time"] = now_str
                self.alarm_received.emit(payload)
            else:
                payload["_topic"] = msg.topic
                payload["_time"] = now_str
                self.data_received.emit(payload)
        except Exception as e:
            self.log_message.emit(f"[MQTT] \u89e3\u6790\u9519\u8bef: {e}")

    def _on_disconnect(self, client, userdata, rc):
        self.connected.emit(False)
        self.log_message.emit(f"[MQTT] \u65ad\u5f00\u8fde\u63a5 (rc={rc})")

    def run(self):
        try:
            self.client.connect(MQTT_CONFIG["broker"], MQTT_CONFIG["port"], 60)
            self.client.loop_forever()
        except Exception as e:
            self.log_message.emit(f"[MQTT] \u8fde\u63a5\u5931\u8d25: {e}")

    def stop(self):
        self.client.disconnect()

    def publish(self, topic, payload):
        self.client.publish(topic, json.dumps(payload), qos=1)


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("\u51b7\u94fe\u8fd0\u8f93\u73af\u5883\u76d1\u6d4b\u7cfb\u7edf - \u4e0a\u4f4d\u673a\u76d1\u63a7 v1.0")
        self.setMinimumSize(1200, 800)

        self.max_points = 3600
        self.data_buffers = {
            "time": deque(maxlen=self.max_points),
            "temp": deque(maxlen=self.max_points),
            "hum": deque(maxlen=self.max_points),
            "ax": deque(maxlen=self.max_points),
            "ay": deque(maxlen=self.max_points),
            "az": deque(maxlen=self.max_points),
        }
        self.latest_data = {}

        self._setup_ui()
        self._setup_mqtt()

        self.plot_timer = QTimer()
        self.plot_timer.timeout.connect(self._update_ui)
        self.plot_timer.start(1000)

    def _setup_ui(self):
        central = QWidget()
        self.setCentralWidget(central)
        main_layout = QVBoxLayout(central)

        self.tabs = QTabWidget()
        main_layout.addWidget(self.tabs)

        # Tab 1: \u76f4\u63a5\u76d1\u63a7
        tab1 = QWidget()
        layout1 = QVBoxLayout(tab1)

        sg = QGroupBox("\u5b9e\u65f6\u6570\u636e")
        sl = QHBoxLayout(sg)

        self.lbl_temp = QLabel("--.- \u2103")
        self.lbl_hum = QLabel("--.- %RH")
        self.lbl_gps = QLabel("\u672a\u5b9a\u4f4d")
        self.lbl_pkts = QLabel("0 \u5305")

        for lbl in [self.lbl_temp, self.lbl_hum, self.lbl_gps, self.lbl_pkts]:
            lbl.setFont(QFont("Arial", 16, QFont.Bold))
            lbl.setAlignment(Qt.AlignCenter)
            lbl.setStyleSheet("background: #2c3e50; color: white; padding: 15px; border-radius: 8px;")
            sl.addWidget(lbl)

        layout1.addWidget(sg)

        # Data log
        self.data_log = QTextEdit()
        self.data_log.setReadOnly(True)
        self.data_log.setFont(QFont("Consolas", 10))
        self.data_log.setMaximumHeight(200)
        layout1.addWidget(self.data_log)

        self.tabs.addTab(tab1, "\u76f4\u63a5\u76d1\u63a7")

        # Tab 2: \u62a5\u8b66\u8bb0\u5f55
        tab2 = QWidget()
        layout2 = QVBoxLayout(tab2)

        btn_clear = QPushButton("\u6e05\u9664\u62a5\u8b66\u8bb0\u5f55")
        btn_clear.clicked.connect(self._clear_alarms)
        layout2.addWidget(btn_clear)

        self.alarm_table = QTableWidget()
        self.alarm_table.setColumnCount(4)
        self.alarm_table.setHorizontalHeaderLabels(["\u65f6\u95f4", "\u62a5\u8b66\u7c7b\u578b", "\u5f53\u524d\u503c", "\u9608\u503c"])
        self.alarm_table.horizontalHeader().setStretchLastSection(True)
        self.alarm_table.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)
        layout2.addWidget(self.alarm_table)

        self.tabs.addTab(tab2, "\u62a5\u8b66\u8bb0\u5f55")

        # Tab 3: \u7cfb\u7edf\u65e5\u5fd7
        tab3 = QWidget()
        layout3 = QVBoxLayout(tab3)
        self.log_text = QTextEdit()
        self.log_text.setReadOnly(True)
        self.log_text.setFont(QFont("Consolas", 10))
        layout3.addWidget(self.log_text)
        self.tabs.addTab(tab3, "\u7cfb\u7edf\u65e5\u5fd7")

        # Tab 4: \u53c2\u6570\u914d\u7f6e
        tab4 = QWidget()
        layout4 = QFormLayout(tab4)

        self.spin_interval = QSpinBox()
        self.spin_interval.setRange(1, 300)
        self.spin_interval.setValue(10)
        self.spin_interval.setSuffix(" \u79d2")
        layout4.addRow("\u91c7\u6837\u95f4\u9694:", self.spin_interval)

        self.spin_th = QSpinBox()
        self.spin_th.setRange(-40, 80)
        self.spin_th.setValue(25)
        self.spin_th.setSuffix(" \u2103")
        layout4.addRow("\u6e29\u5ea6\u4e0a\u9650:", self.spin_th)

        self.spin_tl = QSpinBox()
        self.spin_tl.setRange(-40, 80)
        self.spin_tl.setValue(-5)
        self.spin_tl.setSuffix(" \u2103")
        layout4.addRow("\u6e29\u5ea6\u4e0b\u9650:", self.spin_tl)

        btn_apply = QPushButton("\u5e94\u7528\u914d\u7f6e")
        btn_apply.clicked.connect(self._apply_config)
        layout4.addRow(btn_apply)

        self.tabs.addTab(tab4, "\u53c2\u6570\u914d\u7f6e")

        # Status bar
        self.sb = QStatusBar()
        self.setStatusBar(self.sb)
        self.lbl_mqtt = QLabel("MQTT: \u672a\u8fde\u63a5")
        self.lbl_mqtt.setStyleSheet("color: red; font-weight: bold;")
        self.sb.addPermanentWidget(self.lbl_mqtt)
        self.sb.showMessage("\u7cfb\u7edf\u5c31\u7eea")

    def _setup_mqtt(self):
        self.mqtt = MQTTWorker()
        self.mqtt.data_received.connect(self._on_data)
        self.mqtt.alarm_received.connect(self._on_alarm)
        self.mqtt.connected.connect(self._on_connected)
        self.mqtt.log_message.connect(self._log)
        self.mqtt.start()
        self._log("[\u7cfb\u7edf] MQTT\u5ba2\u6237\u7aef\u542f\u52a8...")

    def _on_connected(self, ok):
        if ok:
            self.lbl_mqtt.setText("MQTT: \u5df2\u8fde\u63a5")
            self.lbl_mqtt.setStyleSheet("color: green; font-weight: bold;")
        else:
            self.lbl_mqtt.setText("MQTT: \u8fde\u63a5\u5931\u8d25")

    def _on_data(self, d):
        self.latest_data = d
        temp = d.get("t", 0)
        hum = d.get("h", 0)
        lat = d.get("lat", 0)
        lon = d.get("lon", 0)
        fix = d.get("fix", 0)

        self.lbl_temp.setText(f"{temp:.1f} \u2103")
        self.lbl_hum.setText(f"{hum:.1f} %RH")

        if fix:
            self.lbl_gps.setText(f"\u5b9a\u4f4dOK (lat:{lat:.3f}, lon:{lon:.3f})")
            self.lbl_gps.setStyleSheet("background: #27ae60; color: white; padding: 15px; border-radius: 8px;")
        else:
            self.lbl_gps.setText("\u672a\u5b9a\u4f4d")
            self.lbl_gps.setStyleSheet("background: #e74c3c; color: white; padding: 15px; border-radius: 8px;")

        ts = d.get("_time", "")
        self.data_log.append(f"[{ts}] T={temp:.1f}\u2103 H={hum:.1f}% fix={fix}")
        self.data_log.verticalScrollBar().setValue(self.data_log.verticalScrollBar().maximum())

        self.data_buffers["time"].append(ts)
        self.data_buffers["temp"].append(temp)
        self.data_buffers["hum"].append(hum)
        self.data_buffers["ax"].append(d.get("ax", 0))
        self.data_buffers["ay"].append(d.get("ay", 0))
        self.data_buffers["az"].append(d.get("az", 0))

    def _on_alarm(self, d):
        now = datetime.now().strftime("%H:%M:%S")
        alarm_type = d.get("type", "\u672a\u77e5")
        value = d.get("value", "")
        threshold = d.get("threshold", "")

        row = self.alarm_table.rowCount()
        self.alarm_table.insertRow(row)
        self.alarm_table.setItem(row, 0, QTableWidgetItem(now))
        self.alarm_table.setItem(row, 1, QTableWidgetItem(alarm_type))
        self.alarm_table.setItem(row, 2, QTableWidgetItem(str(value)))
        self.alarm_table.setItem(row, 3, QTableWidgetItem(str(threshold)))

        self._log(f"[\u62a5\u8b66] {alarm_type}: value={value}, threshold={threshold}")
        self.sb.showMessage(f"\u62a5\u8b66: {alarm_type}", 5000)
        self.tabs.setCurrentIndex(1)

    def _update_ui(self):
        pass

    def _clear_alarms(self):
        self.alarm_table.setRowCount(0)

    def _apply_config(self):
        cfg = {
            "interval": self.spin_interval.value(),
            "temp_high": self.spin_th.value(),
            "temp_low": self.spin_tl.value(),
        }
        self.mqtt.publish(MQTT_CONFIG["topic_cmd"], cfg)
        self._log(f"[\u7cfb\u7edf] \u5df2\u53d1\u9001\u914d\u7f6e: {cfg}")

    def _log(self, msg):
        ts = datetime.now().strftime("[%H:%M:%S]")
        self.log_text.append(f"{ts} {msg}")

    def closeEvent(self, event):
        self.mqtt.stop()
        self.mqtt.wait()
        event.accept()


if __name__ == "__main__":
    app = QApplication(sys.argv)
    app.setStyle("Fusion")
    p = QPalette()
    p.setColor(QPalette.Window, QColor(53, 53, 53))
    p.setColor(QPalette.WindowText, Qt.white)
    p.setColor(QPalette.Base, QColor(25, 25, 25))
    p.setColor(QPalette.Text, Qt.white)
    p.setColor(QPalette.Button, QColor(53, 53, 53))
    p.setColor(QPalette.ButtonText, Qt.white)
    p.setColor(QPalette.Highlight, QColor(142, 45, 197))
    app.setPalette(p)
    w = MainWindow()
    w.show()
    sys.exit(app.exec_())
