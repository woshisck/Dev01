"""UE5 default style colors, from SlateCore/Private/Styling/StyleColors.cpp.

Native Slate uses FStyleColors directly; the standalone app mirrors its dark theme.
No gradients, drop shadows, or decorative color coding for workflow categories.
"""
COLORS = {
    'background': '#151515', 'recessed': '#1a1a1a', 'panel': '#242424',
    'header': '#2f2f2f', 'input': '#0f0f0f', 'border': '#383838',
    'text': '#dddddd', 'muted': '#aaaaaa', 'disabled': '#777777',
    'primary': '#0070e0', 'hover': '#0e86ff', 'pressed': '#0050a0',
    'white': '#ffffff', 'warning': '#efb65b', 'success': '#7fca8a', 'error': '#ef8b8b',
}


def configure_style(style):
    c = COLORS
    style.theme_use('clam')
    style.configure('.', font=('Microsoft YaHei UI', 10), background=c['background'], foreground=c['text'], bordercolor=c['border'], lightcolor=c['border'], darkcolor=c['border'])
    style.configure('Panel.TFrame', background=c['panel'])
    style.configure('Panel.TLabel', background=c['panel'])
    style.configure('Muted.TLabel', foreground=c['muted'])
    style.configure('TButton', padding=(14, 8), background=c['header'], foreground=c['text'], borderwidth=0, relief='flat', focusthickness=1, focuscolor=c['primary'])
    style.map('TButton', background=[('disabled', c['panel']), ('pressed', c['border']), ('active', '#3d3d3d')], foreground=[('disabled', c['disabled'])])
    style.configure('Primary.TButton', background=c['primary'], foreground=c['white'])
    style.map('Primary.TButton', background=[('disabled', c['panel']), ('pressed', c['pressed']), ('active', c['hover'])], foreground=[('disabled', c['disabled']), ('!disabled', c['white'])])
    style.configure('Danger.TButton', foreground=c['warning'])
    style.configure('TNotebook', borderwidth=0, background=c['background'], tabmargins=0)
    style.layout('TNotebook.Tab', [])
    style.configure('TEntry', fieldbackground=c['input'], foreground=c['text'], padding=7, borderwidth=1, insertcolor=c['white'])
    style.map('TEntry', bordercolor=[('focus', c['primary'])])
    style.configure('Treeview', background=c['panel'], fieldbackground=c['panel'], foreground=c['text'], rowheight=34, borderwidth=0, indent=18)
    style.configure('Treeview.Heading', background=c['header'], foreground=c['text'], relief='flat', padding=8)
    style.map('Treeview', background=[('selected', c['primary'])], foreground=[('selected', c['white'])])
    style.configure('TScrollbar', background=c['border'], troughcolor=c['background'], borderwidth=0, arrowsize=12, relief='flat')
    style.configure('TProgressbar', background=c['primary'], troughcolor=c['panel'], borderwidth=0, thickness=3)
