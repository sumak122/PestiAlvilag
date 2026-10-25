"""Place street furniture and lay out Tisza Kálmán tér from the graybox data.

Reads the graybox JSON written by osm_to_graybox.py and writes a second JSON
that the AStreetFurniture actor reads:

- lamps: the period street lamp (a slim pole with a scrolled crook and a
  hanging bell lantern, Fortepan 100450) along the pavements of every street
  wide enough to have them, facing the road;
- columns: round advertising columns (hirdetőoszlop) near street corners;
- benches: round the edges of the parks;
- sidewalks: the lines people walk along, on both pavements of every
  street and down the paths of Tisza Kálmán tér, as flat [x0, y0, x1, y1,
  ...] lists;
- bins: litter bins on a post, beside some street lamps and next to the
  park benches;
- park: the 1932 layout of Tisza Kálmán tér (today II. János Pál pápa tér):
  gravel paths, round flower beds, rows of trees, benches and lamps along
  the paths, and a low iron railing with gaps where the paths come in;
  in autumn also raked leaf piles and the park keepers who rake them.

Everything is in Unreal centimetres (X east, Y south), [x, y, yaw] with yaw in
degrees; heights come from the ground in the game.

Usage:
    python street_furniture.py ../../Data/OsmGraybox/blaha_test_area.json \
        ../../Data/OsmGraybox/street_furniture.json
"""

import argparse
import json
import math
import zlib

from shapely.geometry import LineString, Point, Polygon
from shapely.ops import unary_union
from shapely.prepared import prep
from shapely.strtree import STRtree

# Streets narrower than this (cm) get no lamps; service roads never do.
MIN_LAMP_WIDTH = 600
# Narrow streets have lamps on alternate sides, wide ones on both.
BOTH_SIDES_WIDTH = 900
LAMP_SPACING = 3000
# Keep lamps this far from street ends, where other streets cross (cm).
LAMP_END_CLEARANCE = 1200
LAMP_MIN_GAP = 1500

# How far from things the furniture must stand (cm).
CLEAR_OF_BUILDINGS = 70
CLEAR_OF_ROAD = 40
CLEAR_OF_LANES = 230
CLEAR_OF_TRAMS = 120

COLUMN_CHANCE = 0.4
COLUMN_RADIUS = 70
COLUMN_MIN_GAP = 8000
COLUMN_ALONG = 900

BENCH_SPACING = 2000
BENCH_INSET = 350

# People walk this far out from the edge of the carriageway, in a line
# kept clear of the buildings; pieces shorter than this are left out (cm).
SIDEWALK_OUT = 170
SIDEWALK_CLEAR = 60
SIDEWALK_MIN = 800
SIDEWALK_SIMPLIFY = 50

# Litter bins: beside this share of the street lamps, and next to park
# benches, just past the bench's end (a bench is 180 cm long).
BIN_CHANCE = 0.3
BIN_RADIUS = 25
BIN_MIN_GAP = 2500
BIN_BESIDE_LAMP = 90
BIN_BESIDE_BENCH = 125

# Tisza Kálmán tér: the park polygon in the graybox has its middle near here.
TISZA_PARK_NEAR = (61300, 1100)
# The round places in today's park sit where the 1932 flower beds go.
TISZA_ROUNDS = [((59600, 700), 2300, 800), ((64050, -9900), 1900, 600)]
PATH_WIDTH = 400
SIDE_PATH_WIDTH = 300
TREE_SPACING = 1100
TREE_ROW_OFFSET = 450
TREE_FILL_SPACING = 1500
PATH_LAMP_SPACING = 2500
PATH_BENCH_SPACING = 1600
PATH_BENCH_INSET = 30
FENCE_INSET = 60
# A knee-high wire fence on little posts keeps people off the lawns along
# the paths, and low clipped hedges ring the round places (Fortepan 70268).
LAWN_FENCE_OFFSET = 15
LAWN_FENCE_MIN = 300
HEDGE_OFFSET = 70
HEDGE_GAP = 150
# Autumn: raked leaf piles on the lawns beside the paths, and park keepers
# who go from pile to pile with their rakes.
LEAF_PILE_SPACING = 1800
LEAF_PILE_OUT = 260
LEAF_PILE_CHANCE = 0.55
LEAF_PILE_MIN_GAP = 1500
KEEPERS = 2
PILES_PER_KEEPER = 6
FENCE_GAP = 900


def ring_points(flat):
    return list(zip(flat[0::2], flat[1::2]))


def unit(dx, dy):
    length = math.hypot(dx, dy)
    return (dx / length, dy / length) if length > 0 else (0.0, 0.0)


def yaw_of(dx, dy):
    return round(math.degrees(math.atan2(dy, dx)), 1)


def chance(*key):
    """A repeatable random number in [0, 1) for this key."""
    return (zlib.crc32(repr(key).encode()) % 10000) / 10000.0


def walk(points, spacing, start, end_clearance):
    """Points every `spacing` along a polyline, with the direction there."""
    line = LineString(points)
    total = line.length
    s = start
    while s <= total - end_clearance:
        p = line.interpolate(s)
        q = line.interpolate(min(s + 50, total))
        r = line.interpolate(max(s - 50, 0))
        yield (p.x, p.y), unit(q.x - r.x, q.y - r.y), s
        s += spacing


class Space:
    """What is in the way: buildings, carriageways, lanes, tram tracks."""

    def __init__(self, data):
        self.buildings = [Polygon(ring_points(b["rings"][0])) for b in data["buildings"]]
        self.building_tree = STRtree(self.buildings)
        roads = []
        trams = []
        self.parks = []
        for area in data["areas"]:
            for poly in area["polygons"]:
                shape = Polygon(ring_points(poly["rings"][0]), [ring_points(r) for r in poly["rings"][1:]])
                if not shape.is_valid:
                    shape = shape.buffer(0)
                if area["layer"] == "road":
                    roads.append(shape)
                elif area["layer"] == "tram":
                    trams.append(shape)
                elif area["layer"] == "park":
                    self.parks.append(shape)
        self.roads = prep(unary_union(roads))
        self.road_shape = unary_union(roads)
        self.trams = unary_union(trams) if trams else None
        self.lanes = [LineString(ring_points(l["points"])) for l in data["lanes"] if len(l["points"]) >= 4]
        self.lane_tree = STRtree(self.lanes)

    def near_building(self, point, distance):
        for i in self.building_tree.query(point.buffer(distance)):
            if self.buildings[i].distance(point) < distance:
                return True
        return False

    def near_lane(self, point, distance):
        for i in self.lane_tree.query(point.buffer(distance)):
            if self.lanes[i].distance(point) < distance:
                return True
        return False

    def free(self, x, y, radius, on_road_ok=False):
        point = Point(x, y)
        if self.near_building(point, CLEAR_OF_BUILDINGS + radius):
            return False
        if self.near_lane(point, CLEAR_OF_LANES + radius):
            return False
        if self.trams is not None and self.trams.distance(point) < CLEAR_OF_TRAMS + radius:
            return False
        if not on_road_ok and self.road_shape.distance(point) < CLEAR_OF_ROAD + radius and self.roads.intersects(point.buffer(CLEAR_OF_ROAD + radius)):
            return False
        return True


class Spread:
    """Keeps things of one kind from bunching up."""

    def __init__(self, gap):
        self.gap = gap
        self.cell = gap
        self.grid = {}

    def ok(self, x, y):
        cx, cy = int(x // self.cell), int(y // self.cell)
        for i in range(cx - 1, cx + 2):
            for j in range(cy - 1, cy + 2):
                for (px, py) in self.grid.get((i, j), []):
                    if math.hypot(px - x, py - y) < self.gap:
                        return False
        return True

    def add(self, x, y):
        self.grid.setdefault((int(x // self.cell), int(y // self.cell)), []).append((x, y))


def place_lamps(data, space, tisza):
    lamps = []
    spread = Spread(LAMP_MIN_GAP)
    for street in data["centerlines"]:
        width = street.get("width", 0)
        if street["highway"] == "service" or width < MIN_LAMP_WIDTH:
            continue
        points = ring_points(street["points"])
        if len(points) < 2:
            continue
        both = width >= BOTH_SIDES_WIDTH
        spacing = LAMP_SPACING if both else LAMP_SPACING / 2
        for n, ((x, y), (ax, ay), s) in enumerate(walk(points, spacing, LAMP_END_CLEARANCE, LAMP_END_CLEARANCE)):
            sides = (1, -1) if both else ((1,) if n % 2 == 0 else (-1,))
            for side in sides:
                # Left of the way the street is drawn is (ay, -ax).
                nx, ny = ay * side, -ax * side
                for out in (40, 90, 140, 200, 260):
                    lx = x + nx * (width / 2 + out)
                    ly = y + ny * (width / 2 + out)
                    if tisza.contains(Point(lx, ly)):
                        break
                    if space.free(lx, ly, 20) and spread.ok(lx, ly):
                        # The crook reaches out over the road.
                        lamps.append([round(lx), round(ly), yaw_of(-nx, -ny)])
                        spread.add(lx, ly)
                        break
    return lamps


def place_columns(data, space, tisza):
    ends = {}
    for street in data["centerlines"]:
        points = ring_points(street["points"])
        for end in (points[0], points[-1]):
            ends.setdefault((round(end[0] / 100), round(end[1] / 100)), []).append(street)
    columns = []
    spread = Spread(COLUMN_MIN_GAP)
    for key, streets in sorted(ends.items()):
        if len(streets) < 3 or chance("column", key) > COLUMN_CHANCE:
            continue
        for street in streets:
            width = street.get("width", 0)
            if street["highway"] == "service" or width < MIN_LAMP_WIDTH:
                continue
            points = ring_points(street["points"])
            if (round(points[-1][0] / 100), round(points[-1][1] / 100)) == key:
                points = points[::-1]
            line = LineString(points)
            if line.length < COLUMN_ALONG * 2:
                continue
            p = line.interpolate(COLUMN_ALONG)
            q = line.interpolate(COLUMN_ALONG + 50)
            ax, ay = unit(q.x - p.x, q.y - p.y)
            placed = False
            for side in (1, -1):
                nx, ny = ay * side, -ax * side
                for out in (150, 220, 300):
                    cx = p.x + nx * (width / 2 + out)
                    cy = p.y + ny * (width / 2 + out)
                    if not tisza.contains(Point(cx, cy)) and space.free(cx, cy, COLUMN_RADIUS) and spread.ok(cx, cy):
                        variant = int(chance("poster", key) * 3)
                        columns.append([round(cx), round(cy), yaw_of(-nx, -ny), variant])
                        spread.add(cx, cy)
                        placed = True
                        break
                if placed:
                    break
            if placed:
                break
    return columns


def place_sidewalks(data, space, tisza, park):
    """Walking lines along both pavements of each street, and the park paths."""
    blocked = unary_union([b.buffer(SIDEWALK_CLEAR) for b in space.buildings] + [tisza.buffer(-100)])
    if space.trams is not None:
        blocked = blocked.union(space.trams.buffer(SIDEWALK_CLEAR))
    blocked = prep(blocked)
    sidewalks = []

    def add(line):
        pieces = [line] if not blocked.intersects(line) else []
        if not pieces:
            rest = line.difference(blocked.context)
            pieces = [p for p in getattr(rest, "geoms", [rest]) if p.geom_type == "LineString"]
        for piece in pieces:
            piece = piece.simplify(SIDEWALK_SIMPLIFY)
            if piece.length >= SIDEWALK_MIN:
                sidewalks.append([round(v) for xy in piece.coords for v in xy])

    for street in data["centerlines"]:
        width = street.get("width", 0)
        points = ring_points(street["points"])
        if street["highway"] == "service" or len(points) < 2:
            continue
        line = LineString(points)
        if line.length < 100:
            continue
        for side in ("left", "right"):
            edge = line.parallel_offset(width / 2 + SIDEWALK_OUT, side, join_style=2)
            for part in getattr(edge, "geoms", [edge]):
                if part.geom_type == "LineString" and not part.is_empty:
                    add(part)
    for p in park["paths"]:
        sidewalks.append([p[0], p[1], p[2], p[3]])
    return sidewalks


def place_lamp_bins(lamps, space, tisza):
    """Bins on a share of the street lamps, a step along the pavement."""
    bins = []
    spread = Spread(BIN_MIN_GAP)
    for x, y, yaw in lamps:
        if chance("bin", x, y) > BIN_CHANCE:
            continue
        # The lamp faces the road; the pavement runs across that.
        a = math.radians(yaw)
        for along in (1, -1):
            bx = x - math.sin(a) * along * BIN_BESIDE_LAMP
            by = y + math.cos(a) * along * BIN_BESIDE_LAMP
            if not tisza.contains(Point(bx, by)) and space.free(bx, by, BIN_RADIUS) and spread.ok(bx, by):
                bins.append([round(bx), round(by), yaw])
                spread.add(bx, by)
                break
    return bins


def place_park_benches(space, tisza_park, bins):
    benches = []
    for park in space.parks:
        if park is tisza_park or park.area < 30e4:
            continue
        inner = park.buffer(-BENCH_INSET)
        if inner.is_empty:
            continue
        rings = [inner] if inner.geom_type == "Polygon" else list(inner.geoms)
        for ring_shape in rings:
            ring = list(ring_shape.exterior.coords)
            for (x, y), (ax, ay), _ in walk(ring, BENCH_SPACING, BENCH_SPACING / 2, BENCH_SPACING / 2):
                # Face into the park.
                nx, ny = ay, -ax
                if not park.contains(Point(x + nx * 200, y + ny * 200)):
                    nx, ny = -nx, -ny
                if space.free(x, y, 60):
                    benches.append([round(x), round(y), yaw_of(nx, ny)])
                    bx, by = x + ax * BIN_BESIDE_BENCH, y + ay * BIN_BESIDE_BENCH
                    if len(benches) % 2 == 0 and space.free(bx, by, BIN_RADIUS):
                        bins.append([round(bx), round(by), yaw_of(nx, ny)])
    return benches


def layout_tisza(space, park):
    """Paths, beds, trees, benches, lamps and railing for Tisza Kálmán tér."""
    hull = park.simplify(400)
    corners = [c for c in list(hull.exterior.coords)[:-1]]
    (cx, cy), center_r, _ = TISZA_ROUNDS[0]

    paths = []

    def add_path(a, b, width):
        paths.append([round(a[0]), round(a[1]), round(b[0]), round(b[1]), width])

    # Paths from each corner of the square to the round place in the middle,
    # stopping at its edge, and a ring path just inside the railing.
    for corner in corners:
        dx, dy = unit(cx - corner[0], cy - corner[1])
        start = (corner[0] + dx * 300, corner[1] + dy * 300)
        end = (cx - dx * center_r, cy - dy * center_r)
        add_path(start, end, PATH_WIDTH)
    # A path to the second round bed from the middle.
    (sx, sy), side_r, _ = TISZA_ROUNDS[1]
    dx, dy = unit(sx - cx, sy - cy)
    add_path((cx + dx * center_r, cy + dy * center_r), (sx - dx * side_r, sy - dy * side_r), SIDE_PATH_WIDTH)
    ring = park.buffer(-700)
    ring = ring if ring.geom_type == "Polygon" else max(ring.geoms, key=lambda g: g.area)
    ring = ring.simplify(300)
    ring_coords = list(ring.exterior.coords)
    for a, b in zip(ring_coords[:-1], ring_coords[1:]):
        add_path(a, b, SIDE_PATH_WIDTH)

    rounds = [[round(x), round(y), r, bed] for (x, y), r, bed in TISZA_ROUNDS]

    path_lines = [LineString([(p[0], p[1]), (p[2], p[3])]) for p in paths]
    path_area = unary_union([l.buffer(p[4] / 2) for l, p in zip(path_lines, paths)]
                            + [Point(x, y).buffer(r) for (x, y), r, _ in TISZA_ROUNDS])

    def in_park(x, y, margin):
        return park.buffer(-margin).contains(Point(x, y))

    # Path furniture keeps off the round places; their own benches go round the edge.
    round_places = unary_union([Point(x, y).buffer(r) for (x, y), r, _ in TISZA_ROUNDS])

    def off_rounds(x, y, margin):
        return not round_places.buffer(margin).contains(Point(x, y))

    # Where paths cross, a bench beside one path must not stand on another.
    walkways = unary_union([l.buffer(p[4] / 2 + 20) for l, p in zip(path_lines, paths)])

    def off_walkways(x, y):
        return not walkways.contains(Point(x, y))

    trees = []
    tree_spread = Spread(700)
    benches = []
    bins = []
    lamps = []
    bench_spread = Spread(800)
    lamp_spread = Spread(1200)

    # Trees in rows either side of every path, benches and lamps along them.
    for k, (line, p) in enumerate(zip(path_lines, paths)):
        coords = list(line.coords)
        other_walkways = unary_union([l.buffer(q[4] / 2 + 60) for j, (l, q) in enumerate(zip(path_lines, paths)) if j != k])
        if line.length < 600:
            continue
        for (x, y), (ax, ay), s in walk(coords, TREE_SPACING, TREE_SPACING / 2, 300):
            for side in (1, -1):
                nx, ny = ay * side, -ax * side
                tx, ty = x + nx * (p[4] / 2 + TREE_ROW_OFFSET), y + ny * (p[4] / 2 + TREE_ROW_OFFSET)
                if in_park(tx, ty, 300) and not path_area.buffer(150).contains(Point(tx, ty)) \
                        and not space.near_building(Point(tx, ty), 400) and tree_spread.ok(tx, ty):
                    trees.append([round(tx), round(ty), round(0.85 + 0.3 * chance("tree", round(tx), round(ty)), 2)])
                    tree_spread.add(tx, ty)
        for n, ((x, y), (ax, ay), s) in enumerate(walk(coords, PATH_BENCH_SPACING, PATH_BENCH_SPACING / 2, 500)):
            side = 1 if n % 2 == 0 else -1
            nx, ny = ay * side, -ax * side
            # On the gravel at the path's edge, its back to the lawn fence.
            bx, by = x + nx * (p[4] / 2 - PATH_BENCH_INSET), y + ny * (p[4] / 2 - PATH_BENCH_INSET)
            if in_park(bx, by, 200) and off_rounds(bx, by, 100) and not other_walkways.contains(Point(bx, by)) \
                    and bench_spread.ok(bx, by) \
                    and not space.near_building(Point(bx, by), 200):
                benches.append([round(bx), round(by), yaw_of(-nx, -ny)])
                bench_spread.add(bx, by)
                # Every other bench has a bin just past one end, on the same edge.
                for along in ((1, -1) if len(benches) % 2 else ()):
                    ix, iy = bx + ax * along * BIN_BESIDE_BENCH, by + ay * along * BIN_BESIDE_BENCH
                    if in_park(ix, iy, 200) and off_rounds(ix, iy, 100) and not other_walkways.contains(Point(ix, iy)):
                        bins.append([round(ix), round(iy), yaw_of(-nx, -ny)])
                        break
        for n, ((x, y), (ax, ay), s) in enumerate(walk(coords, PATH_LAMP_SPACING, PATH_LAMP_SPACING / 2, 500)):
            nx, ny = -ay, ax
            lx, ly = x + nx * (p[4] / 2 + 50), y + ny * (p[4] / 2 + 50)
            if in_park(lx, ly, 150) and off_rounds(lx, ly, 100) and off_walkways(lx, ly) and lamp_spread.ok(lx, ly) \
                    and not space.near_building(Point(lx, ly), 200):
                lamps.append([round(lx), round(ly), yaw_of(-nx, -ny)])
                lamp_spread.add(lx, ly)

    # Benches round the middle, facing the flower bed, leaving the path ends free.
    for (x, y), r, bed in TISZA_ROUNDS:
        count = max(6, int(2 * math.pi * r / 700))
        for i in range(count):
            a = 2 * math.pi * (i + 0.5) / count
            bx, by = x + math.cos(a) * (r - 120), y + math.sin(a) * (r - 120)
            if not walkways.buffer(150).contains(Point(bx, by)) and bench_spread.ok(bx, by):
                benches.append([round(bx), round(by), yaw_of(-math.cos(a), -math.sin(a))])
                bench_spread.add(bx, by)
                # Every other bench round the bed has a bin beside it.
                b = a + BIN_BESIDE_BENCH / (r - 120)
                ix, iy = x + math.cos(b) * (r - 120), y + math.sin(b) * (r - 120)
                if i % 2 == 0 and not walkways.buffer(100).contains(Point(ix, iy)):
                    bins.append([round(ix), round(iy), yaw_of(-math.cos(b), -math.sin(b))])

    # Fill the lawns with more trees, away from the paths.
    minx, miny, maxx, maxy = park.bounds
    y = miny
    row = 0
    while y < maxy:
        x = minx + (TREE_FILL_SPACING / 2 if row % 2 else 0)
        while x < maxx:
            jx = x + (chance("fx", row, round(x)) - 0.5) * 500
            jy = y + (chance("fy", row, round(x)) - 0.5) * 500
            pt = Point(jx, jy)
            if in_park(jx, jy, 400) and not path_area.buffer(500).contains(pt) \
                    and not space.near_building(pt, 500) and tree_spread.ok(jx, jy) and chance("keep", row, round(x)) < 0.75:
                trees.append([round(jx), round(jy), round(0.8 + 0.4 * chance("ts", row, round(x)), 2)])
                tree_spread.add(jx, jy)
            x += TREE_FILL_SPACING
        y += TREE_FILL_SPACING * 0.87
        row += 1

    # Leaf piles on the lawn a few steps off the paths, clear of the trees.
    tree_points = unary_union([Point(t[0], t[1]).buffer(160) for t in trees])
    leaf_piles = []
    pile_spread = Spread(LEAF_PILE_MIN_GAP)
    for k, (line, p) in enumerate(zip(path_lines, paths)):
        if line.length < 1500:
            continue
        for n, ((x, y), (ax, ay), s) in enumerate(walk(list(line.coords), LEAF_PILE_SPACING, LEAF_PILE_SPACING / 2, 800)):
            side = 1 if chance("pileside", k, n) < 0.5 else -1
            nx, ny = ay * side, -ax * side
            lx, ly = x + nx * (p[4] / 2 + LEAF_PILE_OUT), y + ny * (p[4] / 2 + LEAF_PILE_OUT)
            pt = Point(lx, ly)
            if chance("pile", k, n) < LEAF_PILE_CHANCE and in_park(lx, ly, 300) and off_rounds(lx, ly, 400) \
                    and not path_area.buffer(150).contains(pt) and not tree_points.contains(pt) and pile_spread.ok(lx, ly):
                leaf_piles.append([round(lx), round(ly), round(0.8 + 0.5 * chance("pilesize", k, n), 2)])
                pile_spread.add(lx, ly)

    # The keepers share the piles out by where they lie round the middle,
    # each taking a run of neighbouring ones: [x, y, pile index, ...].
    by_angle = sorted(range(len(leaf_piles)), key=lambda i: math.atan2(leaf_piles[i][1] - cy, leaf_piles[i][0] - cx))
    keepers = []
    for j in range(KEEPERS):
        start = j * len(by_angle) // KEEPERS
        run = by_angle[start:start + PILES_PER_KEEPER]
        if run:
            first = leaf_piles[run[0]]
            keepers.append([first[0] + 150, first[1]] + run)

    # Wire fences along both edges of every path, broken where other paths
    # cross and at the round places; hedges round the round places, open
    # where the paths come in.
    rounds_area = unary_union([Point(x, y).buffer(r + HEDGE_OFFSET + 60) for (x, y), r, _ in TISZA_ROUNDS])
    lawn_fences = []
    for k, (line, p) in enumerate(zip(path_lines, paths)):
        others = unary_union([l.buffer(q[4] / 2 + 40) for j, (l, q) in enumerate(zip(path_lines, paths)) if j != k])
        keep_off = unary_union([others, rounds_area, park.exterior.buffer(250)])
        for side in ("left", "right"):
            edge = line.parallel_offset(p[4] / 2 + LAWN_FENCE_OFFSET, side)
            for piece in getattr(edge.difference(keep_off), "geoms", [edge.difference(keep_off)]):
                if piece.geom_type != "LineString" or piece.length < LAWN_FENCE_MIN:
                    continue
                pts = list(piece.coords)
                for a, b in zip(pts[:-1], pts[1:]):
                    lawn_fences.append([round(a[0]), round(a[1]), round(b[0]), round(b[1])])

    hedges = []
    hedge_gaps = walkways.buffer(HEDGE_GAP)
    for (x, y), r, _ in TISZA_ROUNDS:
        ring = Point(x, y).buffer(r + HEDGE_OFFSET, 16).exterior
        rest = ring.difference(hedge_gaps)
        for piece in getattr(rest, "geoms", [rest]):
            if piece.geom_type != "LineString" or piece.length < 100:
                continue
            pts = list(piece.coords)
            for a, b in zip(pts[:-1], pts[1:]):
                hedges.append([round(a[0]), round(a[1]), round(b[0]), round(b[1])])

    # The railing just inside the edge, with gaps where paths come in.
    fence_ring = park.buffer(-FENCE_INSET).simplify(100)
    fence_ring = fence_ring if fence_ring.geom_type == "Polygon" else max(fence_ring.geoms, key=lambda g: g.area)
    fence = []
    entrances = [Point(p[0], p[1]) for p in paths[:len(corners)]]
    coords = list(fence_ring.exterior.coords)
    for a, b in zip(coords[:-1], coords[1:]):
        seg = LineString([a, b])
        cuts = sorted(seg.project(e) for e in entrances if seg.distance(e) < 800)
        s = 0.0
        for c in cuts + [seg.length + FENCE_GAP]:
            e = min(c - FENCE_GAP / 2, seg.length)
            if e - s > 100:
                p0 = seg.interpolate(s)
                p1 = seg.interpolate(e)
                fence.append([round(p0.x), round(p0.y), round(p1.x), round(p1.y)])
            s = max(s, c + FENCE_GAP / 2)

    return {
        "paths": paths,
        "rounds": rounds,
        "trees": trees,
        "benches": benches,
        "bins": bins,
        "lamps": lamps,
        "fence": fence,
        "lawn_fences": lawn_fences,
        "hedges": hedges,
        "leaf_piles": leaf_piles,
        "keepers": keepers,
        "middle": [round(cx), round(cy)],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("graybox")
    parser.add_argument("output")
    args = parser.parse_args()

    with open(args.graybox, encoding="utf-8") as f:
        data = json.load(f)
    space = Space(data)

    tisza = min(space.parks, key=lambda p: p.centroid.distance(Point(*TISZA_PARK_NEAR)))
    lamps = place_lamps(data, space, tisza)
    columns = place_columns(data, space, tisza)
    bins = []
    benches = place_park_benches(space, tisza, bins)
    bins += place_lamp_bins(lamps, space, tisza)
    park = layout_tisza(space, tisza)
    sidewalks = place_sidewalks(data, space, tisza, park)

    out = {
        "version": 1,
        "source": "OpenStreetMap contributors, ODbL; lamp after Fortepan 100450",
        "lamps": lamps,
        "columns": columns,
        "benches": benches,
        "bins": bins,
        "sidewalks": sidewalks,
        "tisza": park,
    }
    with open(args.output, "w", encoding="utf-8") as f:
        json.dump(out, f, separators=(",", ":"))
    print(f"{len(lamps)} lamps, {len(columns)} columns, {len(benches)} benches, {len(bins)} bins, {len(sidewalks)} sidewalks; "
          f"Tisza Kálmán tér: {len(park['paths'])} paths, {len(park['trees'])} trees, "
          f"{len(park['benches'])} benches, {len(park['bins'])} bins, {len(park['lamps'])} lamps, {len(park['fence'])} railing runs, "
          f"{len(park['lawn_fences'])} lawn fence and {len(park['hedges'])} hedge pieces, "
          f"{len(park['leaf_piles'])} leaf piles, {len(park['keepers'])} keepers")


if __name__ == "__main__":
    main()
