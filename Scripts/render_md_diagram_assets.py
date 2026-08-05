#!/usr/bin/env python3
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont


OUT = Path("assets/ppt/md_diagrams")
BG = "#1f1f1f"
PANEL = "#252525"
TEXT = "#d7d7d7"
MUTED = "#b8b8b8"
LINE = "#777777"
DASH = "#bdbdbd"


def font(size, bold=False):
    regular = "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"
    bold_path = "/usr/share/fonts/opentype/noto/NotoSansCJK-Bold.ttc"
    return ImageFont.truetype(bold_path if bold and Path(bold_path).exists() else regular, size)


F = {
    "title": font(38, True),
    "node": font(21),
    "small": font(18),
    "tiny": font(16),
    "mini": font(12),
}


def wrap(text, max_chars):
    lines = []
    for part in str(text).split("\n"):
        cur = ""
        for ch in part:
            cur += ch
            if len(cur) >= max_chars:
                lines.append(cur)
                cur = ""
        if cur:
            lines.append(cur)
    return lines or [""]


def center_text(d, box, text, f=None, fill=TEXT, spacing=6):
    f = f or F["node"]
    x1, y1, x2, y2 = box
    # These assets emulate Mermaid screenshots, where labels are compact but
    # should not wrap every few Latin characters.
    lines = wrap(text, max(10, int((x2 - x1) / 10)))
    h = len(lines) * (f.size + spacing) - spacing
    y = y1 + ((y2 - y1) - h) / 2
    for line in lines:
        tw = d.textlength(line, font=f)
        d.text((x1 + ((x2 - x1) - tw) / 2, y), line, font=f, fill=fill)
        y += f.size + spacing


def node(d, box, text, f=None):
    d.rectangle(box, fill=PANEL, outline="#9a9a9a", width=2)
    center_text(d, box, text, f=f)


def line(d, a, b, fill=LINE, width=2):
    d.line((a, b), fill=fill, width=width)


def arrow(d, a, b, fill=LINE, width=2, dashed=False):
    if dashed:
        dashed_line(d, a, b, fill=fill, width=width)
    else:
        d.line((a, b), fill=fill, width=width)
    x1, y1 = a
    x2, y2 = b
    if abs(x2 - x1) >= abs(y2 - y1):
        pts = [(x2, y2), (x2 - 13 if x2 >= x1 else x2 + 13, y2 - 8), (x2 - 13 if x2 >= x1 else x2 + 13, y2 + 8)]
    else:
        pts = [(x2, y2), (x2 - 8, y2 - 13 if y2 >= y1 else y2 + 13), (x2 + 8, y2 - 13 if y2 >= y1 else y2 + 13)]
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
    d.text((24, 22), text, font=F["title"], fill=TEXT)


def save(img, name):
    OUT.mkdir(parents=True, exist_ok=True)
    path = OUT / name
    img.save(path)
    print(path)


def project_hierarchy():
    img = Image.new("RGB", (1920, 700), BG)
    d = ImageDraw.Draw(img)
    title(d, "1.3 项目层级图")

    mini = F["mini"]
    root = (865, 105, 1085, 170)
    node(d, root, "最终目标\n自制 STM32L476RG +\nSX1280 comboard", mini)

    cats = [
        ((195, 210, 315, 260), "硬件平台"),
        ((785, 210, 905, 260), "固件工程"),
        ((1245, 210, 1425, 260), "LWB / Gloria 协议栈"),
        ((1645, 210, 1795, 260), "下一阶段 PCB"),
    ]
    children = [
        [(20, 315, 170, 370, "STM32L476RG\nNUCLEO-L476RG"), (215, 315, 425, 370, "SX1280 / DLP-RFS1280\n2.4 GHz radio"), (520, 315, 710, 370, "DIO1 dual net\nPB4 EXTI + PB11 TIM2")],
        [(685, 315, 835, 370, "Src\nmain / FreeRTOS / HAL"), (905, 315, 1070, 370, "Lib\nprotocol / radio / time"), (1140, 315, 1285, 370, "Inc\napp_config / main.h")],
        [(1290, 315, 1430, 370, "LWB\nround / schedule / slot"), (1470, 315, 1610, 370, "Gloria / Glossy\n同步泛洪"), (1645, 315, 1815, 370, "SX1280 driver\nSPI / IRQ / timing")],
        [(1530, 315, 1610, 370, "原理图"), (1660, 315, 1745, 370, "布局布线"), (1790, 315, 1900, 370, "投板与 bring-up")],
    ]
    for cat, items in zip(cats, children):
        box, label = cat
        line(d, ((root[0] + root[2]) / 2, root[3]), ((box[0] + box[2]) / 2, box[1]))
        node(d, box, label, mini)
        for x1, y1, x2, y2, txt in items:
            line(d, ((box[0] + box[2]) / 2, box[3]), ((x1 + x2) / 2, y1))
            node(d, (x1, y1, x2, y2), txt, mini)
    d.line((28, 638, 1860, 638), fill="#777777", width=1)
    save(img, "project_hierarchy.png")


def lwb_principle():
    img = Image.new("RGB", (1920, 900), BG)
    d = ImageDraw.Draw(img)
    title(d, "3.4 LWB 原理图")

    participants = [
        (220, "Host\n调度器 / 时间基准"),
        (660, "Virtual Shared Bus\n逻辑共享总线"),
        (1050, "Source node 1"),
        (1330, "Source node 2"),
        (1610, "Source node N"),
    ]
    top_y, bot_y = 115, 785
    for x, label in participants:
        node(d, (x - 105, 90, x + 105, 178), label, F["small"])
        node(d, (x - 105, 795, x + 105, 865), label, F["small"])
        d.line((x, 178, x, 795), fill="#b5b5b5", width=2)

    arrow(d, (220, 250), (660, 250), fill="#d7d7d7", width=2)
    d.text((270, 220), "SCHED flood：广播本轮 slot 分配", font=F["small"], fill=TEXT)
    arrow(d, (660, 315), (1050, 315), fill=DASH, width=2, dashed=True)
    d.text((750, 288), "slot 1 分配给 node 1", font=F["small"], fill=TEXT)
    arrow(d, (660, 380), (1330, 380), fill=DASH, width=2, dashed=True)
    d.text((890, 352), "slot 2 分配给 node 2", font=F["small"], fill=TEXT)
    arrow(d, (660, 445), (1610, 445), fill=DASH, width=2, dashed=True)
    d.text((1060, 418), "slot n 分配给 node N", font=F["small"], fill=TEXT)

    for y, src_x, label_x in [(520, 1050, 720), (625, 1330, 860), (735, 1610, 1005)]:
        arrow(d, (src_x, y), (660, y), fill="#d7d7d7", width=2)
        d.text((label_x, y - 28), "DATA flood：Gloria 同步泛洪", font=F["small"], fill=TEXT)
        arrow(d, (660, y + 55), (220, y + 55), fill=DASH, width=2, dashed=True)
        d.text((305, y + 25), "Host 接收 / 全网节点可转发", font=F["small"], fill=TEXT)
    save(img, "lwb_principle.png")


def runtime_logic():
    img = Image.new("RGB", (1920, 900), BG)
    d = ImageDraw.Draw(img)
    title(d, "4.6 运行逻辑图")

    top = [
        ("MCU reset", 140, 105),
        ("main\nHAL / clock / GPIO / SPI / UART / timers", 430, 105),
        ("FreeRTOS start\n创建 lwbTask / preTask / postTask", 805, 105),
        ("vTask_com\n配置 radio / Gloria / LWB", 1200, 105),
        ("lwb_init\n计算 slot 时长 / 初始化队列", 1545, 105),
    ]
    boxes = []
    for txt, x, y in top:
        box = (x, y, x + 245, y + 82)
        boxes.append(box)
        node(d, box, txt, F["tiny"])
    for a, b in zip(boxes, boxes[1:]):
        arrow(d, (a[2], (a[1] + a[3]) / 2), (b[0], (b[1] + b[3]) / 2))

    start = (820, 255, 1100, 335)
    node(d, start, "lwb_start\n进入 round 循环", F["small"])
    arrow(d, ((boxes[-1][0] + boxes[-1][2]) / 2, boxes[-1][3]), ((start[0] + start[2]) / 2, start[1]))

    cycle = [
        ("Round start", 160, 465),
        ("PREPROCESS\npreTask 生成数据", 425, 465),
        ("SCHED1\nhost 发 schedule\nnode 接收同步", 730, 465),
        ("DATA slots\n按 schedule 发送或接收", 1040, 465),
        ("CONTENTION\n新节点注册 / IPI 更新", 1360, 465),
        ("SCHED2\n尾部确认 / 下轮信息", 1500, 650),
        ("POSTPROCESS\n处理 RX queue / 统计", 1120, 650),
        ("等待下一轮\nLPTIM / STOP2 / DIO1 唤醒", 700, 650),
    ]
    cycle_boxes = []
    for txt, x, y in cycle:
        box = (x, y, x + 250, y + 95)
        cycle_boxes.append(box)
        node(d, box, txt, F["tiny"])
    arrow(d, ((start[0] + start[2]) / 2, start[3]), ((cycle_boxes[0][0] + cycle_boxes[0][2]) / 2, cycle_boxes[0][1]))
    for a, b in zip(cycle_boxes[:5], cycle_boxes[1:5]):
        arrow(d, (a[2], (a[1] + a[3]) / 2), (b[0], (b[1] + b[3]) / 2))
    arrow(d, ((cycle_boxes[4][0] + cycle_boxes[4][2]) / 2, cycle_boxes[4][3]), ((cycle_boxes[5][0] + cycle_boxes[5][2]) / 2, cycle_boxes[5][1]))
    arrow(d, (cycle_boxes[5][0], (cycle_boxes[5][1] + cycle_boxes[5][3]) / 2), (cycle_boxes[6][2], (cycle_boxes[6][1] + cycle_boxes[6][3]) / 2))
    arrow(d, (cycle_boxes[6][0], (cycle_boxes[6][1] + cycle_boxes[6][3]) / 2), (cycle_boxes[7][2], (cycle_boxes[7][1] + cycle_boxes[7][3]) / 2))
    arrow(d, (cycle_boxes[7][0], (cycle_boxes[7][1] + cycle_boxes[7][3]) / 2), (cycle_boxes[0][0], cycle_boxes[0][3] + 65), dashed=True)
    line(d, (cycle_boxes[0][0], cycle_boxes[0][3] + 65), ((cycle_boxes[0][0] + cycle_boxes[0][2]) / 2, cycle_boxes[0][3]))
    save(img, "runtime_logic.png")


def code_structure():
    img = Image.new("RGB", (1920, 820), BG)
    d = ImageDraw.Draw(img)
    title(d, "6.2 代码结构图")

    layers = [
        ("应用层\nSrc/task_pre.c\nSrc/task_post.c", 70, 110, 310, 205),
        ("通信任务入口\nSrc/task_com.c", 420, 110, 650, 205),
        ("LWB 协议层\nLib/protocol/lwb", 780, 110, 1040, 205),
        ("Gloria 泛洪层\nLib/protocol/gloria", 1165, 110, 1430, 205),
        ("Radio 抽象层\nLib/radio/radio.c", 1550, 110, 1815, 205),
        ("SX1280 driver\nLib/radio/semtech", 1550, 325, 1815, 420),
        ("Time base\nhs_timer / lptimer / rtc", 1165, 325, 1430, 420),
        ("System glue\nlpm / gpio_exti / uart", 780, 325, 1040, 420),
        ("HAL / MCU\nDrivers / STM32L476", 420, 325, 650, 420),
        ("Hardware\nSPI / DIO1 / BUSY / RF", 70, 325, 310, 420),
    ]
    boxes = []
    for txt, x1, y1, x2, y2 in layers:
        box = (x1, y1, x2, y2)
        boxes.append(box)
        node(d, box, txt, F["tiny"])

    for i in range(4):
        arrow(d, (boxes[i][2], 158), (boxes[i + 1][0], 158))
    arrow(d, ((boxes[4][0] + boxes[4][2]) / 2, boxes[4][3]), ((boxes[5][0] + boxes[5][2]) / 2, boxes[5][1]))
    arrow(d, (boxes[5][0], 373), (boxes[6][2], 373))
    arrow(d, (boxes[6][0], 373), (boxes[7][2], 373))
    arrow(d, (boxes[7][0], 373), (boxes[8][2], 373))
    arrow(d, (boxes[8][0], 373), (boxes[9][2], 373))

    note = (360, 560, 1560, 670)
    node(d, note, "核心移植链路：LWB 调度不变，重建 SX1280 radio driver、radio timing 和板级 glue。", F["small"])
    save(img, "code_structure.png")


def main():
    project_hierarchy()
    lwb_principle()
    runtime_logic()
    code_structure()


if __name__ == "__main__":
    main()
