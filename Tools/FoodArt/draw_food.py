"""Paint the dish pictures for the food menus (Data/Food/*.png).

Each dish is painted at 4x size with simple shapes, shading and speckle
texture, then scaled down for smooth edges. No assets are needed: run this
again after changing a dish.

Usage:
    python draw_food.py ../../Data/Food
"""

import math
import os
import random
import sys

from PIL import Image, ImageDraw, ImageFilter

S = 1024          # painting size
OUT = 256         # picture size in the game


def mix(a, b, t):
    return tuple(int(round(a[i] + (b[i] - a[i]) * t)) for i in range(len(a)))


def shade(c, k):
    """Darker (k < 1) or lighter (k > 1) colour."""
    if k <= 1:
        return tuple(int(v * k) for v in c[:3]) + tuple(c[3:])
    return tuple(int(v + (255 - v) * (k - 1)) for v in c[:3]) + tuple(c[3:])


class Painter:
    def __init__(self, background):
        self.img = Image.new("RGBA", (S, S), (0, 0, 0, 255))
        self.d = ImageDraw.Draw(self.img)
        self.rng = random.Random(7)
        background(self)

    # -- building blocks -------------------------------------------------

    def layer(self):
        return Image.new("RGBA", (S, S), (0, 0, 0, 0))

    def paste(self, layer, blur=0):
        if blur:
            layer = layer.filter(ImageFilter.GaussianBlur(blur))
        self.img.alpha_composite(layer)

    def shadow(self, box, strength=110, blur=28):
        """A soft shadow on the table under something."""
        l = self.layer()
        ImageDraw.Draw(l).ellipse(box, fill=(0, 0, 0, strength))
        self.paste(l, blur)

    def gradient_ellipse(self, box, inner, outer, centre=(0.4, 0.35), steps=40):
        """An ellipse lit from the top left: rings from outer to inner colour."""
        x0, y0, x1, y1 = box
        cx = x0 + (x1 - x0) * centre[0]
        cy = y0 + (y1 - y0) * centre[1]
        for i in range(steps):
            t = i / (steps - 1)
            k = 1 - t
            bx0 = cx + (x0 - cx) * k
            by0 = cy + (y0 - cy) * k
            bx1 = cx + (x1 - cx) * k
            by1 = cy + (y1 - cy) * k
            if bx1 - bx0 < 1 or by1 - by0 < 1:
                break
            self.d.ellipse((bx0, by0, bx1, by1), fill=mix(outer, inner, t))

    def gradient_polygon(self, points, top, bottom):
        """A polygon shaded from top to bottom."""
        l = self.layer()
        mask = Image.new("L", (S, S), 0)
        ImageDraw.Draw(mask).polygon(points, fill=255)
        ys = [p[1] for p in points]
        y0, y1 = int(min(ys)), int(max(ys)) + 1
        g = ImageDraw.Draw(l)
        for y in range(y0, y1):
            g.line((0, y, S, y), fill=mix(top, bottom, (y - y0) / max(1, y1 - y0)))
        out = self.layer()
        out.paste(l, (0, 0), mask)
        self.paste(out)

    def speckle(self, box, colours, count, size=(3, 7), inside=None):
        x0, y0, x1, y1 = box
        for _ in range(count):
            x = self.rng.uniform(x0, x1)
            y = self.rng.uniform(y0, y1)
            if inside and not inside(x, y):
                continue
            r = self.rng.uniform(*size)
            self.d.ellipse((x - r, y - r * 0.8, x + r, y + r * 0.8), fill=self.rng.choice(colours))

    def highlight(self, box, alpha=90, blur=10):
        l = self.layer()
        ImageDraw.Draw(l).ellipse(box, fill=(255, 255, 255, alpha))
        self.paste(l, blur)

    # -- tableware ----------------------------------------------------------

    def plate(self, cx, cy, rx, ry, rim=(244, 242, 236), well=(232, 229, 222), band=None):
        self.shadow((cx - rx * 0.95, cy - ry * 0.75 + 18, cx + rx * 1.02, cy + ry * 1.1 + 18))
        self.d.ellipse((cx - rx, cy - ry + 10, cx + rx, cy + ry + 10), fill=shade(rim, 0.78))
        self.gradient_ellipse((cx - rx, cy - ry, cx + rx, cy + ry), rim, shade(rim, 0.9))
        if band:
            self.d.ellipse((cx - rx * 0.9, cy - ry * 0.9, cx + rx * 0.9, cy + ry * 0.9), outline=band, width=6)
        self.gradient_ellipse((cx - rx * 0.7, cy - ry * 0.7, cx + rx * 0.7, cy + ry * 0.7), well, shade(well, 0.92), centre=(0.6, 0.7))

    def bowl(self, cx, cy, rx, ry, depth, soup, rim=(240, 238, 230)):
        """A deep plate seen from the side and above, filled with soup."""
        self.plate(cx, cy + depth * 0.55, rx * 1.25, ry * 1.25)
        body = [(cx - rx, cy)]
        for i in range(21):
            a = math.pi * i / 20
            body.append((cx - rx * math.cos(a) * (1 - 0.18 * math.sin(a)), cy + depth * math.sin(a) + ry * 0.4 * math.sin(a)))
        body.append((cx + rx, cy))
        self.gradient_polygon(body, shade(rim, 0.95), shade(rim, 0.7))
        self.d.ellipse((cx - rx, cy - ry, cx + rx, cy + ry), fill=shade(rim, 1.0))
        self.gradient_ellipse((cx - rx * 0.88, cy - ry * 0.82, cx + rx * 0.88, cy + ry * 0.86), shade(soup, 1.15), shade(soup, 0.8), centre=(0.35, 0.3))

    def cup(self, cx, cy, rx, ry, height, drink, body=(246, 244, 238), handle=True, saucer=True):
        if saucer:
            self.plate(cx, cy + height + ry * 0.1, rx * 1.75, ry * 1.6)
        else:
            self.shadow((cx - rx * 1.1, cy + height - ry * 0.5, cx + rx * 1.2, cy + height + ry * 1.3))
        if handle:
            hx = cx + rx * 0.9
            self.d.ellipse((hx - rx * 0.15, cy + height * 0.15, hx + rx * 0.55, cy + height * 0.8), outline=shade(body, 0.85), width=int(rx * 0.14))
        pts = [(cx - rx, cy)]
        for i in range(21):
            a = math.pi * i / 20
            pts.append((cx - rx * math.cos(a) * (0.82 + 0.18 * (1 - math.sin(a))), cy + height * math.sin(a) ** 0.5))
        pts.append((cx + rx, cy))
        self.gradient_polygon(pts, body, shade(body, 0.72))
        # Light down the left side of the cup.
        l = self.layer()
        ImageDraw.Draw(l).rectangle((cx - rx * 0.62, cy + height * 0.1, cx - rx * 0.42, cy + height * 0.8), fill=(255, 255, 255, 80))
        self.paste(l, 14)
        self.d.ellipse((cx - rx, cy - ry, cx + rx, cy + ry), fill=shade(body, 0.95))
        self.gradient_ellipse((cx - rx * 0.86, cy - ry * 0.8, cx + rx * 0.86, cy + ry * 0.8), shade(drink, 1.2), drink, centre=(0.6, 0.6))

    def glass(self, x0, y0, x1, y1, fill_top, liquid, foam=None):
        """A straight glass with a drink, seen a little from above."""
        ry = (x1 - x0) * 0.16
        self.shadow((x0 - 10, y1 - ry, x1 + 30, y1 + ry * 1.5), 90)
        l = self.layer()
        g = ImageDraw.Draw(l)
        g.rectangle((x0, y0, x1, y1), fill=(220, 230, 235, 70))
        g.ellipse((x0, y1 - ry, x1, y1 + ry), fill=(220, 230, 235, 90))
        if liquid:
            g.rectangle((x0 + 6, fill_top, x1 - 6, y1), fill=liquid)
            g.ellipse((x0 + 6, y1 - ry, x1 - 6, y1 + ry), fill=liquid)
            g.ellipse((x0 + 6, fill_top - ry, x1 - 6, fill_top + ry), fill=shade(liquid, 1.25))
        if foam:
            g.rectangle((x0 + 4, fill_top - ry * 1.6, x1 - 4, fill_top), fill=foam)
            g.ellipse((x0 + 4, fill_top - ry * 2.6, x1 - 4, fill_top - ry * 0.6), fill=shade(foam, 1.05))
        g.ellipse((x0, y0 - ry, x1, y0 + ry), outline=(255, 255, 255, 150), width=5)
        g.line((x0, y0, x0, y1), fill=(255, 255, 255, 120), width=5)
        g.line((x1, y0, x1, y1), fill=(255, 255, 255, 90), width=5)
        g.rectangle((x0 + (x1 - x0) * 0.16, y0 + 20, x0 + (x1 - x0) * 0.26, y1 - 20), fill=(255, 255, 255, 70))
        self.paste(l)

    def save(self, path):
        img = self.img.resize((OUT, OUT), Image.LANCZOS)
        # A dark vignette round the edge, like a photo in a menu.
        v = Image.new("L", (OUT, OUT), 0)
        ImageDraw.Draw(v).ellipse((-OUT * 0.25, -OUT * 0.25, OUT * 1.25, OUT * 1.25), fill=255)
        v = v.filter(ImageFilter.GaussianBlur(OUT * 0.12))
        dark = Image.new("RGBA", (OUT, OUT), (0, 0, 0, 255))
        img = Image.composite(img, Image.blend(img, dark, 0.55), v)
        img.convert("RGB").save(path)


# -- table tops ------------------------------------------------------------

def marble(p):
    """The café's white marble table top with grey veins."""
    p.d.rectangle((0, 0, S, S), fill=(214, 210, 202))
    l = p.layer()
    g = ImageDraw.Draw(l)
    for _ in range(9):
        x, y = p.rng.uniform(-200, S), p.rng.uniform(-100, S)
        pts = []
        for i in range(30):
            x += p.rng.uniform(10, 50)
            y += p.rng.uniform(-30, 40)
            pts.append((x, y))
        g.line(pts, fill=(150, 146, 140, 120), width=int(p.rng.uniform(2, 7)))
    p.paste(l, 3)


def tablecloth(p):
    """The station restaurant's white damask cloth."""
    p.d.rectangle((0, 0, S, S), fill=(236, 232, 222))
    l = p.layer()
    g = ImageDraw.Draw(l)
    for i in range(-S, S, 64):
        g.line((i, 0, i + S, S), fill=(210, 205, 192, 70), width=10)
        g.line((i + S, 0, i, S), fill=(210, 205, 192, 50), width=6)
    p.paste(l, 4)


def newspaper(p):
    """Street food comes on a sheet of yesterday's paper on a wooden board."""
    p.d.rectangle((0, 0, S, S), fill=(110, 76, 46))
    for y in range(0, S, 22):
        p.d.line((0, y + p.rng.uniform(-4, 4), S, y + p.rng.uniform(-4, 4)), fill=(98, 66, 40), width=4)
    l = p.layer()
    g = ImageDraw.Draw(l)
    g.polygon([(90, 150), (930, 110), (970, 900), (60, 930)], fill=(226, 218, 196, 255))
    for row in range(26):
        y = 200 + row * 27
        for col in range(3):
            x = 120 + col * 270
            g.line((x, y, x + 240 - p.rng.uniform(0, 60), y), fill=(120, 116, 104, 160), width=8)
    g.rectangle((130, 165, 860, 190), fill=(80, 76, 70, 200))
    p.paste(l, 1)


# -- dishes -------------------------------------------------------------------

def feketekave(p):
    p.glass(640, 300, 790, 640, 360, (200, 220, 230, 90))
    p.cup(400, 470, 150, 60, 150, (40, 22, 12))
    p.d.ellipse((330, 455, 470, 490), fill=(70, 40, 22))
    p.highlight((300, 440, 380, 470), 70, 6)
    # Sugar cubes and a spoon on the saucer.
    for x, y in ((180, 650), (225, 670)):
        p.d.polygon([(x, y), (x + 36, y - 14), (x + 66, y), (x + 30, y + 16)], fill=(250, 250, 246))
        p.d.polygon([(x, y), (x + 30, y + 16), (x + 30, y + 48), (x, y + 32)], fill=(222, 222, 216))
        p.d.polygon([(x + 30, y + 16), (x + 66, y), (x + 66, y + 32), (x + 30, y + 48)], fill=(200, 200, 194))
    p.d.line((520, 690, 700, 640), fill=(170, 170, 176), width=12)
    p.d.ellipse((480, 670, 545, 712), fill=(186, 186, 192))


def tejeskave(p):
    p.glass(660, 320, 800, 660, 380, (200, 220, 230, 90))
    p.cup(420, 430, 170, 66, 200, (186, 136, 92))
    # Whipped milk on top.
    p.gradient_ellipse((300, 380, 540, 470), (252, 250, 244), (226, 212, 190))
    p.d.arc((360, 400, 470, 450), 200, 520, fill=(206, 176, 140), width=8)
    p.speckle((330, 390, 510, 455), [(150, 96, 50)], 30, (2, 4))


def vajas_kifli(p):
    p.plate(500, 560, 360, 230)
    # The crescent: overlapping rolled segments along an arc.
    for i in range(12):
        t = i / 11
        a = math.radians(200 + 140 * t)
        r = 230
        x = 500 + r * math.cos(a)
        y = 650 + r * 0.75 * math.sin(a)
        w = 70 + 70 * math.sin(math.pi * t)
        h = 60 + 50 * math.sin(math.pi * t)
        p.gradient_ellipse((x - w, y - h, x + w, y + h), (232, 172, 88), (150, 86, 30), centre=(0.4, 0.3))
    p.speckle((300, 420, 720, 600), [(250, 236, 210)], 60, (2, 4))
    # A curl of butter.
    p.gradient_ellipse((640, 600, 760, 660), (255, 240, 170), (236, 206, 110))


def dobos_torta(p):
    p.plate(500, 600, 340, 210, band=(170, 140, 70))
    # Front face: six sponge layers with chocolate cream between.
    x0, x1, top, bottom = 290, 640, 380, 640
    h = (bottom - top) / 11
    for i in range(11):
        c = (236, 200, 120) if i % 2 == 0 else (92, 52, 30)
        p.d.rectangle((x0, top + i * h, x1, top + (i + 1) * h + 1), fill=c)
    # Side face going back to the right.
    side = [(x1, top), (760, top - 90), (760, bottom - 90), (x1, bottom)]
    p.gradient_polygon(side, (110, 66, 36), (70, 40, 22))
    # Glossy caramel top.
    p.d.polygon([(x0, top), (x1, top), (760, top - 90), (420, top - 90)], fill=(196, 120, 36))
    p.highlight((470, 300, 640, 340), 120, 8)
    p.d.line((x0, top, x1, top), fill=(120, 66, 20), width=6)


def almas_retes(p):
    p.plate(500, 580, 360, 220)
    for cx, cy in ((380, 520), (600, 560)):
        # The roll seen end on: a spiral of thin pastry round apple filling.
        p.gradient_ellipse((cx - 140, cy - 90, cx + 140, cy + 90), (238, 196, 120), (196, 140, 70))
        p.d.ellipse((cx - 110, cy - 68, cx + 110, cy + 68), fill=(222, 196, 120))
        pts = []
        for i in range(200):
            a = i / 200 * 6 * math.pi
            r = 6 + i / 200 * 100
            pts.append((cx + r * math.cos(a), cy + r * 0.62 * math.sin(a)))
        p.d.line(pts, fill=(244, 222, 170), width=5)
        p.speckle((cx - 100, cy - 60, cx + 100, cy + 60), [(200, 150, 70), (170, 120, 50)], 40, (3, 6),
                  inside=lambda x, y, cx=cx, cy=cy: ((x - cx) / 100) ** 2 + ((y - cy) / 60) ** 2 < 1)
    p.speckle((200, 380, 800, 700), [(252, 252, 250), (240, 240, 236)], 500, (2, 5))


def pogacsa(p):
    p.plate(500, 580, 360, 220)
    for cx, cy in ((380, 520), (560, 500), (470, 640), (650, 620)):
        p.d.ellipse((cx - 100, cy - 30, cx + 100, cy + 70), fill=(176, 120, 52))
        p.gradient_ellipse((cx - 100, cy - 70, cx + 100, cy + 50), (236, 180, 92), (196, 132, 54))
        for k in range(-3, 4):
            p.d.line((cx + k * 22 - 40, cy - 40, cx + k * 22 + 40, cy + 30), fill=(170, 108, 40), width=4)
            p.d.line((cx + k * 22 + 40, cy - 40, cx + k * 22 - 40, cy + 30), fill=(170, 108, 40), width=4)


def gulyasleves(p):
    p.bowl(470, 480, 300, 130, 170, (176, 58, 20))
    rng = p.rng
    for _ in range(26):
        x = rng.uniform(250, 690)
        y = rng.uniform(400, 560)
        if ((x - 470) / 250) ** 2 + ((y - 480) / 105) ** 2 > 1:
            continue
        kind = rng.choice(("potato", "carrot", "meat"))
        if kind == "potato":
            p.d.rectangle((x - 22, y - 14, x + 22, y + 14), fill=(232, 196, 120))
        elif kind == "carrot":
            p.d.ellipse((x - 18, y - 10, x + 18, y + 10), fill=(236, 120, 30))
        else:
            p.gradient_ellipse((x - 30, y - 18, x + 30, y + 18), (140, 64, 40), (90, 36, 22))
    p.speckle((230, 380, 710, 590), [(220, 90, 30), (250, 140, 60)], 120, (2, 4))
    # A slice of bread beside it.
    p.d.polygon([(700, 690), (900, 640), (940, 790), (740, 850)], fill=(168, 112, 56))
    p.d.polygon([(715, 700), (890, 655), (922, 780), (750, 830)], fill=(238, 222, 186))


def rantott_szelet(p):
    p.plate(500, 560, 400, 250)
    # The breaded cutlet: a wavy golden shape with crumbs.
    pts = []
    for i in range(40):
        a = 2 * math.pi * i / 40
        r = 1 + 0.08 * math.sin(5 * a) + 0.05 * math.sin(11 * a)
        pts.append((430 + 230 * r * math.cos(a), 520 + 140 * r * math.sin(a)))
    p.gradient_polygon(pts, (226, 160, 60), (170, 100, 30))
    p.speckle((220, 400, 660, 650), [(240, 186, 90), (150, 86, 26), (200, 130, 50)], 400, (2, 6),
              inside=lambda x, y: ((x - 430) / 220) ** 2 + ((y - 520) / 130) ** 2 < 1)
    # A wedge of lemon and parsley potatoes.
    p.d.pieslice((600, 380, 760, 520), 200, 340, fill=(250, 226, 80))
    p.d.pieslice((620, 400, 740, 500), 200, 340, fill=(255, 244, 170))
    for x, y in ((700, 620), (780, 580), (760, 680)):
        p.gradient_ellipse((x - 50, y - 36, x + 50, y + 36), (248, 216, 120), (210, 170, 70))
        p.speckle((x - 40, y - 30, x + 40, y + 30), [(60, 130, 40)], 12, (3, 5))


def paprikas_csirke(p):
    p.plate(500, 560, 400, 250)
    p.gradient_ellipse((220, 420, 600, 680), (226, 100, 36), (180, 60, 20))
    for x, y in ((380, 520), (470, 590)):
        p.gradient_ellipse((x - 90, y - 60, x + 90, y + 60), (236, 140, 70), (170, 76, 30))
    p.d.arc((300, 470, 520, 640), 190, 330, fill=(250, 246, 236), width=14)
    # A heap of nokedli dumplings.
    for _ in range(70):
        x = p.rng.uniform(560, 800)
        y = p.rng.uniform(420, 640)
        if ((x - 680) / 120) ** 2 + ((y - 530) / 110) ** 2 > 1:
            continue
        p.gradient_ellipse((x - 22, y - 12, x + 22, y + 12), (252, 240, 196), (220, 200, 140))


def halaszle(p):
    p.bowl(470, 480, 300, 130, 170, (214, 40, 20))
    for x, y in ((380, 470), (540, 500)):
        p.gradient_ellipse((x - 70, y - 40, x + 70, y + 40), (252, 236, 220), (220, 196, 176))
        p.d.arc((x - 70, y - 40, x + 70, y + 40), 180, 360, fill=(110, 110, 120), width=10)
    # A ring of hot pepper floating on top.
    p.d.ellipse((440, 410, 500, 440), outline=(40, 140, 40), width=10)
    p.speckle((230, 380, 710, 590), [(240, 80, 30), (250, 120, 50)], 120, (2, 4))


def turos_csusza(p):
    p.plate(500, 560, 380, 240)
    l = p.layer()
    g = ImageDraw.Draw(l)
    for _ in range(90):
        x = p.rng.uniform(260, 740)
        y = p.rng.uniform(430, 690)
        if ((x - 500) / 230) ** 2 + ((y - 560) / 130) ** 2 > 1:
            continue
        a = p.rng.uniform(0, math.pi)
        g.line((x - 40 * math.cos(a), y - 20 * math.sin(a), x + 40 * math.cos(a), y + 20 * math.sin(a)), fill=(240, 222, 160, 255), width=18)
    p.paste(l)
    p.speckle((260, 430, 740, 690), [(252, 252, 246), (238, 236, 226)], 260, (5, 10),
              inside=lambda x, y: ((x - 500) / 210) ** 2 + ((y - 560) / 115) ** 2 < 1)
    p.speckle((300, 450, 700, 670), [(160, 60, 40), (190, 90, 60)], 50, (6, 10),
              inside=lambda x, y: ((x - 500) / 200) ** 2 + ((y - 560) / 105) ** 2 < 1)


def palacsinta(p):
    p.plate(500, 580, 380, 230)
    for y in (500, 620):
        p.gradient_polygon([(250, y - 50), (740, y - 70), (760, y + 40), (260, y + 60)], (238, 196, 120), (200, 150, 80))
        p.gradient_ellipse((720, y - 72, 790, y + 42), (220, 120, 40), (180, 80, 20))
        p.speckle((280, y - 50, 720, y + 50), [(196, 140, 70)], 30, (4, 8))
    p.speckle((230, 420, 790, 700), [(252, 252, 250)], 400, (2, 4))


def sor(p):
    p.d.ellipse((330, 700, 690, 820), fill=(150, 40, 30))
    p.d.ellipse((345, 710, 675, 805), fill=(190, 60, 40))
    p.glass(380, 230, 640, 760, 340, (220, 150, 40, 220), foam=(250, 246, 236, 255))
    p.d.arc((600, 360, 730, 620), 270, 90, fill=(220, 230, 235), width=22)
    p.speckle((400, 380, 620, 740), [(255, 230, 150)], 40, (2, 3))


def virsli(p):
    # A roll and two hot sausages with a dollop of mustard.
    p.shadow((240, 520, 820, 760))
    p.gradient_ellipse((560, 420, 840, 620), (220, 150, 70), (170, 100, 40))
    for k, y in enumerate((520, 600)):
        pts = [(250 + i * 25, y + 20 * math.sin(i / 20 * math.pi)) for i in range(21)]
        for x, yy in pts:
            p.gradient_ellipse((x - 34, yy - 30, x + 34, yy + 30), (236, 160, 140), (190, 100, 90))
    p.gradient_ellipse((420, 470, 520, 520), (240, 200, 40), (200, 150, 20))


def debreceni(p):
    p.shadow((220, 560, 840, 760))
    pts = [(260 + i * 24, 560 + 70 * math.sin(i / 22 * math.pi)) for i in range(23)]
    for x, y in pts:
        p.gradient_ellipse((x - 40, y - 34, x + 40, y + 34), (196, 76, 44), (130, 40, 24))
    p.speckle((240, 500, 800, 660), [(240, 220, 200)], 60, (3, 5))
    # Bread and a fresh pepper.
    p.d.polygon([(560, 640), (780, 600), (810, 760), (590, 800)], fill=(168, 112, 56))
    p.d.polygon([(575, 650), (770, 615), (795, 750), (600, 785)], fill=(238, 222, 186))
    p.gradient_ellipse((230, 640, 420, 720), (120, 200, 70), (60, 140, 40))


def perec(p):
    p.shadow((240, 330, 800, 790))
    l = p.layer()
    g = ImageDraw.Draw(l)
    knot = []
    for i in range(240):
        t = i / 240 * 2 * math.pi
        knot.append((520 + 250 * math.sin(t), 520 + 170 * math.sin(t) * math.cos(t) * 2.0 - 60 * math.cos(t)))
    g.line(knot + knot[:2], fill=(110, 60, 20, 255), width=96, joint="curve")
    g.line(knot + knot[:2], fill=(170, 100, 40, 255), width=78, joint="curve")
    g.line([(x - 8, y - 12) for x, y in knot], fill=(206, 140, 70, 255), width=26, joint="curve")
    p.paste(l)
    p.speckle((280, 330, 780, 720), [(255, 255, 255), (236, 236, 240)], 140, (4, 7),
              inside=lambda x, y: l.getpixel((int(x), int(y)))[3] > 0)


def gesztenye(p):
    # Hot roasted chestnuts in a cone of newspaper.
    cone = [(300, 380), (720, 380), (520, 880)]
    p.shadow((330, 760, 700, 900))
    for _ in range(14):
        x = p.rng.uniform(340, 680)
        y = p.rng.uniform(300, 420)
        p.gradient_ellipse((x - 60, y - 46, x + 60, y + 46), (120, 64, 34), (60, 28, 14))
        p.d.arc((x - 30, y - 30, x + 30, y + 20), 200, 340, fill=(236, 200, 120), width=12)
    p.gradient_polygon(cone, (236, 230, 210), (196, 190, 170))
    for i in range(16):
        y = 410 + i * 26
        w = (880 - y) / 500 * 180
        p.d.line((520 - w, y, 520 + w - 20, y), fill=(110, 106, 96), width=6)


def sult_tok(p):
    # Two thick slices of oven-baked pumpkin, caramelised at the edges.
    for dy in (0, 150):
        p.shadow((220, 520 + dy, 820, 700 + dy))
        top = [(230, 500 + dy), (790, 400 + dy), (810, 470 + dy), (260, 600 + dy)]
        p.gradient_polygon(top, (250, 160, 50), (226, 120, 30))
        front = [(260, 600 + dy), (810, 470 + dy), (812, 520 + dy), (262, 650 + dy)]
        p.gradient_polygon(front, (200, 96, 20), (150, 66, 10))
        p.d.line([(262, 650 + dy), (812, 520 + dy)], fill=(110, 130, 50), width=16)
        p.d.line([(230, 500 + dy), (790, 400 + dy)], fill=(150, 66, 10), width=14)
        p.speckle((260, 420 + dy, 780, 590 + dy), [(170, 80, 10), (255, 190, 90)], 70, (4, 8),
                  inside=lambda x, y, dy=dy: 500 + dy - (x - 230) * 0.18 < y < 600 + dy - (x - 260) * 0.24)


DISHES = {
    "feketekave": (marble, feketekave),
    "tejeskave": (marble, tejeskave),
    "vajas_kifli": (marble, vajas_kifli),
    "dobos_torta": (marble, dobos_torta),
    "almas_retes": (marble, almas_retes),
    "pogacsa": (marble, pogacsa),
    "gulyasleves": (tablecloth, gulyasleves),
    "rantott_szelet": (tablecloth, rantott_szelet),
    "paprikas_csirke": (tablecloth, paprikas_csirke),
    "halaszle": (tablecloth, halaszle),
    "turos_csusza": (tablecloth, turos_csusza),
    "palacsinta": (tablecloth, palacsinta),
    "sor": (tablecloth, sor),
    "virsli": (newspaper, virsli),
    "debreceni": (newspaper, debreceni),
    "perec": (newspaper, perec),
    "gesztenye": (newspaper, gesztenye),
    "sult_tok": (newspaper, sult_tok),
}


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "."
    os.makedirs(out, exist_ok=True)
    for name, (background, dish) in DISHES.items():
        p = Painter(background)
        dish(p)
        p.save(os.path.join(out, name + ".png"))
    print(f"{len(DISHES)} dishes painted into {out}")


if __name__ == "__main__":
    main()
