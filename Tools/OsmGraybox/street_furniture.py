"""Place street furniture and lay out Tisza Kálmán tér from the graybox data.

Reads the graybox JSON written by osm_to_graybox.py and writes a second JSON
that the AStreetFurniture actor reads:

- lamps: the period street lamp (a slim pole with a scrolled crook and a
  hanging bell lantern, Fortepan 100450) along the pavements of every street
  wide enough to have them, facing the road;
- columns: round advertising columns (hirdetőoszlop) near street corners;
- benches: round the edges of the parks;
- sidewalks: the lines people walk along: round the pavements of every
  block, turning at its corners, over the road from corner to corner,
  and down the paths of Tisza Kálmán tér, as flat [x0, y0, x1, y1,
  ...] lists;
- bins: litter bins on a post, beside some street lamps and next to the
  park benches;
- taxi_stands: a rank of parked taxis by the kerb near the Népszínház and
  Keleti station, each {"name", "taxis": [[x, y, yaw], ...], "sign": [x, y, yaw]};
- taxi_destinations: where a taxi can take the player, a pavement spot at
  each, {"name", "at": [x, y, yaw]};
- food_vendors: street food sellers near story places, {"kind", "at":
  [x, y, yaw]} beside the pavement walking line, facing it;
- tram_stops: one per stop place on the tram tracks (lines sharing a track
  share it), {"name", "at": [x, y, yaw] on the track facing the way the
  tram runs, "sign": [x, y, yaw] on the kerb beside it facing the track,
  "wait": [x, y] where passengers wait and get off};
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
SIDEWALK_MIN = 300
SIDEWALK_SIMPLIFY = 50
# The pavement lines are cut where they turn more than this (degrees), and
# from each corner people cross to up to two corners across the road within
# this distance (cm).
SIDEWALK_CORNER_TURN = 40
SIDEWALK_MIN_CROSS = 400
SIDEWALK_MAX_CROSS = 2600
CROSSINGS_PER_CORNER = 2
# In the park people walk round the flower beds this far out from them.
ROUND_WALK_OUT = 300

# Taxi stands: a rank of taxis parked along the kerb as near as can be to
# each landmark's front, clear of the traffic lanes and tram tracks, with a
# "TAXI" sign on the pavement beside the first.
TAXI_STANDS = [
    ("N\u00e9psz\u00ednh\u00e1z", (-6700, 200)),
    ("Keleti p\u00e1lyaudvar", (91200, -43178)),
]
# A stand laid out by hand instead, where the kerbs nearby are all too close
# to the traffic: Keleti's cab rank on Baross tér in front of the station,
# the taxis in a row along the front facing south, the sign at its head.
FIXED_TAXI_STANDS = {
    "Keleti p\u00e1lyaudvar": {
        "taxis": [[91000, -44700, 90.0], [91000, -44160, 90.0], [91000, -43620, 90.0]],
        "sign": [91250, -44950, 180.0],
    },
}
# Where taxis take the player: a name and a point near the place; the drop
# is on the nearest pavement walking line.
TAXI_DESTINATIONS = [
    ("N\u00e9psz\u00ednh\u00e1z", (-6700, 200)),
    ("Keleti p\u00e1lyaudvar", (91200, -43178)),
    ("B\u00f6zsi n\u00e9ni (home)", (58550, 21720)),
    ("Teleki t\u00e9r", (105715, 29355)),
    ("Tisza K\u00e1lm\u00e1n t\u00e9r", (61300, 1100)),
    ("Taxi company", (-12000, 3100)),
]
TAXIS_PER_STAND = 3
TAXI_SPACING = 540
TAXI_STEP = 270
TAXI_FROM_KERB = 120
TAXI_SEARCH = 5000
TAXI_CLEAR_OF_LANES = 340
TAXI_CLEAR_OF_TRAMS = 160
TAXI_SIGN_OUT = 160

# Litter bins: beside this share of the street lamps, and next to park
# benches, just past the bench's end (a bench is 180 cm long).
BIN_CHANCE = 0.3
BIN_RADIUS = 25
BIN_MIN_GAP = 2500
BIN_BESIDE_LAMP = 90
BIN_BESIDE_BENCH = 125

# Tisza Kálmán tér: the park polygon in the graybox has its middle near here.
TISZA_PARK_NEAR = (61300, 1100)
# The 1932 layout, traced from the 1944 aerial photo (Fortepan 109126) laid
# over the map: a round place with a flower bed in the middle, two smaller
# ones on the diagonal to the north-east and south-west corners, and the
# park's own frame, its north-south axis (TISZA_AXIS, pointing south) a little
# off the map's because the square is turned.
TISZA_MIDDLE = (60000, 500)
TISZA_AXIS = (0.297, 0.955)
TISZA_ROUNDS = [(TISZA_MIDDLE, 2600, 1300), ((63500, -11150), 1700, 550), ((56750, 11750), 1700, 550)]
# Twin paths run north and south from the middle either side of a long lawn,
# wide promenades west (to the Népopera) and east (to the statue), diagonals
# through the small round places to the corners and one to the south-east
# corner, arcs of a ring round the middle, and a ring just inside the railing.
TWIN_PATH_APART = 1000
PROMENADE_WIDTH = 600
RING_RADIUS = 7300
RING_ARCS = [(-125, -20), (100, 170)]
# The statue stands east of the middle on a gravel terrace, facing it.
STATUE_OUT = 6300
STATUE_TERRACE = 1400
# The big playground fills the south-east quarter: sand, with swings,
# seesaws, sandpits, a roundabout, a climbing frame, a slide and a drinking
# fountain, benches round it, children playing and their mothers and
# nannies on the benches. Sizes are those in ParkModel.h (cm).
PLAYGROUND_CLEAR = 300
PLAYGROUND_KIT = [
    # kind, length along its yaw, width across, how many
    ("sandpit", 550, 550, 4),
    ("swings", 300, 520, 4),
    ("seesaw", 440, 120, 4),
    ("roundabout", 320, 320, 2),
    ("climbing_frame", 260, 260, 2),
    ("slide", 480, 120, 2),
    ("fountain", 80, 80, 2),
]
SWING_SEATS = (-95, 95)
SWING_SEAT_HEIGHT = 50
SEESAW_END = 170
SEESAW_TILT = 8.0
SEESAW_PIVOT_HEIGHT = 45
ROUNDABOUT_RADIUS = 150
ROUNDABOUT_TOP = 44
PLAYGROUND_BENCH_SPACING = 900
# Shade trees in a row just inside the edge, between the benches.
PLAYGROUND_TREE_IN = 260
PLAYGROUND_RUNNERS = 14
PLAYGROUND_SHADE_TREES = 7
NANNY_CHANCE = 0.45
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


def split_at_corners(points):
    """Cuts a closed ring of points into runs that end at its sharp corners."""
    n = len(points)
    cuts = []
    for k in range(n):
        a, b, c = points[k - 1], points[k], points[(k + 1) % n]
        turn = abs(math.atan2((b[0] - a[0]) * (c[1] - b[1]) - (b[1] - a[1]) * (c[0] - b[0]),
                              (b[0] - a[0]) * (c[0] - b[0]) + (b[1] - a[1]) * (c[1] - b[1])))
        if turn > math.radians(SIDEWALK_CORNER_TURN):
            cuts.append(k)
    if len(cuts) < 2:
        # A ring without corners: cut it in halves so it has ends to turn at.
        cuts = [0, n // 2]
    runs = []
    for m, k in enumerate(cuts):
        end = cuts[(m + 1) % len(cuts)]
        run = [points[k]]
        i = k
        while True:
            i = (i + 1) % n
            run.append(points[i])
            if i == end:
                break
        runs.append(run)
    return runs


def place_sidewalks(data, space, tisza, park):
    """Walking lines along both pavements of each street, and the park paths."""
    blocked = unary_union([b.buffer(SIDEWALK_CLEAR) for b in space.buildings] + [tisza.buffer(-100)])
    if space.trams is not None:
        blocked = blocked.union(space.trams.buffer(SIDEWALK_CLEAR))
    blocked = prep(blocked)
    houses = prep(unary_union([b.buffer(SIDEWALK_CLEAR) for b in space.buildings]))
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

    # The pavements: the outline of all the streets together, pushed out to
    # the pavement, so each block has one line round it that turns its
    # corners. The outline is cut at the corners, and at each corner a
    # crossing goes over the road to the nearest corner across it, so every
    # line ends where others start and people turn or cross there.
    widened = []
    for street in data["centerlines"]:
        points = ring_points(street["points"])
        if street["highway"] == "service" or len(points) < 2:
            continue
        line = LineString(points)
        if line.length >= 100:
            widened.append(line.buffer(street.get("width", 0) / 2 + SIDEWALK_OUT, cap_style=2, join_style=2))
    streets = unary_union(widened)
    corners = []
    for shape in getattr(streets, "geoms", [streets]):
        for ring in [shape.exterior, *shape.interiors]:
            ring = LineString(ring.coords).simplify(SIDEWALK_SIMPLIFY)
            pieces = split_at_corners(list(ring.coords)[:-1])
            for piece in pieces:
                add(LineString(piece))
                corners.append(piece[0])
    crossed = set()
    for i, a in enumerate(corners):
        across = sorted((math.dist(a, b), j) for j, b in enumerate(corners)
                        if j != i and SIDEWALK_MIN_CROSS < math.dist(a, b) < SIDEWALK_MAX_CROSS)
        for length, j in across[:CROSSINGS_PER_CORNER]:
            key = (min(i, j), max(i, j))
            crossing = LineString([a, corners[j]])
            if key in crossed or houses.intersects(crossing):
                continue
            crossed.add(key)
            sidewalks.append([round(v) for xy in crossing.coords for v in xy])
    # The park paths, cut where they reach a round place. From each cut a
    # short way leads in to a walk round the flower bed, and that walk is
    # split where the ways come in, so every line ends where others start.
    rounds = [(Point(r[0], r[1]), r[2], r[3]) for r in park["rounds"]]
    places = unary_union([c.buffer(r) for c, r, _ in rounds])
    entries = [[] for _ in rounds]
    for p in park["paths"]:
        rest = LineString([(p[0], p[1]), (p[2], p[3])]).difference(places)
        for piece in getattr(rest, "geoms", [rest]):
            if piece.geom_type != "LineString" or piece.length < 100:
                continue
            sidewalks.append([round(v) for xy in piece.coords for v in xy])
            for end in (piece.coords[0], piece.coords[-1]):
                for k, (c, r, bed) in enumerate(rounds):
                    if abs(c.distance(Point(end)) - r) < 30:
                        entries[k].append(math.atan2(end[1] - c.y, end[0] - c.x))
                        ring = bed + ROUND_WALK_OUT
                        a = entries[k][-1]
                        sidewalks.append([round(end[0]), round(end[1]),
                                          round(c.x + math.cos(a) * ring), round(c.y + math.sin(a) * ring)])
    for k, (c, r, bed) in enumerate(rounds):
        angles = sorted(entries[k])
        ring = bed + ROUND_WALK_OUT
        for i, a0 in enumerate(angles):
            a1 = angles[(i + 1) % len(angles)] + (2 * math.pi if i + 1 == len(angles) else 0.0)
            steps = max(2, int((a1 - a0) * ring / 300))
            arc = [(c.x + math.cos(a0 + (a1 - a0) * t / steps) * ring, c.y + math.sin(a0 + (a1 - a0) * t / steps) * ring)
                   for t in range(steps + 1)]
            sidewalks.append([round(v) for xy in arc for v in xy])
    return sidewalks


def place_taxi_stands(data, space):
    """Taxi ranks by the kerb near the landmarks (see TAXI_STANDS)."""
    stands = []
    for name, anchor in TAXI_STANDS:
        if name in FIXED_TAXI_STANDS:
            stands.append({"name": name, **FIXED_TAXI_STANDS[name]})
            continue
        near = Point(anchor)
        spots = {}
        for k, street in enumerate(data["centerlines"]):
            width = street.get("width", 0)
            points = ring_points(street["points"])
            if street["highway"] == "service" or width < MIN_LAMP_WIDTH or len(points) < 2:
                continue
            line = LineString(points)
            if line.distance(near) > TAXI_SEARCH:
                continue
            for (x, y), (dx, dy), along in walk(points, TAXI_STEP, TAXI_STEP, TAXI_STEP):
                for side in (-1, 1):
                    out = width / 2 - TAXI_FROM_KERB
                    px, py = x - dy * out * side, y + dx * out * side
                    spot = Point(px, py)
                    if (spot.distance(near) > TAXI_SEARCH or space.near_lane(spot, TAXI_CLEAR_OF_LANES)
                            or space.near_building(spot, 200)
                            or (space.trams is not None and space.trams.distance(spot) < TAXI_CLEAR_OF_TRAMS)):
                        continue
                    spots[(k, side, round(along / TAXI_STEP))] = (px, py, dx, dy, side, spot.distance(near))
        best = None
        per = TAXI_SPACING // TAXI_STEP
        for (k, side, step), (px, py, dx, dy, sd, dist) in spots.items():
            rank = [spots.get((k, side, step + per * i)) for i in range(TAXIS_PER_STAND)]
            if all(rank):
                score = sum(t[5] for t in rank)
                if best is None or score < best[0]:
                    best = (score, rank)
        if best is None:
            print(f"No room for a taxi stand near {name}")
            continue
        rank = best[1]
        # Face the way the traffic runs on this side of the street: along
        # the nearest lane (lanes run in their direction of travel).
        first = Point(rank[0][0], rank[0][1])
        lane = min((space.lanes[i] for i in space.lane_tree.query(first.buffer(2000))), key=lambda l: l.distance(first))
        along = lane.project(first)
        a = lane.interpolate(max(along - 50, 0))
        b = lane.interpolate(min(along + 50, lane.length))
        lx, ly = unit(b.x - a.x, b.y - a.y)
        if lx * rank[0][2] + ly * rank[0][3] < 0:
            rank = rank[::-1]
        taxis = [[round(t[0]), round(t[1]), yaw_of(lx, ly)] for t in rank]
        first = rank[0]
        # The sign stands on the pavement beside the first taxi, facing the road.
        sx = first[0] - first[3] * TAXI_SIGN_OUT * first[4]
        sy = first[1] + first[2] * TAXI_SIGN_OUT * first[4]
        stands.append({"name": name, "taxis": taxis, "sign": [round(sx), round(sy), yaw_of(first[3] * first[4], -first[2] * first[4])]})
    return stands


def place_taxi_destinations(sidewalks):
    """The pavement spot nearest each of TAXI_DESTINATIONS."""
    lines = [LineString(ring_points(w)) for w in sidewalks]
    out = []
    for name, anchor in TAXI_DESTINATIONS:
        near = Point(anchor)
        line = min(lines, key=lambda l: l.distance(near))
        along = line.project(near)
        p = line.interpolate(along)
        q = line.interpolate(min(along + 50, line.length))
        o = line.interpolate(max(along - 50, 0))
        out.append({"name": name, "at": [round(p.x), round(p.y), yaw_of(q.x - o.x, q.y - o.y)]})
    return out


TRAM_STOP_SAME = 300
TRAM_KERB_SEARCH = 3000
TRAM_SIGN_IN = 100
TRAM_NO_KERB_OUT = 260


def place_tram_stops(data, space, sidewalks):
    """The stops of the tram routes, with a waiting spot on the pavement on
    the tram's outer side (away from the other track) and a sign at its
    kerb edge."""
    walks = [LineString(ring_points(w)) for w in sidewalks]
    walk_tree = STRtree(walks)
    tracks = [LineString(ring_points(r["points"])) for r in data["trams"]]
    stops = []
    for route, track in zip(data["trams"], tracks):
        for stop in route["stops"]:
            p = track.interpolate(stop["at"])
            if any(math.hypot(p.x - q["at"][0], p.y - q["at"][1]) < TRAM_STOP_SAME for q in stops):
                continue
            a = track.interpolate(max(stop["at"] - 100, 0))
            b = track.interpolate(min(stop["at"] + 100, track.length))
            dx, dy = unit(b.x - a.x, b.y - a.y)
            # Outward: away from the track the other way.
            nx, ny = -dy, dx
            other = min((t for t in tracks if t.distance(p) > 100), key=lambda t: t.distance(p), default=None)
            if other is not None and other.distance(p) < 800:
                q = other.interpolate(other.project(p))
                if nx * (q.x - p.x) + ny * (q.y - p.y) > 0:
                    nx, ny = -nx, -ny
            ray = LineString([(p.x + nx * 150, p.y + ny * 150), (p.x + nx * TRAM_KERB_SEARCH, p.y + ny * TRAM_KERB_SEARCH)])
            reach = None
            for i in walk_tree.query(ray):
                hit = walks[i].intersection(ray)
                if hit.is_empty:
                    continue
                for g in getattr(hit, "geoms", [hit]):
                    if g.geom_type != "Point":
                        continue
                    # Only a pavement running along the street, not a crossing.
                    w = walks[i]
                    at = w.project(g)
                    u = w.interpolate(min(at + 50, w.length))
                    v = w.interpolate(max(at - 50, 0))
                    wx, wy = unit(u.x - v.x, u.y - v.y)
                    if abs(wx * dx + wy * dy) > 0.85:
                        d = math.hypot(g.x - p.x, g.y - p.y)
                        reach = d if reach is None else min(reach, d)
            if reach is None:
                sign_out, wait_out = TRAM_NO_KERB_OUT, TRAM_NO_KERB_OUT
            else:
                sign_out, wait_out = reach - TRAM_SIGN_IN, reach
            stops.append({
                "name": stop.get("name", ""),
                "at": [round(p.x), round(p.y), yaw_of(dx, dy)],
                "sign": [round(p.x + nx * sign_out + dx * 150), round(p.y + ny * sign_out + dy * 150), yaw_of(-nx, -ny)],
                "wait": [round(p.x + nx * wait_out), round(p.y + ny * wait_out)],
            })
    return stops


# Street food sellers near story places: what they sell, a point near the
# place, or a spot laid out by hand [x, y, yaw] where no pavement is near.
FOOD_VENDORS = [
    ("chestnuts", (-6000, 1500), None),        # by the Népszínház
    ("sausages", None, [90700, -41500, 180.0]), # Baross tér, in front of Keleti
    ("pretzels", (105715, 29355), None),        # Teleki tér
    ("chestnuts", (61300, 1100), None),         # Tisza Kálmán tér
]
VENDOR_OFF_WALK = 150


def place_food_vendors(space, sidewalks):
    """Each seller a step off the nearest walking line, on the side away
    from the traffic, facing the passers-by."""
    lines = [LineString(ring_points(w)) for w in sidewalks]
    out = []
    for kind, anchor, fixed in FOOD_VENDORS:
        if fixed:
            out.append({"kind": kind, "at": fixed})
            continue
        near = Point(anchor)
        line = min(lines, key=lambda l: l.distance(near))
        along = line.project(near)
        p = line.interpolate(along)
        a = line.interpolate(max(along - 50, 0))
        b = line.interpolate(min(along + 50, line.length))
        dx, dy = unit(b.x - a.x, b.y - a.y)
        best = None
        for side in (1, -1):
            x, y = p.x - dy * VENDOR_OFF_WALK * side, p.y + dx * VENDOR_OFF_WALK * side
            spot = Point(x, y)
            if space.near_building(spot, 80):
                continue
            room = min((space.lanes[i].distance(spot) for i in space.lane_tree.query(spot.buffer(3000))), default=3000)
            if best is None or room > best[0]:
                best = (room, x, y, side)
        if best is None:
            print(f"No room for a {kind} seller near {anchor}")
            continue
        _, x, y, side = best
        out.append({"kind": kind, "at": [round(x), round(y), yaw_of(dy * side, -dx * side)]})
    return out


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


# The people in the playground: [kind, x, y, height above the ground, yaw].
CHILD_STANDING, CHILD_SEATED, GROWN_UP_SEATED, GROWN_UP_STANDING = range(4)


def layout_playground(park, at, along, walkways, ring):
    """The big playground in the south-east quarter, in the park's frame.

    Returns its frame (corners as [a, b] east and south of the middle), the
    sand, the things to play on as [kind, x, y, yaw] (kind is the index in
    PLAYGROUND_KIT), the people, and the benches round its edge."""
    vx, vy = along
    a0 = TWIN_PATH_APART + SIDE_PATH_WIDTH / 2 + PLAYGROUND_CLEAR
    b0 = STATUE_TERRACE / 2 + PLAYGROUND_CLEAR
    inner = ring.buffer(-SIDE_PATH_WIDTH / 2 - PLAYGROUND_CLEAR)

    def box(a_lo, b_lo, a_hi, b_hi):
        return Polygon([at(a_lo, b_lo), at(a_hi, b_lo), at(a_hi, b_hi), at(a_lo, b_hi)])

    # The biggest frame that fits inside the ring path.
    best = None
    for a1 in range(int(a0) + 3000, 25000, 200):
        if not inner.contains(box(a0, b0, a1, b0 + 100)):
            break
        for b1 in range(int(b0) + 3000, 25000, 200):
            if not inner.contains(box(a0, b0, a1, b1)):
                break
            if best is None or (a1 - a0) * (b1 - b0) > (best[0] - a0) * (best[1] - b0):
                best = (a1, b1)
    a1, b1 = best
    frame = [[a0, b0], [a1, b0], [a1, b1], [a0, b1]]
    # A strip in the game is longer than its ends by its width, so the ends
    # sit half a width in from the frame's.
    if a1 - a0 >= b1 - b0:
        half = (b1 - b0) / 2
        s0, s1 = at(a0 + half, b0 + half), at(a1 - half, b0 + half)
    else:
        half = (a1 - a0) / 2
        s0, s1 = at(a0 + half, b0 + half), at(a0 + half, b1 - half)
    sand = [round(s0[0]), round(s0[1]), round(s1[0]), round(s1[1]), round(2 * half)]

    # Things to play on, each where it fits, off the path that crosses.
    yaw_a = yaw_of(vx, vy)
    yaw_b = round(yaw_a + 90.0, 1)
    blocked = walkways.buffer(PLAYGROUND_CLEAR)
    taken = []
    items = []
    people = []
    candidates = [(a, b) for a in range(int(a0) + 400, int(a1) - 300, 100) for b in range(int(b0) + 400, int(b1) - 300, 100)]
    # Each goes where it is farthest from the ones already there, so they
    # spread over the whole sand, tried at a few hundred spots.
    inside = box(a0 + 100, b0 + 100, a1 - 100, b1 - 100)
    placed = []
    for kind, (name, length, width, count) in enumerate(PLAYGROUND_KIT):
        for n in range(count):
            best = None
            for a, b in sorted(candidates, key=lambda c: chance("play", name, n, c[0], c[1]))[:400]:
                turned = chance("turn", name, n, a, b) < 0.5
                ha, hb = ((width, length) if turned else (length, width))
                ha, hb = ha / 2 + 150, hb / 2 + 150
                spot = box(a - ha, b - hb, a + ha, b + hb)
                if not inside.contains(spot) or spot.intersects(blocked) or any(spot.intersects(t) for t in taken):
                    continue
                room = min([math.hypot(a - pa, b - pb) for pa, pb in placed] + [1e9])
                if best is None or room > best[0]:
                    best = (room, a, b, turned, spot)
            if best is None:
                continue
            _, a, b, turned, spot = best
            taken.append(spot)
            placed.append((a, b))
            x, y = at(a, b)
            yaw = yaw_b if turned else yaw_a
            items.append([kind, round(x), round(y), yaw])
            people += playground_people(name, (x, y), yaw, (kind, n))

    # Children running about on the open sand.
    runners = 0
    for a, b in sorted(candidates, key=lambda c: chance("run", c[0], c[1])):
        point = Point(at(a, b))
        if runners >= PLAYGROUND_RUNNERS:
            break
        if not point.buffer(80).intersects(blocked) and not any(t.contains(point) for t in taken):
            x, y = at(a, b)
            people.append([CHILD_STANDING, round(x), round(y), 0, round(360 * chance("runyaw", a, b), 1)])
            taken.append(point.buffer(150))
            runners += 1

    # Benches all round the edge, facing in, with mothers and nannies on
    # some, and shade trees between them.
    benches = []
    trees = []
    corners = [(a0, b0), (a1, b0), (a1, b1), (a0, b1)]
    for k in range(4):
        (pa, pb), (qa, qb) = corners[k], corners[(k + 1) % 4]
        length = math.hypot(qa - pa, qb - pb)
        da, db = (qa - pa) / length, (qb - pb) / length
        # In towards the middle of the frame.
        na, nb = db, -da
        if na * ((a0 + a1) / 2 - pa) + nb * ((b0 + b1) / 2 - pb) < 0:
            na, nb = -na, -nb
        s = PLAYGROUND_BENCH_SPACING / 2
        while s < length - PLAYGROUND_BENCH_SPACING / 2:
            ba, bb = pa + da * s + na * 60, pb + db * s + nb * 60
            x, y = at(ba, bb)
            fx, fy = at(ba + na, bb + nb)
            yaw = yaw_of(fx - x, fy - y)
            spot = Point(x, y).buffer(120)
            if not spot.intersects(blocked) and not any(spot.intersects(t) for t in taken):
                benches.append([round(x), round(y), yaw])
                if chance("nanny", round(x), round(y)) < NANNY_CHANCE:
                    people.append([GROWN_UP_SEATED, round(x + (fx - x) * 8), round(y + (fy - y) * 8), 45, yaw])
            ta, tb = pa + da * (s + PLAYGROUND_BENCH_SPACING / 2) + na * PLAYGROUND_TREE_IN, pb + db * (s + PLAYGROUND_BENCH_SPACING / 2) + nb * PLAYGROUND_TREE_IN
            tx, ty = at(ta, tb)
            spot = Point(tx, ty).buffer(150)
            if s + PLAYGROUND_BENCH_SPACING < length and not spot.intersects(blocked) and not any(spot.intersects(t) for t in taken):
                trees.append([round(tx), round(ty), round(0.85 + 0.3 * chance("ptree", round(tx), round(ty)), 2)])
            s += PLAYGROUND_BENCH_SPACING

    # A few big shade trees out on the sand, in the widest gaps.
    for n in range(PLAYGROUND_SHADE_TREES):
        best = None
        for a, b in sorted(candidates, key=lambda c: chance("shade", n, c[0], c[1]))[:400]:
            spot = Point(at(a, b)).buffer(250)
            if spot.intersects(blocked) or any(spot.intersects(t) for t in taken):
                continue
            room = min([math.hypot(a - pa, b - pb) for pa, pb in placed] + [1e9])
            if best is None or room > best[0]:
                best = (room, a, b, spot)
        if best:
            _, a, b, spot = best
            taken.append(spot)
            placed.append((a, b))
            x, y = at(a, b)
            trees.append([round(x), round(y), round(1.05 + 0.2 * chance("shadesize", n), 2)])

    return {"frame": frame, "sand": sand, "items": items, "people": people, "benches": benches, "trees": trees}


def playground_people(name, middle, yaw, key):
    """Who plays on one thing in the playground."""
    x, y = middle
    fx, fy = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    rx, ry = -fy, fx

    def spot(forward, right):
        return round(x + fx * forward + rx * right), round(y + fy * forward + ry * right)

    people = []
    if name == "swings":
        for i, seat in enumerate(SWING_SEATS):
            if i == 0 or chance("swing", *key) < 0.5:
                people.append([CHILD_SEATED, *spot(0, seat), SWING_SEAT_HEIGHT, yaw])
    elif name == "seesaw":
        rise = SEESAW_END * math.sin(math.radians(SEESAW_TILT))
        people.append([CHILD_SEATED, *spot(SEESAW_END, 0), round(SEESAW_PIVOT_HEIGHT + rise), round(yaw + 180, 1)])
        people.append([CHILD_SEATED, *spot(-SEESAW_END, 0), round(SEESAW_PIVOT_HEIGHT - rise), yaw])
    elif name == "sandpit":
        for i in range(2 + int(chance("dig", *key) * 2)):
            a = 2 * math.pi * chance("digat", i, *key)
            px, py = spot(math.cos(a) * 140, math.sin(a) * 140)
            people.append([CHILD_STANDING, px, py, 15, yaw_of(x - px, y - py)])
        px, py = spot(0, 380)
        people.append([GROWN_UP_STANDING, px, py, 0, yaw_of(x - px, y - py)])
    elif name == "roundabout":
        px, py = spot(ROUNDABOUT_RADIUS * 0.6, 0)
        people.append([CHILD_STANDING, px, py, ROUNDABOUT_TOP, round(yaw + 90, 1)])
        px, py = spot(0, -(ROUNDABOUT_RADIUS + 45))
        people.append([CHILD_STANDING, px, py, 0, round(yaw, 1)])
    elif name == "climbing_frame":
        px, py = spot(-190, 40)
        people.append([CHILD_STANDING, px, py, 0, yaw])
    elif name == "slide":
        px, py = spot(-290, 0)
        people.append([CHILD_STANDING, px, py, 0, yaw])
    return people


def layout_tisza(space, park):
    """Paths, beds, trees, benches, lamps, railing, statue and playground for Tisza Kálmán tér."""
    cx, cy = TISZA_MIDDLE
    center_r = TISZA_ROUNDS[0][1]
    ux, uy = unit(*TISZA_AXIS)
    vx, vy = -uy, ux
    if vx < 0:
        vx, vy = -vx, -vy

    def at(a, b):
        """The point a cm east and b cm south of the middle, in the park's frame."""
        return (cx + a * vx + b * ux, cy + a * vy + b * uy)

    edge = park.exterior

    def to_edge(start, direction, short=100):
        """Where a path from start going this way reaches the railing."""
        far = (start[0] + direction[0] * 40000, start[1] + direction[1] * 40000)
        hit = LineString([start, far]).intersection(edge)
        points = [hit] if hit.geom_type == "Point" else list(getattr(hit, "geoms", []))
        if not points:
            return far
        nearest = min(points, key=lambda q: math.hypot(q.x - start[0], q.y - start[1]))
        return (nearest.x - direction[0] * short, nearest.y - direction[1] * short)

    paths = []
    entrances = []

    def add_path(a, b, width):
        paths.append([round(a[0]), round(a[1]), round(b[0]), round(b[1]), width])

    def add_ray(start, direction, width):
        end = to_edge(start, direction)
        add_path(start, end, width)
        entrances.append(Point(end))

    # The twin paths north and south of the middle.
    for a in (-TWIN_PATH_APART, TWIN_PATH_APART):
        b = math.sqrt(center_r ** 2 - a ** 2)
        add_ray(at(a, -b), (-ux, -uy), SIDE_PATH_WIDTH)
        add_ray(at(a, b), (ux, uy), SIDE_PATH_WIDTH)
    # The promenades west and east; the statue's terrace on the east one.
    add_ray(at(-center_r, 0), (-vx, -vy), PROMENADE_WIDTH)
    statue = at(STATUE_OUT, 0)
    add_path(at(center_r, 0), at(STATUE_OUT - STATUE_TERRACE / 2, 0), PROMENADE_WIDTH)
    add_path(at(STATUE_OUT - STATUE_TERRACE / 2, 0), at(STATUE_OUT + STATUE_TERRACE / 2, 0), STATUE_TERRACE)
    add_ray(at(STATUE_OUT + STATUE_TERRACE / 2, 0), (vx, vy), PATH_WIDTH)
    # Diagonals through the small round places to the corners, and one
    # straight to the south-east corner, across the playground.
    corners = list(park.simplify(400).exterior.coords)[:-1]
    for (rx, ry), r, _ in TISZA_ROUNDS[1:]:
        dx, dy = unit(rx - cx, ry - cy)
        add_path((cx + dx * center_r, cy + dy * center_r), (rx - dx * r, ry - dy * r), PATH_WIDTH)
        corner = min(corners, key=lambda c: math.hypot(c[0] - rx, c[1] - ry))
        ex, ey = unit(corner[0] - rx, corner[1] - ry)
        add_ray((rx + ex * r, ry + ey * r), (ex, ey), PATH_WIDTH)
    south_east = max(corners, key=lambda c: (c[0] - cx) * vx + (c[1] - cy) * vy + (c[0] - cx) * ux + (c[1] - cy) * uy)
    dx, dy = unit(south_east[0] - cx, south_east[1] - cy)
    add_ray((cx + dx * center_r, cy + dy * center_r), (dx, dy), PATH_WIDTH)
    # A path east from the south-west round place to the twin paths.
    (rx, ry), r, _ = TISZA_ROUNDS[2]
    ra = (rx - cx) * vx + (ry - cy) * vy
    rb = (rx - cx) * ux + (ry - cy) * uy
    add_path(at(ra + r, rb), at(TWIN_PATH_APART, rb), SIDE_PATH_WIDTH)
    # The arcs of the ring round the middle.
    for start, end in RING_ARCS:
        steps = max(2, int((end - start) / 8))
        points = [at(RING_RADIUS * math.cos(math.radians(start + (end - start) * i / steps)),
                     RING_RADIUS * math.sin(math.radians(start + (end - start) * i / steps))) for i in range(steps + 1)]
        for p0, p1 in zip(points[:-1], points[1:]):
            add_path(p0, p1, SIDE_PATH_WIDTH)
    # The ring path just inside the railing.
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

    playground = layout_playground(park, at, (vx, vy), walkways, ring)
    # Trees, lamps, leaf piles and lawn fences keep off the playground and the terrace.
    keep_clear = unary_union([Polygon([at(*c) for c in playground.pop("frame")]).buffer(100),
                              Point(statue).buffer(STATUE_TERRACE)])

    def clear(x, y, margin=0):
        return not keep_clear.buffer(margin).contains(Point(x, y)) if margin else not keep_clear.contains(Point(x, y))

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
                        and not space.near_building(Point(tx, ty), 400) and clear(tx, ty, 300) and tree_spread.ok(tx, ty):
                    trees.append([round(tx), round(ty), round(0.85 + 0.3 * chance("tree", round(tx), round(ty)), 2)])
                    tree_spread.add(tx, ty)
        for n, ((x, y), (ax, ay), s) in enumerate(walk(coords, PATH_BENCH_SPACING, PATH_BENCH_SPACING / 2, 500)):
            side = 1 if n % 2 == 0 else -1
            nx, ny = ay * side, -ax * side
            # On the gravel at the path's edge, its back to the lawn fence.
            bx, by = x + nx * (p[4] / 2 - PATH_BENCH_INSET), y + ny * (p[4] / 2 - PATH_BENCH_INSET)
            if in_park(bx, by, 200) and off_rounds(bx, by, 100) and not other_walkways.contains(Point(bx, by)) \
                    and bench_spread.ok(bx, by) and clear(bx, by) \
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
            if in_park(lx, ly, 150) and off_rounds(lx, ly, 100) and off_walkways(lx, ly) and clear(lx, ly) and lamp_spread.ok(lx, ly) \
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

    # Benches either side of the statue, facing it, and the playground's own.
    for side in (-1, 1):
        for along in (-350, 350):
            bx, by = at(STATUE_OUT + along, side * (STATUE_TERRACE / 2 - PATH_BENCH_INSET))
            benches.append([round(bx), round(by), yaw_of(-side * ux, -side * uy)])
    benches += playground.pop("benches")
    trees += playground.pop("trees")

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
                    and not space.near_building(pt, 500) and clear(jx, jy, 400) and tree_spread.ok(jx, jy) and chance("keep", row, round(x)) < 0.75:
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
                    and not path_area.buffer(150).contains(pt) and not tree_points.contains(pt) and clear(lx, ly, 200) and pile_spread.ok(lx, ly):
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
        keep_off = unary_union([others, rounds_area, park.exterior.buffer(250), keep_clear])
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
        "statue": [round(statue[0]), round(statue[1]), yaw_of(-vx, -vy)],
        "playground": playground,
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
    taxi_stands = place_taxi_stands(data, space)
    taxi_destinations = place_taxi_destinations(sidewalks)
    tram_stops = place_tram_stops(data, space, sidewalks)
    food_vendors = place_food_vendors(space, sidewalks)

    out = {
        "version": 1,
        "source": "OpenStreetMap contributors, ODbL; lamp after Fortepan 100450",
        "lamps": lamps,
        "columns": columns,
        "benches": benches,
        "bins": bins,
        "sidewalks": sidewalks,
        "taxi_stands": taxi_stands,
        "taxi_destinations": taxi_destinations,
        "tram_stops": tram_stops,
        "food_vendors": food_vendors,
        "tisza": park,
    }
    with open(args.output, "w", encoding="utf-8") as f:
        json.dump(out, f, separators=(",", ":"))
    print(f"{len(lamps)} lamps, {len(columns)} columns, {len(benches)} benches, {len(bins)} bins, {len(sidewalks)} sidewalks, {len(taxi_stands)} taxi stands, {len(tram_stops)} tram stops; "
          f"Tisza Kálmán tér: {len(park['paths'])} paths, {len(park['trees'])} trees, "
          f"{len(park['benches'])} benches, {len(park['bins'])} bins, {len(park['lamps'])} lamps, {len(park['fence'])} railing runs, "
          f"{len(park['lawn_fences'])} lawn fence and {len(park['hedges'])} hedge pieces, "
          f"{len(park['leaf_piles'])} leaf piles, {len(park['keepers'])} keepers, "
          f"playground {len(park['playground']['items'])} things, {len(park['playground']['people'])} people")


if __name__ == "__main__":
    main()
