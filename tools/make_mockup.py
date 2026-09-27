# make_mockup.py — regenerates docs/images/screen-mockup.svg.
# Uses the tokens and grid numbers from docs/style-guide.md (regular density).
# Run from the repo root: python3 tools/make_mockup.py
SW = {'graphite':('#2C3034','#F2F3F4'),'steel':('#8A96A3','#0E0F10'),'volt':('#C6F432','#0E0F10'),
      'mint':('#3EE6A8','#0E0F10'),'aqua':('#2BD4E6','#0E0F10'),'azure':('#3D8BFF','#0E0F10'),
      'violet':('#9B6BFF','#0E0F10'),'magenta':('#F24FD1','#0E0F10'),'coral':('#FF5C5C','#0E0F10'),
      'tangerine':('#FF8A1F','#0E0F10'),'sun':('#FFD60A','#0E0F10'),'crimson':('#C8233B','#F2F3F4')}
BG,S1,S2,TEXT,MUTED,FAINT,OK='#101112','#1A1C1E','#24272A','#F2F3F4','#A0A6AC','#62696F','#3DDC84'
TILE,GUT,PADX,TOP,RAD=88,6,8,22+6,14
BEZ=18  # bezel around the 480x320 screen
FONT="'Space Grotesk','Montserrat','Segoe UI',Helvetica,Arial,sans-serif"

def mix(a,b,t):  # t = share of a
    a=[int(a[i:i+2],16) for i in (1,3,5)]; b=[int(b[i:i+2],16) for i in (1,3,5)]
    return '#%02X%02X%02X'%tuple(round(x*t+y*(1-t)) for x,y in zip(a,b))

# icons drawn in a 20x20 box, stroke style
I = {
 'copy':'<rect x="6" y="6" width="11" height="11" rx="2"/><path d="M3 13V5a2 2 0 0 1 2-2h8"/>',
 'paste':'<rect x="4" y="4" width="12" height="14" rx="2"/><rect x="7" y="2" width="6" height="4" rx="1"/>',
 'mail':'<rect x="2" y="4" width="16" height="12" rx="2"/><path d="M2.5 5.5 10 11l7.5-5.5"/>',
 'undo':'<path d="M7 5 3 9l4 4"/><path d="M3 9h9a5 5 0 0 1 0 10h-2"/>',
 'playpause':'<path d="M3 4v12l8-6z" fill="currentColor"/><path d="M14 4v12M18 4v12"/>',
 'volup':'<path d="M2 8h3l4-4v12l-4-4H2z"/><path d="M13 10h6M16 7v6"/>',
 'voldown':'<path d="M2 8h3l4-4v12l-4-4H2z"/><path d="M13 10h6"/>',
 'mute':'<path d="M2 8h3l4-4v12l-4-4H2z"/><path d="m13 7 6 6M19 7l-6 6"/>',
 'shot':'<path d="M2 7V4a2 2 0 0 1 2-2h3M13 2h3a2 2 0 0 1 2 2v3M18 13v3a2 2 0 0 1-2 2h-3M7 18H4a2 2 0 0 1-2-2v-3"/><circle cx="10" cy="10" r="3"/>',
 'lock':'<rect x="3" y="9" width="14" height="10" rx="2"/><path d="M6 9V6a4 4 0 0 1 8 0v3"/>',
}
tiles = [  # label, col, row, w, h, colour, style, icon
 ('Copy',0,0,1,1,'azure','solid','copy'),
 ('Paste',1,0,1,1,'azure','solid','paste'),
 ('Email sig',2,0,2,1,'violet','soft','mail'),
 ('Undo',4,0,1,1,'graphite','solid','undo'),
 ('Play / Pause',0,1,2,2,'volt','solid','playpause'),
 ('Vol +',2,1,1,1,'mint','solid','volup'),
 ('Mute',3,1,1,1,'coral','soft','mute'),
 ('Screen shot',4,1,1,2,'sun','solid','shot'),
 ('Vol −',2,2,1,1,'mint','solid','voldown'),
 ('Lock',3,2,1,1,'crimson','solid','lock'),
]
o=[]
W,H=480+2*BEZ,320+2*BEZ
o.append(f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}" font-family="{FONT}">')
o.append('<title>TriggerGrid screen mockup: one page of bento macro tiles</title>')
o.append(f'<rect width="{W}" height="{H}" rx="22" fill="#050506"/>')
o.append(f'<rect x="1" y="1" width="{W-2}" height="{H-2}" rx="21" fill="none" stroke="#2A2D30"/>')
o.append(f'<g transform="translate({BEZ} {BEZ})">')
o.append(f'<clipPath id="scr"><rect width="480" height="320" rx="4"/></clipPath><g clip-path="url(#scr)">')
o.append(f'<rect width="480" height="320" fill="{BG}"/>')
# status bar
o.append(f'<rect width="480" height="22" fill="{S2}"/>')
o.append(f'<text x="10" y="15.5" font-size="12" font-weight="500" fill="{TEXT}">Editing</text>')
o.append(f'<g fill="none" stroke="{OK}" stroke-width="1.6" stroke-linecap="round" transform="translate(382 5)">'
         '<path d="M1 5.5a9 9 0 0 1 12 0M3.5 8.2a5.5 5.5 0 0 1 7 0"/><circle cx="7" cy="11" r="0.9" fill="'+OK+'"/></g>')
o.append(f'<g fill="none" stroke="{OK}" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round" transform="translate(403 4)">'
         '<path d="M2 4.5 9 10.5 5.5 13.5V0.5L9 3.5 2 9.5"/></g>')
o.append(f'<text x="470" y="15.5" font-size="12" text-anchor="end" fill="{MUTED}">Desk-PC</text>')
# tiles
for n,(lab,c,r,w,h,col,sty,ic) in enumerate(tiles):
    x=PADX+c*(TILE+GUT); y=TOP+r*(TILE+GUT)
    tw=w*TILE+(w-1)*GUT; th=h*TILE+(h-1)*GUT
    sw,on=SW[col]
    if sty=='solid':
        fill,lc,icc=sw,on,on
    else:
        fill,lc,icc=mix(sw,S1,0.16),TEXT,sw
    o.append(f'<clipPath id="t{n}"><rect x="{x}" y="{y}" width="{tw}" height="{th}" rx="{RAD}"/></clipPath>')
    o.append(f'<rect x="{x}" y="{y}" width="{tw}" height="{th}" rx="{RAD}" fill="{fill}"/>')
    if sty=='soft':
        o.append(f'<rect x="{x}" y="{y+th-3}" width="{tw}" height="3" fill="{sw}" clip-path="url(#t{n})"/>')
    # label, up to 2 lines, top-left, 10px padding
    words=lab.split(' ')
    lines=[lab] if (tw>100 or len(lab)<=8) else [' '.join(words[:-1]),words[-1]]
    for i,l in enumerate(lines):
        o.append(f'<text x="{x+10}" y="{y+24+i*19}" font-size="16" font-weight="500" fill="{lc}">{l}</text>')
    o.append(f'<g transform="translate({x+10} {y+th-10-20})" fill="none" stroke="{icc}" color="{icc}" '
             f'stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round">{I[ic]}</g>')
# page dots
cx=240-14
for i in range(3):
    o.append(f'<circle cx="{cx+i*14}" cy="315" r="3" fill="{TEXT if i==0 else FAINT}"/>')
o.append('</g></g></svg>')
open('docs/images/screen-mockup.svg','w').write('\n'.join(o)+'\n')
