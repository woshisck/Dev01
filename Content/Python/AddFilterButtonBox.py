"""
AddFilterButtonBox.py — 在 EUW_CombatLog 里插入 FilterButtonBox (WrapBox)
"""
import unreal

ASSET_PATH = "/Game/UI/DebugUI/EUW_CombatLog"


def get_widget_tree(bp):
    """EditorUtilityWidgetBlueprint 的 widget_tree 需用 get_editor_property 访问。"""
    for name in ("widget_tree", "WidgetTree"):
        try:
            wt = getattr(bp, name, None)
            if wt:
                return wt
        except Exception:
            pass
    try:
        return bp.get_editor_property("widget_tree")
    except Exception:
        pass
    return None


def collect_widgets(widget_tree):
    result = []
    widget_tree.for_each_widget(lambda w: result.append(w))
    return result


def add_filter_button_box():
    widget_bp = unreal.load_asset(ASSET_PATH)
    if not widget_bp:
        unreal.log_error(f"找不到资产: {ASSET_PATH}")
        return

    unreal.log(f"资产类型: {type(widget_bp).__name__}")

    widget_tree = get_widget_tree(widget_bp)
    if not widget_tree:
        # 打印可用属性辅助诊断
        attrs = [a for a in dir(widget_bp) if not a.startswith("_")]
        unreal.log_error("无法获取 widget_tree，可用属性: " + str(attrs[:30]))
        return

    root = widget_tree.root_widget
    if not root:
        unreal.log_error("根控件为空")
        return

    unreal.log(f"根控件: {root.get_fname()} ({type(root).__name__})")

    # 检查是否已存在
    all_widgets = collect_widgets(widget_tree)
    for w in all_widgets:
        if str(w.get_fname()) == "FilterButtonBox":
            unreal.log("FilterButtonBox 已存在，跳过。")
            return

    with unreal.ScopedEditorTransaction("[CombatLog] Add FilterButtonBox"):
        wrap_box = widget_tree.construct_widget(unreal.WrapBox, "FilterButtonBox")

        if not isinstance(root, unreal.PanelWidget):
            unreal.log_error(f"根控件 {type(root).__name__} 不是 PanelWidget，无法添加子控件。")
            return

        slot = root.add_child(wrap_box)

        if isinstance(slot, unreal.CanvasPanelSlot):
            slot.set_anchors(unreal.Anchors(0.0, 0.0, 0.0, 0.0))
            slot.set_position(unreal.Vector2D(10.0, 10.0))
            slot.set_size(unreal.Vector2D(900.0, 36.0))
            slot.set_auto_size(True)
            unreal.log("CanvasPanelSlot 已配置。")

    unreal.BlueprintEditorLibrary.compile_blueprint(widget_bp)
    unreal.EditorAssetLibrary.save_asset(ASSET_PATH, only_if_is_dirty=False)
    unreal.log("✓ FilterButtonBox 添加完成！重新打开 EUW_CombatLog 即可看到。")


add_filter_button_box()
