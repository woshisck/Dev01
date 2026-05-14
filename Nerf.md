                                        
  Searched for 3 patterns (ctrl+o to expand)                                                                                                  
                           
● Did a full audit. Short answer: no, several widgets escape the Subsystem. Three categories, ranked by risk.                                 
                                                            
  Category A — escapes the Subsystem entirely (worst — these cause D-pad bugs)                                                                
                                                                                                                                              
  These create Activatable widgets directly from world actors / pawns. The Subsystem never sees OnActivated/OnDeactivated, so ActivatedScreens
   is wrong, top-layer recompute misses them, and ApplyInputModeForLayer doesn't fire on close. These are the most likely culprits for "D-pad 
  bugs when interacting with prop X".                       

  ┌─────────────────────────────┬───────────────────────────┬───────┬──────────────────────────────────────────────────────────────────────┐
  │            Site             │          Widget           │ Layer │                             Why it's bad                             │
  ├─────────────────────────────┼───────────────────────────┼───────┼──────────────────────────────────────────────────────────────────────┤
  │ ShopActor.cpp:65            │ UShopSelectionWidget      │ Menu  │ Activatable, but actor-owned. Subsystem-blind.                       │
  ├─────────────────────────────┼───────────────────────────┼───────┼──────────────────────────────────────────────────────────────────────┤
  │ AltarActor.cpp:64           │ UAltarMenuWidget          │ Menu  │ Same                                                                 │
  ├─────────────────────────────┼───────────────────────────┼───────┼──────────────────────────────────────────────────────────────────────┤
  │ AltarActor.cpp:152          │ USacrificeSelectionWidget │ Menu  │ Same — opened by Altar after Sacrifice click                         │
  ├─────────────────────────────┼───────────────────────────┼───────┼──────────────────────────────────────────────────────────────────────┤
  │ PlayerCharacterBase.cpp:649 │ UDamageEdgeFlashWidget    │ Game  │ Pawn creates its own copy, ignoring the Registry entry for           │
  │                             │                           │       │ DamageEdgeFlash                                                      │
  └─────────────────────────────┴───────────────────────────┴───────┴──────────────────────────────────────────────────────────────────────┘

  These 4 should all be migrated to Subsystem->PushScreen(...) / EnsureWidget(...). After that, opening the Altar or Shop becomes a one-liner,
   and closing them auto-restores input mode through OnDeactivated → Subsystem.

  Category B — still on legacy Ensure*Widget (HUD-owned, lower risk)

  These are in YogHUD.cpp. They still go through ResolveManagedWidgetClass so the Registry's WidgetClass is honored — they just don't register
   with the Subsystem's Instances map. Layer tracking is incomplete, but mouse/InputMode rarely matters for these (all Overlays).

  ┌─────────────────┬──────────────────────────┬──────────────────┬───────────────────────────────────────────────────────────────────────┐
  │      Site       │          Widget          │      Layer       │                            Migration cost                             │
  ├─────────────────┼──────────────────────────┼──────────────────┼───────────────────────────────────────────────────────────────────────┤
  │ YogHUD.cpp:336  │ PauseMenuWidget          │ Modal ⚠️          │ High value — this is the Modal layer's gateway and is still using raw │
  │                 │                          │                  │  CreateWidget                                                         │
  ├─────────────────┼──────────────────────────┼──────────────────┼───────────────────────────────────────────────────────────────────────┤
  │ YogHUD.cpp:602  │ WeaponFloatWidget        │ Game             │ Trivial                                                               │
  ├─────────────────┼──────────────────────────┼──────────────────┼───────────────────────────────────────────────────────────────────────┤
  │ YogHUD.cpp:847  │ CurrentRoomBuffWidget    │ Game             │ Trivial                                                               │
  ├─────────────────┼──────────────────────────┼──────────────────┼───────────────────────────────────────────────────────────────────────┤
  │ YogHUD.cpp:896  │ WeaponThumbnailFlyWidget │ Game             │ One-shot, trivial                                                     │
  ├─────────────────┼──────────────────────────┼──────────────────┼───────────────────────────────────────────────────────────────────────┤
  │ YogHUD.cpp:905  │ WeaponTrailWidget        │ Game             │ One-shot, trivial                                                     │
  ├─────────────────┼──────────────────────────┼──────────────────┼───────────────────────────────────────────────────────────────────────┤
  │ YogHUD.cpp:1138 │ LevelEndRevealWidget     │ Game             │ One-shot, trivial                                                     │
  ├─────────────────┼──────────────────────────┼──────────────────┼───────────────────────────────────────────────────────────────────────┤
  │ YogHUD.cpp:1245 │ CombatItemBarWidget      │ Game (child of   │ Stays as child — no migration needed                                  │
  │                 │                          │ HUD)             │                                                                       │
  ├─────────────────┼──────────────────────────┼──────────────────┼───────────────────────────────────────────────────────────────────────┤
  │ YogHUD.cpp:1285 │ FinisherQTEWidget        │ Game             │ Trivial                                                               │
  ├─────────────────┼──────────────────────────┼──────────────────┼───────────────────────────────────────────────────────────────────────┤
  │ YogHUD.cpp:1435 │ PortalPreviewWidget      │ Game             │ Trivial                                                               │
  ├─────────────────┼──────────────────────────┼──────────────────┼───────────────────────────────────────────────────────────────────────┤
  │ YogHUD.cpp:1449 │ PortalDirectionWidget    │ Game             │ Trivial                                                               │
  └─────────────────┴──────────────────────────┴──────────────────┴───────────────────────────────────────────────────────────────────────┘

  PauseMenu is the only one with real urgency here — it's Modal layer, so the Subsystem's mouse/focus/InputMode flip should be authoritative
  for it. The rest are overlays where the layer = Game regardless.

  Category C — separate Slate world (acceptable as-is, document it)

  YogGameInstanceBase.cpp builds the frontend (title / loading / pause-blocker / version banner / disconnect dialog) with raw SNew +
  AddViewportWidgetContent(..., ZOrder=10000). ~30+ Slate calls. This is outside the LocalPlayer's lifetime — the GameInstance lives across
  maps and lacks a LocalPlayer to host the Subsystem at boot. So the Subsystem genuinely can't own these (you'd lose the title screen during
  LocalPlayer-less moments like map loads).

  Two options here: (a) leave it as a separate "frontend Slate" world, document in UIManager_Guide.md that it's intentionally outside the
  system, or (b) eventually migrate to a UCommonGameViewportClient / UCommonUIExtensions pattern that has its own root layout for
  pre-LocalPlayer UI. Not urgent.

  Child widgets — legitimate, ignore

  GenericEffectListWidget (entries), CombatDeckEditWidget (card slots / floating drag), BackpackGridWidget (rune slots), PendingGridWidget
  (rune slots), LootSelectionWidget (rune cards), AltarMenuWidget (Purification/Sacrifice child widgets), InputActionRichTextDecorator (rich
  text). These are internal composition — they're never "screens" the player navigates between. They should keep being created with
  CreateWidget from the parent widget. The Subsystem is for top-level screens only.

  Recommendation order

  1. Migrate ShopActor + AltarActor (Category A) — biggest payoff. Each currently does CreateWidget + AddToViewport + manual SetInputMode +
  bespoke focus calls. Replace with Subsystem->PushScreen(EYogUIScreenId::ShopSelection) and the input/focus chain takes care of itself. ~15
  minutes per actor.
  2. Add DamageEdgeFlash to Subsystem in PlayerCharacterBase — single-line change, but consistent.
  3. Migrate PauseMenu in YogHUD — most-touched Modal path.
  4. Sweep the remaining HUD overlays as time permits — pure mechanical work.
  5. Document Category C in the UIManager guide as "frontend Slate is intentionally separate".

  Want me to start with Shop + Altar (Category A)? Those are the structural ones, and once they're routed through the Subsystem, your
  "monitoring tool" Tier 2 widget will actually show every relevant screen.

✻ Cogitated for 1m 6s              

──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
❯                  
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
  ⏵⏵ accept edits on (shift+tab to cycle)                   

 *  History restored 

PS D:\Fab\Meta\VTSH_LASE_Dev\VTSH_ShaderLib_Test\VTSH_ShaderDev_Test> .\sunchuankai_run_claude_code.bat
ANTHROPIC_DEFAULT_SONNET_MODEL is: anthropic/claude-sonnet-latest
No environment specified, defaulting to PRO
Using PRO environment: https://mlaas.virtuosgames.com/

Configuration:
  ANTHROPIC_BASE_URL: https://mlaas.virtuosgames.com/proxy/anthropic
  API Key: [HIDDEN]

 ▐▛███▜▌   Claude Code v2.1.118
▝▜█████▛▘  anthropic/claude-opus-latest · API Usage Billing
  ▘▘ ▝▝    D:\Fab\Meta\VTSH_LASE_Dev\VTSH_ShaderLib_Test\VTSH_ShaderDev_Test

❯ can you remember the last time you mentioned category A,B,C about the widget handling problem ? export it as MD for me please                
                                                                                                                                
  Read 1 file, listed 2 directories, recalled 1 memory (ctrl+o to expand)                                                                      
                                                                                                                                             
● I have no memory of a prior conversation about a "widget handling problem" with categories A, B, C. My memory directory only contains:       
                                                                                                                                               
  - feedback_language.md — preference to reply in English                                                                                      
  - project_backpack_removal.md — notes on the BackpackGridComponent removal                                                                   
                                                                                                                     
  Neither references "categories A, B, C" or a widget handling discussion. I don't retain transcripts across sessions — only what's been       
  explicitly saved as memory — so I can't reconstruct or export something I never recorded.                                                  
                                                                                                                                             
  Could you give me more context? For example:                                                                                                 
  - Was it about a specific widget (e.g., BackpackGridWidget, HeatBarWidget, RunePurificationWidget)?
  - Roughly when the conversation happened, or any keywords you remember?                                                                      
                                                                                                                                               
  If you can share the gist, I can re-derive the categorization from the current code and write it to an MD file for you. Otherwise I'd just be
   fabricating content, which I won't do.

❯ can you remember the last time the widget handling probelmd we are discusss?                                                                 
                                                                               
● No — I have no memory of a prior discussion about widget handling. My saved memory for this project contains only two entries:               
                                                                                                                                             
  - feedback_language.md — reply in English                                                                                                    
  - project_backpack_removal.md — BackpackGridComponent removal notes (mentions widget stubs like BackpackGridWidget, HeatBarWidget,           
  RunePurificationWidget, but no A/B/C categorization)                                                                                         
                                                                                                                                               
  I don't retain conversation transcripts across sessions — only what was explicitly saved as memory. If we discussed widget handling        
  categories in a past session, it wasn't recorded.

  I notice you have LootSelectionWidget.cpp open. If the widget handling problem relates to that file (or another specific widget), tell me
  which one and what aspect concerned you, and I can analyze the current code and produce a categorized MD breakdown. I won't invent A/B/C
  categories from a conversation I don't have.

❯ it's ok, just leave it, now can you help me to find that if all the widget has been handled by the UIManagerSystems? are there any wild      
  widget created by Slate and no handled that can cause input issue or not safe to work with?                                                  
                                                                                                                                               
● Explore(Audit widget management vs wild widgets)                                                                                             
  ⎿  Done (0 tool uses · 0 tokens · 3m 13s)                                                                                                    
  (ctrl+o to expand)                                                                                                                           
                                                                                                                                               
● Explore(Audit widget management vs wild widgets)                                                                                             
  ⎿  Done (0 tool uses · 0 tokens · 3m 5s)                                                                                                     
  (ctrl+o to expand)                                                                                                                           
                                                                                                                                               
● The Agent tool is hitting a server error. Let me investigate directly.                                                                       
                                                                                                                                               
  Searched for 3 patterns, read 1 file (ctrl+o to expand)                                                                                      
                                                                                                                                               
● Found YogUIManagerSubsystem.h and YogUIRegistry.h. Let me read them and survey widget creation patterns.                                     
                                                                                                                                               
  Searched for 4 patterns, read 10 files (ctrl+o to expand)                                                                                    
                                                                                                                                               
● I have enough. Here's the audit.                                                                                                             
                                                                                                                                               
  Verdict: No — not all widgets are managed                                                                                                    
                                                                                                                                               
  A canonical UIManager exists (UYogUIManagerSubsystem + UYogUIRegistry), but a significant fraction of viewport widgets bypass it, and most   
  "managed" widgets also call SetInputMode/SetShowMouseCursor themselves, which fights the subsystem's bAutoManageInputMode path. There are    
  also pure-Slate widgets created for the frontend menu that exist entirely outside the UMG/CommonUI pipeline.                               

  UIManager canonical API

  Source/DevKit/Public/UI/YogUIManagerSubsystem.h

  - EnsureWidget(EYogUIScreenId) — create + AddToViewport once, idempotent (impl at YogUIManagerSubsystem.cpp:128)
  - PushScreen / PushScreenAsync / PopScreen — activate/deactivate via CommonUI
  - GetWidget / IsScreenActive / GetTypedWidget<T>
  - CreateAutoStartWidgets() — iterates registry's bCreateOnHUDStart=true
  - Owns input-mode/focus switching via top activated layer (Game / Menu / Modal) when bAutoManageInputMode = true
  - Registry enum (EYogUIScreenId): MainHUD, Backpack, LootSelection, PauseMenu, TutorialPopup, SacrificeGraceOption, InfoPopup, PortalPreview,
   PortalDirection, CurrentRoomBuff, CombatItemBar, FinisherQTE, LevelEndReveal, WeaponFloat, WeaponThumbnailFly, WeaponTrail, DamageEdgeFlash

  B. Wild widgets (created + AddToViewport outside the manager)

  #: 1
  File:Line: Map/ShopActor.cpp:65-68
  Widget: UShopSelectionWidget
  Touches input?: Yes — ShopSelectionWidget.cpp:88-92 calls SetShowMouseCursor + SetInputMode
  Risk: Shop not in registry. Pure wild.
  ────────────────────────────────────────
  #: 2
  File:Line: Map/AltarActor.cpp:64-66
  Widget: UAltarMenuWidget
  Touches input?: Yes — AltarMenuWidget.cpp:42-46
  Risk: Altar not in registry.
  ────────────────────────────────────────
  #: 3
  File:Line: Map/AltarActor.cpp:152-155
  Widget: USacrificeSelectionWidget
  Touches input?: Yes — SacrificeSelectionWidget.cpp:162-166
  Risk: Not in registry.
  ────────────────────────────────────────
  #: 4
  File:Line: UI/AltarMenuWidget.cpp:120-122
  Widget: URunePurificationWidget
  Touches input?: Yes — RunePurificationWidget.cpp:25-29
  Risk: Not in registry.
  ────────────────────────────────────────
  #: 5
  File:Line: UI/AltarMenuWidget.cpp:126-128
  Widget: USacrificeSelectionWidget (2nd path)
  Touches input?: Yes
  Risk: Same widget gets two creation routes (#3 and #5). Re-entry hazard.
  ────────────────────────────────────────
  #: 6
  File:Line: UI/CombatDeckEditWidget.cpp:832-848
  Widget: UCombatDeckEditCardSlotWidget (AddToPlayerScreen(10000))
  Touches input?: No (HitTestInvisible)
  Risk: Floating drag preview. Z=10000 puts it above everything including modal — likely intentional but unowned.
  ────────────────────────────────────────
  #: 7
  File:Line: UI/GameDialogWidget.cpp:25
  Widget: UTutorialPopupWidget (self-AddToViewport(200))
  Touches input?: No (deliberately avoids)
  Risk: TutorialPopup IS in the registry — should be PushScreen(TutorialPopup) instead of self-adding.
  ────────────────────────────────────────
  #: 8
  File:Line: Character/PlayerCharacterBase.cpp:649-652
  Widget: UDamageEdgeFlashWidget
  Touches input?: No
  Risk: DamageEdgeFlash IS in the registry — should be EnsureWidget(DamageEdgeFlash).

  C. Pure-Slate "wild" UI (entirely outside UMG/CommonUI)

  - System/YogGameInstanceBase.cpp:385-513, 677-832 — SDevKitFrontendMenuRoot +
  SButton/SOverlay/SBorder/SScaleBox/SVerticalBox/STextBlock/SImage/SBox tree for the frontend main menu and death menu. Calls
  PC->SetInputMode(FInputModeUIOnly/GameOnly) (lines 884-906) and FSlateApplication::Get().SetKeyboardFocus(...) (lines 897, 952, 984)
  directly.
    - Risk: this path competes with the subsystem on input-mode and focus. If a frontend menu is up while the subsystem's
  ApplyInputModeForLayer also runs (e.g. early ASC events fire), you get oscillating input mode.
    - InputActionRichTextDecorator.cpp:45-107 uses SNew(SBox/STextBlock/SImage) — these are inline rich-text decorations, not standalone
  screens. Not a problem.

  Bonus risk: managed widgets that still drive input themselves

  Even when these screens ARE in the registry, they manually call SetInputMode/SetShowMouseCursor on activate. With bAutoManageInputMode = true
   the subsystem will also set it from HandleScreenActivated, racing against the widget's own call:

  - LootSelectionWidget.cpp:106-110, 443-447, 560-564 ← matches the file you have open
  - PauseMenuWidget.cpp:67-71
  - BackpackScreenWidget.cpp:925 (SetShowMouseCursor only)
  - SacrificeGraceOptionWidget.cpp:45-49
  - RunePurificationWidget.cpp:25-29
  - Plus YogPlayerControllerBase.cpp:360-371 and YogGameMode.cpp:881, 945 set input mode directly from outside any widget at all.

  This is precisely the kind of "two sources of truth" pattern that produces "cursor sometimes stuck visible / D-pad sometimes has no focus
  after revive" bugs.

  Recommended next steps (priority order)

  1. Migrate the two registry-eligible offenders first (cheap, mechanical):
    - TutorialPopup: replace GameDialogWidget.cpp:25 AddToViewport(200) with UIManager->PushScreen(EYogUIScreenId::TutorialPopup).
    - DamageEdgeFlash: replace PlayerCharacterBase.cpp:649-652 with UIManager->EnsureWidget(EYogUIScreenId::DamageEdgeFlash).
  2. Register the altar/shop/purification/sacrifice screens in EYogUIScreenId and route through PushScreen. Delete the per-widget SetInputMode
  blocks once the subsystem owns it.
  3. Pick one owner for input mode. Either keep bAutoManageInputMode = true and strip the widget-side PC->SetInputMode calls, or turn it off
  and centralize in a project policy file — but not both.
  4. Decide what to do with the Slate frontend menu. Either accept it as an isolated boot/death screen (and gate the subsystem off while it's
  up) or rebuild it as a UCommonActivatableWidget so it joins the same input model as everything else.
  5. The CombatDeckEdit gamepad floating drag slot (Z=10000) is fine functionally, but worth wrapping in a small helper on the parent widget so
   its lifetime is obviously paired with the drag operation.

✻ Cooked for 8m 9s                  

※ recap: You're auditing whether all widgets route through `UYogUIManagerSubsystem`; several wild `AddToViewport` sites exist (ShopActor,      
  AltarActor, AltarMenuWidget, TutorialPopup, DamageEdgeFlash) plus a Slate frontend menu. Next: migrate `TutorialPopup` and `DamageEdgeFlash` 
  to `PushScreen`/`EnsureWidget` since both are already in the registry. (disable recaps in /config)                                           
                                                                                                                                             
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
❯ 
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
  ? for shortcuts                                                                  Update available! Run: winget upgrade Anthropic.ClaudeCode
  #: 3
  File:Line: Map/AltarActor.cpp:152-155
  Widget: USacrificeSelectionWidget
  Touches input?: Yes — SacrificeSelectionWidget.cpp:162-166
  Risk: Not in registry.
  ────────────────────────────────────────
  #: 4
  File:Line: UI/AltarMenuWidget.cpp:120-122
  Widget: URunePurificationWidget
  Touches input?: Yes — RunePurificationWidget.cpp:25-29
  Risk: Not in registry.
  ────────────────────────────────────────
  #: 5
  File:Line: UI/AltarMenuWidget.cpp:126-128
  Widget: USacrificeSelectionWidget (2nd path)
  Touches input?: Yes
  Risk: Same widget gets two creation routes (#3 and #5). Re-entry hazard.
  ────────────────────────────────────────
  #: 6
  File:Line: UI/CombatDeckEditWidget.cpp:832-848
  Widget: UCombatDeckEditCardSlotWidget (AddToPlayerScreen(10000))
  Touches input?: No (HitTestInvisible)
  Risk: Floating drag preview. Z=10000 puts it above everything including modal — likely intentional but unowned.
  ────────────────────────────────────────
  #: 7
  File:Line: UI/GameDialogWidget.cpp:25
  Widget: UTutorialPopupWidget (self-AddToViewport(200))
  Touches input?: No (deliberately avoids)
  Risk: TutorialPopup IS in the registry — should be PushScreen(TutorialPopup) instead of self-adding.
  ────────────────────────────────────────
  #: 8
  File:Line: Character/PlayerCharacterBase.cpp:649-652
  Widget: UDamageEdgeFlashWidget
  Touches input?: No
  Risk: DamageEdgeFlash IS in the registry — should be EnsureWidget(DamageEdgeFlash).

  C. Pure-Slate "wild" UI (entirely outside UMG/CommonUI)

  - System/YogGameInstanceBase.cpp:385-513, 677-832 — SDevKitFrontendMenuRoot +
  SButton/SOverlay/SBorder/SScaleBox/SVerticalBox/STextBlock/SImage/SBox tree for the frontend main menu and death menu. Calls
  PC->SetInputMode(FInputModeUIOnly/GameOnly) (lines 884-906) and FSlateApplication::Get().SetKeyboardFocus(...) (lines 897, 952, 984)
  directly.
    - Risk: this path competes with the subsystem on input-mode and focus. If a frontend menu is up while the subsystem's
  ApplyInputModeForLayer also runs (e.g. early ASC events fire), you get oscillating input mode.
    - InputActionRichTextDecorator.cpp:45-107 uses SNew(SBox/STextBlock/SImage) — these are inline rich-text decorations, not standalone
  screens. Not a problem.

  Bonus risk: managed widgets that still drive input themselves

  Even when these screens ARE in the registry, they manually call SetInputMode/SetShowMouseCursor on activate. With bAutoManageInputMode = true
   the subsystem will also set it from HandleScreenActivated, racing against the widget's own call:

  - LootSelectionWidget.cpp:106-110, 443-447, 560-564 ← matches the file you have open
  - PauseMenuWidget.cpp:67-71
  - BackpackScreenWidget.cpp:925 (SetShowMouseCursor only)
  - SacrificeGraceOptionWidget.cpp:45-49
  - RunePurificationWidget.cpp:25-29
  - Plus YogPlayerControllerBase.cpp:360-371 and YogGameMode.cpp:881, 945 set input mode directly from outside any widget at all.

  This is precisely the kind of "two sources of truth" pattern that produces "cursor sometimes stuck visible / D-pad sometimes has no focus
  after revive" bugs.

  Recommended next steps (priority order)

  1. Migrate the two registry-eligible offenders first (cheap, mechanical):
    - TutorialPopup: replace GameDialogWidget.cpp:25 AddToViewport(200) with UIManager->PushScreen(EYogUIScreenId::TutorialPopup).
    - DamageEdgeFlash: replace PlayerCharacterBase.cpp:649-652 with UIManager->EnsureWidget(EYogUIScreenId::DamageEdgeFlash).
  2. Register the altar/shop/purification/sacrifice screens in EYogUIScreenId and route through PushScreen. Delete the per-widget SetInputMode
  blocks once the subsystem owns it.
  3. Pick one owner for input mode. Either keep bAutoManageInputMode = true and strip the widget-side PC->SetInputMode calls, or turn it off
  and centralize in a project policy file — but not both.
  4. Decide what to do with the Slate frontend menu. Either accept it as an isolated boot/death screen (and gate the subsystem off while it's
  up) or rebuild it as a UCommonActivatableWidget so it joins the same input model as everything else.
  5. The CombatDeckEdit gamepad floating drag slot (Z=10000) is fine functionally, but worth wrapping in a small helper on the parent widget so
   its lifetime is obviously paired with the drag operation.

✻ Cooked for 8m 9s

※ recap: You're auditing whether all widgets route through `UYogUIManagerSubsystem`; several wild `AddToViewport` sites exist (ShopActor,
  AltarActor, AltarMenuWidget, TutorialPopup, DamageEdgeFlash) plus a Slate frontend menu. Next: migrate `TutorialPopup` and `DamageEdgeFlash`
  to `PushScreen`/`EnsureWidget` since both are already in the registry. (disable recaps in /config)

───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
❯ 
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
  ? for shortcuts                                                                  Update available! Run: winget upgrade Anthropic.ClaudeCode

  Searched for 4 patterns, read 10 files (ctrl+o to expand)

● I have enough. Here's the audit.

  Verdict: No — not all widgets are managed

  A canonical UIManager exists (UYogUIManagerSubsystem + UYogUIRegistry), but a significant fraction of viewport widgets bypass it, and most "managed" widgets also call SetInputMode/SetShowMouseCursor themselves, which fights the subsystem's bAutoManageInputMode path.
  There are also pure-Slate widgets created for the frontend menu that exist entirely outside the UMG/CommonUI pipeline.

  UIManager canonical API

  Source/DevKit/Public/UI/YogUIManagerSubsystem.h

  - EnsureWidget(EYogUIScreenId) — create + AddToViewport once, idempotent (impl at YogUIManagerSubsystem.cpp:128)
  - PushScreen / PushScreenAsync / PopScreen — activate/deactivate via CommonUI
  - GetWidget / IsScreenActive / GetTypedWidget<T>
  - CreateAutoStartWidgets() — iterates registry's bCreateOnHUDStart=true
  - Owns input-mode/focus switching via top activated layer (Game / Menu / Modal) when bAutoManageInputMode = true
  - Registry enum (EYogUIScreenId): MainHUD, Backpack, LootSelection, PauseMenu, TutorialPopup, SacrificeGraceOption, InfoPopup, PortalPreview, PortalDirection, CurrentRoomBuff, CombatItemBar, FinisherQTE, LevelEndReveal, WeaponFloat, WeaponThumbnailFly, WeaponTrail,
  DamageEdgeFlash

  B. Wild widgets (created + AddToViewport outside the manager)

  ┌─────┬───────────────────────────────────────────┬────────────────────────────────────────────────────┬────────────────────────────────────────────────────────────────────┬──────────────────────────────────────────────────────────────────────────────────────────┐
  │  #  │                 File:Line                 │                       Widget                       │                           Touches input?                      │                                           Risk                                           │
  ├─────┼───────────────────────────────────────────┼────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────────┤
  │ 1   │ Map/ShopActor.cpp:65-68                   │ UShopSelectionWidget                               │ Yes — ShopSelectionWidget.cpp:88-92 calls SetShowMouseCursor +     │ Shop not in registry. Pure wild.                                                         │
  │     │                                           │                                                    │ SetInputMode                      │                                                                                          │
  ├─────┼───────────────────────────────────────────┼────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────────┤
  │ 2   │ Map/AltarActor.cpp:64-66                  │ UAltarMenuWidget                                   │ Yes — AltarMenuWidget.cpp:42-46                      │ Altar not in registry.                                                                   │
  ├─────┼───────────────────────────────────────────┼────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────────┤
  │ 3   │ Map/AltarActor.cpp:152-155                │ USacrificeSelectionWidget                          │ Yes — SacrificeSelectionWidget.cpp:162-166                      │ Not in registry.                                                                         │
  ├─────┼───────────────────────────────────────────┼────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────────┤
  │ 4   │ UI/AltarMenuWidget.cpp:120-122            │ URunePurificationWidget                            │ Yes — RunePurificationWidget.cpp:25-29                      │ Not in registry.                                                                         │
  ├─────┼───────────────────────────────────────────┼────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────────┤
  │ 5   │ UI/AltarMenuWidget.cpp:126-128            │ USacrificeSelectionWidget (2nd path)               │ Yes                      │ Same widget gets two creation routes (#3 and #5). Re-entry hazard.                       │
  ├─────┼───────────────────────────────────────────┼────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────────┤
  │ 6   │ UI/CombatDeckEditWidget.cpp:832-848       │ UCombatDeckEditCardSlotWidget                      │ No (HitTestInvisible)                      │ Floating drag preview. Z=10000 puts it above everything including modal — likely         │
  │     │                                           │ (AddToPlayerScreen(10000))                         │                      │ intentional but unowned.                                                                 │
  ├─────┼───────────────────────────────────────────┼────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────────┤
  │ 7   │ UI/GameDialogWidget.cpp:25                │ UTutorialPopupWidget (self-AddToViewport(200))     │ No (deliberately avoids)                      │ TutorialPopup IS in the registry — should be PushScreen(TutorialPopup) instead of        │
  │     │                                           │                                                    │                      │ self-adding.                                                                             │
  ├─────┼───────────────────────────────────────────┼────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────────┤
  │ 8   │ Character/PlayerCharacterBase.cpp:649-652 │ UDamageEdgeFlashWidget                             │ No                      │ DamageEdgeFlash IS in the registry — should be EnsureWidget(DamageEdgeFlash).            │
  └─────┴───────────────────────────────────────────┴────────────────────────────────────────────────────┴────────────────────────────────────────────────────────────────────┴──────────────────────────────────────────────────────────────────────────────────────────┘

  C. Pure-Slate "wild" UI (entirely outside UMG/CommonUI)

  - System/YogGameInstanceBase.cpp:385-513, 677-832 — SDevKitFrontendMenuRoot + SButton/SOverlay/SBorder/SScaleBox/SVerticalBox/STextBlock/SImage/SBox tree for the frontend main menu and death menu. Calls PC->SetInputMode(FInputModeUIOnly/GameOnly) (lines 884-906) and
  FSlateApplication::Get().SetKeyboardFocus(...) (lines 897, 952, 984) directly.
    - Risk: this path competes with the subsystem on input-mode and focus. If a frontend menu is up while the subsystem's ApplyInputModeForLayer also runs (e.g. early ASC events fire), you get oscillating input mode.
    - InputActionRichTextDecorator.cpp:45-107 uses SNew(SBox/STextBlock/SImage) — these are inline rich-text decorations, not standalone screens. Not aproblem.

  Bonus risk: managed widgets that still drive input themselves

  Even when these screens ARE in the registry, they manually call SetInputMode/SetShowMouseCursor on activate. With bAutoManageInputMode = true the subsystem will also set it from HandleScreenActivated, racing against the widget's own call:

  - LootSelectionWidget.cpp:106-110, 443-447, 560-564 ← matches the file you have open
  - PauseMenuWidget.cpp:67-71
  - BackpackScreenWidget.cpp:925 (SetShowMouseCursor only)
  - SacrificeGraceOptionWidget.cpp:45-49
  - RunePurificationWidget.cpp:25-29
  - Plus YogPlayerControllerBase.cpp:360-371 and YogGameMode.cpp:881, 945 set input mode directly from outside any widget at all.

  This is precisely the kind of "two sources of truth" pattern that produces "cursor sometimes stuck visible / D-pad sometimes has no focus after revive" bugs.

  Recommended next steps (priority order)

  1. Migrate the two registry-eligible offenders first (cheap, mechanical):
    - TutorialPopup: replace GameDialogWidget.cpp:25 AddToViewport(200) with UIManager->PushScreen(EYogUIScreenId::TutorialPopup).
    - DamageEdgeFlash: replace PlayerCharacterBase.cpp:649-652 with UIManager->EnsureWidget(EYogUIScreenId::DamageEdgeFlash).
  2. Register the altar/shop/purification/sacrifice screens in EYogUIScreenId and route through PushScreen. Delete the per-widget SetInputMode blocks once the subsystem owns it.
  3. Pick one owner for input mode. Either keep bAutoManageInputMode = true and strip the widget-side PC->SetInputMode calls, or turn it off and centralize in a project policy file — but not both.
  4. Decide what to do with the Slate frontend menu. Either accept it as an isolated boot/death screen (and gate the subsystem off while it's up) or rebuild it as a UCommonActivatableWidget so it joins the same input model as everything else.
  5. The CombatDeckEdit gamepad floating drag slot (Z=10000) is fine functionally, but worth wrapping in a small helper on the parent widget so its lifetime is obviously paired with the drag operation.

✻ Cooked for 8m 9s

※ recap: You're auditing whether all widgets route through `UYogUIManagerSubsystem`; several wild `AddToViewport` sites exist (ShopActor, AltarActor, AltarMenuWidget, TutorialPopup, DamageEdgeFlash) plus a Slate frontend menu. Next: migrate `TutorialPopup` and
  `DamageEdgeFlash` to `PushScreen`/`EnsureWidget` since both are already in the registry. (disable recaps in /config)

─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
❯ 
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
  ? for shortcuts                                                         Update available! Run: winget upgrade Anthropic.ClaudeCode

  Searched for 4 patterns, read 10 files (ctrl+o to expand)

● I have enough. Here's the audit.

  Verdict: No — not all widgets are managed

  A canonical UIManager exists (UYogUIManagerSubsystem + UYogUIRegistry), but a significant fraction of viewport widgets bypass it, and most "managed" widgets also call SetInputMode/SetShowMouseCursor themselves, which fights the subsystem's bAutoManageInputMode path.
  There are also pure-Slate widgets created for the frontend menu that exist entirely outside the UMG/CommonUI pipeline.

  UIManager canonical API

  Source/DevKit/Public/UI/YogUIManagerSubsystem.h

  - EnsureWidget(EYogUIScreenId) — create + AddToViewport once, idempotent (impl at YogUIManagerSubsystem.cpp:128)
  - PushScreen / PushScreenAsync / PopScreen — activate/deactivate via CommonUI
  - GetWidget / IsScreenActive / GetTypedWidget<T>
  - CreateAutoStartWidgets() — iterates registry's bCreateOnHUDStart=true
  - Owns input-mode/focus switching via top activated layer (Game / Menu / Modal) when bAutoManageInputMode = true
  - Registry enum (EYogUIScreenId): MainHUD, Backpack, LootSelection, PauseMenu, TutorialPopup, SacrificeGraceOption, InfoPopup, PortalPreview, PortalDirection, CurrentRoomBuff, CombatItemBar, FinisherQTE, LevelEndReveal, WeaponFloat, WeaponThumbnailFly, WeaponTrail,
  DamageEdgeFlash

  B. Wild widgets (created + AddToViewport outside the manager)

  ┌─────┬───────────────────────────────────────────┬────────────────────────────────────────────────────┬────────────────────────────────────────────────────────────────────┬──────────────────────────────────────────────────────────────────────────────────────────┐
  │  #  │                 File:Line                 │                       Widget                       │                           Touches input?                      │                                           Risk                                           │
  ├─────┼───────────────────────────────────────────┼────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────────┤
  │ 1   │ Map/ShopActor.cpp:65-68                   │ UShopSelectionWidget                               │ Yes — ShopSelectionWidget.cpp:88-92 calls SetShowMouseCursor +     │ Shop not in registry. Pure wild.                                                         │
  │     │                                           │                                                    │ SetInputMode                      │                                                                                          │
  ├─────┼───────────────────────────────────────────┼────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────────┤
  │ 2   │ Map/AltarActor.cpp:64-66                  │ UAltarMenuWidget                                   │ Yes — AltarMenuWidget.cpp:42-46                      │ Altar not in registry.                                                                   │
  ├─────┼───────────────────────────────────────────┼────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────────┤
  │ 3   │ Map/AltarActor.cpp:152-155                │ USacrificeSelectionWidget                          │ Yes — SacrificeSelectionWidget.cpp:162-166                      │ Not in registry.                                                                         │
  ├─────┼───────────────────────────────────────────┼────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────────┤
  │ 4   │ UI/AltarMenuWidget.cpp:120-122            │ URunePurificationWidget                            │ Yes — RunePurificationWidget.cpp:25-29                      │ Not in registry.                                                                         │
  ├─────┼───────────────────────────────────────────┼────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────────┤
  │ 5   │ UI/AltarMenuWidget.cpp:126-128            │ USacrificeSelectionWidget (2nd path)               │ Yes                      │ Same widget gets two creation routes (#3 and #5). Re-entry hazard.                       │
  ├─────┼───────────────────────────────────────────┼────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────────┤
  │ 6   │ UI/CombatDeckEditWidget.cpp:832-848       │ UCombatDeckEditCardSlotWidget                      │ No (HitTestInvisible)                      │ Floating drag preview. Z=10000 puts it above everything including modal — likely         │
  │     │                                           │ (AddToPlayerScreen(10000))                         │                      │ intentional but unowned.                                                                 │
  ├─────┼───────────────────────────────────────────┼────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────────┤
  │ 7   │ UI/GameDialogWidget.cpp:25                │ UTutorialPopupWidget (self-AddToViewport(200))     │ No (deliberately avoids)                      │ TutorialPopup IS in the registry — should be PushScreen(TutorialPopup) instead of        │
  │     │                                           │                                                    │                      │ self-adding.                                                                             │
  ├─────┼───────────────────────────────────────────┼────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────────┤
  │ 8   │ Character/PlayerCharacterBase.cpp:649-652 │ UDamageEdgeFlashWidget                             │ No                      │ DamageEdgeFlash IS in the registry — should be EnsureWidget(DamageEdgeFlash).            │
  └─────┴───────────────────────────────────────────┴────────────────────────────────────────────────────┴────────────────────────────────────────────────────────────────────┴──────────────────────────────────────────────────────────────────────────────────────────┘

  C. Pure-Slate "wild" UI (entirely outside UMG/CommonUI)

  - System/YogGameInstanceBase.cpp:385-513, 677-832 — SDevKitFrontendMenuRoot + SButton/SOverlay/SBorder/SScaleBox/SVerticalBox/STextBlock/SImage/SBox tree for the frontend main menu and death menu. Calls PC->SetInputMode(FInputModeUIOnly/GameOnly) (lines 884-906) and
  FSlateApplication::Get().SetKeyboardFocus(...) (lines 897, 952, 984) directly.
    - Risk: this path competes with the subsystem on input-mode and focus. If a frontend menu is up while the subsystem's ApplyInputModeForLayer also runs (e.g. early ASC events fire), you get oscillating input mode.
    - InputActionRichTextDecorator.cpp:45-107 uses SNew(SBox/STextBlock/SImage) — these are inline rich-text decorations, not standalone screens. Not aproblem.

  Bonus risk: managed widgets that still drive input themselves

  Even when these screens ARE in the registry, they manually call SetInputMode/SetShowMouseCursor on activate. With bAutoManageInputMode = true the subsystem will also set it from HandleScreenActivated, racing against the widget's own call:

  - LootSelectionWidget.cpp:106-110, 443-447, 560-564 ← matches the file you have open
  - PauseMenuWidget.cpp:67-71
  - BackpackScreenWidget.cpp:925 (SetShowMouseCursor only)
  - SacrificeGraceOptionWidget.cpp:45-49
  - RunePurificationWidget.cpp:25-29
  - Plus YogPlayerControllerBase.cpp:360-371 and YogGameMode.cpp:881, 945 set input mode directly from outside any widget at all.

  This is precisely the kind of "two sources of truth" pattern that produces "cursor sometimes stuck visible / D-pad sometimes has no focus after revive" bugs.

  Recommended next steps (priority order)

  1. Migrate the two registry-eligible offenders first (cheap, mechanical):
    - TutorialPopup: replace GameDialogWidget.cpp:25 AddToViewport(200) with UIManager->PushScreen(EYogUIScreenId::TutorialPopup).
    - DamageEdgeFlash: replace PlayerCharacterBase.cpp:649-652 with UIManager->EnsureWidget(EYogUIScreenId::DamageEdgeFlash).
  2. Register the altar/shop/purification/sacrifice screens in EYogUIScreenId and route through PushScreen. Delete the per-widget SetInputMode blocks once the subsystem owns it.
  3. Pick one owner for input mode. Either keep bAutoManageInputMode = true and strip the widget-side PC->SetInputMode calls, or turn it off and centralize in a project policy file — but not both.
  4. Decide what to do with the Slate frontend menu. Either accept it as an isolated boot/death screen (and gate the subsystem off while it's up) or rebuild it as a UCommonActivatableWidget so it joins the same input model as everything else.
  5. The CombatDeckEdit gamepad floating drag slot (Z=10000) is fine functionally, but worth wrapping in a small helper on the parent widget so its lifetime is obviously paired with the drag operation.

✻ Cooked for 8m 9s

※ recap: You're auditing whether all widgets route through `UYogUIManagerSubsystem`; several wild `AddToViewport` sites exist (ShopActor, AltarActor, AltarMenuWidget, TutorialPopup, DamageEdgeFlash) plus a Slate frontend menu. Next: migrate `TutorialPopup` and
  `DamageEdgeFlash` to `PushScreen`/`EnsureWidget` since both are already in the registry. (disable recaps in /config)

─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
❯ 
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
  ? for shortcuts                                                         Update available! Run: winget upgrade Anthropic.ClaudeCode
  #: 4
  File:Line: UI/AltarMenuWidget.cpp:120-122
  Widget: URunePurificationWidget
  Touches input?: Yes — RunePurificationWidget.cpp:25-29
  Risk: Not in registry.
  ────────────────────────────────────────
  #: 5
  File:Line: UI/AltarMenuWidget.cpp:126-128
  Widget: USacrificeSelectionWidget (2nd path)
  Touches input?: Yes
  Risk: Same widget gets two creation routes (#3 and #5). Re-entry hazard.
  ────────────────────────────────────────
  #: 6
  File:Line: UI/CombatDeckEditWidget.cpp:832-848
  Widget: UCombatDeckEditCardSlotWidget (AddToPlayerScreen(10000))
  Touches input?: No (HitTestInvisible)
  Risk: Floating drag preview. Z=10000 puts it above everything including modal — likely intentional but unowned.
  ────────────────────────────────────────
  #: 7
  File:Line: UI/GameDialogWidget.cpp:25
  Widget: UTutorialPopupWidget (self-AddToViewport(200))
  Touches input?: No (deliberately avoids)
  Risk: TutorialPopup IS in the registry — should be PushScreen(TutorialPopup) instead of self-adding.
  ────────────────────────────────────────
  #: 8
  File:Line: Character/PlayerCharacterBase.cpp:649-652
  Widget: UDamageEdgeFlashWidget
  Touches input?: No
  Risk: DamageEdgeFlash IS in the registry — should be EnsureWidget(DamageEdgeFlash).

  C. Pure-Slate "wild" UI (entirely outside UMG/CommonUI)

  - System/YogGameInstanceBase.cpp:385-513, 677-832 — SDevKitFrontendMenuRoot + SButton/SOverlay/SBorder/SScaleBox/SVerticalBox/STextBlock/SImage/SBox
  tree for the frontend main menu and death menu. Calls PC->SetInputMode(FInputModeUIOnly/GameOnly) (lines 884-906) and
  FSlateApplication::Get().SetKeyboardFocus(...) (lines 897, 952, 984) directly.
    - Risk: this path competes with the subsystem on input-mode and focus. If a frontend menu is up while the subsystem's ApplyInputModeForLayer also
  runs (e.g. early ASC events fire), you get oscillating input mode.
    - InputActionRichTextDecorator.cpp:45-107 uses SNew(SBox/STextBlock/SImage) — these are inline rich-text decorations, not standalone screens. Not a
  problem.

  Bonus risk: managed widgets that still drive input themselves

  Even when these screens ARE in the registry, they manually call SetInputMode/SetShowMouseCursor on activate. With bAutoManageInputMode = true the
  subsystem will also set it from HandleScreenActivated, racing against the widget's own call:

  - LootSelectionWidget.cpp:106-110, 443-447, 560-564 ← matches the file you have open
  - PauseMenuWidget.cpp:67-71
  - BackpackScreenWidget.cpp:925 (SetShowMouseCursor only)
  - SacrificeGraceOptionWidget.cpp:45-49
  - RunePurificationWidget.cpp:25-29
  - Plus YogPlayerControllerBase.cpp:360-371 and YogGameMode.cpp:881, 945 set input mode directly from outside any widget at all.

  This is precisely the kind of "two sources of truth" pattern that produces "cursor sometimes stuck visible / D-pad sometimes has no focus after
  revive" bugs.

  Recommended next steps (priority order)

  1. Migrate the two registry-eligible offenders first (cheap, mechanical):
    - TutorialPopup: replace GameDialogWidget.cpp:25 AddToViewport(200) with UIManager->PushScreen(EYogUIScreenId::TutorialPopup).
    - DamageEdgeFlash: replace PlayerCharacterBase.cpp:649-652 with UIManager->EnsureWidget(EYogUIScreenId::DamageEdgeFlash).
  2. Register the altar/shop/purification/sacrifice screens in EYogUIScreenId and route through PushScreen. Delete the per-widget SetInputMode blocks
  once the subsystem owns it.
  3. Pick one owner for input mode. Either keep bAutoManageInputMode = true and strip the widget-side PC->SetInputMode calls, or turn it off and
  centralize in a project policy file — but not both.
  4. Decide what to do with the Slate frontend menu. Either accept it as an isolated boot/death screen (and gate the subsystem off while it's up) or
  rebuild it as a UCommonActivatableWidget so it joins the same input model as everything else.
  5. The CombatDeckEdit gamepad floating drag slot (Z=10000) is fine functionally, but worth wrapping in a small helper on the parent widget so its
  lifetime is obviously paired with the drag operation.

✻ Cooked for 8m 9s

※ recap: You're auditing whether all widgets route through `UYogUIManagerSubsystem`; several wild `AddToViewport` sites exist (ShopActor, AltarActor,
  AltarMenuWidget, TutorialPopup, DamageEdgeFlash) plus a Slate frontend menu. Next: migrate `TutorialPopup` and `DamageEdgeFlash` to
  `PushScreen`/`EnsureWidget` since both are already in the registry. (disable recaps in /config)

────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
❯ 
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
  ? for shortcuts                                                                           Update available! Run: winget upgrade Anthropic.ClaudeCode
  runs (e.g. early ASC events fire), you get oscillating input mode.
    - InputActionRichTextDecorator.cpp:45-107 uses SNew(SBox/STextBlock/SImage) — these are inline rich-text decorations, not standalone screens. Not a
  problem.

  Bonus risk: managed widgets that still drive input themselves

  Even when these screens ARE in the registry, they manually call SetInputMode/SetShowMouseCursor on activate. With bAutoManageInputMode = true the
  subsystem will also set it from HandleScreenActivated, racing against the widget's own call:

  - LootSelectionWidget.cpp:106-110, 443-447, 560-564 ← matches the file you have open
  - PauseMenuWidget.cpp:67-71
  - BackpackScreenWidget.cpp:925 (SetShowMouseCursor only)
  - SacrificeGraceOptionWidget.cpp:45-49
  - RunePurificationWidget.cpp:25-29
  - Plus YogPlayerControllerBase.cpp:360-371 and YogGameMode.cpp:881, 945 set input mode directly from outside any widget at all.

  This is precisely the kind of "two sources of truth" pattern that produces "cursor sometimes stuck visible / D-pad sometimes has no focus after
  revive" bugs.

  Recommended next steps (priority order)

  1. Migrate the two registry-eligible offenders first (cheap, mechanical):
    - TutorialPopup: replace GameDialogWidget.cpp:25 AddToViewport(200) with UIManager->PushScreen(EYogUIScreenId::TutorialPopup).
    - DamageEdgeFlash: replace PlayerCharacterBase.cpp:649-652 with UIManager->EnsureWidget(EYogUIScreenId::DamageEdgeFlash).
  2. Register the altar/shop/purification/sacrifice screens in EYogUIScreenId and route through PushScreen. Delete the per-widget SetInputMode blocks
  once the subsystem owns it.
  3. Pick one owner for input mode. Either keep bAutoManageInputMode = true and strip the widget-side PC->SetInputMode calls, or turn it off and
  centralize in a project policy file — but not both.
  4. Decide what to do with the Slate frontend menu. Either accept it as an isolated boot/death screen (and gate the subsystem off while it's up) or
  rebuild it as a UCommonActivatableWidget so it joins the same input model as everything else.
  5. The CombatDeckEdit gamepad floating drag slot (Z=10000) is fine functionally, but worth wrapping in a small helper on the parent widget so its
  lifetime is obviously paired with the drag operation.

✻ Cooked for 8m 9s

※ recap: You're auditing whether all widgets route through `UYogUIManagerSubsystem`; several wild `AddToViewport` sites exist (ShopActor, AltarActor,
  AltarMenuWidget, TutorialPopup, DamageEdgeFlash) plus a Slate frontend menu. Next: migrate `TutorialPopup` and `DamageEdgeFlash` to
  `PushScreen`/`EnsureWidget` since both are already in the registry. (disable recaps in /config)

────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
❯  
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
  ? for shortcuts                                                                           Update available! Run: winget upgrade Anthropic.ClaudeCode
