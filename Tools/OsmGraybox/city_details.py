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
  data (or not at all: "gone"), each in the graybox's own building form ({"id", "rings", "tris",
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
- churches: [kind ("rokus" or "jezus"), x, y, yaw, width, depth], the
  front's middle at the foot, yaw out of the front (ChurchModel.h); their map
  blocks are listed in "buildings" as {"gone": true} pieces, so AOsmGraybox
  leaves them out;
- window_folk: [x, y, sill z, yaw out, kind, seed] at windows that have no
  flower box or washing (FacadeLifeModel::WindowHash, the same sums here);
- hydrants [x, y, yaw], manholes [x, y, yaw, variant], hatches (cellar
  hatches against the house fronts) [x, y, yaw out, variant], troughs (horse
  troughs) [x, y, yaw], coal_drops (the coal man's drop at a cellar hatch)
  [x, y, yaw out, seed] (CityLifeModel.h).

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
from shapely.geometry import box
from shapely.ops import polylabel, unary_union
from shapely.prepared import prep

from street_furniture import Space, Spread, chance, in_cart_gate, ring_points, unit, yaw_of

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
               "Keleti pályaudvar", "Erzsébet körút", "Fiumei út"]
# Kiosks out on a square, where people can walk round them: near this spot,
# put after the clocks, with this much more room round them than a kerb one.
# (The one by Kiss József utca stood on Rakoczi ut's narrow pavement.)
KIOSK_SQUARES = [("Blaha Lujza tér", (-9200, 1800))]
KIOSK_SQUARE_CLEAR = 250
KIOSK_SQUARE_ROOM = 500  # from the clock, lamps and the like
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


def place_square_kiosks(space, f, spots, kiosks):
    walks = [LineString(ring_points(w)) for w in f["sidewalks"]]
    for name, (cx, cy) in KIOSK_SQUARES:
        best = None
        for r in range(300, 4000, 100):
            for k in range(16):
                a = 2 * math.pi * k / 16
                x, y = cx + r * math.cos(a), cy + r * math.sin(a)
                if not space.free(x, y, KIOSK_RADIUS + KIOSK_SQUARE_CLEAR) or not room(spots, x, y, KIOSK_SQUARE_ROOM):
                    continue
                p = Point(x, y)
                if any(w.distance(p) < KIOSK_RADIUS + KIOSK_SQUARE_CLEAR for w in walks if w.distance(p) < 2000):
                    continue
                best = (x, y)
                break
            if best:
                break
        if best:
            # The counter faces the nearest road.
            p = Point(*best)
            q = space.road_shape.boundary.interpolate(space.road_shape.boundary.project(p))
            kiosks.append([round(best[0]), round(best[1]), yaw_of(q.x - best[0], q.y - best[1]), len(kiosks) % KIOSK_VARIANTS])
            spots.append(best)


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


# The cafe on Tisza Kalman ter (PestiStart::CafeAt), the Nepszinhaz
# (PestiStart::TheatreAt), Keleti's front (AKeletiStation::FrontAt) and the
# market hall's front (AMarketHall::FrontAt), all built by the game itself.
CAFE_AT = (62685, 14408)
# Bozsi neni's door on Nepszinhaz utca (PestiStart::HomeDoorAt).
BOZSI_AT = (58392, 21268)
THEATRE_AT = (-3600, 200)
KELETI_AT = (92766, -43178)
HALL_AT = (18109, 40941)
CORVIN_AT = (-9440, 3697)
# The New York café, the middle of the palace's körút front.
NEW_YORK_CAFE_AT = (158, -21244)
# The Városi Színház, the middle of its front on Tisza Kálmán tér.
CITY_THEATRE_AT = (51700, -4660)
# Tram stops with the same name nearer than this share one mark on the map.
STOP_MARK_SAME = 6000


def place_map_icons(data, f, courtyards):
    """The little pictures on the maps: [kind, x, y] with kind one of cafe,
    market, tram, phone, yard, theatre, station, store, church."""
    icons = [["cafe", *CAFE_AT], ["theatre", *THEATRE_AT], ["station", *KELETI_AT],
             ["market", *HALL_AT], ["store", *CORVIN_AT], ["cafe", *NEW_YORK_CAFE_AT], ["theatre", *CITY_THEATRE_AT]]
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

# ---------------------------------------------------------------------------
# Round 2: the churches, the people at the windows and the street clutter.

# The churches the game builds itself (ChurchModel.h), in place of their map
# blocks: the block's id, the model, and the block's edges along its front.
CHURCHES = [
    ("w256539415", "rokus", [6]),       # Szent Rókus-kápolna, Rákóczi út
    ("w191191238", "jezus", [6, 7]),    # Jézus Szíve templom, Mária utca
]
BOZSI_ID = "r3333877"

# The New York palace on Erzsébet körút (NewYorkModel.h dresses its street
# walls): its block keeps only the Osvát utca wall's plain facade, and the
# game builds the rest from these runs of its outline's corners.
NEW_YORK_ID = "w259011268"
NEW_YORK_RUNS = [("korut", 8, 10), ("corner", 10, 12), ("dohany", 12, 0)]
NEW_YORK_PLAIN_FRONT = [0]

# The Városi Színház (the Népopera of 1911) on Tisza Kálmán tér: today's
# outline steps in and out along the square; in 1932 the front was one
# straight wall, which CityTheatreModel.h dresses. The block keeps its
# outline round the back and gets that straight front from the corner
# after THEATRE_FROM to the one before THEATRE_TO.
THEATRE_ID = "w26829510"
THEATRE_FROM, THEATRE_TO = 6, 17
THEATRE_FRONT_X = 51560

# The Kerepesi cemetery (CemeteryModel.h): its wall round the part inside
# the map, the gates on Fiumei út, and graves in rows between the alleys
# (street_furniture.json "kerepesi" has the outline, alleys and trees).
CEMETERY_MAP_EDGE = 300          # the wall stops this far in from the map's edge
CEMETERY_MAIN_GATE_PATH = "Fő út"   # the main gate is where this alley meets the wall
CEMETERY_SIDE_GATES = [2, 3]     # outline corners with a side gate ("2. kapu", "3. kapu")
CEMETERY_GATE_WIDTH = {"main": 1500, "side": 600}
CEMETERY_ROW_GAP = 300           # graves: rows this far apart
CEMETERY_GRAVE_GAP = 175         # and graves this far apart along a row
CEMETERY_KEEP = 0.55             # share of the grid's spots with a grave
CEMETERY_NEAR_ALLEY = 8000       # graves only this near an alley
CEMETERY_MAX_GRAVES = 8000
CEMETERY_GRAVE_KINDS = [0, 0, 0, 1, 1, 2, 2, 3, 4, 4, 5]   # CemeteryModel::EGrave, weighted
# Built after 1932 (the workers' pantheon of 1959, the Károlyi tomb of 1962): left out.
# Buildings inside the cemetery the map makes four floors tall: lodges,
# offices, chapels and tombs are lower (by name; anything else one floor).
CEMETERY_HEIGHTS = {"Árkádsor": 750, "kápolna": 1100, "Batthyány": 1300, "Kossuth": 1500, "mauzóleum": 800}
CEMETERY_LOW = 650
CEMETERY_LATER = ["w526250716", "w526250726"]

# The facade's sums (AddFacade in OsmGraybox.cpp, Facade:: constants).
FACADE_STOREY = 380.0
FACADE_GROUND_FLOOR = 450.0
FACADE_CORNICE = 60.0
FACADE_BAY = 320.0
FACADE_MARGIN = 60.0
FACADE_MIN_LENGTH = 250.0
# FacadeLifeModel.h: windows with a hash in this range out of 1000 have
# nothing on them, and the people are picked from them.
FOLK_FROM, FOLK_TO = 200, 260
FOLK_GAP = 2800
FOLK_MAX_FLOOR = 3
FOLK_KINDS = 5           # CityLifeModel::EFolk: old woman, man, young woman, duvet, cat
FOLK_WEIGHTS = [3, 3, 3, 2, 1]


def floor_to_int(v):
    return int(math.floor(v))


def window_hash(x, y, floor):
    """FacadeLifeModel::WindowHash, in uint32 sums."""
    m = 0xFFFFFFFF
    cx = floor_to_int(x / 50.0) & m
    cy = floor_to_int(y / 50.0) & m
    h = ((cx * 73856093) & m) ^ ((cy * 19349663) & m) ^ ((floor * 83492791) & m)
    h ^= h >> 13
    h = (h * 0x5bd1e995) & m
    h ^= h >> 15
    return h


def facade_buildings(data, replaced):
    """The blocks the facades are drawn on, as OsmGraybox merges them."""
    out = [b for b in data["buildings"] if b["id"] not in replaced]
    for k, pieces in replaced.items():
        out += [p for p in pieces if not p.get("gone")]
    return [b for b in out if b.get("id") not in (CORVIN_ID, BOZSI_ID) and not b.get("base")]


def facade_walls(buildings):
    """Each street-facing wall: (p0, p1, height, levels, building)."""
    for b in buildings:
        front = set(b.get("front", []))
        index = 0
        for flat in b["rings"]:
            ring = ring_points(flat)
            for i in range(len(ring)):
                if index in front:
                    yield ring[i], ring[(i + 1) % len(ring)], b["height"], int(b.get("levels", 0)), b
                index += 1


def wall_windows(p0, p1, height, levels):
    """The upper floor windows of one wall, as AddFacade lays them out:
    (x, y, sill z, floor, out x, out y)."""
    length = math.dist(p0, p1)
    if length < 1.0:
        return
    ground_top = min(FACADE_GROUND_FLOOR, height - FACADE_CORNICE)
    if length < FACADE_MIN_LENGTH or ground_top < 300.0:
        return
    storeys = levels if levels > 0 else max(1, min(8, floor_to_int(height / FACADE_STOREY + 0.5)))
    if storeys < 2:
        return
    upper = (height - FACADE_CORNICE - ground_top) / (storeys - 1)
    ax, ay = (p1[0] - p0[0]) / length, (p1[1] - p0[1]) / length
    bays = max(1, floor_to_int((length - 2 * FACADE_MARGIN) / FACADE_BAY))
    bay = (length - 2 * FACADE_MARGIN) / bays
    for k in range(bays):
        mid = FACADE_MARGIN + bay * (k + 0.5)
        x, y = p0[0] + ax * mid, p0[1] + ay * mid
        for floor in range(1, storeys):
            z0 = ground_top + (floor - 1) * upper + upper * 0.22
            yield x, y, z0, floor, ay, -ax


def place_window_folk(buildings):
    """People and things at windows with nothing else on them:
    [x, y, sill z, yaw out, kind, seed]."""
    spread = Spread(FOLK_GAP)
    folk = []
    weights = [k for k, w in enumerate(FOLK_WEIGHTS) for _ in range(w)]
    for p0, p1, height, levels, b in facade_walls(buildings):
        for x, y, z0, floor, ox, oy in wall_windows(p0, p1, height, levels):
            if floor > FOLK_MAX_FLOOR or not FOLK_FROM <= window_hash(x, y, floor) % 1000 < FOLK_TO:
                continue
            if not spread.ok(x, y):
                continue
            spread.add(x, y)
            kind = weights[int(chance("folk", round(x), round(y)) * len(weights)) % len(weights)]
            folk.append([round(x), round(y), round(z0, 1), yaw_of(ox, oy), kind, int(chance("folkseed", round(x), round(y)) * 1000)])
    return folk


def new_york(data):
    """The palace's block with the dressed walls' facades taken off, and its
    pieces: [part, x, y, yaw, length], each run's start at the foot, yaw out
    of the wall, the run along the frame's Y (NewYorkModel::FPiece)."""
    building = next(b for b in data["buildings"] if b["id"] == NEW_YORK_ID)
    form = {k: building[k] for k in ("rings", "tris", "height", "levels")}
    form["front"] = NEW_YORK_PLAIN_FRONT
    ring = ring_points(building["rings"][0])
    shape = Polygon(ring)
    pieces = []
    for part, i0, i1 in NEW_YORK_RUNS:
        a, z = ring[i0], ring[i1]
        ax, ay = unit(z[0] - a[0], z[1] - a[1])
        ox, oy = ay, -ax
        mx, my = (a[0] + z[0]) / 2, (a[1] + z[1]) / 2
        # The model's Y runs along out turned a quarter right, so the run must go that way.
        assert not shape.contains(Point(mx + ox * 100, my + oy * 100)), part
        pieces.append([part, round(a[0]), round(a[1]), yaw_of(ox, oy), round(math.dist(a, z))])
    return form, pieces


def city_theatre(data):
    """The theatre's block with one straight front on the square, and that
    front: [x, y, yaw, length], its start at the foot, yaw out of the wall,
    the wall along the frame's Y (CityTheatreModel.h)."""
    building = next(b for b in data["buildings"] if b["id"] == THEATRE_ID)
    ring = ring_points(building["rings"][0])
    a = (THEATRE_FRONT_X, ring[THEATRE_FROM][1])
    z = (THEATRE_FRONT_X, ring[THEATRE_TO][1])
    outline = ring[:THEATRE_FROM + 1] + [a, z] + ring[THEATRE_TO:]
    shape = Polygon(outline)
    assert shape.is_valid
    keep = [line for line in front_lines_of(building) if min(line.coords[0][0], line.coords[1][0]) < THEATRE_FRONT_X - 2000]
    form = building_form(shape, building, keep, building["height"], int(building.get("levels", 0)))
    ax, ay = unit(z[0] - a[0], z[1] - a[1])
    ox, oy = ay, -ax
    if shape.contains(Point((a[0] + z[0]) / 2 + ox * 100, (a[1] + z[1]) / 2 + oy * 100)):
        a, z = z, a
        ax, ay, ox, oy = -ax, -ay, -ox, -oy
    return form, [round(a[0]), round(a[1]), yaw_of(ox, oy), round(math.dist(a, z))]


def cemetery_buildings(data, f):
    """The cemetery's tall graybox buildings made low, by id."""
    k = f.get("kerepesi")
    if not k:
        return {}
    grounds = Polygon(k["outline"]).buffer(300)
    out = {}
    for b in data["buildings"]:
        footprint = Polygon(ring_points(b["rings"][0]))
        if b["id"] in CEMETERY_LATER or b["height"] <= CEMETERY_LOW or not grounds.contains(footprint.representative_point()):
            continue
        name = b.get("name", "")
        height = next((h for word, h in CEMETERY_HEIGHTS.items() if word in name), CEMETERY_LOW)
        if height < b["height"]:
            out[b["id"]] = [building_form(footprint, b, front_lines_of(b), height, 1)]
    return out


def place_cemetery(data, f):
    """The cemetery's wall runs [x0, y0, x1, y1] (outside on the right as
    the run goes, as house fronts), gates [kind, x, y, yaw out, width] and
    graves [x, y, yaw, kind, seed]."""
    k = f.get("kerepesi")
    if not k:
        return None, []
    outline = [tuple(p) for p in k["outline"]]
    shape = Polygon(outline)
    b = data["bounds"]
    inside = box(b["minX"] + CEMETERY_MAP_EDGE, b["minY"] + CEMETERY_MAP_EDGE, b["maxX"] - CEMETERY_MAP_EDGE, b["maxY"] - CEMETERY_MAP_EDGE)
    # Run the outline the way that puts the street on the right.
    a, z = outline[0], outline[1]
    ax, ay = unit(z[0] - a[0], z[1] - a[1])
    if shape.contains(Point((a[0] + z[0]) / 2 + ay * 100, (a[1] + z[1]) / 2 - ax * 100)):
        outline.reverse()
    ring = outline + [outline[0]]

    gates = []
    def gate_at(kind, x, y, dx, dy):
        gates.append([kind, round(x), round(y), yaw_of(dy, -dx), CEMETERY_GATE_WIDTH[kind]])
    alley = next((c for c in data["centerlines"] if c.get("name") == CEMETERY_MAIN_GATE_PATH), None)
    edges = LineString(ring)
    if alley:
        start = Point(ring_points(alley["points"])[0])
        t = edges.project(start)
        p, q = edges.interpolate(t - 50), edges.interpolate(t + 50)
        dx, dy = unit(q.x - p.x, q.y - p.y)
        hit = edges.interpolate(t)
        gate_at("main", hit.x, hit.y, dx, dy)
    original = [tuple(p) for p in k["outline"]]
    for corner in CEMETERY_SIDE_GATES:
        cx, cy = original[corner]
        t = edges.project(Point(cx, cy))
        p, q = edges.interpolate(t - 50), edges.interpolate(t + 50)
        dx, dy = unit(q.x - p.x, q.y - p.y)
        gate_at("side", cx, cy, dx, dy)

    # The wall: each outline edge inside the map, cut open at the gates.
    openings = unary_union([Point(g[1], g[2]).buffer(g[4] * 0.5) for g in gates])
    walls = []
    for p0, p1 in zip(ring, ring[1:]):
        line = LineString([p0, p1]).intersection(inside).difference(openings)
        for piece in getattr(line, "geoms", [line]):
            if piece.is_empty or piece.geom_type != "LineString" or piece.length < 5:
                continue
            (x0, y0), (x1, y1) = piece.coords[0], piece.coords[-1]
            # Keep the run's direction (shapely may turn a piece round).
            if (x1 - x0) * (p1[0] - p0[0]) + (y1 - y0) * (p1[1] - p0[1]) < 0:
                (x0, y0), (x1, y1) = (x1, y1), (x0, y0)
            walls.append([round(x0), round(y0), round(x1), round(y1)])

    # Graves in rows square to Fiumei út, between the alleys and trees.
    area = shape.buffer(-350).intersection(inside.buffer(-500))
    alleys = [LineString([(p[0], p[1]), (p[2], p[3])]) for p in k["paths"]]
    keep_off = unary_union([l.buffer(p[4] * 0.5 + 130) for l, p in zip(alleys, k["paths"])]
                           + [Point(t[0], t[1]).buffer(140) for t in k["trees"]]
                           + [Point(bn[0], bn[1]).buffer(150) for bn in k["benches"]]
                           + [Point(g[1], g[2]).buffer(1600) for g in gates]
                           + [Polygon(ring_points(bl["rings"][0])).buffer(200) for bl in data["buildings"]
                              if area.intersects(Point(ring_points(bl["rings"][0])[0]))])
    near = unary_union([l.buffer(CEMETERY_NEAR_ALLEY) for l in alleys])
    ground = prep(area.intersection(near).difference(keep_off))
    fx, fy = unit(original[1][0] - original[0][0], original[1][1] - original[0][1])
    rx, ry = -fy, fx
    yaw = yaw_of(rx, ry)
    x0, y0, x1, y1 = area.bounds
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    reach = math.hypot(x1 - x0, y1 - y0) / 2
    graves = []
    for i in range(int(-reach // CEMETERY_ROW_GAP), int(reach // CEMETERY_ROW_GAP) + 1):
        for j in range(int(-reach // CEMETERY_GRAVE_GAP), int(reach // CEMETERY_GRAVE_GAP) + 1):
            x = cx + rx * i * CEMETERY_ROW_GAP + fx * j * CEMETERY_GRAVE_GAP
            y = cy + ry * i * CEMETERY_ROW_GAP + fy * j * CEMETERY_GRAVE_GAP
            if chance("grave", i, j) >= CEMETERY_KEEP or not ground.contains(Point(x, y)):
                continue
            kind = CEMETERY_GRAVE_KINDS[int(chance("gravekind", i, j) * len(CEMETERY_GRAVE_KINDS)) % len(CEMETERY_GRAVE_KINDS)]
            graves.append([round(x), round(y), round(yaw + (chance("graveyaw", i, j) - 0.5) * 6, 1), kind, int(chance("graveseed", i, j) * 1000)])
    if len(graves) > CEMETERY_MAX_GRAVES:
        step = len(graves) / CEMETERY_MAX_GRAVES
        graves = [graves[int(n * step)] for n in range(CEMETERY_MAX_GRAVES)]
    # Grass over the whole ground inside the wall (OsmGraybox adds it as a park area).
    park = next(pg for a in data["areas"] if a["layer"] == "park" for pg in a["polygons"])
    lawn_shape = shape.intersection(inside)
    lawn = building_form(lawn_shape, park, [], 0, 0)
    areas = [{"layer": "park", "polygons": [{"rings": lawn["rings"], "tris": lawn["tris"]}]}]
    return {"walls": walls, "gates": gates, "graves": graves}, areas


def place_churches(data):
    """[kind, x, y, yaw, width, depth]: the front's middle at the foot, yaw out of the front."""
    churches = []
    by_id = {b["id"]: b for b in data["buildings"]}
    for bid, kind, edges in CHURCHES:
        b = by_id[bid]
        ring = ring_points(b["rings"][0])
        shape = Polygon(ring)
        a = ring[edges[0]]
        z = ring[(edges[-1] + 1) % len(ring)]
        ax, ay = unit(z[0] - a[0], z[1] - a[1])
        ox, oy = ay, -ax
        mx, my = (a[0] + z[0]) / 2, (a[1] + z[1]) / 2
        if shape.contains(Point(mx + ox * 100, my + oy * 100)):
            ox, oy = -ox, -oy
        # Its extent along the front and back from it, the frame put in the middle.
        along = [(px - mx) * ax + (py - my) * ay for px, py in ring]
        back = [-((px - mx) * ox + (py - my) * oy) for px, py in ring]
        lo, hi = min(along), max(along)
        cx, cy = mx + ax * (lo + hi) / 2, my + ay * (lo + hi) / 2
        churches.append([kind, round(cx), round(cy), yaw_of(ox, oy), round(hi - lo), round(max(back))])
    return churches


# Street clutter.
HYDRANT_GAP = 7000
HYDRANT_IN = 45          # in from the kerb (cm)
MANHOLE_STEP = 2500
MANHOLE_GAP = 3200
MANHOLE_TRAM_CLEAR = 120
HATCH_SHARE = 0.10
HATCH_GAP = 2200
HATCH_SHOP_CLEAR = 80
TROUGHS_NEAR = [BOZSI_AT, HALL_AT, KELETI_AT, (46577, 33461), (99969, 28889), (-7034, -1747)]
TROUGH_ROOM = 220
COAL_DROPS = 4
COAL_GAP = 12000


def kerb_spot(space, x, y, inset):
    """The nearest kerb to a pavement spot, and a spot inset from it onto the
    pavement: (x, y, yaw towards the road) or None."""
    p = Point(x, y)
    edge = space.road_shape.boundary
    q = edge.interpolate(edge.project(p))
    d = p.distance(q)
    if d < 1 or d > 700:
        return None
    ux, uy = (x - q.x) / d, (y - q.y) / d
    return q.x + ux * inset, q.y + uy * inset, yaw_of(-ux, -uy)


def clear_of(space, x, y, building_gap, lane_gap=150):
    pt = Point(x, y)
    if space.near_building(pt, building_gap) or space.near_lane(pt, lane_gap):
        return False
    if space.trams is not None and space.trams.distance(pt) < CLEAR_OF_TRAMS_HERE:
        return False
    return True


CLEAR_OF_TRAMS_HERE = 150


def place_hydrants(space, f, spots):
    spread = Spread(HYDRANT_GAP)
    hydrants = []
    for w in f["sidewalks"]:
        line = LineString(ring_points(w))
        steps = max(1, int(line.length // 1500))
        for i in range(steps + 1):
            on = line.interpolate(i / steps, normalized=True)
            spot = kerb_spot(space, on.x, on.y, HYDRANT_IN)
            if spot is None:
                continue
            x, y, yaw = spot
            if not spread.ok(x, y) or not room(spots, x, y, 150) or not clear_of(space, x, y, 150, 120):
                continue
            if space.road_shape.distance(Point(x, y)) < HYDRANT_IN - 5:
                continue
            spread.add(x, y)
            hydrants.append([round(x), round(y), yaw])
            spots.append((x, y))
    return hydrants


def place_manholes(space):
    spread = Spread(MANHOLE_GAP)
    manholes = []
    for lane in space.lanes:
        steps = int(lane.length // MANHOLE_STEP)
        for i in range(1, steps + 1):
            t = i * MANHOLE_STEP - MANHOLE_STEP / 2
            on = lane.interpolate(t)
            if not spread.ok(on.x, on.y):
                continue
            if space.trams is not None and space.trams.distance(on) < MANHOLE_TRAM_CLEAR:
                continue
            a, b = lane.interpolate(max(t - 50, 0)), lane.interpolate(min(t + 50, lane.length))
            spread.add(on.x, on.y)
            manholes.append([round(on.x), round(on.y), yaw_of(b.x - a.x, b.y - a.y), len(manholes) % 2])
    return manholes


def place_hatches(space, buildings, f, spots, skip_ids):
    """Cellar hatches against the house fronts, between the windows of the
    ground floor: [x, y, yaw out, variant]."""
    spread = Spread(HATCH_GAP)
    fronts = [(s["at"], s["width"]) for s in f["shops"]] + [(g["at"], g["width"]) for g in f["gates"]]
    hatches = []
    for p0, p1, height, levels, b in facade_walls(buildings):
        if b.get("id") in skip_ids:
            continue
        length = math.dist(p0, p1)
        if length < 700:
            continue
        ax, ay = (p1[0] - p0[0]) / length, (p1[1] - p0[1]) / length
        ox, oy = ay, -ax
        bays = max(1, floor_to_int((length - 2 * FACADE_MARGIN) / FACADE_BAY))
        bay = (length - 2 * FACADE_MARGIN) / bays
        for k in range(1, bays):
            if k in (bays // 2, bays // 2 + 1) or chance("hatch", b.get("id", ""), k, round(p0[0])) >= HATCH_SHARE:
                continue
            s_at = FACADE_MARGIN + bay * k
            x, y = p0[0] + ax * s_at, p0[1] + ay * s_at
            if any(math.hypot(x - a[0], y - a[1]) < w / 2 + HATCH_SHOP_CLEAR for a, w in fronts):
                continue
            out = Point(x + ox * 60, y + oy * 60)
            if space.road_shape.distance(out) < 120 or space.near_lane(out, 250) or not room(spots, x, y, 160):
                continue
            if not spread.ok(x, y):
                continue
            spread.add(x, y)
            hatches.append([round(x), round(y), yaw_of(ox, oy), int(chance("hatchkind", b.get("id", ""), k, round(p0[1])) * 3) % 2])
    return hatches


def place_coal_drops(space, hatches, spots):
    """Some of the hatches get the coal man's drop: [x, y, yaw out, seed]."""
    spread = Spread(COAL_GAP)
    drops = []
    for h in sorted(hatches, key=lambda h: chance("coal", h[0], h[1])):
        x, y, yaw = h[0], h[1], h[2]
        ox, oy = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
        ax, ay = -oy, ox
        ok = True
        for out in (120, 200, 330):
            for side in (-250, -100, 0, 150, 300, 450):
                px, py = x + ox * out + ax * side, y + oy * out + ay * side
                pt = Point(px, py)
                if space.road_shape.distance(pt) < 50 or space.near_lane(pt, 200) or space.near_building(pt, 30) or not room(spots, px, py, 110):
                    ok = False
        if not ok or not spread.ok(x, y):
            continue
        spread.add(x, y)
        drops.append([x, y, yaw, int(chance("coalseed", x, y) * 1000)])
        hatches.remove(h)
        if len(drops) >= COAL_DROPS:
            break
    return drops


def place_troughs(space, f, spots):
    walks = [LineString(ring_points(w)) for w in f["sidewalks"]]
    troughs = []
    for ax_, ay_ in TROUGHS_NEAR:
        best = None
        for walk in sorted(walks, key=lambda w: w.distance(Point(ax_, ay_)))[:12]:
            for i in range(0, int(walk.length), 200):
                on = walk.interpolate(i)
                if math.hypot(on.x - ax_, on.y - ay_) > 3500:
                    continue
                spot = kerb_spot(space, on.x, on.y, 70)
                if spot is None:
                    continue
                x, y, yaw = spot
                if not room(spots, x, y, TROUGH_ROOM) or not clear_of(space, x, y, 200, 150):
                    continue
                # A trough runs about 2.4 m along the kerb: keep all of it out of a cart gateway.
                if in_cart_gate(x, y, 150):
                    continue
                d = math.hypot(x - ax_, y - ay_)
                if best is None or d < best[0]:
                    best = (d, x, y, yaw)
        if best:
            _, x, y, yaw = best
            troughs.append([round(x), round(y), yaw])
            spots.append((x, y))
    return troughs



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
    new_york_block, new_york_pieces = new_york(data)
    replaced[NEW_YORK_ID] = [new_york_block]
    theatre_block, theatre_front = city_theatre(data)
    replaced[THEATRE_ID] = [theatre_block]
    for bid in CEMETERY_LATER:
        replaced[bid] = [{"gone": True}]
    replaced.update(cemetery_buildings(data, f))
    cemetery, extra_areas = place_cemetery(data, f)
    churches = place_churches(data)
    for bid, _, _ in CHURCHES:
        replaced[bid] = [{"gone": True}]
    spots = furniture_spots(f)
    ads = place_wall_ads(data, space)
    kiosks = place_kiosks(space, f, spots)
    clocks = place_clocks(space, spots)
    place_square_kiosks(space, f, spots, kiosks)
    fronts = facade_buildings(data, replaced)
    folk = place_window_folk(fronts)
    troughs = place_troughs(space, f, spots)
    hydrants = place_hydrants(space, f, spots)
    hatches = place_hatches(space, fronts, f, spots, {CORVIN_ID})
    coal_drops = place_coal_drops(space, hatches, spots)
    manholes = place_manholes(space)
    out = {
        "version": 1,
        "courtyards": courtyards,
        "buildings": [dict(piece, replaces=k) for k, pieces in replaced.items() for piece in pieces],
        "wall_ads": ads,
        "kiosks": kiosks,
        "clocks": clocks,
        "map_icons": place_map_icons(data, f, courtyards),
        "churches": churches,
        "newyork": new_york_pieces,
        "city_theatre": theatre_front,
        "cemetery": cemetery,
        "areas": extra_areas,
        "window_folk": folk,
        "hydrants": hydrants,
        "manholes": manholes,
        "hatches": hatches,
        "troughs": troughs,
        "coal_drops": coal_drops,
    }
    with open(args.output, "w", encoding="utf-8") as fh:
        json.dump(out, fh, ensure_ascii=False, separators=(",", ":"))
    print(f"{len(courtyards)} courtyards, {len(out['buildings'])} building pieces, {len(ads)} wall ads, {len(kiosks)} kiosks, {len(clocks)} clocks, {len(out['map_icons'])} map icons, "
          f"{len(churches)} churches, {len(folk)} at windows, {len(hydrants)} hydrants, {len(manholes)} manholes, {len(hatches)} hatches, "
          f"{len(troughs)} troughs, {len(coal_drops)} coal drops")


if __name__ == "__main__":
    main()
