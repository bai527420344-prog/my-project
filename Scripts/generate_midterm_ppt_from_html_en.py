#!/usr/bin/env python3
from pathlib import Path
import sys

from PIL import Image, ImageDraw, ImageOps

sys.path.insert(0, str(Path(__file__).resolve().parent))
import generate_midterm_ppt as base  # noqa: E402
from generate_midterm_ppt import Slide, write_pptx  # noqa: E402
from generate_midterm_ppt_raster import (  # noqa: E402
    W, H, BG, DARK, BLUE, TEAL, AMBER, TEXT, MUTED, LINE, F,
    new, rr, card, bullet_list, fit_image, photo_box, arrow, node, table,
)


OUT_DIR = Path("assets/ppt/rendered_slides_en")
PPT_OUT = Path("MID_TERM_DEFENSE_SLIDES_EN.pptx")
DOC_TITLE = "Integration of SX1280 Transceiver for High-Rate-Low-Power Wireless Bus (LWB) Networks"


def asset_ppt_dir():
    packaged = Path("ppt/MID_TERM_DEFENSE_EN_HTML_PPT/assets/ppt")
    if packaged.exists():
        return packaged
    return Path("assets/ppt")


def save(slide, name):
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    path = OUT_DIR / f"{name}.png"
    slide.save(path)
    return path


def draw_title(d, title, subtitle=None, dark=False):
    color = "white" if dark else TEXT
    sub = "#D9E6F2" if dark else MUTED
    d.text((64, 54), title, font=F["h1"], fill=color)
    if subtitle:
        d.text((68, 122), subtitle, font=F["body"], fill=sub)


def text_block(d, text, xy, font, fill=TEXT, max_chars=60, line_gap=8):
    x, y = xy
    line = ""
    for word in text.split():
        trial = f"{line} {word}".strip()
        if len(trial) > max_chars:
            d.text((x, y), line, font=font, fill=fill)
            y += font.size + line_gap
            line = word
        else:
            line = trial
    if line:
        d.text((x, y), line, font=font, fill=fill)
    return y


def word_lines(text, max_chars):
    lines = []
    for paragraph in str(text).split("\n"):
        line = ""
        for word in paragraph.split():
            trial = f"{line} {word}".strip()
            if len(trial) > max_chars and line:
                lines.append(line)
                line = word
            else:
                line = trial
        if line:
            lines.append(line)
    return lines or [""]


def bullet_words(d, items, x, y, w, font, color=TEXT, bullet_color=BLUE, gap=18, line_gap=8):
    max_chars = max(18, int(w / 19))
    for item in items:
        lines = word_lines(item, max_chars)
        d.text((x, y), "•", font=font, fill=bullet_color)
        for i, line in enumerate(lines):
            d.text((x + 34, y + i * (font.size + line_gap)), line, font=font, fill=color)
        y += len(lines) * (font.size + line_gap) + gap
    return y


def word_card(d, box, title, body, accent=BLUE, title_font=None, body_font=None, max_chars=None):
    rr(d, box, fill="white", outline=LINE, width=2, r=12)
    x1, y1, x2, y2 = box
    d.rectangle((x1, y1, x1 + 8, y2), fill=accent)
    title_font = title_font or F["h2"]
    body_font = body_font or F["body"]
    d.text((x1 + 34, y1 + 34), title, font=title_font, fill=TEXT)
    max_chars = max_chars or max(24, int((x2 - x1 - 70) / 20))
    y = y1 + 92
    for line in word_lines(body, max_chars):
        d.text((x1 + 34, y), line, font=body_font, fill=MUTED)
        y += body_font.size + 9
    return y


def cover_image(path, box, bg="#EEF2F6", centering=(0.5, 0.5)):
    x1, y1, x2, y2 = box
    frame = Image.new("RGB", (x2 - x1, y2 - y1), bg)
    if not path or not Path(path).exists():
        return frame
    img = Image.open(path).convert("RGB")
    return ImageOps.fit(img, (x2 - x1, y2 - y1), Image.LANCZOS, centering=centering)


def diagram_slide(path, name):
    s = new("#1f1f1f")
    d = ImageDraw.Draw(s)
    if path.exists():
        img = fit_image(path, (40, 40, W - 40, H - 40), "#1f1f1f")
        s.paste(img, (40, 40))
    else:
        d.text((100, 120), f"Missing diagram: {path}", font=F["h2"], fill="white")
    return save(s, name)


def slide_cover(dev):
    s = new(DARK)
    d = ImageDraw.Draw(s)
    if dev.exists():
        img = Image.open(dev).convert("RGB")
        img = img.resize((int(img.width * H / img.height), H))
        s.paste(img.crop((max(0, img.width - 980), 0, img.width, H)), (940, 0))
    d.rectangle((0, 0, W, H), fill=(19, 32, 43))
    if dev.exists():
        img = fit_image(dev, (1050, 80, 1840, 980), DARK)
        s.paste(img, (1050, 80))
        d.rectangle((940, 0, W, H), outline="#2c4d66", width=2)
    d.text((72, 92), "Mid-Term Defense", font=F["h2"], fill="#9BD2F0")
    text_block(
        d,
        "Integration of SX1280 Transceiver for High-Rate-Low-Power Wireless Bus (LWB) Networks",
        (72, 205),
        F["title"],
        "white",
        max_chars=29,
        line_gap=16,
    )
    d.text((76, 558), "Migrating from L433 + SX1262 to L476 + SX1280, toward a custom comboard.", font=F["body"], fill="#D9E6F2")
    card(d, (84, 730, 390, 870), "Current", "Stage 4 development board", fill="#173A55", accent=TEAL)
    card(d, (430, 730, 736, 870), "Next", "Stage 5 custom comboard", fill="#173A55", accent=AMBER)
    return save(s, "slide01_cover")


def slide_project_definition():
    s = new(); d = ImageDraw.Draw(s)
    draw_title(d, "What This Project Is", "First prove feasibility, then integrate the validated prototype into custom hardware")
    cards = [
        ("1", "Project Definition", "Migrate LWB from the original L433 + SX1262 platform to L476 + SX1280. The work covers firmware, radio timing, board-level wiring, and final hardware preparation.", BLUE),
        ("2", "Feasibility Study", "Verify whether the LWB/Gloria stack can keep its timing behavior on a 2.4 GHz SX1280 radio and a higher-resource L476 MCU.", TEAL),
        ("3", "Final Design", "Use the validated development-board prototype as the basis for a compact custom comboard with fixed power, clock, SPI, DIO1, and debug connections.", AMBER),
    ]
    x = 88
    for n, title, body, accent in cards:
        rr(d, (x, 230, x + 540, 650), fill="white", outline=LINE, width=2, r=14)
        d.rectangle((x, 230, x + 8, 650), fill=accent)
        d.text((x + 34, 270), n, font=F["title"], fill=BLUE)
        d.text((x + 34, 368), title, font=F["h2"], fill=TEXT)
        text_block(d, body, (x + 34, 430), F["body"], MUTED, max_chars=38, line_gap=8)
        x += 604
    rr(d, (120, 735, 1800, 855), fill="#EAF8F6", outline=TEAL, width=3, r=0)
    d.text((445, 775), "Core idea: reduce migration risk by splitting the work into controlled stages.", font=F["h2"], fill=TEXT)
    return save(s, "slide02_project_definition")


def slide_lwb_intro():
    s = new(); d = ImageDraw.Draw(s)
    draw_title(d, "What LWB Is", "LWB abstracts a multi-hop wireless network as a virtual shared bus")
    items = [
        ("Traditional Mesh", [
            "Each node maintains neighbor tables, routes, and forwarding states.",
            "Topology changes are complex and continuous listening increases power use.",
            "Communication reliability depends heavily on routing and local link state.",
        ], AMBER),
        ("LWB", [
            "The host centrally assigns time slots.",
            "Each slot uses Glossy / Gloria synchronous flooding.",
            "Nodes sleep and wake according to schedule, which supports low power.",
            "The network behaves like a shared bus, but packets are propagated wirelessly.",
        ], TEAL),
    ]
    y = 225
    heights = [270, 340]
    for idx, (title, bullets, accent) in enumerate(items):
        h = heights[idx]
        rr(d, (120, y, 1800, y + h), fill="white", outline=LINE, width=2, r=12)
        d.rectangle((120, y, 128, y + h), fill=accent)
        d.text((160, y + 58), title, font=F["h2"], fill=TEXT)
        bullet_words(d, bullets, 620, y + 42, 1060, F["body"], gap=14, line_gap=6)
        y += h + 35
    rr(d, (120, 890, 1800, 955), fill="white", outline=LINE, width=2, r=0)
    d.text((470, 908), "Key point: slots are centrally scheduled, while packet propagation is network-wide flooding.", font=F["body_b"], fill=TEXT)
    return save(s, "slide03_lwb_intro")


def slide_migration_reason():
    s = new(); d = ImageDraw.Draw(s)
    draw_title(d, "Why Migrate to L476 + SX1280", "The original platform is useful as a baseline, but it does not fully match the project target")
    cards = [
        ("Original Platform Limits", [
            "L433 has limited Flash, RAM, and GPIO resources.",
            "SX1262 is a Sub-GHz radio, while this project targets high-frequency communication.",
            "The original software is tightly coupled with the custom comboard.",
            "Going directly to a new PCB would make failures hard to diagnose.",
        ], BLUE),
        ("Target Platform Value", [
            "L476 provides larger Flash, RAM, and GPIO margin.",
            "SX1280 supports 2.4 GHz, LoRa, GFSK, FLRC, and ranging.",
            "It better matches indoor mesh and future localization use cases.",
            "The staged route separates MCU risk, radio risk, timing risk, and PCB risk.",
        ], TEAL),
    ]
    y = 215
    for title, bullets, accent in cards:
        rr(d, (105, y, 1815, y + 325), fill="white", outline=LINE, width=2, r=12)
        d.rectangle((105, y, 113, y + 325), fill=accent)
        d.text((145, y + 44), title, font=F["h2"], fill=TEXT)
        bullet_words(d, bullets, 565, y + 34, 1160, F["body"], gap=18, line_gap=8)
        y += 360
    return save(s, "slide05_migration_reason")


def slide_stage_stack():
    s = new(); d = ImageDraw.Draw(s)
    draw_title(d, "Overall Route: Five-Stage Progress", "Change only one key variable at a time")
    rows = [
        ("1. L433 + SX1262 comboard", "Original baseline and final form-factor reference.", "Done", TEAL),
        ("2. L433 + SX1262 dev board", "Remove dependency on the original custom comboard.", "Done", TEAL),
        ("3. L476 + SX1262 dev board", "Replace only the MCU and validate MCU portability.", "Done", TEAL),
        ("4. L476 + SX1280 dev board", "Replace only the radio and validate the 2.4 GHz platform.", "Current", BLUE),
        ("5. L476 + SX1280 comboard", "Custom PCB, board-level bring-up, and final integration.", "Next", "#9AA8B6"),
    ]
    y = 230
    for i, (title, desc, status, color) in enumerate(rows):
        fill = "#EAF8F6" if status == "Done" else "#EAF1F8" if status == "Current" else "#F2F4F7"
        rr(d, (115, y, 1805, y + 120), fill=fill, outline=color, width=3, r=0)
        d.text((150, y + 24), title, font=F["h2"], fill=TEXT)
        d.text((740, y + 36), desc, font=F["body"], fill=MUTED)
        rr(d, (1650, y + 38, 1768, y + 84), fill=color if status != "Next" else "#DFE5EC", outline=color, r=0)
        d.text((1672, y + 47), status, font=F["small_b"], fill="white" if status != "Next" else TEXT)
        if i < 4:
            arrow(d, (160, y + 120), (160, y + 146), color=AMBER, width=6)
        y += 145
    return save(s, "slide06_stage_stack")


def slide_hardware_route(imgs):
    s = new(); d = ImageDraw.Draw(s)
    draw_title(d, "Five-Stage Hardware Route", "Hardware evolution from original comboard to the final custom comboard")
    labels = [
        "1. L433 + SX1262 comboard\nOriginal platform reference.",
        "2. L433 + SX1262 dev board\nSeparated from custom comboard.",
        "3. L476 + SX1262 dev board\nMCU migration completed.",
        "4. L476 + SX1280 dev board\nRadio migration completed.",
        "5. L476 + SX1280 comboard\nNext-stage custom PCB.",
    ]
    x = 58
    for img, label in zip(imgs, labels):
        photo_box(s, d, img, (x, 190, x + 340, 820), label)
        x += 364
    return save(s, "slide07_hardware_route")


def slide_stage12(img):
    s = new(); d = ImageDraw.Draw(s)
    draw_title(d, "Stage 1 and Stage 2: From Comboard to Development Board", "First remove dependency on the original custom hardware")
    if img:
        s.paste(fit_image(img, (70, 190, 860, 1010), "#FFFFFF"), (70, 190))
    word_card(d, (910, 205, 1810, 520), "Stage 1: Original Platform", "MCU: STM32L433RC; radio: SX1262. Used to understand original hardware assumptions and call chains. This stage defines the baseline behavior that later ports must preserve.", accent=BLUE, body_font=F["small"], max_chars=72)
    word_card(d, (910, 575, 1810, 910), "Stage 2: Development Board Adaptation", "Keep L433 + SX1262 unchanged, only change the hardware carrier. Adapt pin mapping, HAL init, SPI, GPIO, and UART. This isolates board-adaptation issues before changing MCU or radio.", accent=TEAL, body_font=F["small"], max_chars=72)
    return save(s, "slide09_stage12")


def slide_l476_compare():
    s = new(); d = ImageDraw.Draw(s)
    draw_title(d, "L476 vs L433 Data Comparison", "The main reason for choosing L476 is resource margin, not CPU frequency")
    table(d, 65, 205, [235, 245, 410], 62, ["Item", "L433RC", "L476RG"], [
        ["Core", "Cortex-M4F", "Cortex-M4F"],
        ["Max frequency", "80 MHz", "80 MHz"],
        ["Flash", "256 KB", "1024 KB"],
        ["RAM", "64 KB", "128 KB"],
        ["GPIO", "about 37", "about 51"],
        ["Project timing", "original platform", "48 MHz SYSCLK, TIM2 = 8 MHz"],
    ])
    for i, (lab, old, newv) in enumerate([("Flash KB", 256, 1024), ("RAM KB", 64, 128), ("GPIO", 37, 51)]):
        y = 250 + i * 160
        d.text((1015, y), lab, font=F["h2"], fill=TEXT)
        d.rectangle((1225, y + 5, 1725, y + 38), fill="#DFE7EF")
        d.rectangle((1225, y + 5, 1225 + int(500 * old / max(old, newv)), y + 38), fill="#9DB8D2")
        d.rectangle((1225, y + 58, 1225 + int(500 * newv / max(old, newv)), y + 91), fill=TEAL)
        d.text((1750, y + 5), f"{old}", font=F["small_b"], fill=TEXT)
        d.text((1750, y + 58), f"{newv}", font=F["small_b"], fill=TEXT)
    rr(d, (1015, 780, 1810, 930), fill="white", outline=LINE, width=2, r=0)
    text_block(d, "Conclusion: the frequency is not the key point. Flash, RAM, and GPIO headroom are the real migration value.", (1045, 815), F["h2"], TEXT, max_chars=55)
    return save(s, "slide10_l476_compare")


def slide_sx_compare():
    s = new(); d = ImageDraw.Draw(s)
    draw_title(d, "SX1280 vs SX1262 Data Comparison", "The radio migration changes the communication band and protocol capability, not only the driver API")
    table(d, 65, 205, [210, 330, 435], 62, ["Item", "SX1262", "SX1280"], [
        ["Band", "Sub-GHz", "2.4 GHz ISM"],
        ["Modulations", "LoRa / FSK", "LoRa / GFSK / FLRC / BLE / Ranging"],
        ["Max TX power", "+22 dBm", "+12.5 dBm"],
        ["Antenna size", "Larger", "Smaller"],
        ["Best fit", "Long-range Sub-GHz links", "Indoor mesh / ranging"],
        ["Main cost", "Regional band complexity", "Shorter range / weaker walls"],
    ])
    for i, (lab, old, newv, old_label, new_label) in enumerate([
        ("Band GHz", 0.9, 2.4, "Sub-GHz", "2.4"),
        ("TX dBm", 22, 12.5, "22", "12.5"),
        ("Modes", 2, 5, "2", "5"),
    ]):
        y = 250 + i * 160
        d.text((1080, y), lab, font=F["h2"], fill=TEXT)
        mx = max(old, newv)
        d.rectangle((1255, y + 5, 1745, y + 38), fill="#DFE7EF")
        d.rectangle((1255, y + 5, 1255 + int(490 * old / mx), y + 38), fill="#9DB8D2")
        d.rectangle((1255, y + 58, 1255 + int(490 * newv / mx), y + 91), fill=TEAL)
        d.text((1770, y + 5), old_label, font=F["small_b"], fill=TEXT)
        d.text((1770, y + 58), new_label, font=F["small_b"], fill=TEXT)
    rr(d, (1080, 790, 1810, 945), fill="white", outline=LINE, width=2, r=0)
    text_block(d, "Conclusion: SX1280 trades long-range Sub-GHz power for 2.4 GHz operation, richer modulation options, smaller antennas, and future indoor ranging support.", (1110, 820), F["body_b"], TEXT, max_chars=52)
    return save(s, "slide11_sx_compare")


def slide_stage4(img):
    s = new(); d = ImageDraw.Draw(s)
    draw_title(d, "Stage 4: L476 + SX1280 Radio Migration", "Current focus: verify LWB on a 2.4 GHz high-frequency radio platform")
    if img:
        s.paste(cover_image(img, (60, 185, 910, 985), "#FFFFFF", centering=(0.45, 0.55)), (60, 185))
        d.rectangle((60, 185, 910, 985), outline=LINE, width=3)
    table(d, 950, 185, [155, 245, 500], 58, ["Item", "SX1262", "SX1280"], [
        ["Band", "Sub-GHz", "2.4 GHz ISM"],
        ["Modulations", "LoRa / FSK", "LoRa / GFSK / FLRC / Ranging"],
        ["Advantage", "Long range", "Small antenna, multi-modulation, indoor use"],
        ["Cost", "Regulation complexity", "Weaker wall penetration than Sub-GHz"],
    ])
    word_card(d, (950, 510, 1810, 930), "Main Migration Work", "Rewrite SPI opcodes, packet types, CRC, whitening, and timeout handling. Rebuild 2.4 GHz frequency, power, and modulation tables. Recalibrate Gloria timing and LWB slot durations. Validate LoRa, GFSK, and FLRC with the same upper-layer LWB logic.", accent=TEAL, body_font=F["body"], max_chars=56)
    return save(s, "slide12_stage4")


def slide_params():
    s = new(); d = ImageDraw.Draw(s)
    draw_title(d, "Experimental Platform and Key Parameters", "The current system has been validated with two boards: HOST + NODE")
    table(d, 65, 175, [430, 1380], 58, ["Item", "Current setup"], [
        ["MCU", "NUCLEO-L476RG"],
        ["Radio", "DLP-RFS1280 / SX1280"],
        ["Wiring", "jumper wires, DIO1 connected to both PB4 and PB11"],
        ["Network size", "1 host + 1 source node"],
        ["Round period", "15 s"],
    ])
    table(d, 65, 545, [1070, 740], 54, ["Parameter", "Current value"], [
        ["GLORIA_INTERFACE_MODULATION", "11 = FLRC 260 kbit/s"],
        ["GLORIA_INTERFACE_RF_BAND", "24 = 2450 MHz"],
        ["LWB_SCHED_PERIOD", "15 s"],
        ["LWB_N_TX", "2"],
        ["LWB_NUM_HOPS", "6"],
        ["HS_TIMER_FREQUENCY", "8 MHz"],
    ])
    return save(s, "slide15_params")


def slide_logic(logic):
    s = new(); d = ImageDraw.Draw(s)
    draw_title(d, "Logic Analyzer Validation", "Correct TX/RX switching across HOST and client pins")
    if logic.exists():
        s.paste(fit_image(logic, (60, 170, 1860, 780), "#101820"), (60, 170))
    tags = [("D4 client PA12 RX", "#F4D03F"), ("D5 client PA11 TX", "#58D68D"), ("D6 HOST PA12 RX", "#5DADE2"), ("D7 HOST PA11 TX", "#AF7AC5")]
    for i, (text, color) in enumerate(tags):
        rr(d, (105 + i * 450, 820, 500 + i * 450, 875), fill=color, outline=color, r=0)
        d.text((130 + i * 450, 835), text, font=F["small_b"], fill="#101820")
    rr(d, (110, 925, 1810, 1000), fill="white", outline=LINE, width=2, r=0)
    d.text((210, 944), "The waveform shows expected TX/RX switching during schedule and data slots.", font=F["body_b"], fill=TEXT)
    return save(s, "slide16_logic")


def slide_results():
    s = new(); d = ImageDraw.Draw(s)
    draw_title(d, "Current Results and Mid-Term Conclusion", "After Stage 4, software migration risk has mostly converged")
    table(d, 65, 190, [270, 1540], 62, ["Item", "Current result"], [
        ["Validation platform", "NUCLEO-L476RG + DLP-RFS1280 + PB4/PB11 jumper"],
        ["Network size", "two-board validation: 1 host + 1 source node"],
        ["Validated modulations", "LoRa SF5, GFSK 125K, FLRC 260K"],
        ["Build size", "text=123400, data=4272, bss=30408"],
        ["Flash usage", "about 127.7 KB / 1024 KB"],
        ["RAM usage", "about 34.7 KB / 96 KB SRAM1"],
    ])
    word_card(d, (65, 650, 1810, 900), "Mid-Term Conclusion", "LWB/Gloria can run on the L476 + SX1280 platform. LoRa, GFSK, and FLRC have all been validated end-to-end. The next major risk is custom PCB design and board-level bring-up. The migration method keeps the debugging scope narrow at each stage.", accent=TEAL, body_font=F["body"], max_chars=118)
    return save(s, "slide17_results")


def slide_pcb():
    s = new(); d = ImageDraw.Draw(s)
    draw_title(d, "Stage 5: Custom L476 + SX1280 Comboard", "Integrate the jumper-wire prototype into deliverable hardware")
    word_card(d, (65, 185, 1810, 455), "Critical PCB Connections", "MCU: STM32L476RGTx LQFP64 minimum system. Radio: first version uses DLP-RFS1280 module. SPI1: PA5 SCK / PA6 MISO / PA7 MOSI / PA8 NSS. DIO1: must connect to both PB4 and PB11. BUSY: connect to PB3; keep SWD, UART, HSE, LSE, and test points.", accent=BLUE, body_font=F["small"], max_chars=132)
    table(d, 65, 505, [190, 1120, 220], 58, ["Period", "Task", "Duration"], [
        ["Week 1", "Schematic", "1 week"],
        ["Week 2-3", "Placement, routing, DRC", "2 weeks"],
        ["Week 4", "PCB manufacturing", "1 week"],
        ["Week 5-6", "Soldering, flashing, board bring-up", "2 weeks"],
        ["Week 7-8", "Results and thesis writing", "2 weeks"],
    ])
    rr(d, (65, 895, 1810, 980), fill="white", outline=LINE, width=2, r=0)
    d.text((330, 924), "The PCB must fix DIO1 dual connection, power, clocks, debug interface, and radio wiring.", font=F["body_b"], fill=TEXT)
    return save(s, "slide18_pcb")


def slide_qa(dev):
    s = new(DARK); d = ImageDraw.Draw(s)
    if dev.exists():
        img = fit_image(dev, (1000, 80, 1840, 980), DARK)
        s.paste(img, (1000, 80))
    d.text((72, 90), "Summary", font=F["h2"], fill="#9BD2F0")
    text_block(d, "This stage proves that LWB can run on the L476 + SX1280 development-board platform", (72, 210), F["title"], "white", max_chars=31, line_gap=14)
    d.text((76, 540), "The next stage is schematic design, PCB layout, manufacturing, and bring-up of the custom comboard.", font=F["body"], fill="#D9E6F2")
    card(d, (84, 720, 410, 870), "Proven", "protocol feasibility", fill="#173A55", accent=TEAL)
    card(d, (450, 720, 780, 870), "Next", "hardware integration", fill="#173A55", accent=AMBER)
    d.text((1370, 855), "Q&A", font=F["title"], fill="white")
    return save(s, "slide19_qa")


def build_slides():
    ppt = asset_ppt_dir()
    l433_comboard = ppt / "l433-sx1262-comboard.jpg"
    l433_dev = ppt / "l433-sx1262-devboard.jpg"
    l476_sx1262 = ppt / "l476-sx1262-devboard.jpg"
    l476_sx1280 = ppt / "l476-sx1280-devboard.jpg"
    l476_comboard = ppt / "l476-sx1280-comboard.jpg"
    logic = ppt / "logic_analyzer_round_view.png"
    diagrams = ppt / "md_diagrams_en"
    slides = [
        slide_cover(l476_sx1280),
        slide_project_definition(),
        slide_lwb_intro(),
        diagram_slide(diagrams / "lwb_principle.png", "slide04_lwb_principle"),
        slide_migration_reason(),
        slide_l476_compare(),
        slide_sx_compare(),
        slide_stage_stack(),
        slide_hardware_route([l433_comboard, l433_dev, l476_sx1262, l476_sx1280, l476_comboard]),
        slide_stage12(l433_dev),
        slide_stage4(l476_sx1280),
        diagram_slide(diagrams / "runtime_logic.png", "slide13_runtime_logic"),
        diagram_slide(diagrams / "code_structure.png", "slide14_code_structure"),
        slide_params(),
        slide_logic(logic),
        slide_results(),
        slide_pcb(),
        slide_qa(l476_sx1280),
    ]
    return slides


def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for old_slide in OUT_DIR.glob("slide*.png"):
        old_slide.unlink()
    slide_paths = build_slides()
    ppt_slides = []
    for path in slide_paths:
        slide = Slide()
        slide.image(path)
        ppt_slides.append(slide)

    def core_props_en():
        return f'''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<cp:coreProperties xmlns:cp="http://schemas.openxmlformats.org/package/2006/metadata/core-properties"
 xmlns:dc="http://purl.org/dc/elements/1.1/"
 xmlns:dcterms="http://purl.org/dc/terms/"
 xmlns:dcmitype="http://purl.org/dc/dcmitype/"
 xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
  <dc:title>{DOC_TITLE}</dc:title>
  <dc:creator>Codex</dc:creator>
  <cp:lastModifiedBy>Codex</cp:lastModifiedBy>
</cp:coreProperties>'''

    base.core_props = core_props_en
    base.OUT = PPT_OUT
    write_pptx(ppt_slides)
    print(f"wrote {PPT_OUT} with {len(ppt_slides)} slides")


if __name__ == "__main__":
    main()
