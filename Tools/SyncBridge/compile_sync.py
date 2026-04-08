#!/usr/bin/env python3
"""
Compile Sync - 手动触发 UE5 编译，然后将编译产物同步到 P4。
仅同步 Binaries/ 中编译后新增或修改的文件，不做全量同步。

用法:
  python compile_sync.py              # 使用默认 config.json
  python compile_sync.py -c my.json  # 指定配置文件
"""

import argparse
import sys
from pathlib import Path

# 从同目录导入 SyncBridge
sys.path.insert(0, str(Path(__file__).parent))
from sync_bridge import SyncBridge


def main():
    parser = argparse.ArgumentParser(description="编译 UE5 并将产物同步到 P4")
    parser.add_argument("-c", "--config", default="config.json", help="配置文件路径")
    args = parser.parse_args()

    bridge = SyncBridge(args.config)
    ok = bridge.compile_and_sync()
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()