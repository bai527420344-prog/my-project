#!/usr/bin/env python3
from pathlib import Path
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "Scripts"))

from generate_midterm_ppt_from_html_en import build_slides  # noqa: E402

import uno  # noqa: E402
from com.sun.star.awt import Point, Size  # noqa: E402
from com.sun.star.beans import PropertyValue  # noqa: E402


OUT = ROOT / "MID_TERM_DEFENSE_SLIDES_EN_REAL_IMAGES_FIXED_V2.pptx"


def prop(name, value):
    p = PropertyValue()
    p.Name = name
    p.Value = value
    return p


def connect():
    local_ctx = uno.getComponentContext()
    resolver = local_ctx.ServiceManager.createInstanceWithContext(
        "com.sun.star.bridge.UnoUrlResolver", local_ctx
    )
    last = None
    for _ in range(30):
        try:
            ctx = resolver.resolve(
                "uno:socket,host=127.0.0.1,port=2002;urp;StarOffice.ComponentContext"
            )
            return ctx
        except Exception as exc:
            last = exc
            time.sleep(0.2)
    raise RuntimeError(f"could not connect to LibreOffice: {last}")


def main():
    slide_images = [Path(p).resolve() for p in build_slides()]
    ctx = connect()
    smgr = ctx.ServiceManager
    desktop = smgr.createInstanceWithContext("com.sun.star.frame.Desktop", ctx)
    doc = desktop.loadComponentFromURL("private:factory/simpress", "_blank", 0, ())

    pages = doc.getDrawPages()
    width, height = 28000, 15750  # 16:9, in 1/100 mm.

    while pages.getCount() < len(slide_images):
        pages.insertNewByIndex(pages.getCount())
    while pages.getCount() > len(slide_images):
        pages.remove(pages.getByIndex(pages.getCount() - 1))

    for idx, image in enumerate(slide_images):
        page = pages.getByIndex(idx)
        page.Width = width
        page.Height = height
        shape = doc.createInstance("com.sun.star.drawing.GraphicObjectShape")
        shape.Position = Point(0, 0)
        shape.Size = Size(width, height)
        shape.GraphicURL = uno.systemPathToFileUrl(str(image))
        page.add(shape)

    url = uno.systemPathToFileUrl(str(OUT))
    doc.storeAsURL(url, (prop("FilterName", "Impress MS PowerPoint 2007 XML"),))
    doc.close(True)
    print(f"wrote {OUT} with {len(slide_images)} image slides")


if __name__ == "__main__":
    main()
