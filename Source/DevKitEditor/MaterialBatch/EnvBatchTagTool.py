# -*- coding: utf-8 -*-
"""
EnvBatch tag tool for the Unreal Editor Python console.

Usage:
    import importlib.util
    spec = importlib.util.spec_from_file_location(
        "EnvBatchTagTool",
        r"X:\\Project\\Dev01\\Source\\DevKitEditor\\MaterialBatch\\EnvBatchTagTool.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)

The native Slate tagger is the preferred editor tool. This Python version is a
small fallback that keeps the same Source/Proxy/Baked/Exclude tag contract.
"""

from functools import partial
import re
import tkinter as tk

import unreal


ENV_BATCH_PREFIX = "EnvBatch."
MUTUALLY_EXCLUSIVE_PREFIXES = (
    "EnvBatch.Source.",
    "EnvBatch.Proxy.",
    "EnvBatch.Baked.",
)

STATIC_SOURCE_PRESETS = (
    ("标记 Source 石材建筑", "Building_Stone"),
    ("标记 Source 木制建筑", "Building_Wood"),
    ("标记 Source 金属建筑", "Building_Metal"),
    ("标记 Source 地面 A", "Ground_A"),
    ("标记 Source 地面 B", "Ground_B"),
    ("标记 Source 小道具", "Props_Small"),
    ("标记 Source 大道具", "Props_Large"),
)

BAKED_SURFACE_TAGS = (
    ("标记 Baked 地面 Mid", "EnvBatch.Baked.Ground.Mid"),
    ("标记 Baked 地面 Low", "EnvBatch.Baked.Ground.Low"),
    ("标记 Baked 墙面 Mid", "EnvBatch.Baked.Wall.Mid"),
    ("标记 Baked 墙面 Low", "EnvBatch.Baked.Wall.Low"),
)


COLORS = {
    "bg": "#1c1c1c",
    "panel": "#252525",
    "source": "#1a472a",
    "proxy": "#1a3a5c",
    "baked": "#5c3317",
    "other": "#3a3040",
    "clear": "#4a1010",
    "active": "#e9c46a",
    "fg": "#dcdcdc",
    "label": "#aaaaaa",
    "sep": "#333333",
}

_root = None
_status_var = None
_group_var = None
_tag_buttons = {}


def _selected_actors():
    return list(unreal.EditorLevelLibrary.get_selected_level_actors())


def _actor_tags(actor):
    return [str(tag) for tag in actor.tags]


def _set_actor_tags(actor, tags):
    actor.tags = [unreal.Name(tag) for tag in tags]
    actor.modify()


def _has_static_mesh_asset(actor):
    try:
        components = actor.get_components_by_class(unreal.StaticMeshComponent)
    except Exception:
        return False

    for component in components:
        try:
            if component.get_static_mesh() is not None:
                return True
        except Exception:
            continue
    return False


def _sanitize_group_name(value):
    text = (value or "").strip()
    text = re.sub(r"\s+", "_", text)
    text = re.sub(r"[^A-Za-z0-9_]+", "_", text)
    text = text.strip("_")
    return text or "Default"


def _group_name():
    value = _group_var.get() if _group_var is not None else ""
    return _sanitize_group_name(value)


def _clear_mutually_exclusive(tags):
    return [
        tag for tag in tags
        if not any(tag.startswith(prefix) for prefix in MUTUALLY_EXCLUSIVE_PREFIXES)
    ]


def _has_prefix(tags, prefix):
    return any(tag.startswith(prefix) for tag in tags)


def _asset_readiness_rows(actors):
    rows = []
    for actor in actors:
        tags = _actor_tags(actor)
        has_source = _has_prefix(tags, "EnvBatch.Source.")
        has_proxy = _has_prefix(tags, "EnvBatch.Proxy.")
        has_baked = _has_prefix(tags, "EnvBatch.Baked.")
        has_exclude = "EnvBatch.Exclude" in tags
        active_layer_tags = sum(1 for value in (has_source, has_proxy, has_baked) if value)
        has_mesh = _has_static_mesh_asset(actor)

        if has_exclude:
            role = "Excluded"
            ready = True
            status = "NotRequiredExcluded"
        elif active_layer_tags > 1:
            role = "Conflict"
            ready = False
            status = "BlockedByLayerTagConflict"
        elif has_source:
            role = "Source"
            ready = has_mesh
            status = "ReadyGeneratedProxy" if has_mesh else "MissingSourceMesh"
        elif has_proxy:
            role = "Proxy"
            ready = False
            status = "MissingSourceAssetReference"
            if not has_mesh:
                status = "MissingSourceAssetReferenceAndProxyMesh"
        elif has_baked:
            role = "Baked"
            ready = True
            status = "NotRequiredBaked"
        else:
            role = "Unassigned"
            ready = False
            status = "MissingLayerTag"

        rows.append({
            "actor": actor.get_name(),
            "role": role,
            "ready": ready,
            "status": status,
            "has_mesh": has_mesh,
            "source_lod": 0,
            "proxy_lod": 1,
            "tags": tags,
        })
    return rows


def _asset_readiness_summary(actors):
    if not actors:
        return "Source/Proxy 资产准备度：请选择静态环境 Actor，用于检查 SourceLOD0 / ProxyLOD1 配对。"

    rows = _asset_readiness_rows(actors)
    source_rows = [row for row in rows if row["role"] == "Source"]
    proxy_rows = [row for row in rows if row["role"] == "Proxy"]
    baked_or_excluded = [row for row in rows if row["role"] in ("Baked", "Excluded")]
    conflict_rows = [row for row in rows if row["role"] == "Conflict"]
    unassigned_rows = [row for row in rows if row["role"] == "Unassigned"]
    source_ready = [row for row in source_rows if row["ready"]]
    proxy_missing_mesh = [row for row in proxy_rows if not row["has_mesh"]]

    return (
        "Source/Proxy 资产准备度："
        f"Source 可用 {len(source_ready)}/{len(source_rows)}，"
        f"自动代理 fallback {len(source_rows)}，"
        f"Proxy 缺少显式 Source 配置 {len(proxy_rows)}/{len(proxy_rows)}，"
        f"Proxy 缺少网格 {len(proxy_missing_mesh)}，"
        f"Baked/Excluded 不要求配对 {len(baked_or_excluded)}，"
        f"冲突 {len(conflict_rows)}，"
        f"未分配 {len(unassigned_rows)}。"
        "默认规则：SourceLOD0，ProxyLOD1。"
    )


def print_asset_readiness():
    actors = _selected_actors()
    if not actors:
        unreal.log_warning("EnvBatch: no selected actors.")
        return

    unreal.log("EnvBatch Source/Proxy asset readiness detail:")
    for row in _asset_readiness_rows(actors):
        tag_text = ", ".join(sorted(row["tags"])) if row["tags"] else "(none)"
        unreal.log(
            " - "
            f"{row['actor']}: role={row['role']}, ready={row['ready']}, "
            f"status={row['status']}, hasStaticMesh={row['has_mesh']}, "
            f"SourceLOD{row['source_lod']} ProxyLOD{row['proxy_lod']}, tags={tag_text}"
        )
    _refresh_status_once()


def apply_tag(tag, remove_only=False):
    actors = _selected_actors()
    if not actors:
        unreal.log_warning("EnvBatch: no selected actors.")
        return

    modified = 0
    with unreal.ScopedEditorTransaction(
        f"EnvBatch: {'Remove' if remove_only else 'Apply'} {tag}"
    ):
        for actor in actors:
            old_tags = _actor_tags(actor)
            if remove_only:
                new_tags = [existing for existing in old_tags if existing != tag]
            elif tag.startswith(MUTUALLY_EXCLUSIVE_PREFIXES):
                new_tags = _clear_mutually_exclusive(old_tags)
                if tag not in new_tags:
                    new_tags.append(tag)
            else:
                new_tags = list(old_tags)
                if tag not in new_tags:
                    new_tags.append(tag)

            if new_tags != old_tags:
                _set_actor_tags(actor, new_tags)
                modified += 1

    unreal.log(f"EnvBatch: {'removed' if remove_only else 'applied'} {tag} on {modified}/{len(actors)} actor(s).")
    _refresh_status_once()


def apply_source_group(group_name=None):
    group = _sanitize_group_name(group_name or _group_name())
    apply_tag(f"EnvBatch.Source.{group}")


def apply_proxy_mid():
    apply_tag(f"EnvBatch.Proxy.{_group_name()}.Mid")


def apply_proxy_low():
    apply_tag(f"EnvBatch.Proxy.{_group_name()}.Low")


def clear_all(tag_prefix=ENV_BATCH_PREFIX):
    actors = _selected_actors()
    if not actors:
        unreal.log_warning("EnvBatch: no selected actors.")
        return

    modified = 0
    with unreal.ScopedEditorTransaction("EnvBatch: Clear All"):
        for actor in actors:
            old_tags = _actor_tags(actor)
            new_tags = [tag for tag in old_tags if not tag.startswith(tag_prefix)]
            if new_tags != old_tags:
                _set_actor_tags(actor, new_tags)
                modified += 1

    unreal.log(f"EnvBatch: cleared {tag_prefix} tags on {modified}/{len(actors)} actor(s).")
    _refresh_status_once()


def _button(parent, text, color, command):
    btn = tk.Button(
        parent,
        text=text,
        bg=color,
        fg=COLORS["fg"],
        activebackground="#666666",
        activeforeground="white",
        relief="flat",
        padx=6,
        pady=5,
        font=("Microsoft YaHei", 9),
        command=command,
    )
    btn.pack(side="left", fill="x", expand=True, padx=3, pady=3)
    return btn


def _section(parent, title):
    frame = tk.LabelFrame(
        parent,
        text=f"  {title}  ",
        bg=COLORS["panel"],
        fg=COLORS["label"],
        font=("Microsoft YaHei", 8, "bold"),
        bd=1,
        relief="groove",
        padx=6,
        pady=6,
    )
    frame.pack(fill="x", padx=10, pady=(0, 6))
    return frame


def _make_source_section(parent):
    frame = _section(parent, "Source 原始件")
    for label, group in STATIC_SOURCE_PRESETS:
        btn = _button(frame, label, COLORS["source"], partial(apply_source_group, group))
        _tag_buttons[f"EnvBatch.Source.{group}"] = btn

    custom_row = tk.Frame(frame, bg=COLORS["panel"])
    custom_row.pack(fill="x", padx=0, pady=(4, 0))
    _button(custom_row, "标记 Source <Group>", COLORS["source"], apply_source_group)


def _make_proxy_section(parent):
    frame = _section(parent, "Proxy 代理件")
    _button(frame, "标记 Proxy <Group> Mid", COLORS["proxy"], apply_proxy_mid)
    _button(frame, "标记 Proxy <Group> Low", COLORS["proxy"], apply_proxy_low)


def _make_baked_section(parent):
    frame = _section(parent, "Baked 静态烘焙面")
    for label, tag in BAKED_SURFACE_TAGS:
        btn = _button(frame, label, COLORS["baked"], partial(apply_tag, tag))
        _tag_buttons[tag] = btn


def _make_other_section(parent):
    frame = _section(parent, "其他")
    btn = _button(frame, "排除合批", COLORS["other"], partial(apply_tag, "EnvBatch.Exclude"))
    _tag_buttons["EnvBatch.Exclude"] = btn
    _button(frame, "打印资产准备度", COLORS["other"], print_asset_readiness)
    _button(frame, "清除 EnvBatch.*", COLORS["clear"], clear_all)


def _refresh_status_once():
    if _status_var is None:
        return

    actors = _selected_actors()
    if not actors:
        _status_var.set("未选择 Actor。")
        active_tags = set()
    else:
        active_tags = set()
        for actor in actors:
            active_tags.update(tag for tag in _actor_tags(actor) if tag.startswith(ENV_BATCH_PREFIX))
        tag_text = " | ".join(sorted(active_tags)) if active_tags else "(无 EnvBatch 标记)"
        _status_var.set(f"当前选择 Actor 数量：{len(actors)}    {tag_text}\n{_asset_readiness_summary(actors)}")

    for tag, btn in _tag_buttons.items():
        if tag in active_tags:
            btn.config(relief="solid", bd=2, highlightbackground=COLORS["active"], highlightthickness=2)
        else:
            btn.config(relief="flat", bd=0, highlightthickness=0)


def _refresh_status_loop():
    if _root is None:
        return
    _refresh_status_once()
    _root.after(600, _refresh_status_loop)


def open_tool():
    global _root, _status_var, _group_var

    if _root is not None:
        try:
            _root.lift()
            _root.focus_force()
            return
        except Exception:
            _root = None

    _root = tk.Tk()
    _root.title("环境合批标记")
    _root.configure(bg=COLORS["bg"])
    _root.resizable(False, False)

    header = tk.Frame(_root, bg=COLORS["bg"])
    header.pack(fill="x", padx=10, pady=(12, 2))
    tk.Label(
        header,
        text="环境合批标记",
        bg=COLORS["bg"],
        fg=COLORS["fg"],
        font=("Microsoft YaHei", 13, "bold"),
    ).pack(side="left")
    tk.Label(
        header,
        text="Source / Proxy / Baked 互斥。",
        bg=COLORS["bg"],
        fg=COLORS["label"],
        font=("Microsoft YaHei", 8),
    ).pack(side="right", pady=4)

    tk.Frame(_root, bg=COLORS["sep"], height=1).pack(fill="x", padx=10, pady=(0, 8))

    group_row = tk.Frame(_root, bg=COLORS["bg"])
    group_row.pack(fill="x", padx=10, pady=(0, 8))
    tk.Label(group_row, text="分组：", bg=COLORS["bg"], fg=COLORS["label"]).pack(side="left")
    _group_var = tk.StringVar(value="Corridor_01b")
    tk.Entry(group_row, textvariable=_group_var, width=28).pack(side="left", fill="x", expand=True, padx=(6, 0))

    _make_source_section(_root)
    _make_proxy_section(_root)
    _make_baked_section(_root)
    _make_other_section(_root)

    _status_var = tk.StringVar(value="未选择 Actor。")
    tk.Label(
        _root,
        textvariable=_status_var,
        bg="#111111",
        fg=COLORS["label"],
        font=("Microsoft YaHei", 8),
        anchor="w",
        padx=10,
        pady=5,
    ).pack(fill="x", side="bottom")

    _root.after(300, _refresh_status_loop)
    _root.protocol("WM_DELETE_WINDOW", _on_close)
    _root.mainloop()


def _on_close():
    global _root
    if _root is not None:
        _root.destroy()
        _root = None


open_tool()
