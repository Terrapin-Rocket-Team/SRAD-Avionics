#!/usr/bin/env python3
"""
Live BLE viewer for the STM32 AVITELEM packet stream.

Requires:
  pip install bleak matplotlib
"""

from airbrake_viz.app import run


if __name__ == "__main__":
    run()
