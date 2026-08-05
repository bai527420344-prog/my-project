#!/usr/bin/env python3
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont


OUT = Path("assets/ppt/md_diagrams_en")
BG = "#1f1f1f"
PANEL = "#252525"
TEXT = "#d7d7d7"
LINE = "#8d8d8d"
DASH = "#cfcfcf"


def font(size, bold=False):
    regular = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
    bold_path = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
    return ImageFont.truetype(bold_path if bold else regular, size)


F = {
    "title": font(38, True),
    "node": font(20),
    "small": font(17),
    "tiny": font(14),
}


def wrap(text, max_chars):
    lines = []
    for para in str(text).split("\n"):
        words = para.split(" ")
        cur = ""
        for word in words:
            nxt = word if not cur else cur + " " + word
            if len(nxt) > max_chars and cur:
                lines.append(cur)
                cur = word
            else:
                cur = nxt
        if cur:
            lines.append(cur)
    return lines or [""]


def center_text(d, box, text, f=None):
    f = f or F["node"]
    x1, y1, x2, y2 = box
    lines = wrap(text, max(10, int((x2 - x1) / 9)))
    h = len(lines) * (f.size + 5) - 5
    y = y1 + ((y2 - y1) - h) / 2
    for line in lines:
        tw = d.textlength(line, font=f)
        d.text((x1 + ((x2 - x1) - tw) / 2, y), line, font=f, fill=TEXT)
        y += f.size + 5


def node(d, box, text, f=None):
    d.rectangle(box, fill=PANEL, outline="#a0a0a0", width=2)
    center_text(d, box, text, f=f)


def arrow(d, a, b, fill=LINE, width=2, dashed=False):
    if dashed:
        dashed_line(d, a, b, fill, width)
    else:
        d.line((a, b), fill=fill, width=width)
    x1, y1 = a
    x2, y2 = b
    if abs(x2 - x1) >= abs(y2 - y1):
        direction = 1 if x2 >= x1 else -1
        pts = [(x2, y2), (x2 - direction * 14, y2 - 8), (x2 - direction * 14, y2 + 8)]
    else:
        direction = 1 if y2 >= y1 else -1
        pts = [(x2, y2), (x2 - 8, y2 - direction * 14), (x2 + 8, y2 - direction * 14)]
    d.polygon(pts, fill=fill)


def dashed_line(d, a, b, fill=DASH, width=2, dash=12, gap=8):
    x1, y1 = a
    x2, y2 = b
    dx, dy = x2 - x1, y2 - y1
    dist = (dx * dx + dy * dy) ** 0.5
    if dist == 0:
        return
    t = 0
    while t < dist:
        t2 = min(t + dash, dist)
        p1 = (x1 + dx * t / dist, y1 + dy * t / dist)
        p2 = (x1 + dx * t2 / dist, y1 + dy * t2 / dist)
        d.line((p1, p2), fill=fill, width=width)
        t += dash + gap


def title(d, text):
    d.text((30, 28), text, font=F["title"], fill=TEXT)


def save(img, name):
    OUT.mkdir(parents=True, exist_ok=True)
    path = OUT / name
    img.save(path)
    print(path)


def project_hierarchy():
    img = Image.new("RGB", (1920, 900), BG)
    d = ImageDraw.Draw(img)
    title_font = font(50, True)
    root_font = font(22)
    group_font = font(24)
    child_font = font(18)
    note_font = font(21)
    d.text((30, 24), "Project Hierarchy", font=title_font, fill=TEXT)

    def poly_arrow(points, fill=LINE, width=2):
        for a, b in zip(points[:-2], points[1:-1]):
            d.line((a, b), fill=fill, width=width)
        arrow(d, points[-2], points[-1], fill=fill, width=width)

    def panel(box):
        x1, y1, x2, y2 = box
        d.rectangle(box, outline="#52606b", width=2)

    root = (660, 100, 1260, 205)
    node(d, root, "Final target\ncustom STM32L476RG + SX1280 comboard", root_font)

    columns = [
        {
            "panel": (70, 260, 455, 820),
            "group": (120, 310, 405, 395),
            "label": "Hardware\nplatform",
            "children": [
                "STM32L476RG\nNUCLEO-L476RG",
                "SX1280 / DLP-RFS1280\n2.4 GHz radio",
                "DIO1 dual net\nPB4 EXTI + PB11 TIM2",
            ],
        },
        {
            "panel": (535, 260, 920, 820),
            "group": (585, 310, 870, 395),
            "label": "Firmware\nproject",
            "children": [
                "Src\nmain / FreeRTOS / HAL",
                "Lib\nprotocol / radio / time",
                "Inc\napp_config / main.h",
            ],
        },
        {
            "panel": (1000, 260, 1385, 820),
            "group": (1050, 310, 1335, 395),
            "label": "LWB / Gloria\nprotocol stack",
            "children": [
                "LWB\nround / schedule / slot",
                "Gloria / Glossy\nsynchronous flood",
                "SX1280 driver\nSPI / IRQ / timing",
            ],
        },
        {
            "panel": (1465, 260, 1850, 820),
            "group": (1515, 310, 1800, 395),
            "label": "Next-stage\nPCB",
            "children": [
                "Schematic\nverified nets",
                "Layout\npower / RF / debug",
                "Bring-up\nflash / test / validate",
            ],
        },
    ]

    bus_y = 235
    root_cx = (root[0] + root[2]) / 2
    d.line((root_cx, root[3], root_cx, bus_y), fill=LINE, width=2)
    d.line((262, bus_y, 1658, bus_y), fill=LINE, width=2)

    for col in columns:
        panel(col["panel"])
        group = col["group"]
        gx = (group[0] + group[2]) / 2
        poly_arrow([(gx, bus_y), (gx, group[1])], width=2)
        node(d, group, col["label"], group_font)

        child_top = 460
        child_h = 88
        gap = 48
        child_x1 = col["panel"][0] + 42
        child_x2 = col["panel"][2] - 42
        child_bus_y = group[3] + 34
        d.line((gx, group[3], gx, child_bus_y), fill=LINE, width=2)

        for i, text in enumerate(col["children"]):
            y1 = child_top + i * (child_h + gap)
            box = (child_x1, y1, child_x2, y1 + child_h)
            cx = (box[0] + box[2]) / 2
            d.line((gx, child_bus_y, cx, child_bus_y), fill=LINE, width=1)
            arrow(d, (cx, child_bus_y), (cx, box[1]), width=1)
            node(d, box, text, child_font)

    d.text((70, 850), "Separated view: hardware, firmware, protocol stack, and next-stage PCB are independent branches, so the hierarchy is readable without cross-links.", font=note_font, fill="#aeb8c2")
    save(img, "project_hierarchy.png")


def lwb_principle():
    img = Image.new("RGB", (1920, 900), BG)
    d = ImageDraw.Draw(img)
    title_font = font(54, True)
    box_font = font(27)
    label_font = font(26)
    small_label_font = font(24)

    d.text((30, 24), "LWB Principle Diagram", font=title_font, fill=TEXT)
    participants = [
        (250, "Host\nscheduler / time base"),
        (700, "Virtual Shared Bus"),
        (1040, "Source node 1"),
        (1350, "Source node 2"),
        (1660, "Source node N"),
    ]
    for x, label in participants:
        half_w = 158 if x <= 700 else 145
        node(d, (x - half_w, 85, x + half_w, 195), label, box_font)
        node(d, (x - half_w, 750, x + half_w, 860), label, box_font)
        d.line((x, 195, x, 750), fill="#a8a8a8", width=3)
    arrow(d, (250, 245), (700, 245), width=3)
    d.text((305, 208), "SCHED flood: slot assignment", font=label_font, fill=TEXT)
    arrow(d, (700, 325), (1040, 325), width=3, dashed=True)
    d.text((775, 288), "slot 1 assigned to node 1", font=label_font, fill=TEXT)
    arrow(d, (700, 400), (1350, 400), width=3, dashed=True)
    d.text((910, 363), "slot 2 assigned to node 2", font=label_font, fill=TEXT)
    arrow(d, (700, 475), (1660, 475), width=3, dashed=True)
    d.text((1080, 438), "slot n assigned to node N", font=label_font, fill=TEXT)
    for y, sx, tx in [(540, 1040, 760), (625, 1350, 850), (710, 1660, 1000)]:
        arrow(d, (sx, y), (700, y), width=3)
        d.text((tx, y - 34), "DATA flood: Gloria synchronous flood", font=small_label_font, fill=TEXT)
        arrow(d, (700, y + 44), (250, y + 44), width=3, dashed=True)
        d.text((335, y + 14), "host receives / network can relay", font=small_label_font, fill=TEXT)
    save(img, "lwb_principle.png")


def runtime_logic():
    img = Image.new("RGB", (1920, 900), BG)
    d = ImageDraw.Draw(img)
    title_font = font(62, True)
    panel_font = font(25)
    node_font = font(20)
    start_font = font(25)
    note_font = font(19)
    d.text((30, 24), "Runtime Logic", font=title_font, fill=TEXT)

    def panel(box, label):
        x1, y1, x2, y2 = box
        d.rectangle(box, outline="#505a63", width=2)
        d.text((x1 + 18, y1 + 12), label, font=panel_font, fill="#9bd2f0")

    def poly_arrow(points, fill=LINE, width=2, dashed=False):
        for a, b in zip(points[:-2], points[1:-1]):
            if dashed:
                dashed_line(d, a, b, fill=fill, width=width)
            else:
                d.line((a, b), fill=fill, width=width)
        arrow(d, points[-2], points[-1], fill=fill, width=width, dashed=dashed)

    panel((72, 95, 1848, 270), "Boot and initialization")
    panel((72, 345, 1848, 810), "LWB round loop")

    top = [
        ("MCU reset", 95, 148),
        ("main\nHAL / clock / GPIO / SPI / UART", 410, 148),
        ("FreeRTOS start\ncreate tasks", 735, 148),
        ("vTask_com\nconfigure radio / Gloria / LWB", 1060, 148),
        ("lwb_init\nslot duration / queues", 1385, 148),
    ]
    boxes = []
    for txt, x, y in top:
        box = (x, y, x + 305, y + 98)
        boxes.append(box)
        node(d, box, txt, node_font)
    for a, b in zip(boxes, boxes[1:]):
        arrow(d, (a[2], 194), (b[0], 194))

    start = (790, 288, 1130, 378)
    node(d, start, "lwb_start\nenter round loop", start_font)
    poly_arrow([
        ((boxes[-1][0] + boxes[-1][2]) / 2, boxes[-1][3]),
        (1525, 255),
        (960, 255),
        (960, start[1]),
    ])

    cycle = [
        ("Round start", 105, 455),
        ("PREPROCESS\npreTask generates data", 420, 455),
        ("SCHED1\nhost sends schedule", 735, 455),
        ("DATA slots\nsend / receive", 1050, 455),
        ("CONTENTION\njoin / IPI update", 1365, 455),
        ("SCHED2\nnext-round info", 1365, 650),
        ("POSTPROCESS\nRX queue / stats", 1050, 650),
        ("Wait next round\nLPTIM / DIO1 wakeup", 735, 650),
    ]
    cb = []
    for txt, x, y in cycle:
        box = (x, y, x + 295, y + 112)
        cb.append(box)
        node(d, box, txt, node_font)

    poly_arrow([
        ((start[0] + start[2]) / 2, start[3]),
        (960, 415),
        ((cb[0][0] + cb[0][2]) / 2, 415),
        ((cb[0][0] + cb[0][2]) / 2, cb[0][1]),
    ])
    for a, b in zip(cb[:5], cb[1:5]):
        arrow(d, (a[2], 502), (b[0], 502))
    arrow(d, ((cb[4][0] + cb[4][2]) / 2, cb[4][3]), ((cb[5][0] + cb[5][2]) / 2, cb[5][1]))
    arrow(d, (cb[5][0], 697), (cb[6][2], 697))
    arrow(d, (cb[6][0], 697), (cb[7][2], 697))

    poly_arrow([
        (cb[7][0], 697),
        (250, 697),
        (250, 585),
        ((cb[0][0] + cb[0][2]) / 2, 585),
        ((cb[0][0] + cb[0][2]) / 2, cb[0][3]),
    ], dashed=True)

    d.text((285, 710), "sleep / wake for the next scheduled round", font=note_font, fill="#aeb8c2")
    save(img, "runtime_logic.png")


def code_structure():
    img = Image.new("RGB", (1920, 820), BG)
    d = ImageDraw.Draw(img)
    title_font = font(58, True)
    node_font = font(22)
    note_font = font(26)
    d.text((30, 24), "Code Structure", font=title_font, fill=TEXT)
    top = [
        ("Application tasks\nSrc/task_pre.c\nSrc/task_post.c", 55, 130, 325, 260),
        ("Communication task\nSrc/task_com.c", 415, 130, 695, 260),
        ("LWB layer\nLib/protocol/lwb", 785, 130, 1085, 260),
        ("Gloria flood layer\nLib/protocol/gloria", 1170, 130, 1470, 260),
        ("Radio abstraction\nLib/radio/radio.c", 1545, 130, 1840, 260),
    ]
    bottom = [
        ("Hardware\nSPI / DIO1 / BUSY / RF", 55, 360, 325, 490),
        ("HAL / MCU\nDrivers / STM32L476", 415, 360, 695, 490),
        ("System glue\nlpm / gpio_exti / uart", 785, 360, 1085, 490),
        ("Time base\nhs_timer / lptimer / rtc", 1170, 360, 1470, 490),
        ("SX1280 driver\nLib/radio/semtech", 1545, 360, 1840, 490),
    ]
    boxes = []
    for txt, x1, y1, x2, y2 in top + bottom:
        box = (x1, y1, x2, y2)
        boxes.append(box)
        node(d, box, txt, node_font)
    for a, b in zip(boxes[:5], boxes[1:5]):
        arrow(d, (a[2], 195), (b[0], 195))
    arrow(d, (1692, 260), (1692, 360))
    for a, b in zip(reversed(boxes[5:]), reversed(boxes[5:-1])):
        pass
    arrow(d, (1545, 425), (1470, 425))
    arrow(d, (1170, 425), (1085, 425))
    arrow(d, (785, 425), (695, 425))
    arrow(d, (415, 425), (325, 425))
    node(d, (240, 610, 1680, 735), "Core migration chain: keep LWB scheduling, rebuild SX1280 driver, radio timing, and board-level glue.", note_font)
    save(img, "code_structure.png")


def main():
    project_hierarchy()
    lwb_principle()
    runtime_logic()
    code_structure()


if __name__ == "__main__":
    main()
