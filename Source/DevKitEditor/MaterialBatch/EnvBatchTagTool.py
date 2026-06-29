"""
EnvBatch Tag 工具 — 编辑器使用
==============================
在 UE5 编辑器 Python 控制台执行：
    import importlib.util, sys
    spec = importlib.util.spec_from_file_location("EnvBatchTagTool",
        r"Source/DevKitEditor/MaterialBatch/EnvBatchTagTool.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)

或直接粘贴到 Python 控制台运行。
"""

import unreal
import tkinter as tk
from tkinter import ttk
from functools import partial

# ─────────────────────────────────────────────────────────────────
# Tag 定义
# ─────────────────────────────────────────────────────────────────

# 每个条目: (按钮显示名, 完整 Tag 字符串)
SECTION_SOURCE = [
    ("建筑·石材",  "EnvBatch.Source.Building_Stone"),
    ("建筑·木材",  "EnvBatch.Source.Building_Wood"),
    ("建筑·金属",  "EnvBatch.Source.Building_Metal"),
    ("地面·A区",   "EnvBatch.Source.Ground_A"),
    ("地面·B区",   "EnvBatch.Source.Ground_B"),
    ("小道具",     "EnvBatch.Source.Props_Small"),
    ("大道具",     "EnvBatch.Source.Props_Large"),
]

SECTION_PROXY = [
    ("代理·中档",  "EnvBatch.Proxy.Medium"),
    ("代理·低档",  "EnvBatch.Proxy.Low"),
]

SECTION_BAKED = [
    ("烘培地面·低档",  "EnvBatch.Baked.Ground.Low"),
    ("烘培地面·中档",  "EnvBatch.Baked.Ground.Medium"),
    ("烘培通用·低档",  "EnvBatch.Baked.General.Low"),
]

SECTION_OTHER = [
    ("排除合批",        "EnvBatch.Exclude"),
]

# ─────────────────────────────────────────────────────────────────
# Block 互斥规则
# 添加某前缀的 Tag 时，自动清除的所有前缀列表
# ─────────────────────────────────────────────────────────────────

BLOCK_RULES = {
    # Source 只能有一个组，且和 Proxy / Baked 互斥
    "EnvBatch.Source.": [
        "EnvBatch.Source.",
        "EnvBatch.Proxy.",
        "EnvBatch.Baked.",
    ],
    # Proxy 只能有一个，且和 Source / Baked 互斥
    "EnvBatch.Proxy.": [
        "EnvBatch.Source.",
        "EnvBatch.Proxy.",
        "EnvBatch.Baked.",
    ],
    # Baked 只能有一个，且和 Source / Proxy 互斥
    "EnvBatch.Baked.": [
        "EnvBatch.Source.",
        "EnvBatch.Proxy.",
        "EnvBatch.Baked.",
    ],
    # Exclude 不和其他互斥（允许同时存在，方便临时排除）
    "EnvBatch.Exclude": [],
}

# ─────────────────────────────────────────────────────────────────
# 核心操作
# ─────────────────────────────────────────────────────────────────

def _get_actors():
    return list(unreal.EditorLevelLibrary.get_selected_level_actors())

def _get_tags(actor):
    return [str(t) for t in actor.tags]

def _set_tags(actor, tags):
    actor.tags = [unreal.Name(t) for t in tags]
    actor.modify()

def _prefixes_to_clear(new_tag):
    for prefix, clears in BLOCK_RULES.items():
        if new_tag.startswith(prefix):
            return clears
    return []

def apply_tag(tag, remove_only=False):
    actors = _get_actors()
    if not actors:
        unreal.log_warning("EnvBatch Tag 工具: 未选中任何 Actor")
        return

    clears = _prefixes_to_clear(tag)
    modified = 0

    with unreal.ScopedEditorTransaction(f"EnvBatch: {'Remove' if remove_only else 'Apply'} {tag}"):
        for actor in actors:
            old = _get_tags(actor)
            # 移除 block 的前缀
            new = [t for t in old if not any(t.startswith(p) for p in clears)]
            # 加入新 tag
            if not remove_only and tag not in new:
                new.append(tag)
            if new != old:
                _set_tags(actor, new)
                modified += 1

    verb = "移除" if remove_only else "设置"
    unreal.log(f"EnvBatch: {verb} [{tag}] → {modified}/{len(actors)} 个 Actor 已修改")

def clear_all(tag_prefix="EnvBatch."):
    actors = _get_actors()
    if not actors:
        return
    with unreal.ScopedEditorTransaction("EnvBatch: Clear All"):
        for actor in actors:
            old = _get_tags(actor)
            new = [t for t in old if not t.startswith(tag_prefix)]
            if new != old:
                _set_tags(actor, new)
    unreal.log("EnvBatch: 已清除所有 EnvBatch.* Tag")

# ─────────────────────────────────────────────────────────────────
# UI
# ─────────────────────────────────────────────────────────────────

C = {
    "bg":      "#1c1c1c",
    "panel":   "#252525",
    "source":  "#1a472a",   # 深绿
    "proxy":   "#1a3a5c",   # 深蓝
    "baked":   "#5c3317",   # 深棕
    "other":   "#3a3040",   # 深紫灰
    "clear":   "#4a1010",   # 深红
    "active":  "#e9c46a",   # 亮黄（已设置状态）
    "fg":      "#dcdcdc",
    "label":   "#aaaaaa",
    "sep":     "#333333",
}

_root = None
_status_var = None
_tag_btns = {}   # tag -> Button widget


def _make_section(parent, title, entries, color):
    frame = tk.LabelFrame(
        parent, text=f"  {title}  ",
        bg=C["panel"], fg=C["label"],
        font=("微软雅黑", 8, "bold"),
        bd=1, relief="groove",
        padx=6, pady=6,
    )
    frame.pack(fill="x", padx=10, pady=(0, 6))

    cols = min(len(entries), 4)
    for i, (label, tag) in enumerate(entries):
        btn = tk.Button(
            frame, text=label,
            bg=color, fg=C["fg"],
            activebackground="#666", activeforeground="white",
            relief="flat", padx=6, pady=5,
            font=("微软雅黑", 9),
            command=partial(apply_tag, tag),
        )
        btn.grid(row=i // cols, column=i % cols, padx=3, pady=3, sticky="ew")
        frame.columnconfigure(i % cols, weight=1)

        # 右键 = 移除该 tag
        btn.bind("<Button-3>", lambda e, t=tag: apply_tag(t, remove_only=True))
        _tag_btns[tag] = btn


def _refresh_status():
    global _status_var
    if _root is None:
        return

    actors = _get_actors()
    n = len(actors)

    if n == 0:
        _status_var.set("未选中 Actor")
        for btn in _tag_btns.values():
            btn.config(relief="flat", highlightthickness=0)
    else:
        all_tags = set()
        for a in actors:
            all_tags.update(_get_tags(a))

        batch_tags = sorted(t for t in all_tags if t.startswith("EnvBatch."))
        tag_str = "  |  ".join(batch_tags) if batch_tags else "（无 EnvBatch Tag）"
        _status_var.set(f"已选中 {n} 个 Actor    {tag_str}")

        for tag, btn in _tag_btns.items():
            if tag in all_tags:
                btn.config(
                    relief="solid", bd=2,
                    highlightbackground=C["active"],
                    highlightthickness=2,
                    bg="#2a5a3a" if tag.startswith("EnvBatch.Source.") else
                       "#1a4a7a" if tag.startswith("EnvBatch.Proxy.") else
                       "#7a4a1a" if tag.startswith("EnvBatch.Baked.") else "#555",
                )
            else:
                color = (
                    C["source"] if tag.startswith("EnvBatch.Source.") else
                    C["proxy"]  if tag.startswith("EnvBatch.Proxy.")  else
                    C["baked"]  if tag.startswith("EnvBatch.Baked.")  else
                    C["other"]
                )
                btn.config(relief="flat", bd=0, highlightthickness=0, bg=color)

    _root.after(600, _refresh_status)


def open_tool():
    global _root, _status_var

    if _root is not None:
        try:
            _root.lift()
            _root.focus_force()
            return
        except Exception:
            _root = None

    _root = tk.Tk()
    _root.title("EnvBatch Tag 工具")
    _root.configure(bg=C["bg"])
    _root.resizable(False, False)

    # ── 标题 ──────────────────────────────────────────────────────
    hdr = tk.Frame(_root, bg=C["bg"])
    hdr.pack(fill="x", padx=10, pady=(12, 2))
    tk.Label(hdr, text="EnvBatch Tag 工具", bg=C["bg"], fg=C["fg"],
             font=("微软雅黑", 13, "bold")).pack(side="left")
    tk.Label(hdr, text="左键=设置  右键=仅移除该Tag", bg=C["bg"],
             fg=C["label"], font=("微软雅黑", 8)).pack(side="right", pady=4)

    tk.Frame(_root, bg=C["sep"], height=1).pack(fill="x", padx=10, pady=(0, 8))

    # ── Tag 区域 ──────────────────────────────────────────────────
    _make_section(_root, "批次来源 Source  ▸ 标记"原始"物件所属批次组", SECTION_SOURCE, C["source"])
    _make_section(_root, "代理Mesh Proxy   ▸ 标记合批后生成的代理网格",  SECTION_PROXY,  C["proxy"])
    _make_section(_root, "烘培替代 Baked   ▸ 标记低档位烘培替代物件",    SECTION_BAKED,  C["baked"])
    _make_section(_root, "其他",                                         SECTION_OTHER,  C["other"])

    # ── Block 规则说明 ──────────────────────────────────────────────
    note = tk.Label(
        _root,
        text="⚑  Block 规则：Source / Proxy / Baked 三类互斥，同类内只保留最后设置的一个",
        bg=C["bg"], fg="#888", font=("微软雅黑", 8), anchor="w",
    )
    note.pack(fill="x", padx=12, pady=(0, 6))

    tk.Frame(_root, bg=C["sep"], height=1).pack(fill="x", padx=10, pady=(0, 6))

    # ── 清除按钮 ──────────────────────────────────────────────────
    tk.Button(
        _root, text="⊗  清除选中 Actor 的所有 EnvBatch.* Tag",
        bg=C["clear"], fg="white",
        activebackground="#7a1010", relief="flat",
        padx=10, pady=7, font=("微软雅黑", 9, "bold"),
        command=clear_all,
    ).pack(fill="x", padx=10, pady=(0, 8))

    # ── 状态栏 ─────────────────────────────────────────────────────
    _status_var = tk.StringVar(value="未选中 Actor")
    status = tk.Label(
        _root, textvariable=_status_var,
        bg="#111", fg=C["label"],
        font=("微软雅黑", 8), anchor="w", padx=10, pady=5,
    )
    status.pack(fill="x", side="bottom")

    _root.after(300, _refresh_status)
    _root.protocol("WM_DELETE_WINDOW", _on_close)
    _root.mainloop()


def _on_close():
    global _root
    if _root:
        _root.destroy()
        _root = None


# 直接运行时自动打开
open_tool()
