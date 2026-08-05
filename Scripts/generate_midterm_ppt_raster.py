#!/usr/bin/env python3
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageOps
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from generate_midterm_ppt import Slide, write_pptx  # noqa: E402


W, H = 1920, 1080
OUT_DIR = Path("assets/ppt/rendered_slides")
PPT_OUT = Path("MID_TERM_DEFENSE_FIXED.pptx")
PDF_OUT = Path("MID_TERM_DEFENSE_FIXED.pdf")

BG = "#F6F8FB"
DARK = "#13202B"
BLUE = "#1F4E79"
TEAL = "#2F9C95"
AMBER = "#D99A2B"
TEXT = "#203040"
MUTED = "#607080"
LINE = "#C9D4DF"


def font(size, bold=False):
    base = "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"
    bold_path = "/usr/share/fonts/opentype/noto/NotoSansCJK-Bold.ttc"
    return ImageFont.truetype(bold_path if bold and Path(bold_path).exists() else base, size)


F = {
    "title": font(52, True),
    "h1": font(42, True),
    "h2": font(30, True),
    "body": font(25),
    "body_b": font(25, True),
    "small": font(20),
    "small_b": font(20, True),
    "tiny": font(17),
}


def new(bg=BG):
    return Image.new("RGB", (W, H), bg)


def draw_title(d, title, subtitle=None, dark=False):
    c = "white" if dark else TEXT
    sc = "#D9E6F2" if dark else MUTED
    d.text((72, 54), title, font=F["h1"], fill=c)
    if subtitle:
        d.text((76, 122), subtitle, font=F["small"], fill=sc)


def wrap_text(text, max_chars):
    lines = []
    for para in str(text).split("\n"):
        cur = ""
        for ch in para:
            cur += ch
            if len(cur) >= max_chars:
                lines.append(cur)
                cur = ""
        if cur:
            lines.append(cur)
    return lines or [""]


def rr(d, box, fill="white", outline=LINE, width=2, r=22):
    d.rounded_rectangle(box, radius=r, fill=fill, outline=outline, width=width)


def card(d, box, title, body="", fill="white", accent=BLUE):
    rr(d, box, fill=fill, outline=LINE, width=2, r=22)
    x1, y1, x2, y2 = box
    d.rounded_rectangle((x1, y1, x1 + 12, y2), radius=12, fill=accent)
    d.text((x1 + 34, y1 + 26), title, font=F["small_b"], fill=TEXT)
    y = y1 + 72
    for line in wrap_text(body, max(12, int((x2 - x1) / 28))):
        d.text((x1 + 34, y), line, font=F["tiny"], fill=MUTED)
        y += 28


def bullet_list(d, items, x, y, w, size="body", color=TEXT, gap=52):
    f = F[size]
    for item in items:
        d.text((x, y), "•", font=f, fill=BLUE)
        for i, line in enumerate(wrap_text(item, max(12, w // 26))):
            d.text((x + 34, y + i * 32), line, font=f, fill=color)
        y += gap + (len(wrap_text(item, max(12, w // 26))) - 1) * 30


def fit_image(path, box, bg="#EEF2F6"):
    x1, y1, x2, y2 = box
    frame = Image.new("RGB", (x2 - x1, y2 - y1), bg)
    if not path or not Path(path).exists():
        return frame
    img = Image.open(path).convert("RGB")
    img.thumbnail((x2 - x1, y2 - y1), Image.LANCZOS)
    px = ((x2 - x1) - img.width) // 2
    py = ((y2 - y1) - img.height) // 2
    frame.paste(img, (px, py))
    return frame


def photo_box(slide, d, path, box, label):
    x1, y1, x2, y2 = box
    rr(d, box, fill="white", outline=LINE, width=2, r=26)
    inner = (x1 + 12, y1 + 12, x2 - 12, y2 - 72)
    if path and Path(path).exists():
        slide.paste(fit_image(path, inner, "#FFFFFF"), (inner[0], inner[1]))
    else:
        rr(d, inner, fill="#E9EEF4", outline="#D7E0EA", width=2, r=18)
        d.text((inner[0] + 38, inner[1] + 90), "图片文件未放入 assets/ppt", font=F["small_b"], fill=TEXT)
        d.text((inner[0] + 38, inner[1] + 135), label, font=F["tiny"], fill=MUTED)
    d.multiline_text((x1 + 28, y2 - 66), label, font=F["tiny"], fill=TEXT, spacing=3)


def arrow(d, start, end, color=AMBER, width=5):
    d.line((start, end), fill=color, width=width)
    ex, ey = end
    sx, sy = start
    if ex >= sx:
        pts = [(ex, ey), (ex - 18, ey - 10), (ex - 18, ey + 10)]
    else:
        pts = [(ex, ey), (ex + 18, ey - 10), (ex + 18, ey + 10)]
    d.polygon(pts, fill=color)


def save(slide, name):
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    path = OUT_DIR / f"{name}.png"
    slide.save(path)
    return path


def diagram_slide(path, name):
    s = new("#1f1f1f")
    if path and Path(path).exists():
        img = fit_image(path, (35, 35, W - 35, H - 35), "#1f1f1f")
        s.paste(img, (35, 35))
    else:
        d = ImageDraw.Draw(s)
        d.text((100, 120), f"Missing diagram: {path}", font=F["h2"], fill="white")
    return save(s, name)


def table(d, x, y, col_w, row_h, headers, rows):
    cx = x
    for i, h in enumerate(headers):
        rr(d, (cx, y, cx + col_w[i], y + row_h), fill=BLUE, outline=BLUE, r=10)
        d.text((cx + 18, y + 16), h, font=F["small_b"], fill="white")
        cx += col_w[i]
    for r, row in enumerate(rows):
        cx = x
        fill = "white" if r % 2 == 0 else "#ECF2F7"
        for i, cell in enumerate(row):
            rr(d, (cx, y + row_h * (r + 1), cx + col_w[i], y + row_h * (r + 2)), fill=fill, outline=LINE, r=8)
            d.text((cx + 16, y + row_h * (r + 1) + 15), cell, font=F["tiny"], fill=TEXT)
            cx += col_w[i]


def node(d, box, text, fill="white", outline=LINE, text_color=TEXT):
    rr(d, box, fill=fill, outline=outline, width=3, r=18)
    x1, y1, x2, y2 = box
    lines = wrap_text(text, max(8, (x2 - x1) // 24))
    total = len(lines) * 28
    y = y1 + ((y2 - y1) - total) // 2
    for line in lines:
        tw = d.textlength(line, font=F["small_b"])
        d.text((x1 + ((x2 - x1) - tw) / 2, y), line, font=F["small_b"], fill=text_color)
        y += 30


def slide_cover(real_dev):
    s = new(DARK)
    d = ImageDraw.Draw(s)
    if real_dev and real_dev.exists():
        img = fit_image(real_dev, (780, 0, W, H), DARK)
        s.paste(img, (780, 0))
        d.rectangle((760, 0, W, H), fill=(19, 32, 43, 120))
    d.rectangle((0, 0, 840, H), fill=DARK)
    d.text((72, 100), "中期答辩", font=F["h1"], fill="#9BD2F0")
    d.text((72, 220), "STM32L476RG + SX1280\nLWB 低功耗无线总线项目", font=F["title"], fill="white", spacing=12)
    d.text((76, 430), "重新设计 comboard，并让 LWB 在自制硬件上稳定运行", font=F["body"], fill="#D9E6F2")
    card(d, (82, 700, 390, 835), "当前状态", "L476 + SX1280 开发板验证", fill="#173A55", accent=TEAL)
    card(d, (430, 700, 738, 835), "下一阶段", "PCB 设计与 bring-up", fill="#173A55", accent=AMBER)
    return save(s, "slide01_cover")


def build_slide_images():
    l433_comboard = next((p for p in [Path("assets/ppt/l433-sx1262-comboard.jpg"), Path("assets/ppt/l433-sx1262-comboard.png"), Path("assets/ppt/l433-sx1262-comboard.jpeg")] if p.exists()), None)
    l433_dev = next((p for p in [Path("assets/ppt/l433-sx1262-devboard.jpg"), Path("assets/ppt/l433-sx1262-devboard.png"), Path("assets/ppt/l433-sx1262-devboard.jpeg")] if p.exists()), None)
    l476_sx1262_dev = next((p for p in [Path("assets/ppt/l476-sx1262-devboard.jpg"), Path("assets/ppt/l476-sx1262-devboard.png"), Path("assets/ppt/l476-sx1262-devboard.jpeg")] if p.exists()), None)
    l476_sx1280_dev = next((p for p in [Path("assets/ppt/l476-sx1280-devboard.jpg"), Path("assets/ppt/l476-sx1280-devboard.png"), Path("assets/ppt/l476-sx1280-devboard.jpeg"), Path("assets/ppt/real-devboard.jpg"), Path("assets/ppt/real-devboard.png"), Path("assets/ppt/real-devboard.jpeg")] if p.exists()), None)
    l476_sx1280_comboard = next((p for p in [Path("assets/ppt/l476-sx1280-comboard.jpg"), Path("assets/ppt/l476-sx1280-comboard.png"), Path("assets/ppt/l476-sx1280-comboard.jpeg")] if p.exists()), None)
    if l476_sx1280_comboard is None:
        l476_sx1280_comboard = l433_comboard or (Path("assets/ppt/custom-pcb.png") if Path("assets/ppt/custom-pcb.png").exists() else None)
    logic = Path("assets/ppt/logic_analyzer_round_view.png")
    md_project = Path("assets/ppt/md_diagrams/project_hierarchy.png")
    md_lwb = Path("assets/ppt/md_diagrams/lwb_principle.png")
    md_runtime = Path("assets/ppt/md_diagrams/runtime_logic.png")
    md_code = Path("assets/ppt/md_diagrams/code_structure.png")
    slide_paths = [slide_cover(l476_sx1280_dev)]

    s = new(); d = ImageDraw.Draw(s); draw_title(d, "项目目标与交付物", "最终交付是一块自制 L476 + SX1280 comboard")
    card(d, (90, 190, 560, 385), "硬件交付", "STM32L476RGTx MCU、SX1280/DLP-RFS1280、SWD、UART、电源、HSE/LSE、SPI、DIO1、BUSY、ANTSEL", accent=BLUE)
    card(d, (90, 425, 560, 620), "软件交付", "在该 PCB 上跑通 LWB 协议栈，验证 LoRa / GFSK / FLRC 三种 modulation", accent=TEAL)
    card(d, (90, 660, 560, 855), "项目价值", "验证 2.4 GHz SX1280 平台运行 LWB 的可行性，形成 IoT mesh 原型平台", accent=AMBER)
    photo_box(s, d, l433_comboard, (700, 165, 1225, 920), "起点：L433 + SX1262 comboard")
    photo_box(s, d, l476_sx1280_dev, (1290, 165, 1815, 920), "当前：L476 + SX1280 开发板")
    slide_paths.append(save(s, "slide02_goal"))

    slide_paths.append(diagram_slide(md_project, "slide03_project_hierarchy_md"))

    s = new(); d = ImageDraw.Draw(s); draw_title(d, "硬件迭代路径", "每次只替换一个关键变量，降低定位难度")
    steps = [
        ("1", "L433 + SX1262\ncomboard", "原始平台"),
        ("2", "L433 + SX1262\n开发板", "开发板适配"),
        ("3", "L476 + SX1262\n开发板", "MCU 迁移"),
        ("4", "L476 + SX1280\n开发板", "Radio 迁移"),
        ("5", "L476 + SX1280\ncomboard", "最终目标"),
    ]
    x = 65
    for i, (n, txt, st) in enumerate(steps):
        node(d, (x, 290, x+300, 480), f"{n}\n{txt}", fill="#FFFFFF", outline=TEAL if i in (2, 3) else BLUE)
        d.text((x+54, 510), st, font=F["tiny"], fill=TEAL if i < 4 else AMBER)
        if i < 4:
            arrow(d, (x+312, 385), (x+370, 385))
        x += 370
    d.text((120, 710), "答辩主线：先用开发板逐步消除 MCU 和 radio 移植风险，最后把已验证连接固化到自制 comboard。", font=F["body_b"], fill=TEXT)
    slide_paths.append(save(s, "slide04_path"))

    s = new(); d = ImageDraw.Draw(s); draw_title(d, "五阶段硬件实物路线", "严格对应 MID_TERM_DEFENSE.md 中的迭代顺序")
    photos = [
        (l433_comboard, "1  L433 + SX1262\ncomboard"),
        (l433_dev, "2  L433 + SX1262\n开发板"),
        (l476_sx1262_dev, "3  L476 + SX1262\n开发板"),
        (l476_sx1280_dev, "4  L476 + SX1280\n开发板"),
        (l476_sx1280_comboard, "5  L476 + SX1280\ncomboard"),
    ]
    card_w = 330
    gap = 45
    x = 55
    for i, (img, label) in enumerate(photos):
        photo_box(s, d, img, (x, 185, x + card_w, 810), label)
        if i < len(photos) - 1:
            arrow(d, (x + card_w + 6, 495), (x + card_w + gap - 10, 495))
        x += card_w + gap
    d.text((95, 875), "说明：第 2-4 步是开发板验证阶段；第 5 步回到第 1 步类似的紧凑 comboard 成品形态。", font=F["body_b"], fill=TEXT)
    slide_paths.append(save(s, "slide05_hardware_photos"))

    s = new(); d = ImageDraw.Draw(s); draw_title(d, "LWB 是什么", "Low-power Wireless Bus：把多跳无线网络抽象成一条共享总线")
    card(d, (110, 230, 760, 560), "传统 mesh", "每个节点维护邻居表、路由表和转发状态；拓扑变化处理成本高。", fill="#FFF7F0", accent="#C56B38")
    arrow(d, (820, 395), (1030, 395), color=AMBER, width=8)
    card(d, (1090, 230, 1750, 560), "LWB", "host 统一调度；slot 内用 Glossy/Gloria 同步泛洪覆盖全网；节点不维护路由。", fill="#EFFAF8", accent=TEAL)
    node(d, (430, 720, 1490, 835), "核心特点：slot 是集中调度的，但每个 slot 内部的 packet 传播是全网同步泛洪。", fill="white", outline=LINE)
    slide_paths.append(save(s, "slide06_lwb_intro"))

    slide_paths.append(diagram_slide(md_lwb, "slide07_lwb_principle_md"))

    s = new(); d = ImageDraw.Draw(s); draw_title(d, "一个 LWB Round", "当前配置：LWB_SCHED_PERIOD = 15 s")
    labels = [("S1", "schedule"), ("C", "contention"), ("D1", "data 1"), ("D2", "data 2"), ("Dn", "data n"), ("S2", "schedule 2")]
    x = 110
    widths = [240, 220, 240, 240, 240, 260]
    for i, (a, b) in enumerate(labels):
        node(d, (x, 330, x+widths[i], 480), f"{a}\n{b}", fill="#FFFFFF" if i % 2 else "#E8F1F8", outline=BLUE)
        x += widths[i] + 35
    d.line((110, 550, 1810, 550), fill=LINE, width=4)
    d.text((100, 575), "t = 0", font=F["small"], fill=MUTED); d.text((1690, 575), "t = 15 s", font=F["small"], fill=MUTED)
    bullet_list(d, ["host 广播 schedule，source 同步网络时间", "每个 data slot 由指定节点发起，其他节点接收/转发", "round 结束后进入等待或低功耗，下一轮由 LPTIM/DIO1 唤醒"], 150, 700, 1550, size="body")
    slide_paths.append(save(s, "slide08_round"))

    slide_paths.append(diagram_slide(md_runtime, "slide09_runtime_logic_md"))
    slide_paths.append(diagram_slide(md_code, "slide10_code_structure_md"))

    s = new(); d = ImageDraw.Draw(s); draw_title(d, "L476 vs L433 数据对比", "选择 L476 的核心原因是资源余量")
    table(d, 130, 210, [260, 330, 330], 68, ["项目", "STM32L433CC", "STM32L476RG"], [["内核","Cortex-M4F","Cortex-M4F"],["最大主频","80 MHz","80 MHz"],["Flash","256 KB","1024 KB"],["RAM","64 KB","128 KB"],["GPIO","约 37","约 51"]])
    for i, (lab, old, newv) in enumerate([("Flash KB", 256, 1024), ("RAM KB", 64, 128), ("GPIO", 37, 51)]):
        y = 260 + i*150
        d.text((1080, y), lab, font=F["small_b"], fill=TEXT)
        d.rectangle((1220, y+5, 1750, y+40), fill="#E3EAF1")
        d.rectangle((1220, y+5, 1220 + int(530*old/max(newv, old)), y+40), fill="#9BB7D4")
        d.rectangle((1220, y+58, 1220 + int(530*newv/max(newv, old)), y+93), fill=TEAL)
        d.text((1760, y+44), f"{newv}", font=F["small_b"], fill=TEXT)
    d.text((1080, 730), "结论：主频不是重点，Flash/RAM/GPIO 更充裕才是迁移价值。", font=F["body_b"], fill=TEXT)
    slide_paths.append(save(s, "slide11_l476_compare"))

    s = new(); d = ImageDraw.Draw(s); draw_title(d, "SX1280 vs SX1262 数据对比", "目标从 Sub-GHz 迁移到 2.4 GHz 多调制平台")
    table(d, 105, 195, [260, 520, 650], 72, ["项目", "SX1262", "SX1280"], [["频段","Sub-GHz","2.4 GHz ISM"],["调制","LoRa / FSK","LoRa / GFSK / FLRC / BLE-compatible / Ranging"],["最大发射功率","+22 dBm","+12.5 dBm"],["优势","远距离、穿墙强","全球通用、天线小、多调制"],["代价","法规更复杂","距离和穿墙弱于 Sub-GHz"]])
    card(d, (1110, 675, 1700, 850), "项目定位", "室内 LWB mesh，小范围多节点通信，并预留室内定位扩展。", fill="#FFF4DD", accent=AMBER)
    slide_paths.append(save(s, "slide12_sx1280_compare"))

    s = new(); d = ImageDraw.Draw(s); draw_title(d, "代码迁移要点", "LWB 上层思想不变，radio driver / timing / board glue 重建")
    layers = [("MCU/时钟层","startup、linker、SYSCLK、TIM2、LPTIM、USART"),("板级 GPIO 层","SPI、NSS、BUSY、DIO1、ANTSEL"),("Radio driver 层","SX1280 opcode、寄存器、packet type、CRC、timeout"),("Radio 参数层","2.4 GHz 频率表、功率表、LoRa/GFSK/FLRC 表"),("协议时序层","Gloria trigger delay、slot overhead、LWB period")]
    y = 190
    for i, (a,b) in enumerate(layers):
        card(d, (140, y, 1780, y+115), a, b, fill="white", accent=[BLUE, TEAL, AMBER, "#7D6B91", "#6B8E23"][i])
        y += 145
    slide_paths.append(save(s, "slide13_migration"))

    s = new(); d = ImageDraw.Draw(s); draw_title(d, "关键参数与修复", "当前默认 FLRC 260 kbit/s，15 秒一个 round")
    table(d, 115, 185, [500, 610], 66, ["参数", "当前值"], [["GLORIA_INTERFACE_MODULATION","11 = FLRC 260 kbit/s"],["GLORIA_INTERFACE_RF_BAND","24 = 2450 MHz"],["LWB_SCHED_PERIOD","15 s"],["LWB_N_TX","2"],["LWB_NUM_HOPS","6"],["HS_TIMER_FREQUENCY","8 MHz"]])
    bullet_list(d, ["continuous RX timeout 修复，保证 NODE bootstrap", "GFSK whitening seed 修复，避免覆盖 CRC poly", "SX1280 packet type / FLRC CRC / max payload 修复", "LWB_MAX_PAYLOAD_LEN 提升到 32"], 1230, 240, 560, size="small", gap=70)
    slide_paths.append(save(s, "slide14_params"))

    s = new(); d = ImageDraw.Draw(s); draw_title(d, "逻辑分析仪验证", "HOST 与 client 的 TX/RX 脉冲按 LWB round 时序出现")
    if logic.exists():
        img = fit_image(logic, (70, 165, 1850, 780), "#101820")
        s.paste(img, (70, 165))
    tags = [("D4 client PA12 RX","#F4D03F"),("D5 client PA11 TX","#58D68D"),("D6 HOST PA12 RX","#5DADE2"),("D7 HOST PA11 TX","#AF7AC5")]
    for i,(t,c) in enumerate(tags):
        rr(d, (130+i*430, 820, 500+i*430, 875), fill=c, outline=c, r=14); d.text((155+i*430, 833), t, font=F["tiny"], fill="#101820")
    node(d, (160, 925, 1760, 1010), "波形证明主机与节点在调度槽和数据槽内按预期切换收发状态，整轮协议时序已在实物上跑通。", fill="white", outline=LINE)
    slide_paths.append(save(s, "slide15_logic_analyzer"))

    s = new(); d = ImageDraw.Draw(s); draw_title(d, "当前验证结果", "软件移植和开发板级验证已经完成")
    table(d, 150, 190, [430, 980], 72, ["项目", "当前结果"], [["验证平台","NUCLEO-L476RG + DLP-RFS1280 + PB4/PB11 飞线"],["网络规模","双板验证：1 host + 1 source node"],["Round 周期","15 s"],["已验证 modulation","LoRa SF5 / GFSK 125k / FLRC 260k"],["构建 size","text=123400, data=4272, bss=30408"],["资源占用","Flash 约 127.7 KB / RAM 约 34.7 KB"]])
    node(d, (250, 830, 1670, 930), "中期结论：LWB/Gloria 协议栈可以在 L476 + SX1280 2.4 GHz 平台上运行。", fill="#EAF6F4", outline=TEAL)
    slide_paths.append(save(s, "slide16_results"))

    s = new(); d = ImageDraw.Draw(s); draw_title(d, "下一阶段 PCB 设计", "把开发板杜邦线方案固化成单块自制 comboard")
    bullet_list(d, ["MCU：STM32L476RGTx LQFP64 最小系统", "Radio：第一版建议 DLP-RFS1280 模组，降低 RF 风险", "SPI1：PA5 SCK / PA6 MISO / PA7 MOSI / PA8 NSS", "DIO1：必须同时连接 PB4 和 PB11", "BUSY：连接 PB3，调试座不要接 SWO", "电源、HSE/LSE、SWD、UART、test point 和 RF 布局"], 110, 190, 880, size="small", gap=62)
    table(d, 1110, 220, [410, 220], 68, ["阶段", "时间"], [["原理图","1 周"],["布局/走线/DRC","2 周"],["投板打样","1 周"],["焊接/烧录/bring-up","2 周"],["实验结果和论文","2 周"]])
    slide_paths.append(save(s, "slide17_pcb"))

    s = new(DARK); d = ImageDraw.Draw(s); draw_title(d, "总结与 Q&A", "本阶段已将主要风险从软件移植推进到 PCB bring-up", dark=True)
    bullet_list(d, ["LWB 核心协议结构保持不变", "完成 L433→L476 与 SX1262→SX1280 两条迁移线", "三种调制模式双板端到端验证通过", "下一阶段目标明确：PCB 原理图、布局布线、投板和板级 bring-up"], 180, 260, 1400, size="h2", color="white", gap=90)
    node(d, (760, 800, 1160, 920), "Q&A", fill=BLUE, outline=BLUE, text_color="white")
    slide_paths.append(save(s, "slide18_qa"))

    return slide_paths


def main():
    slide_paths = build_slide_images()
    slides = []
    for p in slide_paths:
        s = Slide()
        s.image(p)
        slides.append(s)
    import generate_midterm_ppt as base
    base.OUT = PPT_OUT
    write_pptx(slides)
    print(f"wrote {PPT_OUT} with {len(slides)} raster slides")


if __name__ == "__main__":
    main()
