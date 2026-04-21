# 符文启迪动效 · Seedance 提示词文档

> 用途：背包符文格子"被注视"启迪动效参考视频生成  
> 风格定位：维多利亚玻璃器皿 × 克苏鲁宇宙恐怖 × 液态玻璃UI

---

## 主提示词（中文版）

```
一个维多利亚时期风格的液态玻璃容器特写，类似炼金术烧瓶或水晶球。容器内壁从某个角落缓缓凝结水汽，如同有什么东西在玻璃内侧轻轻呼吸。随后，容器内静止的液体无声地开始向上逆流，违反重力，极其缓慢而庄重。液体逆流的同时，雾气在容器深处慢慢凝结出一个模糊的形状——可能是眼睛的轮廓，可能是某种非欧几何的对称图案，始终差一点看清楚。在所有模糊中，有极短的一瞬间，雾中的形状骤然清晰，随后立刻重新消散。最终容器恢复平静，玻璃重新透明，但液体偶尔会有一小段再次向错误方向流动，然后停下。整体氛围克制、庄重、令人不安，如同博物馆展柜里的标本悄悄注视着观察者。色调为深蓝绿、祖母绿、冷白，光线柔和而阴沉。
```

---

## 主提示词（English 版，建议优先使用）

```
Extreme close-up of a Victorian-style liquid glass alchemical flask or crystal orb. The inner wall of the glass container slowly fogs up from one corner, as if something breathes against the glass from inside. Then, the still liquid inside begins to flow silently upward, defying gravity, impossibly slow and deliberate. As the liquid rises, a shape begins to condense within the mist — perhaps the outline of an eye, perhaps a non-Euclidean geometric pattern — always almost visible, never fully resolved. For a single brief moment, the shape inside the mist snaps into sharp focus, then immediately dissolves back into ambiguity. Finally, the container returns to apparent stillness, the glass clears — but the liquid occasionally shifts in the wrong direction for just a moment, then stops. The overall mood is restrained, ceremonial, and deeply unsettling, like a museum specimen quietly observing the viewer. Color palette: deep teal, emerald green, cold white. Soft, dim lighting.
```

---

## 分段提示词（如需逐段生成再拼接）

### 段一：起雾
```
Victorian alchemical glass flask, close-up. Inner glass wall slowly condensing with fog from one corner, like breath on cold glass from inside. Very slow, deliberate motion. Deep teal and cold white tones. Still and quiet but subtly wrong.
```

### 段二：液体逆流
```
Inside a Victorian crystal flask, a thin stream of luminous liquid begins flowing upward against gravity. Silent, unhurried, like it has its own intention. The rest of the liquid remains completely still. Eerie and ceremonial. Deep green-blue palette.
```

### 段三：雾中凝形
```
Inside a foggy Victorian glass orb, a shape begins forming in the mist. The outline of an eye, or an impossible geometric pattern. Always almost visible, always just out of focus. A single frame of sharp clarity, then dissolving back. Deeply unsettling. Emerald and cold white tones.
```

### 段四：稳定态（循环用）
```
A Victorian liquid glass container, seemingly calm and transparent. Occasionally, a thin stream of liquid shifts in the wrong direction for one second, then stops. No other movement. The stillness itself feels watched. Subtle, looping, deeply uncanny.
```

---

## 负面提示词（Negative Prompt）

```
bright colors, neon, explosion, fast motion, cartoon, anime, cheerful, warm tones, orange, red, fire, aggressive, loud, obvious monster, clear creature, full body creature visible
```

---

## 参数建议

| 参数 | 建议值 | 说明 |
|------|--------|------|
| 时长 | 6–8s | 主流程完整一遍 |
| 画面比例 | 1:1 或 9:16 | 背包格子参考用方形更直观 |
| 运镜 | 固定机位 | 不要推拉，保持"展柜凝视"感 |
| 风格关键词 | `cinematic`, `macro photography`, `practical effects` | 避免过度数字感 |

---

## 设计备注

- **"差一点看清楚"原则**：雾中的眼睛/形状永远不要完全清晰，保持玩家怀疑自己是否看到了什么
- **克制优先**：动作幅度越小越有效，不要让任何一段动作看起来像"特效展示"
- **维多利亚仪式感**：每个阶段之间有明确的停顿感，像在进行某种有步骤的程序
- **稳定态循环**：最终状态需要可以无缝循环，用于背包常驻展示

