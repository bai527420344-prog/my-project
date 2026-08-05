#!/usr/bin/env python3
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path("assets/ppt/style_concepts")


def font(size, bold=False):
    regular = "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"
    bold_path = "/usr/share/fonts/opentype/noto/NotoSansCJK-Bold.ttc"
    return ImageFont.truetype(bold_path if bold and Path(bold_path).exists() else regular, size)


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


def rr(draw, box, fill, outline=None, width=2, radius=22):
    draw.rounded_rectangle(box, radius=radius, fill=fill, outline=outline, width=width)


def box(draw, xy, title, body=None, style="light", accent="#1F4E79"):
    x1, y1, x2, y2 = xy
    fill = (255, 255, 255, 225) if style == "light" else (15, 24, 34, 215)
    line = accent
    rr(draw, xy, fill, line, 3, 22)
    draw.rounded_rectangle((x1, y1, x1 + 10, y2), radius=10, fill=accent)
    text = "#1C2B39" if style == "light" else "#FFFFFF"
    muted = "#53687B" if style == "light" else "#D9E7F2"
    draw.text((x1 + 28, y1 + 20), title, font=font(26, True), fill=text)
    if body:
        y = y1 + 62
        for line_text in wrap(body, max(10, (x2 - x1 - 60) // 24)):
            draw.text((x1 + 28, y), line_text, font=font(20), fill=muted)
            y += 30


def table(draw, x, y, rows, style="light"):
    header_fill = "#1F4E79" if style == "light" else "#2F9C95"
    bg = (255, 255, 255, 230) if style == "light" else (16, 28, 40, 225)
    text = "#1C2B39" if style == "light" else "#FFFFFF"
    widths = [245, 410]
    h = 46
    rr(draw, (x, y, x + sum(widths), y + h * (len(rows) + 1)), bg, "#B7C4D0", 2, 16)
    draw.rectangle((x, y, x + sum(widths), y + h), fill=header_fill)
    draw.text((x + 18, y + 10), "项目", font=font(18, True), fill="white")
    draw.text((x + widths[0] + 18, y + 10), "当前结果", font=font(18, True), fill="white")
    for i, (a, b) in enumerate(rows, 1):
        yy = y + h * i
        draw.line((x, yy, x + sum(widths), yy), fill="#B7C4D0", width=1)
        draw.text((x + 18, yy + 9), a, font=font(16, True), fill=text)
        draw.text((x + widths[0] + 18, yy + 9), b, font=font(16), fill=text)


def overlay_style1():
    im = Image.open(ROOT / "style1_academic_raw.png").convert("RGBA")
    draw = ImageDraw.Draw(im, "RGBA")
    draw.rectangle((0, 0, im.width, 150), fill=(246, 248, 251, 230))
    draw.text((70, 38), "L476 + SX1280 LWB 中期答辩", font=font(42, True), fill="#18324A")
    draw.text((74, 98), "从 L433 + SX1262 到 L476 + SX1280：开发板验证完成，下一步自制 comboard", font=font(20), fill="#53687B")
    box(draw, (70, 185, 620, 370), "项目目标", "重新设计 L476 + SX1280 comboard，\n让 LWB 协议稳定运行。", "light", "#1F4E79")
    box(draw, (70, 405, 620, 585), "当前完成", "L476 + SX1280 开发板验证；\nLoRa / GFSK / FLRC 已通过。", "light", "#2F9C95")
    table(draw, 930, 175, [
        ("平台", "NUCLEO-L476RG + DLP-RFS1280"),
        ("网络", "1 host + 1 source node"),
        ("周期", "LWB round = 15 s"),
        ("频点", "2450 MHz"),
        ("资源", "text 123.4 KB / bss 30.4 KB"),
    ], "light")
    box(draw, (930, 590, 1560, 770), "下一阶段", "PCB 原理图、布局布线、投板、\n焊接、烧录和板级 bring-up。", "light", "#D99A2B")
    im.convert("RGB").save(ROOT / "style1_academic_content.png")


def overlay_style2():
    im = Image.open(ROOT / "style2_dark_raw.png").convert("RGBA")
    draw = ImageDraw.Draw(im, "RGBA")
    draw.rectangle((0, 0, im.width, 170), fill=(7, 14, 22, 220))
    draw.text((70, 35), "LWB/Gloria 实物时序验证", font=font(42, True), fill="#FFFFFF")
    draw.text((74, 98), "HOST 与 NODE 在 schedule / data slot 中按预期收发，逻辑分析仪波形验证 round 时序", font=font(20), fill="#B8D7EA")
    box(draw, (70, 215, 610, 390), "核心链路", "应用任务 → LWB 调度\n→ Gloria 同步泛洪\n→ SX1280 driver → 硬件", "dark", "#2F9C95")
    box(draw, (70, 430, 610, 640), "关键硬件约束", "DIO1 同时接入 PB4/EXTI4\n与 PB11/TIM2_CH4：\n既要唤醒，也要微秒级时间戳。", "dark", "#D99A2B")
    table(draw, 950, 220, [
        ("调制", "FLRC 260 kbit/s 默认"),
        ("备选", "LoRa SF5 / GFSK 125k"),
        ("n_tx", "2"),
        ("hops", "6"),
        ("状态", "双板端到端通过"),
    ], "dark")
    box(draw, (950, 610, 1580, 795), "答辩结论", "软件迁移风险已基本解除，\n下一阶段风险转移到 PCB 设计\n和 bring-up。", "dark", "#5DADE2")
    im.convert("RGB").save(ROOT / "style2_dark_content.png")


def overlay_style3():
    im = Image.open(ROOT / "style3_warm_raw.png").convert("RGBA")
    draw = ImageDraw.Draw(im, "RGBA")
    draw.rectangle((0, 0, im.width, 155), fill=(255, 249, 238, 225))
    draw.text((70, 35), "最终成品形态：L476 + SX1280 comboard", font=font(40, True), fill="#2B3340")
    draw.text((74, 96), "参考 L433 + SX1262 comboard 形态，将开发板阶段验证过的连接固化到 PCB", font=font(20), fill="#6B5F50")
    box(draw, (80, 220, 610, 405), "形态继承", "最终不是长期使用 NUCLEO + 杜邦线，\n而是紧凑独立 comboard。", "light", "#D99A2B")
    box(draw, (80, 445, 610, 665), "功能升级", "MCU：L433 → L476；\nRadio：SX1262\nSub-GHz → SX1280\n2.4 GHz。", "light", "#2F9C95")
    box(draw, (960, 215, 1560, 405), "PCB 必须保证", "SPI1、NRESET、BUSY、DIO1、\nANTSEL、电源、HSE/LSE、SWD、\nUART 与 test point。", "light", "#1F4E79")
    box(draw, (960, 445, 1560, 665), "关键连接", "DIO1 同时连接 PB4 和 PB11，\n把开发板飞线固化为\nPCB 上的同一个 net。", "light", "#D99A2B")
    draw.text((390, 825), "路线：原 comboard 参考 → 开发板迁移验证 → 自制 L476 + SX1280 comboard", font=font(24, True), fill="#2B3340")
    im.convert("RGB").save(ROOT / "style3_warm_content.png")


def main():
    overlay_style1()
    overlay_style2()
    overlay_style3()
    print("wrote content overlays to", ROOT)


if __name__ == "__main__":
    main()
