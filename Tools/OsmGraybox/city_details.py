"""Place the city's smaller details from the graybox and street furniture data.

Reads the graybox JSON (osm_to_graybox.py) and the street furniture JSON
(street_furniture.py) and writes a third JSON that ACityDetails and
AOsmGraybox read:

- courtyards: a few tenement gates on Népszínház utca and Rákóczi tér that
  stand open, with a gateway cut through the house into its courtyard,
  {"gate": [x, y, yaw] (the gate's foot on the wall, facing the street),
  "width", "depth" (how deep the gateway runs), "number", "seed",
  "yard": [x0, y0, x1, y1, ...] (the courtyard's outline), "walls":
  [[x0, y0, x1, y1, height, yaw], ...] (the courtyard's walls, for the open
  balconies, yaw facing into the yard), "props": [[kind, x, y, yaw(, length)], ...]
  (pump, rack, line, bench, bins, tree)};
- buildings: map blocks that the game builds differently from the graybox
  data, each in the graybox's own building form ({"id", "rings", "tris",
  "front", "height", "levels", "base"}), under "replaces" (the original
  block's id): the houses the gateways are cut through, in pieces with a
  bridge over the gateway, and the Corvin department store, taller;
- wall_ads: painted advertisements on the bare fire walls that rise above a
  lower neighbour, {"at": [x, y, yaw] (the middle of the painted field at
  the wall foot, facing out), "width", "z0", "z1" (its bottom and top above
  the ground), "ground": [x, y] (a street spot to take the ground height
  at), "kind"};
- kiosks: newspaper and tobacco kiosks near tram stops, [x, y, yaw, variant]
  with the counter facing the pavement walking line;
- clocks: street clocks on a cast iron post on the squares, [x, y, yaw];
- map_icons: the little pictures on the minimap and the big map, [kind, x, y]
  (cafe, market, tram, phone, yard, theatre, station, store, church).

Everything is in Unreal centimetres (X east, Y south), yaw in degrees.

Usage:
    python city_details.py ../../Data/OsmGraybox/blaha_test_area.json \
        ../../Data/OsmGraybox/street_furniture.json ../../Data/OsmGraybox/city_details.json
"""

import argparse
import json
import math

import mapbox_earcut as earcut
import numpy as np
from shapely.geometry import LineString, Point, Polygon
from shapely.ops import polylabel, unary_union

from street_furniture import Space, Spread, chance, ring_points, unit, yaw_of

# The gates that stand open (their house numbers and where they are, as in
# street_furniture.json's "gates"): Népszínház utca 9, 19, 39, 2 and 20,
# and Rákóczi tér 3.
OPEN_GATES = [
    (23598, 9701),
    (40338, 15420),
    (79559, 28548),
    (22808, 11893),
    (81976, 31536),
    (15855, 46218),
]
GATE_FIND = 200
# The gate's stone surround (ShopModel::GateSurround); the gateway is the
# opening between the surrounds.
GATE_SURROUND = 34
# The gateway's vault springs at this height (ShopModel::BuildGate's Spring);
# the house above it is a bridge from here up.
GATEWAY_TOP = 340
# The gateway starts this far out of the wall (so no sliver of wall is left)
# and runs this far past the house's back wall into the courtyard.
GATEWAY_OUT = 60
GATEWAY_PAST = 40

CORVIN_ID = "w261801166"
# The department store of 1926: five storeys and the attic over the square.
CORVIN_HEIGHT = 2350
CORVIN_LEVELS = 5

# Painted wall adverts: walls at least this long and this much taller than
# the neighbour in front of them, near enough to a street to be seen.
AD_MIN_WIDTH = 900
AD_MAX_WIDTH = 1600
AD_MIN_RISE = 700
AD_ROAD_NEAR = 2500
AD_GAP = 7000
AD_COUNT = 14
# Looked at from a street spot this far out from the wall (cm), at eye height.
AD_SEEN_NEAR = 1200
AD_SEEN_FAR = 6000
EYE = 170
AD_KINDS = 8

# Kiosks: one by each of these tram stops, a few metres along the pavement.
KIOSK_STOPS = ["József körút", "Rákóczi út", "Baross tér", "Teleki tér", "Népszínház utca",
               "Keleti pályaudvar", "Erzsébet körút", "Fiumei út", "Kiss József utca"]
KIOSK_RADIUS = 140
KIOSK_ROOM = 260        # from lamps, columns, benches, signs and the like
KIOSK_SLIDE = [500, -500, 800, -800, 1100, -1100]
KIOSK_VARIANTS = 3

# Street clocks: on these squares, near the given spot.
CLOCK_NEAR = [
    ("Blaha Lujza tér", (-9200, 1800)),
    ("Baross tér", (93000, -36000)),
    ("Rákóczi tér", (12000, 40500)),
    ("Teleki tér", (96000, 26000)),
    ("Tisza Kálmán tér", (60500, -5500)),
]
CLOCK_RADIUS = 60
CLOCK_ROOM = 200


def polygon_of(building):
    rings = [ring_points(r) for r in building["rings"]]
    shape = Polygon(rings[0], rings[1:])
    return shape if shape.is_valid else shape.buffer(0)


def signed_area(points):
    return sum(x0 * y1 - x1 * y0 for (x0, y0), (x1, y1) in zip(points, points[1:] + points[:1])) / 2.0


def building_form(shape, like, front_lines, height, levels, base=0):
    """A graybox building from a shapely polygon: rings turned the same way
    as the block `like`, triangles from earcut wound like its triangles, and
    the edges along front_lines as its front."""
    outer_sign = math.copysign(1.0, signed_area(ring_points(like["rings"][0])))
    rings = []
    for k, ring in enumerate([shape.exterior] + list(shape.interiors)):
        pts = [(round(x), round(y)) for x, y in list(ring.coords)[:-1]]
        # Drop repeated points left by rounding.
        clean = [p for i, p in enumerate(pts) if p != pts[i - 1]]
        want = outer_sign if k == 0 else -outer_sign
        if math.copysign(1.0, signed_area(clean)) != want:
            clean.reverse()
        rings.append(clean)
    flat = np.array([p for r in rings for p in r], dtype=np.float64).reshape(-1, 2)
    ends = np.cumsum([len(r) for r in rings]).astype(np.uint32)
    tris = [int(i) for i in earcut.triangulate_float64(flat, ends)]
    # Wind the triangles as the original's.
    lp = ring_points(like["rings"][0]) + [p for r in like["rings"][1:] for p in ring_points(r)]
    a, b, c = (lp[i] for i in like["tris"][:3])
    want = math.copysign(1.0, (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]))
    for t in range(0, len(tris), 3):
        a, b, c = (flat[i] for i in tris[t:t + 3])
        if math.copysign(1.0, (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0])) != want:
            tris[t + 1], tris[t + 2] = tris[t + 2], tris[t + 1]
    front = []
    edge = 0
    for r in rings:
        for i in range(len(r)):
            seg = LineString([r[i], r[(i + 1) % len(r)]])
            if seg.length > 50 and any(line.distance(Point(seg.coords[0])) < 25 and line.distance(Point(seg.coords[1])) < 25 for line in front_lines):
                front.append(edge)
            edge += 1
    out = {"rings": [[v for p in r for v in p] for r in rings], "tris": tris, "front": front, "height": height}
    if levels:
        out["levels"] = levels
    if base:
        out["base"] = base
    return out


def front_lines_of(building):
    rings = [ring_points(r) for r in building["rings"]]
    flat = [(r, i) for r in rings for i in range(len(r))]
    lines = []
    for e in building.get("front", []):
        r, i = flat[e]
        lines.append(LineString([r[i], r[(i + 1) % len(r)]]))
    return lines


def place_courtyards(data, gates):
    by_id = {b["id"]: b for b in data["buildings"]}
    shapes = [(b, polygon_of(b)) for b in data["buildings"]]
    joined = unary_union([s.buffer(30, join_style=2) for _, s in shapes])
    holes = [Polygon(r) for g in getattr(joined, "geoms", [joined]) for r in g.interiors]
    courtyards, replaced = [], {}
    for near in OPEN_GATES:
        gate = min(gates, key=lambda g: math.hypot(g["at"][0] - near[0], g["at"][1] - near[1]))
        x, y, yaw = gate["at"]
        if math.hypot(x - near[0], y - near[1]) > GATE_FIND:
            raise SystemExit(f"no gate near {near}")
        ox, oy = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
        ix, iy = -ox, -oy
        sx, sy = -iy, ix
        inside = Point(x + ix * 30, y + iy * 30)
        building, shape = next((b, s) for b, s in shapes if s.buffer(5).contains(inside))
        ray = LineString([(x, y), (x + ix * 4000, y + iy * 4000)])
        cut = ray.intersection(shape.boundary)
        depth = min(math.hypot(q.x - x, q.y - y) for q in getattr(cut, "geoms", [cut]) if math.hypot(q.x - x, q.y - y) > 100)
        yard_at = Point(x + ix * (depth + 150), y + iy * (depth + 150))
        hole = next(h for h in holes if h.contains(yard_at))
        yard = hole.buffer(30, join_style=2).simplify(10)
        half = gate["width"] / 2 - GATE_SURROUND
        a, b = -GATEWAY_OUT, depth + GATEWAY_PAST
        way = Polygon([(x + ix * a + sx * half, y + iy * a + sy * half), (x + ix * b + sx * half, y + iy * b + sy * half),
                       (x + ix * b - sx * half, y + iy * b - sy * half), (x + ix * a - sx * half, y + iy * a - sy * half)])
        for other, s in shapes:
            if other is not building and s.intersection(way).area > 100:
                raise SystemExit(f"gateway at {near} runs into {other['id']}")
        fronts = front_lines_of(building)
        levels = building.get("levels", 0)
        pieces = []
        rest = shape.difference(way)
        for part in getattr(rest, "geoms", [rest]):
            if part.area > 1e4:
                pieces.append(building_form(part, building, fronts, building["height"], levels))
        bridge = shape.intersection(way)
        for part in getattr(bridge, "geoms", [bridge]):
            if part.geom_type == "Polygon" and part.area > 1e3:
                pieces.append(building_form(part, building, [], building["height"], levels, GATEWAY_TOP))
        replaced[building["id"]] = pieces

        # The courtyard's walls, each with the height of the house it belongs to.
        walls = []
        ring = list(yard.exterior.coords)[:-1]
        for i in range(len(ring)):
            p, q = ring[i], ring[(i + 1) % len(ring)]
            if math.hypot(q[0] - p[0], q[1] - p[1]) < 200:
                continue
            mid = Point((p[0] + q[0]) / 2, (p[1] + q[1]) / 2)
            owner = min(shapes, key=lambda bs: bs[1].distance(mid))
            if owner[1].distance(mid) > 80:
                continue
            dx, dy = unit(q[0] - p[0], q[1] - p[1])
            nx, ny = -dy, dx
            if not yard.contains(Point(mid.x + nx * 60, mid.y + ny * 60)):
                nx, ny = -nx, -ny
            walls.append([round(p[0]), round(p[1]), round(q[0]), round(q[1]), owner[0]["height"], yaw_of(nx, ny)])
        exit_at = (x + ix * depth, y + iy * depth)
        courtyards.append({
            "gate": [x, y, yaw], "width": gate["width"], "depth": round(depth), "number": gate["number"],
            "seed": zlib_seed(gate["at"]),
            "yard": [round(v) for p in ring for v in p], "walls": walls,
            "props": yard_props(yard, walls, exit_at, (ix, iy), (sx, sy), gate["at"]),
        })
    return courtyards, replaced


def yard_props(yard, walls, exit_at, inward, side, key):
    """What stands in a courtyard: the water pump in the middle, a carpet
    beating rack and a washing line along the longest walls, dustbins by the
    gateway, a bench, and a tree in a big one. Each [kind, x, y, yaw] (a
    washing line also its length)."""
    props = []
    inner = yard.buffer(-120)
    if inner.is_empty:
        return props
    taken = [exit_at]

    def clear(x, y, gap):
        return inner.contains(Point(x, y)) and all(math.hypot(x - a, y - b) >= gap for a, b in taken)

    centre = polylabel(inner if inner.geom_type == "Polygon" else max(inner.geoms, key=lambda g: g.area), 10)
    if clear(centre.x, centre.y, 300):
        props.append(["pump", round(centre.x), round(centre.y), round(chance("pump", *key) * 360, 1)])
        taken.append((centre.x, centre.y))
    # Dustbins a step to the side of the gateway's end.
    ex, ey = exit_at
    for k, s in enumerate((1, -1)):
        bx = ex + inward[0] * 120 + side[0] * s * 220
        by = ey + inward[1] * 120 + side[1] * s * 220
        if yard.buffer(-40).contains(Point(bx, by)):
            props.append(["bins", round(bx), round(by), yaw_of(-inward[0], -inward[1])])
            taken.append((bx, by))
            break
    long_walls = sorted(walls, key=lambda w: -math.hypot(w[2] - w[0], w[3] - w[1]))
    for kind, depth_in, need in (("rack", 260, 320), ("line", 330, 700), ("bench", 70, 260)):
        for w in long_walls:
            length = math.hypot(w[2] - w[0], w[3] - w[1])
            if length < need:
                continue
            dx, dy = unit(w[2] - w[0], w[3] - w[1])
            nx, ny = math.cos(math.radians(w[5])), math.sin(math.radians(w[5]))
            placed = False
            for t in (0.5, 0.3, 0.7):
                mx = w[0] + dx * length * t + nx * depth_in
                my = w[1] + dy * length * t + ny * depth_in
                if kind == "line":
                    run = min(length - 300, 900)
                    ends = [(mx + dx * run / 2, my + dy * run / 2), (mx - dx * run / 2, my - dy * run / 2)]
                    if all(yard.buffer(-60).contains(Point(*e)) for e in ends) and clear(mx, my, 200):
                        props.append(["line", round(mx), round(my), yaw_of(dx, dy), round(run)])
                        placed = True
                elif clear(mx, my, 260):
                    props.append([kind, round(mx), round(my), w[5] if kind == "bench" else yaw_of(dx, dy)])
                    placed = True
                if placed:
                    taken.append((mx, my))
                    break
            if placed:
                break
    if yard.area > 2.5e6:
        far = yard.buffer(-300)
        if not far.is_empty:
            for g in getattr(far, "geoms", [far]):
                p = g.representative_point()
                if clear(p.x, p.y, 400):
                    props.append(["tree", round(p.x), round(p.y), 0])
                    break
    return props


def zlib_seed(at):
    return int(chance("yard", at[0], at[1]) * 10000)


def corvin(data):
    building = next(b for b in data["buildings"] if b["id"] == CORVIN_ID)
    form = {k: building[k] for k in ("rings", "tris")}
    form["front"] = building.get("front", [])
    form["height"] = CORVIN_HEIGHT
    form["levels"] = CORVIN_LEVELS
    return form


def seen_from(space, heights, at, out, z):
    """A street spot in front of the wall from which a walker sees height z
    on it over the roofs in between, or None."""
    for dist in range(AD_SEEN_NEAR, AD_SEEN_FAR, 250):
        for turn in (0, -15, 15, -30, 30, -45, 45, -60, 60):
            t = math.atan2(out[1], out[0]) + math.radians(turn)
            cx, cy = at[0] + math.cos(t) * dist, at[1] + math.sin(t) * dist
            eye = Point(cx, cy)
            if not space.roads.contains(eye) or space.near_building(eye, 100):
                continue
            sight = LineString([(cx, cy), (at[0] + out[0] * 30, at[1] + out[1] * 30)])
            length = sight.length
            clear = True
            for i in space.building_tree.query(sight):
                block = space.buildings[i]
                cut = block.intersection(sight)
                if cut.is_empty:
                    continue
                first = min(eye.distance(Point(q)) for g in getattr(cut, "geoms", [cut]) for q in g.coords)
                height = heights[i]
                if height > EYE + (z - EYE) * first / length - 50:
                    clear = False
                    break
            if clear:
                return cx, cy
    return None


def place_wall_ads(data, space):
    shapes = [(b, polygon_of(b)) for b in data["buildings"]]
    # Heights in the order of Space's buildings; the landmarks stand tall.
    heights = [b["height"] for b in data["buildings"]] + [3000] * len(data.get("landmarks", []))
    found = []
    for building, shape in shapes:
        rings = [ring_points(r) for r in building["rings"]][:1]
        sign = math.copysign(1.0, signed_area(rings[0]))
        for r in rings:
            for i in range(len(r)):
                p, q = r[i], r[(i + 1) % len(r)]
                length = math.hypot(q[0] - p[0], q[1] - p[1])
                if length < AD_MIN_WIDTH:
                    continue
                dx, dy = unit(q[0] - p[0], q[1] - p[1])
                # Outward: try both sides, keep the one outside the block.
                nx, ny = dy, -dx
                mid = ((p[0] + q[0]) / 2, (p[1] + q[1]) / 2)
                if shape.contains(Point(mid[0] + nx * 50, mid[1] + ny * 50)):
                    nx, ny = -nx, -ny
                probe = Point(mid[0] + nx * 120, mid[1] + ny * 120)
                lower = [b for b, s in shapes if b is not building and s.contains(probe)]
                if not lower:
                    continue
                rise = building["height"] - lower[0]["height"]
                if rise < AD_MIN_RISE:
                    continue
                # Seen from a street: open ground not far out from the wall,
                # above the lower roof.
                if space.road_shape.distance(probe) > AD_ROAD_NEAR:
                    continue
                # The whole field must stand above the neighbour.
                width = min(length - 200, AD_MAX_WIDTH)
                ok = True
                for t in (-0.5, 0.0, 0.5):
                    s = Point(mid[0] + dx * width * t + nx * 120, mid[1] + dy * width * t + ny * 120)
                    under = [b for b, sh in shapes if b is not building and sh.contains(s)]
                    if not under or building["height"] - under[0]["height"] < AD_MIN_RISE:
                        ok = False
                if not ok:
                    continue
                z0 = lower[0]["height"] + 120
                z1 = min(building["height"] - 120, z0 + width * 0.75)
                # Seen whole from a street at eye height; the ground height is
                # taken there too, as the wall foot is under the neighbour's roof.
                seen = seen_from(space, heights, mid, (nx, ny), (z0 + z1) / 2)
                if seen is None:
                    continue
                found.append((width * (z1 - z0), {
                    "at": [round(mid[0]), round(mid[1]), yaw_of(nx, ny)],
                    "ground": [round(seen[0]), round(seen[1])],
                    "width": round(width), "z0": round(z0), "z1": round(z1),
                }))
    found.sort(key=lambda f: -f[0])
    spread = Spread(AD_GAP)
    ads = []
    for _, ad in found:
        x, y = ad["at"][:2]
        if spread.ok(x, y):
            spread.add(x, y)
            ad["kind"] = len(ads) % AD_KINDS
            ads.append(ad)
            if len(ads) >= AD_COUNT:
                break
    return ads


def furniture_spots(f):
    spots = [(l[0], l[1]) for l in f["lamps"] + f["columns"] + f["bins"] + f["benches"]]
    spots += [tuple(t["sign"][:2]) for t in f["tram_stops"]] + [tuple(v["at"][:2]) for v in f["food_vendors"]]
    spots += [tuple(p["at"][:2]) for p in f["police_boxes"]]
    spots += [tuple(t["sign"][:2]) for t in f["taxi_stands"]] + [tuple(p[:2]) for t in f["taxi_stands"] for p in t["taxis"]]
    spots += [tuple(v["at"][:2]) for v in f["shops"] + f["gates"]]
    return spots


def room(spots, x, y, gap):
    return all(math.hypot(x - a, y - b) >= gap for a, b in spots)


def place_kiosks(space, f, spots):
    walks = [LineString(ring_points(w)) for w in f["sidewalks"]]
    kiosks = []
    for name in KIOSK_STOPS:
        stop = next((t for t in f["tram_stops"] if t["name"] == name and not t.get("bus")), None)
        if stop is None:
            continue
        wx, wy = stop["wait"]
        sx, sy, syaw = stop["sign"]
        # Along the kerb: across the sign's facing.
        ax, ay = -math.sin(math.radians(syaw)), math.cos(math.radians(syaw))
        walk = min(walks, key=lambda w: w.distance(Point(wx, wy)))
        for slide in KIOSK_SLIDE:
            x, y = wx + ax * slide, wy + ay * slide
            # Back from the walking line, away from the road.
            on = walk.interpolate(walk.project(Point(x, y)))
            bx, by = unit(on.x - sx, on.y - sy)
            kx, ky = on.x + bx * 230, on.y + by * 230
            if space.free(kx, ky, KIOSK_RADIUS) and room(spots, kx, ky, KIOSK_ROOM):
                kiosks.append([round(kx), round(ky), yaw_of(-bx, -by), len(kiosks) % KIOSK_VARIANTS])
                spots.append((kx, ky))
                break
    return kiosks


def place_clocks(space, spots):
    clocks = []
    for name, (cx, cy) in CLOCK_NEAR:
        best = None
        for r in range(0, 3000, 100):
            for k in range(16 if r else 1):
                a = 2 * math.pi * k / 16
                x, y = cx + r * math.cos(a), cy + r * math.sin(a)
                if space.free(x, y, CLOCK_RADIUS) and room(spots, x, y, CLOCK_ROOM):
                    best = (x, y)
                    break
            if best:
                break
        if best:
            # Faces the nearest road.
            p = Point(*best)
            q = space.road_shape.boundary.interpolate(space.road_shape.boundary.project(p))
            clocks.append([round(best[0]), round(best[1]), yaw_of(q.x - best[0], q.y - best[1])])
            spots.append(best)
    return clocks


# The cafe by Bozsi neni's house (SoundSpots::CafeAt), the Nepszinhaz
# (PestiStart::TheatreAt), Keleti's front (AKeletiStation::FrontAt) and the
# market hall's front (AMarketHall::FrontAt), all built by the game itself.
CAFE_AT = (58392, 21268)
THEATRE_AT = (-3600, 200)
KELETI_AT = (92766, -43178)
HALL_AT = (18109, 40941)
CORVIN_AT = (-9440, 3697)
# Tram stops with the same name nearer than this share one mark on the map.
STOP_MARK_SAME = 6000


def place_map_icons(data, f, courtyards):
    """The little pictures on the maps: [kind, x, y] with kind one of cafe,
    market, tram, phone, yard, theatre, station, store, church."""
    icons = [["cafe", *CAFE_AT], ["theatre", *THEATRE_AT], ["station", *KELETI_AT],
             ["market", *HALL_AT], ["store", *CORVIN_AT]]
    stalls = f["market"]["stalls"]
    if stalls:
        icons.append(["market", round(sum(s["at"][0] for s in stalls) / len(stalls)), round(sum(s["at"][1] for s in stalls) / len(stalls))])
    marked = []
    for stop in f["tram_stops"]:
        x, y = stop["wait"]
        if any(n == stop["name"] and math.hypot(x - a, y - b) < STOP_MARK_SAME for n, a, b in marked):
            continue
        marked.append((stop["name"], x, y))
        icons.append(["tram", x, y])
    for box in f["police_boxes"]:
        icons.append(["phone", box["at"][0], box["at"][1]])
    for yard in courtyards:
        icons.append(["yard", yard["gate"][0], yard["gate"][1]])
    for b in data["buildings"]:
        name = b.get("name", "")
        if ("templom" in name or "kápolna" in name) and not name.startswith("Egykori"):
            ring = ring_points(b["rings"][0])
            icons.append(["church", round(sum(p[0] for p in ring) / len(ring)), round(sum(p[1] for p in ring) / len(ring))])
    return icons


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("graybox")
    parser.add_argument("furniture")
    parser.add_argument("output")
    args = parser.parse_args()
    with open(args.graybox, encoding="utf-8") as fh:
        data = json.load(fh)
    with open(args.furniture, encoding="utf-8") as fh:
        f = json.load(fh)
    space = Space(data)
    courtyards, replaced = place_courtyards(data, f["gates"])
    replaced[CORVIN_ID] = [corvin(data)]
    spots = furniture_spots(f)
    ads = place_wall_ads(data, space)
    kiosks = place_kiosks(space, f, spots)
    clocks = place_clocks(space, spots)
    out = {
        "version": 1,
        "courtyards": courtyards,
        "buildings": [dict(piece, replaces=k) for k, pieces in replaced.items() for piece in pieces],
        "wall_ads": ads,
        "kiosks": kiosks,
        "clocks": clocks,
        "map_icons": place_map_icons(data, f, courtyards),
    }
    with open(args.output, "w", encoding="utf-8") as fh:
        json.dump(out, fh, ensure_ascii=False, separators=(",", ":"))
    print(f"{len(courtyards)} courtyards, {len(out['buildings'])} building pieces, {len(ads)} wall ads, {len(kiosks)} kiosks, {len(clocks)} clocks, {len(out['map_icons'])} map icons")


if __name__ == "__main__":
    main()
