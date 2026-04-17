# 新手引导 — 图像生成提示词

> 工具：Nano Banana  
> 用途：引导界面 UI 概念图 / 参考图  
> 游戏风格：暗色调魂系俯视角 Roguelite，星骸/遗迹美术风格，霓虹光效符文

---

## 全局风格基底（每条提示词结尾附加）

```
dark fantasy roguelite UI, top-down action game aesthetic, dark slate background with faint stone texture, neon rune glow accents in teal and amber, clean modern game interface with mystical overlay, cinematic moody lighting, 16:9 game screenshot composition
```

---

## 节点 A — 武器世界浮窗

> 场景中悬浮在武器 Actor 上方的世界空间 UI 卡片

### A-1 大剑浮窗

```
In-world floating UI tooltip card hovering above a glowing greatsword on the ground, dark stone floor, the card has a dark semi-transparent panel with golden border, showing weapon name "大剑" in large bold text at top, below it combat style tag "格挡 · 韧性" in small amber label, a 5-cell horizontal grid showing activation zone with 3 center cells lit in teal glow and 2 outer cells dim, below that two rune icons with names listed, bottom of card shows a glowing "按 E 拾取" prompt with pulsing animation indicator, the greatsword on ground emits a warm amber light, top-down perspective with slight tilt, dark fantasy roguelite UI, neon rune glow accents in teal and amber, 16:9 game screenshot
```

### A-2 匕首浮窗

```
In-world floating UI tooltip card hovering above a glowing dagger on the ground, dark stone floor, dark semi-transparent panel with silver-blue border, weapon name "匕首" in bold text, combat style tag "闪避 · 速度" in blue label, a 5-cell grid showing activation zone with diagonal cells lit in electric blue glow and center cells dim, two rune icons listed below, bottom "按 E 拾取" prompt, the dagger emits cool blue-white light on ground, top-down perspective, dark fantasy roguelite UI, neon blue rune glow, 16:9 game screenshot
```

---

## 节点 B — 打断① 背包引导弹窗

> 玩家拾取武器后强制打开背包，弹窗指向激活区

### B-1 背包界面全图（含弹窗）

```
Game UI screenshot of a backpack grid system, dark background, a 7x7 grid of cells in the center with a glowing teal activation zone in the inner 3x3 area, two rune pieces placed in the grid — one glowing brightly inside the activation zone and one dim outside it, on the right side a tutorial popup panel appears with a glowing arrow pointing at the activation zone, popup shows bold title text and two lines of body text, a confirm button at the bottom glows softly, the overall composition shows the backpack grid on the left two-thirds and the popup on the right third, dark slate UI with teal and amber accents, roguelite game interface, 16:9
```

### B-2 弹窗特写

```
Close-up of a game tutorial popup panel, dark semi-transparent background with rounded corners and a subtle teal border glow, at the top a small icon of a rune inside a glowing circle, bold white title text "配置你的符文" below the icon, two lines of smaller body text in light gray describing activation zones, at the bottom a wide confirm button with amber gradient and text "知道了", a glowing arrow indicator pointing left toward an off-screen grid, clean minimal dark UI design, roguelite game aesthetic, neon accent lighting
```

---

## 节点 C — 战斗阶段背包只读模式

> 战斗中打开背包，激活区有热度 VFX，符文无法操作

### C-1 Phase 1 状态（低热度·蓝色）

```
Game UI backpack grid during combat phase, dark background with red HUD elements visible at screen edges indicating active combat, the 7x7 backpack grid shows a small inner activation zone glowing in cool blue, activation zone border has a subtle pulsing light effect, runes inside the zone glow softly, runes outside are 50% transparent and desaturated, small heat gauge in corner shows Phase 1 of 3, a faint overlay text reads "战斗中" at top of panel, overall color tone is dark blue-gray, roguelite game UI, 16:9 screenshot
```

### C-2 Phase 3 状态（满热度·橙红）

```
Game UI backpack grid during combat phase at maximum heat, the activation zone has expanded to cover 5x5 cells and glows intensely in orange-red with particle embers floating upward from the border, all runes inside the expanded zone shine brightly with individual amber halos, runes outside are still dim and semi-transparent, heat gauge in corner shows Phase 3 fully filled in orange-red, the entire backpack panel has a warm orange tint, dramatic contrast between lit and unlit runes, intense magical atmosphere, dark fantasy roguelite UI, 16:9
```

### C-3 符文锁定反馈（红色闪光帧）

```
Close-up of two game rune cells in a backpack grid, the player is attempting to interact with a rune during combat — the selected rune has a bright red border flashing around it with a jagged energy effect, the rune icon inside appears to shake slightly (motion blur on edges), a red prohibition symbol fades in at top-right of the rune cell, surrounding cells remain normal and dim, dark UI background, the red glow is intense and immediate, dramatic warning visual feedback, roguelite game interface close-up, square composition
```

---

## 节点 D — 战斗后三选一界面

> 战斗结束后掉落符文，玩家三选一

### D-1 三选一完整界面

```
Game loot selection screen showing three rune cards displayed horizontally in the center of a dark screen, each card is a tall vertical panel with rounded top, the middle card is highlighted with a bright teal border indicating current selection, each card shows a large rune icon at top (mystical glowing symbol), a rune name in bold text, a rarity indicator dot in amber, and two lines of description text below, background is very dark with faint stone texture and scattered star-dust particles, at the bottom a prompt shows gamepad button prompts for navigation, the overall mood is rewarding and ceremonial, dark fantasy roguelite UI, 16:9
```

### D-2 符文卡片特写（选中状态）

```
Close-up of a single selected rune card in a game loot selection UI, vertical card shape with rounded corners, bright teal glowing border with animated shimmer effect, at the top a large circular icon containing an intricate mystical rune symbol glowing amber-orange on dark background, below the icon the rune name "击杀爆炸" in bold white text, a small amber star rarity indicator, below that two lines of flavor/effect text in gray, at the very bottom a semi-transparent effect tag badge, the card background is dark with a very subtle radial glow behind the rune icon, dark fantasy game card design, elegant and mystical
```

### D-3 三选一手柄导航状态

```
Game loot selection screen with three rune cards, the leftmost card is focused with a glowing highlight, a D-pad navigation hint icon floats below the cards with left and right arrows, an "A" button confirm prompt glows below, the focused card is slightly larger than the other two and has a brighter border, the unfocused cards are slightly dimmed at 80% brightness, dark background with ambient blue particle effects, controller-friendly UI layout, roguelite game screenshot, 16:9
```

---

## 节点 E — 打断② 战斗后背包引导弹窗

> 三选一后自动打开背包，新符文在待放置区高亮

### E-1 背包界面（待放置符文高亮）

```
Game UI backpack grid screen after loot selection, the main 7x7 grid is visible on the left, on the right side there is a pending runes area showing one newly obtained rune icon with a bright pulsing amber glow and slight upward floating animation to indicate it is unplaced, a tutorial popup on the right middle shows bold title text and a body message with an arrow pointing to the pending rune, a confirm button glows at the bottom of the popup, the pending rune has a subtle particle shimmer around it, dark background with the usual teal activation zone visible in the grid, roguelite backpack UI, 16:9
```

### E-2 弹窗特写（放置提示）

```
Close-up of a game tutorial popup panel, dark semi-transparent panel with amber border glow (different from the earlier teal-bordered popup to indicate progression), icon at top shows a rune moving from a holding area into a grid cell, bold white title "放置你的新符文", two lines of body text in light gray reminding the player to place before the next level, wide confirm button with teal gradient at bottom "知道了", a glowing arrow points left toward an off-screen backpack grid, clean game UI design, dark fantasy roguelite aesthetic
```

---

## 补充参考：游戏整体 UI 风格板

### 整体 HUD 参考

```
Full game HUD screenshot of a dark fantasy top-down action roguelite, center of screen shows a stone dungeon floor with the player character from above, at the top a horizontal heat gauge bar divided into 4 segments filling left to right with a gradient from blue to orange-red, at bottom-left a circular health bar with red fill, bottom-center shows attack combo counter in bold numbers, the UI elements are minimal and dark with glowing accents in teal and amber, corners have vignette darkening, the overall feel is soul-like with a mystical rune aesthetic, 16:9 game screenshot
```

### 背包 UI 风格参考（无引导）

```
Clean game backpack inventory UI, dark semi-transparent full-screen panel, center shows a 7x7 grid of square cells with subtle stone-texture borders, inner 3x3 cells have a distinct teal glow overlay indicating the activation zone, two placed rune pieces visible as tetromino-shaped colored blocks with glowing symbols on them — one inside the activation zone glowing brightly and one outside dim, in the corner a small heat phase indicator showing current phase, bottom shows close and confirm button prompts, overall dark sophisticated game UI, roguelite game interface, 16:9
```
