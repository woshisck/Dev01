# Tag 鍛藉悕绌洪棿閫熸煡

> 鏈枃妗ｆ槸椤圭洰 Gameplay Tag 鍛藉悕绌洪棿鐨勬潈濞佸弬鑰冦€傛柊澧炲懡鍚嶇┖闂村墠蹇呴』鍏堝湪姝ゅ鐧昏銆?
> ini 鏂囦欢浣嶇疆锛歚Config/DefaultGameplayTags.ini`

---

## 鍛藉悕绌洪棿鎬昏

| 鍛藉悕绌洪棿 | 鑱岃矗 | 鎸傝浇鏃舵満 |
| --- | --- | --- |
| `Character.State.*` | 瑙掕壊鐘舵€佹満锛堢Щ鍔?鎴樻枟/鍙楀嚮绛夛級 | GAS Ability/GE 婵€娲绘椂 |
| `Character.Attribute.*` | 瑙掕壊灞炴€ф爣璁?| 鍒濆鍖栨椂 |
| `Buff.Status.*` | Buff/绗︽枃鐘舵€侊紙褰撳墠鐢紝鍚庢湡杩佺Щ锛?| BuffFlow 婵€娲?澶辨晥鏃?|
| `EnvBatch.*` | 鍦烘櫙鍚堟壒绯荤粺锛圫ource/Proxy/Baked锛?| 鍏冲崱璁捐/宸ュ叿鍒嗛厤 |
| `Combat.*` | 鎴樻枟浜嬩欢/鎶€鑳藉垎绫?| GA 婵€娲绘椂 |
| `UI.*` | UI 鐘舵€佸拰浜嬩欢 | UI 灞傛帶鍒?|

---

## EnvBatch 鍛藉悕绌洪棿锛堝悎鎵圭郴缁燂級

> 璇︾粏鍒朵綔瑙勮寖瑙侊細`Docs/Design/VFX/BatchMaterial_ArtistGuide.md`
> Tag 鍒嗛厤宸ュ叿锛歚Source/DevKitEditor/MaterialBatch/EnvBatchTagTool.py`

### Tag 鍒楄〃

| Tag | 绫诲瀷 | 鍚箟 | Block 瑙勫垯 |
| --- | --- | --- | --- |
| `EnvBatch.Source.Building_Stone` | Source | 鐭虫潗寤虹瓚鎵规缁勫師濮嬬綉鏍?| 涓?Proxy/Baked 浜掓枼锛屽悓绫诲彧淇濈暀涓€涓?|
| `EnvBatch.Source.Building_Wood` | Source | 鏈ㄦ潗寤虹瓚鎵规缁勫師濮嬬綉鏍?| 鍚屼笂 |
| `EnvBatch.Source.Building_Metal` | Source | 閲戝睘寤虹瓚鎵规缁勫師濮嬬綉鏍?| 鍚屼笂 |
| `EnvBatch.Source.Ground_A` | Source | A 鍖哄湴闈㈡壒娆＄粍鍘熷缃戞牸 | 鍚屼笂 |
| `EnvBatch.Source.Ground_B` | Source | B 鍖哄湴闈㈡壒娆＄粍鍘熷缃戞牸 | 鍚屼笂 |
| `EnvBatch.Source.Props_Small` | Source | 灏忛亾鍏锋壒娆＄粍鍘熷缃戞牸 | 鍚屼笂 |
| `EnvBatch.Source.Props_Large` | Source | 澶ч亾鍏锋壒娆＄粍鍘熷缃戞牸 | 鍚屼笂 |
| `EnvBatch.Proxy.{Group}.Medium` | Proxy | 涓。鍙婁互涓嬫樉绀虹殑鍚堟壒浠ｇ悊缃戞牸 | 涓?Source/Baked 浜掓枼 |
| `EnvBatch.Proxy.{Group}.Low` | Proxy | 浣庢。鏄剧ず鐨勫悎鎵逛唬鐞嗙綉鏍?| 鍚屼笂 |
| `EnvBatch.Baked.Ground.Low` | Baked | 浣庢。鍦伴潰鐑樺煿鏇夸唬骞抽潰 | 涓?Source/Proxy 浜掓枼 |
| `EnvBatch.Baked.Ground.Medium` | Baked | 涓。鍦伴潰鐑樺煿鏇夸唬骞抽潰 | 鍚屼笂 |
| `EnvBatch.Baked.General.Low` | Baked | 浣庢。閫氱敤鐑樺煿鏇夸唬 | 鍚屼笂 |
| `EnvBatch.Exclude` | 鎺掗櫎 | 鏄庣‘鎺掗櫎姝ょ墿浠跺弬涓庡悎鎵?| 涓嶄笌鍏朵粬浜掓枼 |

### 鍒嗙粍璁捐鍘熷垯

鍚屼竴鎵规缁勶紙`{Group}` 鐩稿悓锛夌殑鎵€鏈?Source Actor 浼氳鍚堝苟涓轰竴涓?Proxy Mesh锛?

- **蹇呴』**锛氬悓缁勫唴 Blend Mode 涓€鑷达紙Opaque / Masked 涓嶈兘娣凤級
- **蹇呴』**锛氬悓缁勫唴浣跨敤鍚屼竴濂楀悎鎵规潗璐?
- **寤鸿**锛氬悓缁勫唴鐗╀欢绌洪棿涓婄浉閭伙紙鍑忓皯涓嶅繀瑕佺殑鍚堝苟鑼冨洿锛?
- **寤鸿**锛氭瘡缁?50~200 涓?Actor锛堝お灏戞敹鐩婁綆锛屽お澶氫抚澶卞墧闄ょ矑搴︼級

### 杩愯鏃跺垏鎹㈤€昏緫

```
瓒呴珮/楂樻。锛歋ource 鏄剧ず锛孭roxy 闅愯棌
涓。锛?    Source 闅愯棌锛孭roxy.{Group}.Medium 鏄剧ず
浣庢。锛?    Source 闅愯棌锛孭roxy.{Group}.Low 鏄剧ず
          + Baked.Ground.Low 骞抽潰鏄剧ず锛堝湴闈㈠尯鍩燂級
```

鍒囨崲鐢?`SetActorHiddenInGame` 瀹炵幇锛屼笉渚濊禆 World Partition / Data Layer銆?

---

## Character.State 鍛藉悕绌洪棿

> 褰撳墠浣跨敤 `Buff.Status.*` 鍜?`State.*`锛屽悗鏈熼€氳繃 Tag Redirect 缁熶竴杩佺Щ鍒?`Character.State.*`銆?
> 璇﹁锛歚Docs/04_寮€鍙戝疄鐜颁笌绯荤粺鏂囨。/鏍囩/StateConflict/StateConflict_Technical.md`

---

## 鏂板鍛藉悕绌洪棿鍐崇瓥鏍?

```
闇€瑕佹柊 Tag 鏃讹細

1. 鏄惁灞炰簬宸叉湁鍛藉悕绌洪棿锛?
   鈫?鏄細鍦ㄥ搴斿瓙鑺傜偣涓嬫坊鍔狅紝鏇存柊鏈枃妗?
   鈫?鍚︼細缁х画

2. 浼氬奖鍝嶆覆鏌?鍦烘櫙绯荤粺锛?
   鈫?鏄細鎸傚埌 EnvBatch.* 鎴栨柊寤烘覆鏌撳懡鍚嶇┖闂?
   鈫?鍚︼細缁х画

3. 浼氬奖鍝嶆垬鏂?GAS锛?
   鈫?鏄細鎸傚埌 Character.State.* 鎴?Combat.*
   鈫?鍚︼細缁х画

4. 鏄?UI 鐘舵€侊紵
   鈫?鏄細鎸傚埌 UI.*
   鈫?鍚︼細鎻愪氦缁欎富绋?TA 璁ㄨ鍚庡啀瀹?
```

---

## 淇敼璁板綍

| 鏃ユ湡 | 鍙樻洿 | 鍘熷洜 |
| --- | --- | --- |
| 2026-06 | 鏂板 EnvBatch 鍏ㄥ懡鍚嶇┖闂?| 鍚堟壒绯荤粺璁捐瀹屾垚 |
| 2026-06 | EnvBatch.Source 缁嗗寲涓?{GroupName} 瀛愯妭鐐?| 鍗?Tag 浼氭妸鎵€鏈夌墿浠跺悎鎴愪竴涓法鍨?Mesh锛岄渶鍒嗙粍 |
| 2026-06 | 鏂板 EnvBatch.Baked 鍛藉悕绌洪棿 | 浣庢。鍦伴潰鐑樺煿鏇夸唬鏂规 |
