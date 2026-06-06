# 符文序列帧 VFX 配置说明

## 当前状态

当前序列帧 Flipbook 特效已停用。运行时 FA 不再引用 `Play Flipbook VFX`，燃烧和中毒表现先统一回到 `Play Niagara` 节点。

已生成的序列帧贴图仅作为备用/归档资源保留，不作为当前验收配置。

## 保留资源

源图位置：

```text
X:\Project\Dev01\SourceArt\512RuneVFX\PreviewFrames
```

UE 资源位置：

```text
/Game/Docs/BuffDocs/V2-RuneCard/VFX/Textures
```

| 文件 | 当前用途 |
| --- | --- |
| `T_Rune512_VFX_Burn_Hit.png` | 备用，不参与当前 FA |
| `T_Rune512_VFX_MoonBlade_Flight.png` | 备用预览 |
| `T_Rune512_VFX_Moonlight_Hit.png` | 备用预览 |
| `T_Rune512_VFX_Poison_Hit.png` | 备用预览 |
| `T_Rune512_VFX_Poison_Spread.png` | 备用预览 |
| `T_Rune512_VFX_ShieldPierce.png` | 备用预览 |

## 当前配置方式

不要在 512 燃烧/中毒验收 FA 中配置 `Play Flipbook VFX`。当前应使用：

| 效果 | 节点 | 资源 |
| --- | --- | --- |
| 燃烧命中/持续表现 | `Play Niagara` | `NS_Fire_Floor` |
| 中毒命中 | `Play Niagara` | `NS_Smoke_7_acid` |
| 中毒扩散 | `Play Niagara` | `NS_Smoke_7_acid` |

持续伤害仍由 GE 负责，表现节点只负责视觉。

## 后续恢复条件

只有当序列帧美术质量和朝向/贴附表现验收通过后，才重新在 FA 中启用 `Play Flipbook VFX`。
