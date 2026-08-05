#!/usr/bin/env python3
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path("assets/ppt/five_stage_styles")


def font(size, bold=False):
    regular = "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"
    bold_path = "/usr/share/fonts/opentype/noto/NotoSansCJK-Bold.ttc"
    return ImageFont.truetype(bold_path if bold and Path(bold_path).exists() else regular, size)


def rr(d, box, fill, outline=None, width=2, radius=22):
    d.rounded_rectangle(box, radius=radius, fill=fill, outline=outline, width=width)


def wrap(text, n):
    out = []
    for para in text.split("\n"):
        cur = ""
        for ch in para:
            cur += ch
            if len(cur) >= n:
                out.append(cur)
                cur = ""
        if cur:
            out.append(cur)
    return out


def text_box(d, box, title, body, dark=False, accent="#1F4E79"):
    x1, y1, x2, y2 = box
    fill = (255, 255, 255, 255) if not dark else (12, 22, 32, 255)
    outline = accent
    rr(d, box, fill, outline, 3, 20)
    d.rounded_rectangle((x1, y1, x1 + 10, y2), radius=10, fill=accent)
    title_color = "#1D2D3C" if not dark else "#FFFFFF"
    body_color = "#53687B" if not dark else "#D8E8F2"
    d.text((x1 + 28, y1 + 20), title, font=font(25, True), fill=title_color)
    y = y1 + 62
    for line in wrap(body, max(12, (x2 - x1 - 55) // 22)):
        d.text((x1 + 28, y), line, font=font(18), fill=body_color)
        y += 28


def stage_bar(d, y, dark=False):
    labels = [
        "1  L433+SX1262\ncomboard",
        "2  L433+SX1262\n开发板",
        "3  L476+SX1262\n开发板",
        "4  L476+SX1280\n开发板",
        "5  L476+SX1280\ncomboard",
    ]
    w, gap, x = 270, 34, 72
    for i, lab in enumerate(labels):
        fill = (255, 255, 255, 232) if not dark else (12, 22, 32, 228)
        outline = ["#1F4E79", "#2F9C95", "#D99A2B", "#5DADE2", "#7D6B91"][i]
        rr(d, (x, y, x + w, y + 94), fill, outline, 3, 18)
        d.text((x + 18, y + 16), lab, font=font(18, True), fill="#1D2D3C" if not dark else "#FFFFFF", spacing=2)
        if i < len(labels) - 1:
            d.line((x + w + 5, y + 47, x + w + gap - 8, y + 47), fill="#D99A2B", width=5)
            d.polygon([(x + w + gap - 8, y + 47), (x + w + gap - 22, y + 38), (x + w + gap - 22, y + 56)], fill="#D99A2B")
        x += w + gap


def table(d, x, y, dark=False):
    rows = [
        ("当前阶段", "第 4 步：L476 + SX1280 开发板"),
        ("已验证", "LoRa SF5 / GFSK 125k / FLRC 260k"),
        ("默认配置", "FLRC 260 kbit/s, 2450 MHz"),
        ("LWB round", "15 s, n_tx=2, hops=6"),
        ("下一步", "第 5 步：自制 L476 + SX1280 comboard"),
    ]
    fill = (255, 255, 255, 255) if not dark else (12, 22, 32, 255)
    text = "#1D2D3C" if not dark else "#FFFFFF"
    line = "#AFC0CF" if not dark else "#5DADE2"
    rr(d, (x, y, x + 650, y + 292), fill, line, 2, 16)
    d.rectangle((x, y, x + 650, y + 46), fill="#1F4E79" if not dark else "#2F9C95")
    d.text((x + 18, y + 10), "验证数据", font=font(18, True), fill="#FFFFFF")
    for i, (a, b) in enumerate(rows):
        yy = y + 46 + i * 48
        d.line((x, yy, x + 650, yy), fill=line, width=1)
        d.text((x + 18, yy + 10), a, font=font(16, True), fill=text)
        d.text((x + 190, yy + 10), b, font=font(16), fill=text)


def overlay(src, dst, title, subtitle, dark=False, warm=False):
    im = Image.open(ROOT / src).convert("RGBA")
    d = ImageDraw.Draw(im, "RGBA")
    top_fill = (255, 255, 255, 225) if not dark else (5, 12, 20, 226)
    if warm:
        top_fill = (255, 249, 238, 226)
    d.rectangle((0, 0, im.width, 150), fill=top_fill)
    d.text((70, 34), title, font=font(40, True), fill="#FFFFFF" if dark else "#1D2D3C")
    d.text((74, 94), subtitle, font=font(19), fill="#CFE4F2" if dark else "#53687B")
    stage_bar(d, 760, dark=dark)
    if dark:
        text_box(d, (70, 190, 620, 370), "项目主线", "先从原 comboard 出发，用开发板逐步验证 MCU 与 radio 迁移，最后回到紧凑成品板。", True, "#2F9C95")
        text_box(d, (70, 405, 620, 590), "当前证据", "L476 + SX1280 开发板已完成双板端到端验证，并用逻辑分析仪确认 round 时序。", True, "#D99A2B")
        table(d, 960, 200, dark=True)
    else:
        text_box(d, (70, 185, 620, 365), "项目主线", "五阶段硬件路线：原板 → 开发板迁移验证 → 最终自制板。", False, "#1F4E79")
        text_box(d, (70, 400, 620, 585), "当前进度", "已完成到第 4 步：开发板原型验证。", False, "#2F9C95")
        table(d, 950, 190, dark=False)
    im.convert("RGB").save(ROOT / dst)


def main():
    overlay(
        "style1_light_raw.png",
        "style1_light_content.png",
        "五阶段硬件路线：L476 + SX1280 LWB",
        "严格对应 MID_TERM_DEFENSE.md：从原 comboard 到最终自制 comboard",
        dark=False,
    )
    overlay(
        "style2_dark_raw.png",
        "style2_dark_content.png",
        "LWB/Gloria 硬件迁移与时序验证",
        "当前已到第 4 步：L476 + SX1280 开发板；下一步固化为 comboard",
        dark=True,
    )
    overlay(
        "style3_warm_raw.png",
        "style3_warm_content.png",
        "最终交付形态：L476 + SX1280 comboard",
        "开发板阶段解决软件和时序风险，PCB 阶段回到紧凑 comboard 形态",
        dark=False,
        warm=True,
    )
    print("wrote five-stage content images")


if __name__ == "__main__":
    main()
