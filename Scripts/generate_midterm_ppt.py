#!/usr/bin/env python3
from pathlib import Path
from xml.sax.saxutils import escape
import zipfile


OUT = Path("MID_TERM_DEFENSE.pptx")

SLIDE_W = 12192000
SLIDE_H = 6858000


def emu(x):
    return int(x)


def esc(text):
    return escape(str(text), {'"': "&quot;"})


def color(hex_color):
    return hex_color.replace("#", "").upper()


class Slide:
    def __init__(self, title=None, subtitle=None, dark=False):
        self.shapes = []
        self.image_rels = []
        self.next_id = 2
        self.dark = dark
        self.bg = "101820" if dark else "F7F9FC"
        self.title = title
        self.subtitle = subtitle
        if title:
            self.text(title, 520000, 350000, 10400000, 520000, size=34,
                      bold=True, fill=None, font_color="FFFFFF" if dark else "18324A")
        if subtitle:
            self.text(subtitle, 540000, 910000, 10300000, 360000, size=15,
                      font_color="DDE8F3" if dark else "4D6174")

    def _id(self):
        sid = self.next_id
        self.next_id += 1
        return sid

    def text(self, text, x, y, w, h, size=18, bold=False, font_color="263238",
             fill=None, line=None, radius=False, align="l", valign="mid"):
        sid = self._id()
        bold_attr = 'b="1"' if bold else ""
        fill_xml = ""
        if fill:
            fill_xml = f'<a:solidFill><a:srgbClr val="{color(fill)}"/></a:solidFill>'
        else:
            fill_xml = '<a:noFill/>'
        line_xml = '<a:ln><a:noFill/></a:ln>' if not line else (
            f'<a:ln w="9525"><a:solidFill><a:srgbClr val="{color(line)}"/></a:solidFill></a:ln>'
        )
        geom = "roundRect" if radius else "rect"
        paras = []
        for raw in str(text).split("\n"):
            if not raw:
                raw = " "
            paras.append(
                f'<a:p><a:pPr algn="{align}"/><a:r><a:rPr lang="zh-CN" sz="{size * 100}" '
                f'{bold_attr}><a:solidFill><a:srgbClr val="{color(font_color)}"/>'
                f'</a:solidFill><a:latin typeface="Noto Sans CJK SC"/><a:ea typeface="Noto Sans CJK SC"/>'
                f'</a:rPr><a:t>{esc(raw)}</a:t></a:r></a:p>'
            )
        self.shapes.append(f'''
        <p:sp>
          <p:nvSpPr><p:cNvPr id="{sid}" name="TextBox {sid}"/><p:cNvSpPr txBox="1"/><p:nvPr/></p:nvSpPr>
          <p:spPr><a:xfrm><a:off x="{emu(x)}" y="{emu(y)}"/><a:ext cx="{emu(w)}" cy="{emu(h)}"/></a:xfrm>
            <a:prstGeom prst="{geom}"><a:avLst/></a:prstGeom>{fill_xml}{line_xml}</p:spPr>
          <p:txBody><a:bodyPr wrap="square" anchor="{valign}" lIns="91440" tIns="45720" rIns="91440" bIns="45720"/>
            <a:lstStyle/>{"".join(paras)}</p:txBody>
        </p:sp>''')

    def image(self, path, x=0, y=0, w=SLIDE_W, h=SLIDE_H):
        sid = self._id()
        rid = f"rId{len(self.image_rels) + 2}"
        self.image_rels.append((rid, Path(path)))
        self.shapes.append(f'''
        <p:pic>
          <p:nvPicPr><p:cNvPr id="{sid}" name="Picture {sid}"/><p:cNvPicPr/><p:nvPr/></p:nvPicPr>
          <p:blipFill><a:blip r:embed="{rid}"/><a:stretch><a:fillRect/></a:stretch></p:blipFill>
          <p:spPr><a:xfrm><a:off x="{emu(x)}" y="{emu(y)}"/><a:ext cx="{emu(w)}" cy="{emu(h)}"/></a:xfrm>
            <a:prstGeom prst="rect"><a:avLst/></a:prstGeom></p:spPr>
        </p:pic>''')

    def bar_chart(self, title, labels, values, x, y, w, h, max_value=None, unit=""):
        if max_value is None:
            max_value = max(values)
        self.text(title, x, y, w, 320000, size=17, bold=True, font_color="18324A")
        chart_y = y + 520000
        row_h = int((h - 560000) / len(values))
        for i, (label, val) in enumerate(zip(labels, values)):
            yy = chart_y + i * row_h
            self.text(label, x, yy, 1700000, 280000, size=13, font_color="263238")
            self.box("", x + 1800000, yy + 25000, w - 3000000, 230000, fill="E3EAF1", line="E3EAF1")
            bw = int((w - 3000000) * val / max_value)
            self.box("", x + 1800000, yy + 25000, bw, 230000, fill="3A7CA5", line="3A7CA5")
            self.text(f"{val:g}{unit}", x + w - 1050000, yy, 950000, 280000, size=13, bold=True, font_color="18324A", align="r")

    def bullet_list(self, items, x, y, w, h, size=19, font_color="263238", gap=305000):
        top = y
        for item in items:
            self.text("• " + item, x, top, w, 330000, size=size, font_color=font_color, valign="top")
            top += gap

    def box(self, text, x, y, w, h, fill="FFFFFF", line="B7C4D0", size=17,
            font_color="1F3447", bold=False):
        self.text(text, x, y, w, h, size=size, bold=bold, font_color=font_color,
                  fill=fill, line=line, radius=True)

    def arrow(self, text, x, y, w=420000, h=260000, size=22, font_color="4677A6"):
        self.text(text, x, y, w, h, size=size, bold=True, font_color=font_color, align="c")

    def table(self, headers, rows, x, y, w, row_h=390000, col_weights=None):
        if col_weights is None:
            col_weights = [1] * len(headers)
        total = sum(col_weights)
        col_ws = [int(w * c / total) for c in col_weights]
        cur_x = x
        for i, head in enumerate(headers):
            self.box(head, cur_x, y, col_ws[i], row_h, fill="1F4E79", line="1F4E79",
                     size=13, font_color="FFFFFF", bold=True)
            cur_x += col_ws[i]
        for r, row in enumerate(rows):
            cur_x = x
            fill = "FFFFFF" if r % 2 == 0 else "EDF3F8"
            for i, cell in enumerate(row):
                self.box(cell, cur_x, y + row_h * (r + 1), col_ws[i], row_h,
                         fill=fill, line="D2DBE4", size=12, font_color="22313F")
                cur_x += col_ws[i]

    def xml(self):
        body = "".join(self.shapes)
        return f'''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<p:sld xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main"
       xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"
       xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main">
  <p:cSld><p:bg><p:bgPr><a:solidFill><a:srgbClr val="{self.bg}"/></a:solidFill><a:effectLst/></p:bgPr></p:bg>
    <p:spTree>
      <p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>
      <p:grpSpPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="0" cy="0"/><a:chOff x="0" y="0"/><a:chExt cx="0" cy="0"/></a:xfrm></p:grpSpPr>
      {body}
    </p:spTree>
  </p:cSld><p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr>
</p:sld>'''


def content_types(n):
    slide_overrides = "\n".join(
        f'<Override PartName="/ppt/slides/slide{i}.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slide+xml"/>'
        for i in range(1, n + 1)
    )
    return f'''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Default Extension="png" ContentType="image/png"/>
  <Override PartName="/docProps/core.xml" ContentType="application/vnd.openxmlformats-package.core-properties+xml"/>
  <Override PartName="/docProps/app.xml" ContentType="application/vnd.openxmlformats-officedocument.extended-properties+xml"/>
  <Override PartName="/ppt/presentation.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml"/>
  <Override PartName="/ppt/slideMasters/slideMaster1.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slideMaster+xml"/>
  <Override PartName="/ppt/slideLayouts/slideLayout1.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slideLayout+xml"/>
  <Override PartName="/ppt/theme/theme1.xml" ContentType="application/vnd.openxmlformats-officedocument.theme+xml"/>
  {slide_overrides}
</Types>'''


def rels_root():
    return '''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="ppt/presentation.xml"/>
  <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties" Target="docProps/core.xml"/>
  <Relationship Id="rId3" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties" Target="docProps/app.xml"/>
</Relationships>'''


def presentation(n):
    ids = "\n".join(
        f'<p:sldId id="{255 + i}" r:id="rId{i + 1}"/>' for i in range(1, n + 1)
    )
    return f'''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<p:presentation xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main"
 xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"
 xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main">
  <p:sldMasterIdLst><p:sldMasterId id="2147483648" r:id="rId1"/></p:sldMasterIdLst>
  <p:sldIdLst>{ids}</p:sldIdLst>
  <p:sldSz cx="{SLIDE_W}" cy="{SLIDE_H}" type="wide"/>
  <p:notesSz cx="6858000" cy="9144000"/>
</p:presentation>'''


def presentation_rels(n):
    rels = ['<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster" Target="slideMasters/slideMaster1.xml"/>']
    for i in range(1, n + 1):
        rels.append(f'<Relationship Id="rId{i + 1}" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide" Target="slides/slide{i}.xml"/>')
    return f'''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  {"".join(rels)}
</Relationships>'''


def slide_rels(slide, media_names):
    rels = ['<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout" Target="../slideLayouts/slideLayout1.xml"/>']
    for rid, path in slide.image_rels:
        rels.append(f'<Relationship Id="{rid}" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/image" Target="../media/{media_names[path.resolve()]}"/>')
    return f'''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  {"".join(rels)}
</Relationships>'''


def slide_master():
    return '''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<p:sldMaster xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main"
 xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"
 xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main">
  <p:cSld><p:spTree><p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>
  <p:grpSpPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="0" cy="0"/><a:chOff x="0" y="0"/><a:chExt cx="0" cy="0"/></a:xfrm></p:grpSpPr></p:spTree></p:cSld>
  <p:clrMap bg1="lt1" tx1="dk1" bg2="lt2" tx2="dk2" accent1="accent1" accent2="accent2" accent3="accent3" accent4="accent4" accent5="accent5" accent6="accent6" hlink="hlink" folHlink="folHlink"/>
  <p:sldLayoutIdLst><p:sldLayoutId id="2147483649" r:id="rId1"/></p:sldLayoutIdLst>
  <p:txStyles><p:titleStyle/><p:bodyStyle/><p:otherStyle/></p:txStyles>
</p:sldMaster>'''


def slide_master_rels():
    return '''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout" Target="../slideLayouts/slideLayout1.xml"/>
  <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme" Target="../theme/theme1.xml"/>
</Relationships>'''


def slide_layout():
    return '''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<p:sldLayout xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main"
 xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"
 xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main" type="blank" preserve="1">
  <p:cSld name="Blank"><p:spTree><p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>
  <p:grpSpPr><a:xfrm><a:off x="0" y="0"/><a:ext cx="0" cy="0"/><a:chOff x="0" y="0"/><a:chExt cx="0" cy="0"/></a:xfrm></p:grpSpPr></p:spTree></p:cSld>
  <p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr>
</p:sldLayout>'''


def slide_layout_rels():
    return '''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster" Target="../slideMasters/slideMaster1.xml"/>
</Relationships>'''


def theme():
    return '''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<a:theme xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" name="Midterm Theme">
  <a:themeElements><a:clrScheme name="Custom">
    <a:dk1><a:srgbClr val="1C2733"/></a:dk1><a:lt1><a:srgbClr val="FFFFFF"/></a:lt1>
    <a:dk2><a:srgbClr val="18324A"/></a:dk2><a:lt2><a:srgbClr val="F7F9FC"/></a:lt2>
    <a:accent1><a:srgbClr val="1F4E79"/></a:accent1><a:accent2><a:srgbClr val="3A7CA5"/></a:accent2>
    <a:accent3><a:srgbClr val="5EA8A7"/></a:accent3><a:accent4><a:srgbClr val="D99A2B"/></a:accent4>
    <a:accent5><a:srgbClr val="7D6B91"/></a:accent5><a:accent6><a:srgbClr val="6B8E23"/></a:accent6>
    <a:hlink><a:srgbClr val="0563C1"/></a:hlink><a:folHlink><a:srgbClr val="954F72"/></a:folHlink>
  </a:clrScheme><a:fontScheme name="Custom"><a:majorFont><a:latin typeface="Noto Sans CJK SC"/><a:ea typeface="Noto Sans CJK SC"/></a:majorFont><a:minorFont><a:latin typeface="Noto Sans CJK SC"/><a:ea typeface="Noto Sans CJK SC"/></a:minorFont></a:fontScheme><a:fmtScheme name="Custom"><a:fillStyleLst/><a:lnStyleLst/><a:effectStyleLst/><a:bgFillStyleLst/></a:fmtScheme></a:themeElements>
</a:theme>'''


def app_props(n):
    return f'''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Properties xmlns="http://schemas.openxmlformats.org/officeDocument/2006/extended-properties"
 xmlns:vt="http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes">
  <Application>Codex</Application><PresentationFormat>On-screen Show (16:9)</PresentationFormat><Slides>{n}</Slides>
</Properties>'''


def core_props():
    return '''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<cp:coreProperties xmlns:cp="http://schemas.openxmlformats.org/package/2006/metadata/core-properties"
 xmlns:dc="http://purl.org/dc/elements/1.1/"
 xmlns:dcterms="http://purl.org/dc/terms/"
 xmlns:dcmitype="http://purl.org/dc/dcmitype/"
 xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
  <dc:title>中期答辩材料 - L476 + SX1280 LWB 项目</dc:title>
  <dc:creator>Codex</dc:creator>
</cp:coreProperties>'''


def build_slides():
    slides = []
    cover_img = Path("assets/ppt/cover-lab.png")
    network_img = Path("assets/ppt/lwb-network.png")
    pcb_img = Path("assets/ppt/custom-pcb.png")
    real_devboard_img = next(
        (p for p in [
            Path("assets/ppt/real-devboard.png"),
            Path("assets/ppt/real-devboard.jpg"),
            Path("assets/ppt/real-devboard.jpeg"),
        ] if p.exists()),
        None,
    )
    l433_sx1262_img = next(
        (p for p in [
            Path("assets/ppt/l433-sx1262-devboard.png"),
            Path("assets/ppt/l433-sx1262-devboard.jpg"),
            Path("assets/ppt/l433-sx1262-devboard.jpeg"),
        ] if p.exists()),
        None,
    )
    original_comboard_img = next(
        (p for p in [
            Path("assets/ppt/l433-sx1262-comboard.png"),
            Path("assets/ppt/l433-sx1262-comboard.jpg"),
            Path("assets/ppt/l433-sx1262-comboard.jpeg"),
        ] if p.exists()),
        None,
    )
    logic_analyzer_img = Path("assets/ppt/logic_analyzer_round_view.png")
    if not logic_analyzer_img.exists():
        logic_analyzer_img = None

    s = Slide(dark=True)
    if real_devboard_img:
        s.image(real_devboard_img, 4300000, 0, 7892000, SLIDE_H)
    elif cover_img.exists():
        s.image(cover_img)
    s.box("", 0, 0, 5350000, SLIDE_H, fill="101820", line="101820")
    s.text("中期答辩", 620000, 740000, 4300000, 560000, size=28, bold=True, font_color="9BD2F0")
    s.text("STM32L476RG + SX1280\nLWB 低功耗无线总线项目", 620000, 1420000, 4300000, 1180000, size=31, bold=True, font_color="FFFFFF")
    s.text("重新设计 comboard，并让 LWB 在自制硬件上稳定运行", 650000, 3000000, 4100000, 520000, size=17, font_color="DDE8F3")
    s.box("当前状态\nL476 + SX1280 开发板验证", 760000, 4050000, 1800000, 850000, fill="1F4E79", line="3A7CA5", size=15, font_color="FFFFFF", bold=True)
    s.box("下一阶段\nPCB bring-up", 2780000, 4050000, 1800000, 850000, fill="365C7D", line="5EA8A7", size=17, font_color="FFFFFF", bold=True)
    slides.append(s)

    s = Slide("项目目标与交付物", "硬件集成 + 协议栈运行验证")
    if pcb_img.exists():
        s.image(pcb_img, 6900000, 1260000, 3900000, 2400000)
    s.bullet_list([
        "自制 STM32L476RG + SX1280 comboard",
        "跑通 LWB / Gloria 协议栈",
        "验证 LoRa / GFSK / FLRC 三种调制方式",
        "固化开发板阶段 PB4 / PB11 双用途 DIO1 连接",
    ], 760000, 1500000, 5200000, 2200000)
    s.box("硬件\nMCU / radio / SWD / UART / 电源 / 时钟 / RF", 6800000, 3820000, 3600000, 680000, fill="E8F1F8", line="B7C4D0", size=16, bold=True)
    s.box("软件\nLWB 调度 + Gloria flood + SX1280 driver", 6800000, 4660000, 3600000, 680000, fill="EAF6F4", line="B7C4D0", size=16, bold=True)
    s.box("价值\n2.4 GHz 多调制 IoT mesh 原型平台", 6800000, 5500000, 3600000, 680000, fill="FFF4DD", line="D99A2B", size=16, bold=True)
    slides.append(s)

    s = Slide("项目迭代路径", "每次只替换一个关键变量，降低定位难度")
    xs = [650000, 3350000, 6050000, 8750000]
    labels = [
        "1\nL433 + SX1262\nNUCLEO 适配",
        "2\nL476 + SX1262\nMCU 迁移",
        "3\nL476 + SX1280\n当前仓库",
        "4\n自制 comboard PCB\n下一阶段",
    ]
    fills = ["E8F1F8", "EAF6F4", "DDEBFF", "FFF4DD"]
    for i, x in enumerate(xs):
        s.box(labels[i], x, 2200000, 2200000, 1200000, fill=fills[i], line="5B7894", size=17, bold=True)
        if i < 3:
            s.arrow("→", x + 2200000, 2600000)
    s.text("已完成：开发板适配、MCU 迁移、radio 迁移；下一步：投板、焊接、烧录、板级 bring-up",
           820000, 4400000, 10000000, 460000, size=19, font_color="263238", fill="FFFFFF", line="D2DBE4", radius=True)
    slides.append(s)

    s = Slide("硬件迭代实物对比", "从 L433 + SX1262 开发板连接，过渡到 L476 + SX1280 杜邦线原型")
    if l433_sx1262_img:
        s.image(l433_sx1262_img, 650000, 1350000, 4500000, 3350000)
    else:
        s.box("阶段 1 照片占位\nassets/ppt/l433-sx1262-devboard.jpg\n\nL433 + SX1262\nEvaluation Board", 650000, 1350000, 4500000, 3350000, fill="E8F1F8", line="B7C4D0", size=20, bold=True)
    if real_devboard_img:
        s.image(real_devboard_img, 6650000, 1350000, 4500000, 3350000)
    else:
        s.box("阶段 3 照片占位\nassets/ppt/real-devboard.jpg\n\nL476 + SX1280\n杜邦线连接", 6650000, 1350000, 4500000, 3350000, fill="EAF6F4", line="B7C4D0", size=20, bold=True)
    s.box("阶段 1\nL433 + SX1262\n脱离原专用 comboard，完成开发板适配", 820000, 5000000, 4100000, 700000, fill="FFFFFF", line="5B7894", size=16, bold=True)
    s.arrow("→", 5450000, 5150000, 800000, 300000, size=26, font_color="D99A2B")
    s.box("阶段 3\nL476 + SX1280\n完成 radio 迁移，三种 modulation 端到端验证", 6820000, 5000000, 4100000, 700000, fill="FFFFFF", line="5EA8A7", size=16, bold=True)
    slides.append(s)

    s = Slide("最终成品形态参考", "原 L433 + SX1262 comboard 是形态参考，最终将设计为 L476 + SX1280 comboard")
    if original_comboard_img:
        s.image(original_comboard_img, 780000, 1260000, 4200000, 4250000)
    else:
        s.box("原 comboard 照片占位\nassets/ppt/l433-sx1262-comboard.jpg\n\nL433 + SX1262 comboard", 780000, 1260000, 4200000, 4250000, fill="E8F1F8", line="B7C4D0", size=20, bold=True)
    s.box("已有参考板\nL433 + SX1262 comboard", 5600000, 1500000, 2600000, 820000, fill="1F4E79", line="1F4E79", size=19, font_color="FFFFFF", bold=True)
    s.arrow("形态继承\n功能升级", 8350000, 1700000, 1200000, 520000, size=18, font_color="D99A2B")
    s.box("最终目标\nL476 + SX1280 comboard", 9700000, 1500000, 1700000, 820000, fill="5EA8A7", line="5EA8A7", size=17, font_color="FFFFFF", bold=True)
    s.bullet_list([
        "板形：保持 comboard 独立小板形态，而不是长期使用 NUCLEO + 模组",
        "MCU：从 STM32L433 升级到 STM32L476，获得更充裕 Flash/RAM/GPIO",
        "Radio：从 SX1262 Sub-GHz 切换到 SX1280 2.4 GHz",
        "接口：保留 SWD / UART / 电源 / 天线 / test point 等 bring-up 必需接口",
        "关键连接：将开发板阶段 PB4 ↔ PB11 的 DIO1 双连接固化到 PCB net",
    ], 5600000, 2800000, 5600000, 2300000, size=16, gap=380000)
    s.box("答辩表述：前期用开发板降低移植风险，最终交付会回到这种紧凑 comboard 形态。", 5600000, 5520000, 5600000, 560000, fill="FFF4DD", line="D99A2B", size=16, bold=True)
    slides.append(s)

    s = Slide("LWB 是什么", "Low-power Wireless Bus：把多跳无线网络抽象成一条共享总线")
    s.box("传统 mesh\n邻居表 + 路由表 + 转发状态\n拓扑变化处理成本高", 850000, 1750000, 4300000, 1300000, fill="F4EAEA", line="C98787", size=17)
    s.arrow("对比", 5500000, 2260000, 700000, 300000, size=20, font_color="D99A2B")
    s.box("LWB\nhost 统一调度\nslot 内用 Gloria/ST flood 全网传播\n节点不维护路由", 6650000, 1600000, 4300000, 1600000, fill="EAF6F4", line="5EA8A7", size=17, bold=True)
    s.box("关键点：slot 是集中调度的，但每个 slot 内部的 packet 传播是全网同步泛洪。", 1450000, 4200000, 9000000, 700000, fill="FFFFFF", line="B7C4D0", size=20, bold=True)
    slides.append(s)

    s = Slide(dark=True)
    if network_img.exists():
        s.image(network_img)
    s.box("", 0, 0, SLIDE_W, 1200000, fill="101820", line="101820")
    s.text("LWB 原理图", 520000, 330000, 5000000, 500000, size=34, bold=True, font_color="FFFFFF")
    s.text("host 下发 schedule，节点按 slot 通过 Gloria flood 发送数据", 540000, 840000, 7600000, 300000, size=15, font_color="DDE8F3")
    s.box("", 400000, 1350000, 10950000, 4300000, fill="F7F9FC", line="D2DBE4")
    s.box("Host\n调度器 / 时间基准", 600000, 1550000, 2100000, 760000, fill="1F4E79", line="1F4E79", size=18, font_color="FFFFFF", bold=True)
    s.arrow("SCHED flood", 2950000, 1770000, 1500000, 300000)
    s.box("Virtual Shared Bus\n逻辑共享总线", 4550000, 1550000, 2650000, 760000, fill="EAF6F4", line="5EA8A7", size=18, bold=True)
    for i, label in enumerate(["Node 1\nslot 1", "Node 2\nslot 2", "Node N\nslot n"]):
        x = 2450000 + i * 2500000
        s.box(label, x, 3300000, 1850000, 720000, fill="FFFFFF", line="B7C4D0", size=17, bold=True)
        s.arrow("DATA flood ↑", x + 160000, 2800000, 1400000, 300000, size=16, font_color="4677A6")
    s.box("不维护路由表：每个 data slot 内，数据包通过同步泛洪覆盖全网。", 1350000, 5000000, 9200000, 540000, fill="FFF4DD", line="D99A2B", size=18)
    slides.append(s)

    s = Slide("LWB 一个 Round 怎么运行", "当前配置：LWB_SCHED_PERIOD = 15 s")
    parts = [
        ("S1", "host 广播\nschedule"),
        ("C", "新节点\n入网请求"),
        ("D1", "节点 1\n发数据"),
        ("D2", "节点 2\n发数据"),
        ("Dn", "节点 N\n发数据"),
        ("S2", "确认 / 下轮\n信息同步"),
    ]
    x = 800000
    widths = [1450000, 1200000, 1450000, 1450000, 1450000, 1600000]
    for i, (code, text) in enumerate(parts):
        s.box(f"{code}\n{text}", x, 2350000, widths[i], 1050000, fill="FFFFFF" if i % 2 else "E8F1F8", line="5B7894", size=16, bold=True)
        x += widths[i] + 160000
    s.text("t = 0", 790000, 1900000, 800000, 300000, size=16, font_color="4D6174")
    s.text("t = 15 s", 10100000, 1900000, 1200000, 300000, size=16, font_color="4D6174")
    s.bullet_list([
        "source node 上电后先 bootstrap，长时间 RX 等 schedule",
        "收到有效 schedule 后进入同步状态，只在需要时唤醒",
        "每个 schedule/data 传播都由 Gloria flood 完成",
    ], 1150000, 4300000, 9200000, 1200000, size=18)
    slides.append(s)

    s = Slide("为什么要改硬件", "开发板验证完成后，需要把协议时序约束固化到 PCB")
    if cover_img.exists():
        s.image(cover_img, 7350000, 1220000, 3600000, 1850000)
    s.bullet_list([
        "原始平台绑定 L433 + SX1262 专用 comboard",
        "当前开发板方案依赖 NUCLEO + DLP-RFS1280 + PB4/PB11 飞线",
        "最终交付需要稳定的电源、时钟、调试口、RF 和 test point",
        "DIO1 必须同时进入 EXTI 与 TIM2_CH4，PCB 上要设计为同一个 net",
    ], 800000, 1420000, 6500000, 1800000, size=18)
    s.box("DIO1 双用途", 7800000, 3250000, 2600000, 520000, fill="1F4E79", line="1F4E79", size=20, font_color="FFFFFF", bold=True)
    s.box("PB4 / EXTI4\n低功耗唤醒", 7700000, 4050000, 2800000, 820000, fill="EAF6F4", line="5EA8A7", size=18, bold=True)
    s.box("PB11 / TIM2_CH4\n微秒级时间戳", 7700000, 5200000, 2800000, 820000, fill="FFF4DD", line="D99A2B", size=18, bold=True)
    slides.append(s)

    s = Slide("当前开发板原型", "NUCLEO-L476RG 与 DLP-RFS1280 通过杜邦线连接")
    if real_devboard_img:
        s.image(real_devboard_img, 650000, 1320000, 5200000, 3900000)
    else:
        s.box("真实照片占位\nassets/ppt/real-devboard.jpg", 650000, 1320000, 5200000, 3900000, fill="E8F1F8", line="B7C4D0", size=24, bold=True)
    s.box("验证平台", 6300000, 1380000, 3800000, 520000, fill="1F4E79", line="1F4E79", size=20, font_color="FFFFFF", bold=True)
    s.bullet_list([
        "MCU：NUCLEO-L476RG 开发板",
        "Radio：DLP-RFS1280 / SX1280 2.4 GHz 模组",
        "连接：SPI1、NRESET、BUSY、DIO1、ANTSEL、电源和 GND",
        "DIO1：开发板阶段用杜邦线实现 PB4 与 PB11 同网",
        "用途：验证 LWB/Gloria 在 L476 + SX1280 上端到端运行",
    ], 6250000, 2100000, 4800000, 2300000, size=17, gap=390000)
    s.box("这页建议答辩时作为“真实硬件证据”：说明当前不是仿真，而是双板 HOST + NODE 实物验证。", 6200000, 5050000, 4700000, 700000, fill="FFF4DD", line="D99A2B", size=16, bold=True)
    slides.append(s)

    s = Slide("为什么选 L476", "主频不是关键，资源余量和 GPIO 更重要")
    s.table(["项目", "STM32L433CC", "STM32L476RG"], [
        ["内核", "Cortex-M4F", "Cortex-M4F"],
        ["最大主频", "80 MHz", "80 MHz"],
        ["Flash", "256 KB", "1024 KB"],
        ["RAM", "64 KB", "128 KB"],
        ["GPIO", "约 37", "LQFP64 下约 51"],
    ], 760000, 1350000, 5600000, row_h=430000, col_weights=[1.1, 1.5, 1.5])
    s.bar_chart("资源对比", ["Flash", "RAM", "GPIO"], [1024, 128, 51], 6900000, 1430000, 3600000, 1800000, max_value=1024)
    s.text("L433 基准：Flash 256 KB / RAM 64 KB / GPIO 约 37", 6960000, 3440000, 3500000, 360000, size=13, font_color="4D6174")
    s.box("结论：L476 的价值主要是 Flash/RAM/GPIO 更充裕，同时保留低功耗 MCU 特性。", 1300000, 5000000, 9500000, 560000, fill="EAF6F4", line="5EA8A7", size=18, bold=True)
    slides.append(s)

    s = Slide("为什么选 SX1280", "从 Sub-GHz 转向 2.4 GHz，多调制和后续测距能力更适合目标")
    s.table(["项目", "SX1262", "SX1280"], [
        ["频段", "Sub-GHz", "2.4 GHz ISM"],
        ["调制", "LoRa / FSK", "LoRa / GFSK / FLRC / BLE-compatible / Ranging"],
        ["最大发射功率", "+22 dBm", "+12.5 dBm"],
        ["优势", "远距离、穿墙强", "全球通用、天线小、多调制"],
        ["代价", "法规和频段复杂", "距离和穿墙弱于 Sub-GHz"],
    ], 700000, 1350000, 7600000, row_h=455000, col_weights=[1.0, 1.65, 2.25])
    s.box("2.4 GHz", 8700000, 1480000, 2100000, 620000, fill="1F4E79", line="1F4E79", size=25, font_color="FFFFFF", bold=True)
    s.box("LoRa\nGFSK\nFLRC\nRanging", 8700000, 2350000, 2100000, 1800000, fill="EAF6F4", line="5EA8A7", size=21, bold=True)
    s.box("+12.5 dBm\n最大发射功率", 8700000, 4400000, 2100000, 720000, fill="FFF4DD", line="D99A2B", size=17, bold=True)
    s.box("项目定位：室内 LWB mesh、小范围多节点通信，并预留室内定位扩展。", 1200000, 5600000, 9800000, 560000, fill="FFF4DD", line="D99A2B", size=18, bold=True)
    slides.append(s)

    s = Slide("代码迁移分层", "上层 LWB 思想保持不变，底层 radio 与时序需要重建")
    layers = [
        ("MCU / 时钟层", "startup / linker / SYSCLK / TIM2 / LPTIM / USART"),
        ("板级 GPIO 层", "SPI / NSS / BUSY / DIO1 / ANTSEL"),
        ("Radio driver 层", "SX1280 opcode / register / packet type / CRC / timeout"),
        ("Radio 参数层", "2.4 GHz 频率表 / 功率表 / LoRa-GFSK-FLRC 调制表"),
        ("协议时序层", "Gloria trigger delay / slot overhead / LWB period"),
    ]
    y = 1320000
    for i, (name, desc) in enumerate(layers):
        s.box(name, 900000, y, 2600000, 560000, fill="1F4E79" if i == 2 else "E8F1F8", line="5B7894", size=17, font_color="FFFFFF" if i == 2 else "1F3447", bold=True)
        s.box(desc, 3650000, y, 6900000, 560000, fill="FFFFFF", line="D2DBE4", size=15)
        y += 720000
    slides.append(s)

    s = Slide("代码结构图", "应用任务到硬件驱动的主链路")
    items = [
        "task_pre / task_post",
        "FreeRTOS queues",
        "LWB\nround / schedule / slot",
        "Gloria / Glossy\n同步泛洪",
        "Radio abstraction",
        "SX1280 Radio API",
        "SX1280 command driver",
        "time + system\n底层支撑",
        "STM32L476 + SX1280",
    ]
    x = 620000
    y = 1730000
    w = 1080000
    for i, item in enumerate(items):
        s.box(item, x, y, w, 760000, fill="EAF6F4" if i in [3, 4] else "FFFFFF", line="5B7894", size=12, bold=True)
        if i < len(items) - 1:
            s.arrow("→", x + w, y + 230000, 350000, 260000, size=17)
        x += w + 350000
    s.box("核心三层：LWB scheduler / Gloria synchronous flood / radio abstraction + SX1280 driver",
          1300000, 4300000, 9500000, 620000, fill="FFF4DD", line="D99A2B", size=18, bold=True)
    slides.append(s)

    s = Slide("关键参数与已修复问题", "当前默认使用 FLRC 260 kbit/s，15 秒一个 round")
    s.table(["参数", "当前值"], [
        ["GLORIA_INTERFACE_MODULATION", "11 = FLRC 260 kbit/s"],
        ["GLORIA_INTERFACE_RF_BAND", "24 = 2450 MHz"],
        ["LWB_SCHED_PERIOD", "15 s"],
        ["LWB_N_TX", "2"],
        ["LWB_NUM_HOPS", "6"],
        ["HS_TIMER_FREQUENCY", "8 MHz"],
    ], 800000, 1350000, 4800000, row_h=410000, col_weights=[1.8, 1.6])
    s.bullet_list([
        "修复 SX1280 timeout continuous RX 行为",
        "修复 GFSK whitening seed 覆盖 CRC poly",
        "修复 SX1280 variable/fixed length packet type",
        "补齐 FLRC 3-byte CRC 和 max payload 设置",
        "提升 LWB_MAX_PAYLOAD_LEN，避免 DPP 最小包被拒收",
    ], 6200000, 1450000, 4800000, 1800000, size=16, gap=360000)
    slides.append(s)

    s = Slide("逻辑分析仪验证", "HOST 与 client 的 TX/RX 脉冲按 LWB round 时序出现")
    if logic_analyzer_img:
        s.image(logic_analyzer_img, 520000, 1250000, 11150000, 3860000)
    else:
        s.box("逻辑分析仪波形占位\nassets/ppt/logic_analyzer_round_view.png", 520000, 1250000, 11150000, 3860000, fill="101820", line="4D6174", size=24, font_color="FFFFFF", bold=True)
    s.box("D4 client PA12 RX", 680000, 5250000, 2200000, 460000, fill="FFF4DD", line="D99A2B", size=14, bold=True)
    s.box("D5 client PA11 TX", 3050000, 5250000, 2200000, 460000, fill="EAF6F4", line="5EA8A7", size=14, bold=True)
    s.box("D6 HOST PA12 RX", 5420000, 5250000, 2200000, 460000, fill="E8F1F8", line="3A7CA5", size=14, bold=True)
    s.box("D7 HOST PA11 TX", 7790000, 5250000, 2200000, 460000, fill="EFE8FF", line="7D6B91", size=14, bold=True)
    s.box("说明：波形中可以看到 host/client 在 schedule 与 data slot 内按预期切换收发状态，证明 LWB/Gloria round 时序已经在实物上跑通。", 1250000, 5960000, 9700000, 500000, fill="FFFFFF", line="D2DBE4", size=15, bold=True)
    slides.append(s)

    s = Slide("当前验证结果", "软件移植和开发板级验证已经完成")
    s.table(["项目", "当前结果"], [
        ["验证平台", "NUCLEO-L476RG + DLP-RFS1280 + PB4/PB11 飞线"],
        ["网络规模", "双板验证：1 host + 1 source node"],
        ["Round 周期", "15 s"],
        ["已验证 modulation", "LoRa SF5 / GFSK 125k / FLRC 260k"],
        ["构建 size", "text=123400, data=4272, bss=30408"],
        ["资源占用", "Flash 约 127.7 KB / RAM 约 34.7 KB"],
    ], 820000, 1350000, 10400000, row_h=455000, col_weights=[1.3, 3.0])
    s.box("中期结论：LWB / Gloria 协议栈可以在 L476 + SX1280 2.4 GHz 平台上运行。", 1250000, 5200000, 9600000, 580000, fill="EAF6F4", line="5EA8A7", size=18, bold=True)
    slides.append(s)

    s = Slide("下一阶段 PCB 设计", "把开发板飞线方案固化成单块自制 comboard")
    if pcb_img.exists():
        s.image(pcb_img, 7200000, 1220000, 3700000, 2100000)
    s.bullet_list([
        "MCU：STM32L476RGTx LQFP64 最小系统",
        "Radio：第一版建议 DLP-RFS1280 模组，降低 RF 风险",
        "SPI1：PA5 SCK / PA6 MISO / PA7 MOSI / PA8 NSS",
        "DIO1：必须同时连接 PB4 和 PB11",
        "BUSY：PB3，调试座不要接 SWO",
        "电源、HSE/LSE、SWD、UART、test point 和 RF 布局",
    ], 760000, 1300000, 6200000, 2200000, size=17, gap=370000)
    s.table(["阶段", "时间"], [
        ["原理图", "1 周"],
        ["布局 / 走线 / DRC", "2 周"],
        ["投板打样", "1 周"],
        ["焊接 / 烧录 / bring-up", "2 周"],
        ["实验结果和论文", "2 周"],
    ], 7600000, 3650000, 3100000, row_h=360000, col_weights=[1.7, 1.0])
    slides.append(s)

    s = Slide("总结与 Q&A", "本阶段已将主要风险从软件移植推进到 PCB bring-up")
    s.bullet_list([
        "LWB 核心协议结构保持不变",
        "完成 L433→L476 与 SX1262→SX1280 两条迁移线",
        "三种调制模式双板端到端验证通过",
        "下一阶段目标明确：PCB 原理图、布局布线、投板和板级 bring-up",
    ], 1150000, 1500000, 9400000, 1900000, size=21, gap=470000)
    s.box("Q&A", 4600000, 4700000, 2800000, 720000, fill="1F4E79", line="1F4E79", size=30, font_color="FFFFFF", bold=True)
    slides.append(s)

    return slides


def write_pptx(slides):
    unique_images = []
    media_names = {}
    for slide in slides:
        for _, path in slide.image_rels:
            path = path.resolve()
            if path not in media_names:
                media_names[path] = f"image{len(unique_images) + 1}{path.suffix.lower()}"
                unique_images.append(path)
    with zipfile.ZipFile(OUT, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr("[Content_Types].xml", content_types(len(slides)))
        z.writestr("_rels/.rels", rels_root())
        z.writestr("docProps/app.xml", app_props(len(slides)))
        z.writestr("docProps/core.xml", core_props())
        z.writestr("ppt/presentation.xml", presentation(len(slides)))
        z.writestr("ppt/_rels/presentation.xml.rels", presentation_rels(len(slides)))
        z.writestr("ppt/slideMasters/slideMaster1.xml", slide_master())
        z.writestr("ppt/slideMasters/_rels/slideMaster1.xml.rels", slide_master_rels())
        z.writestr("ppt/slideLayouts/slideLayout1.xml", slide_layout())
        z.writestr("ppt/slideLayouts/_rels/slideLayout1.xml.rels", slide_layout_rels())
        z.writestr("ppt/theme/theme1.xml", theme())
        for path in unique_images:
            z.write(path, f"ppt/media/{media_names[path]}")
        for i, slide in enumerate(slides, 1):
            z.writestr(f"ppt/slides/slide{i}.xml", slide.xml())
            z.writestr(f"ppt/slides/_rels/slide{i}.xml.rels", slide_rels(slide, media_names))


def main():
    slides = build_slides()
    write_pptx(slides)
    print(f"wrote {OUT} with {len(slides)} slides")


if __name__ == "__main__":
    main()
