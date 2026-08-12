"""Substance 3D Painter tools for the Dev01 character texture contract."""

import os
import traceback

from PySide2 import QtGui, QtWidgets

import substance_painter.display
import substance_painter.export
import substance_painter.logging
import substance_painter.project
import substance_painter.textureset
import substance_painter.ui


PLUGIN_NAME = "Dev01 Character Tools"
DIFFUSE_BIAS_LABEL = "Diffuse Bias"
NEUTRAL_BYTE = 128
_UI_ELEMENTS = []


def _message(kind, title, text):
    parent = substance_painter.ui.get_main_window()
    if kind == "error":
        QtWidgets.QMessageBox.critical(parent, title, text)
    elif kind == "warning":
        QtWidgets.QMessageBox.warning(parent, title, text)
    else:
        QtWidgets.QMessageBox.information(parent, title, text)


def _require_project():
    if not substance_painter.project.is_open():
        raise RuntimeError("请先打开一个 Substance Painter 项目。")


def apply_neutral_preview_display():
    """Keep display mapping stable; shader lighting itself is parameter-free."""
    try:
        _require_project()
        try:
            substance_painter.display.set_tone_mapping(
                substance_painter.display.ToneMappingFunction.ACES)
            tone_message = "已将 Painter 显示映射设为 ACES。"
        except RuntimeError:
            tone_message = "当前项目启用了颜色管理，沿用项目的显示变换。"

        _message(
            "info",
            PLUGIN_NAME,
            tone_message
            + "\n\n请在 Shader Settings 中选择 Dev01_StylizedCharacter。"
            + "\nShader 使用固定能量的中性白光，不采样 Painter 环境球。"
            + "\n只通过 Key azimuth / Key elevation 调整方向；色温、强度和曝光不可调。")
    except Exception as error:  # Painter must survive a plug-in action failure.
        substance_painter.logging.error(traceback.format_exc())
        _message("error", PLUGIN_NAME, str(error))


def _simple_stacks():
    """Return non-layered stacks; layered-material stacks are intentionally rejected."""
    _require_project()
    result = []
    unsupported = []
    for texture_set in substance_painter.textureset.all_texture_sets():
        stacks = texture_set.all_stacks()
        if len(stacks) != 1 or stacks[0].name():
            unsupported.append(texture_set.name())
            continue
        result.append(stacks[0])
    if unsupported:
        raise RuntimeError(
            "Dev01 角色导出只支持普通单 Stack Texture Set；以下材质使用了 Material Layering：\n- "
            + "\n- ".join(unsupported))
    if not result:
        raise RuntimeError("当前项目没有可导出的 Texture Set。")
    return result


def _required_channels():
    channel_type = substance_painter.textureset.ChannelType
    channel_format = substance_painter.textureset.ChannelFormat
    return (
        (channel_type.BaseColor, channel_format.sRGB8, None),
        (channel_type.Normal, channel_format.RGB16, None),
        (channel_type.Roughness, channel_format.L8, None),
        (channel_type.Metallic, channel_format.L8, None),
        (channel_type.Specularlevel, channel_format.L8, None),
        (channel_type.User0, channel_format.L8, DIFFUSE_BIAS_LABEL),
    )


def _validate_user0(stack):
    channel_type = substance_painter.textureset.ChannelType
    if not stack.has_channel(channel_type.User0):
        return
    label = stack.get_channel(channel_type.User0).label()
    if label and label != DIFFUSE_BIAS_LABEL:
        raise RuntimeError(
            "Texture Set '{}' 的 User0 已命名为 '{}'，不能安全覆盖。\n"
            "请迁移该通道，或确认后将其改名为 '{}'。".format(
                stack.material().name(), label, DIFFUSE_BIAS_LABEL))


def prepare_channels():
    """Add only missing channels. Never overwrite an existing User0 semantic."""
    try:
        stacks = _simple_stacks()
        additions = []
        for stack in stacks:
            _validate_user0(stack)
            for channel_type, channel_format, label in _required_channels():
                if stack.has_channel(channel_type):
                    continue
                stack.add_channel(channel_type, channel_format, label)
                additions.append("{}: {}".format(
                    stack.material().name(), label or channel_type.name))

        if additions:
            _message(
                "info",
                PLUGIN_NAME,
                "已补齐 Dev01 角色通道：\n- " + "\n- ".join(additions)
                + "\n\nUser0 显示名为 Diffuse Bias；未绘制区域由 Shader/导出按中性值 0.5 处理。")
        else:
            _message("info", PLUGIN_NAME, "当前所有 Texture Set 已满足 Dev01 角色通道要求。")
    except Exception as error:
        substance_painter.logging.error(traceback.format_exc())
        _message("error", PLUGIN_NAME, str(error))


def _validate_channels(stacks):
    missing = []
    for stack in stacks:
        _validate_user0(stack)
        for channel_type, _channel_format, label in _required_channels():
            if not stack.has_channel(channel_type):
                missing.append("{}: {}".format(
                    stack.material().name(), label or channel_type.name))
    if missing:
        raise RuntimeError(
            "缺少导出所需通道。请先运行 Prepare Character Channels：\n- "
            + "\n- ".join(missing))


def _channel(dest, src, map_type, map_name):
    return {
        "destChannel": dest,
        "srcChannel": src,
        "srcMapType": map_type,
        "srcMapName": map_name,
    }


def build_export_config(output_directory, stacks):
    """Build the three-map contract without a machine-local .spexp preset."""
    common_parameters = {
        "fileFormat": "png",
        "bitDepth": "8",
        "dithering": False,
        "paddingAlgorithm": "infinite",
    }
    maps = [
        {
            "fileName": "T_$textureSet_Color",
            "channels": [
                _channel("R", "R", "documentMap", "basecolor"),
                _channel("G", "G", "documentMap", "basecolor"),
                _channel("B", "B", "documentMap", "basecolor"),
            ],
            "parameters": dict(common_parameters),
        },
        {
            "fileName": "T_$textureSet_Normal",
            "channels": [
                _channel("R", "R", "virtualMap", "Normal_DirectX"),
                _channel("G", "G", "virtualMap", "Normal_DirectX"),
                _channel("B", "L", "documentMap", "user0"),
                # Keep authored coverage temporarily. _finalize_normal_map()
                # uses it to restore unpainted Diffuse Bias to neutral B=128.
                _channel("A", "A", "documentMap", "user0"),
            ],
            "parameters": dict(common_parameters, keepAlpha=True),
        },
        {
            "fileName": "T_$textureSet_MixMap",
            "channels": [
                _channel("R", "L", "documentMap", "specularlevel"),
                _channel("G", "L", "documentMap", "roughness"),
                _channel("B", "L", "documentMap", "metallic"),
            ],
            "parameters": dict(common_parameters),
        },
    ]
    return {
        "exportPath": os.path.normpath(output_directory).replace("\\", "/"),
        "exportShaderParams": False,
        "defaultExportPreset": "Dev01 Character 3-Map",
        "exportPresets": [{"name": "Dev01 Character 3-Map", "maps": maps}],
        "exportList": [
            {"rootPath": str(stack), "exportPreset": "Dev01 Character 3-Map"}
            for stack in stacks
        ],
    }


def _finalize_normal_map(file_path):
    """Resolve unpainted Diffuse Bias to 0.5 and remove coverage alpha."""
    image = QtGui.QImage(file_path)
    if image.isNull():
        raise RuntimeError("无法读取刚导出的 Normal 贴图：{}".format(file_path))

    rgba = image.convertToFormat(QtGui.QImage.Format_RGBA8888)
    pixels = rgba.bits()
    for alpha_index in range(3, len(pixels), 4):
        coverage = pixels[alpha_index]
        bias_index = alpha_index - 1
        if coverage == 0:
            pixels[bias_index] = NEUTRAL_BYTE
        elif coverage != 255:
            authored = pixels[bias_index]
            pixels[bias_index] = (
                authored * coverage + NEUTRAL_BYTE * (255 - coverage) + 127
            ) // 255
        pixels[alpha_index] = 255
    del pixels

    rgb = rgba.convertToFormat(QtGui.QImage.Format_RGB888)
    temporary_path = file_path + ".dev01-tmp"
    try:
        if not rgb.save(temporary_path, "PNG"):
            raise RuntimeError("无法写入规范化后的 Normal 贴图：{}".format(file_path))
        os.replace(temporary_path, file_path)
    finally:
        if os.path.exists(temporary_path):
            os.remove(temporary_path)


def validate_export():
    try:
        stacks = _simple_stacks()
        _validate_channels(stacks)
        output_directory = substance_painter.export.get_default_export_path()
        config = build_export_config(output_directory, stacks)
        textures = substance_painter.export.list_project_textures(config)
        flattened = [path for paths in textures.values() for path in paths]
        _message(
            "info",
            PLUGIN_NAME,
            "导出配置有效，将生成 {} 张贴图：\n\n{}".format(
                len(flattened), "\n".join(flattened)))
    except Exception as error:
        substance_painter.logging.error(traceback.format_exc())
        _message("error", PLUGIN_NAME, str(error))


def export_textures():
    try:
        stacks = _simple_stacks()
        _validate_channels(stacks)
        parent = substance_painter.ui.get_main_window()
        start_directory = substance_painter.export.get_default_export_path()
        output_directory = QtWidgets.QFileDialog.getExistingDirectory(
            parent, "Export Dev01 Character Textures", start_directory)
        if not output_directory:
            return

        config = build_export_config(output_directory, stacks)
        expected = substance_painter.export.list_project_textures(config)
        result = substance_painter.export.export_project_textures(config)
        if result.status != substance_painter.export.ExportStatus.Success:
            raise RuntimeError(result.message)

        flattened = [path for paths in result.textures.values() for path in paths]
        normal_maps = [
            path for path in flattened
            if os.path.splitext(path)[0].endswith("_Normal")
        ]
        if len(normal_maps) != len(stacks):
            raise RuntimeError(
                "导出结果中的 Normal 贴图数量异常：预期 {}，实际 {}。".format(
                    len(stacks), len(normal_maps)))
        for normal_map in normal_maps:
            _finalize_normal_map(normal_map)

        _message(
            "info",
            PLUGIN_NAME,
            "Dev01 角色贴图导出成功（{} / {}）。\n"
            "T_Normal.B 已按图层覆盖度补为中性 0.5，并移除临时 Alpha。\n\n{}".format(
                len(flattened),
                sum(len(paths) for paths in expected.values()),
                "\n".join(flattened)))
    except Exception as error:
        substance_painter.logging.error(traceback.format_exc())
        _message("error", PLUGIN_NAME, str(error))


def _add_action(menu, label, callback):
    action = QtWidgets.QAction(label, menu)
    action.triggered.connect(callback)
    menu.addAction(action)
    return action


def start_plugin():
    menu = QtWidgets.QMenu("Dev01 Character")
    menu.setObjectName("dev01-character-tools-menu")
    _add_action(menu, "Apply Neutral Character Preview Display", apply_neutral_preview_display)
    menu.addSeparator()
    _add_action(menu, "Prepare Character Channels", prepare_channels)
    _add_action(menu, "Validate 3-Map Export", validate_export)
    _add_action(menu, "Export T_Color / T_Normal / T_MixMap", export_textures)
    substance_painter.ui.add_menu(menu)
    _UI_ELEMENTS.append(menu)
    substance_painter.logging.info("{} loaded".format(PLUGIN_NAME))


def close_plugin():
    while _UI_ELEMENTS:
        substance_painter.ui.delete_ui_element(_UI_ELEMENTS.pop())
