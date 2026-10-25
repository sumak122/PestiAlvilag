"""Turn an OpenStreetMap export (.osm) into graybox data for Unreal.

The output is a JSON file that the AOsmGraybox actor reads to build simple
3D blocks: a ground plane, road surfaces, parks, tram and rail tracks, and
extruded buildings. Street centerlines are written too, for the traffic work
that comes later.

Coordinates in the output are Unreal centimetres around an origin point:
X points east, Y points south (Unreal is left-handed, so north is -Y), Z is up.

Usage:
    pip install -r requirements.txt
    python osm_to_graybox.py source/blaha_test_area.osm \
        ../../Data/OsmGraybox/blaha_test_area.json
"""

import argparse
import json
import math
import xml.etree.ElementTree as ET

import mapbox_earcut
import numpy as np
from shapely.geometry import LineString, MultiPolygon, Point, Polygon, box
from shapely.geometry.polygon import orient
from shapely.ops import nearest_points, polygonize, unary_union
from shapely.prepared import prep

from trams_1932 import build_1932_tram_routes

# Blaha Lujza tér, the heart of the prototype area.
DEFAULT_ORIGIN = (47.49655, 19.07040)

# Full carriageway width in metres when OSM gives no width or lane count.
ROAD_WIDTHS = {
    "primary": 14.0,
    "primary_link": 7.0,
    "secondary": 11.0,
    "secondary_link": 7.0,
    "tertiary": 9.0,
    "tertiary_link": 6.0,
    "unclassified": 7.0,
    "residential": 7.0,
    "living_street": 5.0,
    "service": 4.0,
    "pedestrian": 6.0,
}
LANE_WIDTH = 3.2
TRACK_WIDTH = {"tram": 2.4, "rail": 3.0}
PARK_TAGS = {
    ("leisure", "park"),
    ("leisure", "garden"),
    ("leisure", "playground"),
    ("landuse", "grass"),
    ("landuse", "recreation_ground"),
}

# Building parts mapped onto the street are trimmed back to this far inside
# the carriageway edge (OSM footprints and guessed road widths don't always agree).
CARRIAGEWAY_TRIM = 0.3
# Canopies (building=roof/carport): a thin roof on posts.
CANOPY_HEIGHT = 4.0
CANOPY_THICKNESS = 1.0
CANOPY_TYPES = ("roof", "carport")

# A wall gets a dressed street facade when the street (road or square,
# widened by a pavement) starts within FRONT_PROBE metres of it.
FRONT_SIDEWALK = 6.0
FRONT_PROBE = 3.0
FRONT_MIN_EDGE = 4.0
FRONT_MIN_HEIGHT = 6.0

# Pest tenements of the period have tall storeys.
STOREY_HEIGHT = 3.8
DEFAULT_LEVELS = 4


class LocalProjection:
    """Flat-earth projection around an origin, good to a few cm over 2 km."""

    def __init__(self, lat0, lon0):
        phi = math.radians(lat0)
        self.lat0 = lat0
        self.lon0 = lon0
        self.m_per_deg_lat = 111132.92 - 559.82 * math.cos(2 * phi) + 1.175 * math.cos(4 * phi)
        self.m_per_deg_lon = 111412.84 * math.cos(phi) - 93.5 * math.cos(3 * phi)

    def to_metres(self, lat, lon):
        """Map metres: x east, y north."""
        return ((lon - self.lon0) * self.m_per_deg_lon, (lat - self.lat0) * self.m_per_deg_lat)

    def to_latlon(self, x, y):
        return (self.lat0 + y / self.m_per_deg_lat, self.lon0 + x / self.m_per_deg_lon)


# Buildings the game puts up itself from code (hero buildings): left out
# of the graybox so they don't stand inside them. r9906133: Keleti pályaudvar.
HERO_BUILDINGS = {"r9906133"}

# Buildings that are too new for 1932 and would stand in the way: the two
# entrance pavilions of the Köztársaság tér / II. János Pál pápa tér metro
# station (2014) in Tisza Kálmán tér.
MODERN_BUILDINGS = {"w269953709", "w272486842"}

# Footprints smaller than this are modern street clutter (tram shelters,
# kiosks, toilets). With no height in OSM they would come out as four-storey
# pillars, and 1932 had none of them, so they are left out.
MIN_BUILDING_AREA = 25.0

# Baross tér in front of Keleti is mapped as it is today: the 1960s flyover
# from Rottenbiller utca to Fiumei út, little roads across the old statue
# island, and the lawns and paths laid out round the metro. They are left out
# and the square of the early 1930s is put back (baross_ter_1932, after the
# 1944 aerial photo, Fortepan 109125).
MODERN_WAYS = {
    "25302553",                                  # Baross téri felüljáró, the flyover
    "238184611", "293201856", "317434806", "864544656",  # across the statue island
    "25444187", "592321939", "1344877828",       # through the roundabout's lawn
    "350672482",                                 # the paved path across the square
    "697548409", "1348982432", "1348982434", "1348982435",  # today's lawns
    "1348982436", "1348982437", "1348982438",
    "615542063",                                 # paved forecourt north of the hall
}

# Positions on Baross tér are measured from the middle of Keleti's main front
# (AKeletiStation::FrontAt), in Unreal cm: X east, Y south.
KELETI_FRONT = (92766, -43178)
# The roundabout where Rákóczi út, Fiumei út and the two sides of the square
# meet: its ring runs through the OSM node at the end of Rákóczi út, its
# middle a little north of east from there (degrees), clear of the houses.
ROUNDABOUT_NODE = "659498280"
ROUNDABOUT_HEADING = 12.0
ROUNDABOUT_RADIUS = 20.5
ROUNDABOUT_LAWN = 15.0
ROUNDABOUT_POINTS = 24
# Today's node at the start of the square's south side, joined to the ring.
ROUNDABOUT_EXIT = "262558625"
# The gravel island with the Baross statue, its outline round from the west
# tip; its east end stops at the road that still crosses from side to side.
STATUE_ISLAND = [(-16160, 6560), (-15340, 5640), (-13940, 4700), (-12300, 3700), (-11600, 3500),
                 (-11600, 5400), (-11245, 6065), (-9500, 7400), (-9400, 8500), (-10180, 8880),
                 (-12500, 9040), (-14850, 8780), (-15970, 7770)]
# Two lawns on the island's west part, cut to the island.
ISLAND_LAWNS = [[(-16380, 5780), (-15040, 5750), (-14900, 7940), (-16250, 7960)],
                [(-13660, 7300), (-12610, 7290), (-12560, 8200), (-13600, 8210)]]
# The long lawn with round ends in front of the south half of the station front.
LONG_LAWN = ((-2320, 2990), (-2320, 5120), 750)
# The tram terminus: three tracks side by side, the trams waiting on them;
# at each end they come together and run into the line on the square's south
# side, the loop round the island's east end.
TERMINUS_TRACKS = [
    [(-5950, 8400), (-5950, 1400), (-6400, 400), (-7300, -100)],
    [(-5200, 8400), (-5200, 1400), (-5700, 300), (-7300, -100)],
    [(-4100, 8400), (-4100, 1400), (-4700, 100), (-6000, -350), (-7300, -100)],
    [(-7300, -100), (-9000, 1000), (-10000, 2400), (-10100, 3600), (-9950, 5000), (-8800, 6000), (-7150, 7600),
     (-6900, 8600), (-6600, 9700)],
    [(-5950, 8400), (-5700, 9700)],
    [(-5200, 8400), (-5200, 8700), (-5700, 9700)],
    [(-4100, 8400), (-4300, 9000), (-5700, 9700)],
]


def from_keleti(local):
    """A Baross tér position (cm from Keleti's front, X east, Y south) in map metres."""
    return ((KELETI_FRONT[0] + local[0]) / 100.0, -(KELETI_FRONT[1] + local[1]) / 100.0)


def baross_ter_1932(nodes, ways, proj):
    """Put back the early-1930s Baross tér: drops the modern ways, adds the
    roundabout's ring, and returns the island, lawns and terminus tracks
    (map metres) for the area layers."""
    for wid in MODERN_WAYS:
        ways.pop(wid, None)

    # The ring, clockwise as traffic kept left (the node order is
    # anticlockwise; one-way main roads run against it, see traffic_direction).
    hub = proj.to_metres(*nodes[ROUNDABOUT_NODE])
    heading = math.radians(ROUNDABOUT_HEADING)
    middle = (hub[0] + math.cos(heading) * ROUNDABOUT_RADIUS, hub[1] + math.sin(heading) * ROUNDABOUT_RADIUS)
    refs = [ROUNDABOUT_NODE]
    for i in range(1, ROUNDABOUT_POINTS):
        a = heading + math.pi + 2.0 * math.pi * i / ROUNDABOUT_POINTS
        ref = f"baross-ring-{i}"
        nodes[ref] = proj.to_latlon(middle[0] + math.cos(a) * ROUNDABOUT_RADIUS, middle[1] + math.sin(a) * ROUNDABOUT_RADIUS)
        refs.append(ref)
    refs.append(ROUNDABOUT_NODE)
    road = {"highway": "primary", "name": "Baross tér", "lanes": "2"}
    ways["baross-ring"] = {"refs": refs, "tags": dict(road, junction="roundabout")}
    exit_at = proj.to_metres(*nodes[ROUNDABOUT_EXIT])
    nearest = min(refs[1:-1], key=lambda r: math.dist(proj.to_metres(*nodes[r]), exit_at))
    ways["baross-ring-exit"] = {"refs": [nearest, ROUNDABOUT_EXIT], "tags": road}

    (x0, y0), (x1, y1), half = LONG_LAWN
    start, end = from_keleti((x0, y0)), from_keleti((x1, y1))
    island = Polygon([from_keleti(p) for p in STATUE_ISLAND])
    lawns = [Polygon([from_keleti(p) for p in lawn]).intersection(island.buffer(-1.0)) for lawn in ISLAND_LAWNS]
    return {
        "plazas": [island],
        "parks": [Point(middle).buffer(ROUNDABOUT_LAWN, 48), LineString([start, end]).buffer(half / 100.0, 24)] + lawns,
        "trams": [LineString([from_keleti(p) for p in track]) for track in TERMINUS_TRACKS],
    }


def parse_osm(path):
    root = ET.parse(path).getroot()
    nodes = {}
    for n in root.iter("node"):
        nodes[n.get("id")] = (float(n.get("lat")), float(n.get("lon")))
    ways = {}
    for w in root.iter("way"):
        ways[w.get("id")] = {
            "refs": [nd.get("ref") for nd in w.iter("nd")],
            "tags": {t.get("k"): t.get("v") for t in w.iter("tag")},
        }
    relations = []
    for r in root.iter("relation"):
        relations.append({
            "id": r.get("id"),
            "members": [(m.get("type"), m.get("ref"), m.get("role")) for m in r.iter("member")],
            "tags": {t.get("k"): t.get("v") for t in r.iter("tag")},
        })
    b = root.find("bounds")
    bounds = tuple(float(b.get(k)) for k in ("minlat", "minlon", "maxlat", "maxlon"))
    return nodes, ways, relations, bounds


def way_coords(way, nodes, proj):
    pts = [proj.to_metres(*nodes[r]) for r in way["refs"] if r in nodes]
    return pts


def is_closed(way):
    refs = way["refs"]
    return len(refs) >= 4 and refs[0] == refs[-1]


def is_underground(tags):
    try:
        if int(tags.get("layer", "0")) < 0:
            return True
    except ValueError:
        pass
    return tags.get("location") == "underground" or tags.get("tunnel") not in (None, "no")


def multipolygon_geometry(rel, ways, nodes, proj):
    outer_lines, inner_lines = [], []
    for mtype, ref, role in rel["members"]:
        if mtype != "way" or ref not in ways:
            continue
        pts = way_coords(ways[ref], nodes, proj)
        if len(pts) < 2:
            continue
        (inner_lines if role == "inner" else outer_lines).append(LineString(pts))
    outers = unary_union(list(polygonize(outer_lines)))
    if outers.is_empty:
        return None
    inners = unary_union(list(polygonize(inner_lines)))
    return outers.difference(inners) if not inners.is_empty else outers


def polygons_of(geom):
    if geom is None or geom.is_empty:
        return []
    if isinstance(geom, Polygon):
        return [geom]
    if isinstance(geom, MultiPolygon):
        return list(geom.geoms)
    if hasattr(geom, "geoms"):
        return [g for sub in geom.geoms for g in polygons_of(sub)]
    return []


def to_ue(x, y):
    """Map metres (x east, y north) to Unreal cm (X east, Y south)."""
    return (round(x * 100.0), round(-y * 100.0))


def polygon_record(poly):
    """Rings as flat Unreal XY lists plus earcut triangles.

    Rings are oriented so that, in Unreal XY, the exterior has positive
    shoelace area and holes negative: the solid is always on the left of each
    edge. The actor relies on this to point walls outwards.
    """
    # Flipping Y mirrors the shape, so orient the opposite way in map space.
    poly = orient(poly, sign=-1.0)
    rings = [poly.exterior] + list(poly.interiors)
    flat_rings, verts, ring_ends = [], [], []
    for ring in rings:
        coords = list(ring.coords)[:-1]
        ue = []
        for x, y in coords:
            ux, uy = to_ue(x, y)
            if ue and ue[-1] == (ux, uy):
                continue
            ue.append((ux, uy))
        if len(ue) > 1 and ue[0] == ue[-1]:
            ue.pop()
        if len(ue) < 3:
            if ring is poly.exterior:
                return None
            continue
        verts.extend(ue)
        ring_ends.append(len(verts))
        flat_rings.append([c for p in ue for c in p])
    tris = mapbox_earcut.triangulate_float64(
        np.array(verts, dtype=np.float64).reshape(-1, 2), np.array(ring_ends, dtype=np.uint32)
    )
    if len(tris) == 0:
        return None
    return {"rings": flat_rings, "tris": [int(i) for i in tris]}


def building_height_m(tags):
    if tags.get("building") in CANOPY_TYPES and "height" not in tags and "building:levels" not in tags:
        return CANOPY_HEIGHT
    h = tags.get("height", "").replace("m", "").strip()
    try:
        if h:
            return max(3.0, float(h))
    except ValueError:
        pass
    try:
        levels = float(tags.get("building:levels", DEFAULT_LEVELS))
    except ValueError:
        levels = DEFAULT_LEVELS
    return max(1.0, levels) * STOREY_HEIGHT


def building_base_m(tags):
    """Height of the underside, for bridges and overhangs above the street."""
    h = tags.get("min_height", "").replace("m", "").strip()
    try:
        if h:
            return max(0.0, float(h))
    except ValueError:
        pass
    try:
        base = max(0.0, float(tags.get("building:min_level", 0))) * STOREY_HEIGHT
    except ValueError:
        base = 0.0
    if base == 0.0 and tags.get("building") in CANOPY_TYPES:
        base = max(0.0, building_height_m(tags) - CANOPY_THICKNESS)
    return base


def building_levels(tags):
    """Storeys above ground from OSM, or None."""
    try:
        levels = int(float(tags.get("building:levels", "")))
    except ValueError:
        return None
    return levels if levels > 0 else None


def street_edges(rec, poly, street_area):
    """Indices of the wall edges (counted through all rings in order, edge i
    running from point i to point i+1) whose outside faces a street."""
    front, index = [], 0
    for flat in rec["rings"]:
        pts = [(flat[i] / 100.0, -flat[i + 1] / 100.0) for i in range(0, len(flat), 2)]
        for i, (x0, y0) in enumerate(pts):
            x1, y1 = pts[(i + 1) % len(pts)]
            length = math.hypot(x1 - x0, y1 - y0)
            if length >= FRONT_MIN_EDGE:
                mx, my = (x0 + x1) / 2.0, (y0 + y1) / 2.0
                nx, ny = (y1 - y0) / length, -(x1 - x0) / length
                if poly.contains(Point(mx + nx, my + ny)):
                    nx, ny = -nx, -ny
                if street_area.contains(Point(mx + nx * FRONT_PROBE, my + ny * FRONT_PROBE)):
                    front.append(index)
            index += 1
    return front


def road_width_m(tags):
    for key in ("width", "est_width"):
        try:
            return max(2.0, float(tags[key].replace("m", "").strip()))
        except (KeyError, ValueError):
            pass
    try:
        return max(1, int(tags["lanes"])) * LANE_WIDTH
    except (KeyError, ValueError):
        return ROAD_WIDTHS[tags["highway"]]


# Roads that AI traffic drives on, and its cruising speed there in km/h.
TRAFFIC_SPEEDS = {
    "primary": 40,
    "primary_link": 30,
    "secondary": 35,
    "secondary_link": 30,
    "tertiary": 30,
    "tertiary_link": 25,
    "unclassified": 25,
    "residential": 25,
    "living_street": 15,
}


# Main roads mapped today as split one-way pairs (often with the tram between
# them). They stay one-way, with the direction flipped for left-hand traffic.
MAIN_ROADS = {"primary", "primary_link", "secondary", "secondary_link", "tertiary", "tertiary_link"}
# Side streets narrower than this keep their one-way rule; wider ones carry
# traffic both ways, as most did in 1932.
MIN_TWO_WAY_WIDTH = 6.0


def traffic_direction(tags, width):
    """"both", "forward" or "backward" along the way's node order."""
    oneway = tags.get("oneway", "no")
    if tags.get("junction") == "roundabout" and oneway == "no":
        oneway = "yes"
    if oneway in ("yes", "true", "1"):
        if tags["highway"] in MAIN_ROADS:
            return "backward"
        return "forward" if width < MIN_TWO_WAY_WIDTH else "both"
    if oneway == "-1":
        if tags["highway"] in MAIN_ROADS:
            return "forward"
        return "backward" if width < MIN_TWO_WAY_WIDTH else "both"
    return "both"


# Cars are 1.7 m wide; keep lane centres this far from walls so they don't scrape.
WALL_CLEARANCE = 1.3
# Buildings whose underside is higher than this are bridges cars drive under.
DRIVE_UNDER_HEIGHT = 2.5


def keep_clear_of_walls(lane, walls):
    """Nudge lane points away from building walls that crowd the street."""
    if walls is None or lane.distance(walls) >= WALL_CLEARANCE:
        return lane
    pts = list(lane.segmentize(3.0).coords)
    for i, (x, y) in enumerate(pts):
        for _ in range(3):
            here = Point(x, y)
            gap = here.distance(walls)
            if gap >= WALL_CLEARANCE:
                break
            wall = nearest_points(here, walls)[1]
            dx, dy = x - wall.x, y - wall.y
            norm = math.hypot(dx, dy)
            if norm < 1e-6:
                break  # inside a wall; leave it rather than guess a side
            push = WALL_CLEARANCE - gap
            x, y = x + dx / norm * push, y + dy / norm * push
        pts[i] = (x, y)
    return LineString(pts)


def build_lanes(ways, nodes, proj, clip, walls=None):
    """Directed driving lanes for AI traffic, keeping LEFT as Budapest did in 1932.

    Ways are split at junctions (nodes shared by two or more drivable ways).
    Two-way pieces get one lane each way, offset to the left of travel; one-way
    pieces get a single lane down the middle (see traffic_direction). A one-way
    lane that would trap cars at a dead end gets a lane back as well. Lanes
    leaving the map are dropped, so cars turn round at the map edge.
    """
    drivable = {}
    for wid, way in ways.items():
        tags = way["tags"]
        if tags.get("highway") in TRAFFIC_SPEEDS and not is_underground(tags) and tags.get("area") != "yes":
            refs = [r for r in way["refs"] if r in nodes]
            if len(refs) >= 2:
                drivable[wid] = (refs, tags)

    use_count = {}
    for refs, _ in drivable.values():
        for r in set(refs):
            use_count[r] = use_count.get(r, 0) + 1

    inner = clip.buffer(-5.0)
    node_index = {}

    def node_id(ref):
        return node_index.setdefault(ref, len(node_index))

    # Street pieces between junctions: (way id, node refs, points, direction, width, speed).
    pieces = []
    for wid, (refs, tags) in drivable.items():
        width = road_width_m(tags)
        speed = round(TRAFFIC_SPEEDS[tags["highway"]] / 0.036)
        direction = traffic_direction(tags, width)
        start = 0
        for i in range(1, len(refs)):
            if i < len(refs) - 1 and use_count.get(refs[i], 0) < 2:
                continue
            piece = refs[start:i + 1]
            start = i
            pts = [proj.to_metres(*nodes[r]) for r in piece]
            line = LineString(pts)
            if line.length >= 1.0 and inner.contains(line):
                pieces.append(["w" + wid, piece, pts, direction, width, speed])

    # Open up one-way pieces that lead into a dead end until every lane has a way out.
    while True:
        outgoing = {}
        for _, piece, _, direction, _, _ in pieces:
            if direction in ("both", "forward"):
                outgoing[piece[0]] = outgoing.get(piece[0], 0) + 1
            if direction in ("both", "backward"):
                outgoing[piece[-1]] = outgoing.get(piece[-1], 0) + 1
        changed = False
        for p in pieces:
            _, piece, _, direction, _, _ = p
            end = piece[-1] if direction == "forward" else piece[0]
            if direction != "both" and not outgoing.get(end):
                p[3] = "both"
                changed = True
        if not changed:
            break

    lanes = []
    for wid, piece, pts, direction, width, speed in pieces:
        forward = (LineString(pts), piece[0], piece[-1])
        backward = (LineString(pts[::-1]), piece[-1], piece[0])
        if direction == "both":
            offset = min(width / 4.0, 2.5)
            paths = [forward, backward]
        else:
            offset = 0.0
            paths = [forward if direction == "forward" else backward]
        for path, a, b in paths:
            # Positive offset_curve distance is to the left of travel.
            lane = path.offset_curve(offset, join_style="mitre", mitre_limit=2.0) if offset else path
            if lane.is_empty or lane.geom_type != "LineString":
                lane = path
            lane = keep_clear_of_walls(lane, walls)
            lanes.append({
                "from": node_id(a),
                "to": node_id(b),
                "way": wid,
                "speed": speed,
                "points": [c for x, y in lane.coords for c in to_ue(x, y)],
            })
    return lanes


def convert(osm_path, origin):
    nodes, ways, relations, (minlat, minlon, maxlat, maxlon) = parse_osm(osm_path)
    proj = LocalProjection(*origin)
    baross = baross_ter_1932(nodes, ways, proj)
    x0, y0 = proj.to_metres(minlat, minlon)
    x1, y1 = proj.to_metres(maxlat, maxlon)
    clip = box(x0, y0, x1, y1)

    buildings, roads, plazas, parks, trams, rails, centerlines = [], [], [], [], [], [], []
    carriageways = []

    def add_area(target, geom, tags):
        for poly in polygons_of(geom.intersection(clip) if geom is not None else None):
            if poly.area > 1.0:
                target.append((poly, tags))

    for wid, way in ways.items():
        tags = way["tags"]
        if not tags:
            continue
        pts = way_coords(way, nodes, proj)
        if len(pts) < 2:
            continue
        building = tags.get("building")
        highway = tags.get("highway")
        railway = tags.get("railway")

        if building and building != "no" and is_closed(way) and not is_underground(tags):
            footprint = Polygon(pts).buffer(0)
            if "w" + wid in HERO_BUILDINGS | MODERN_BUILDINGS or footprint.area < MIN_BUILDING_AREA:
                continue
            add_area(buildings, footprint, dict(tags, id="w" + wid))
        elif any((k, tags.get(k)) in PARK_TAGS for k in ("leisure", "landuse")) and is_closed(way):
            add_area(parks, Polygon(pts).buffer(0), tags)
        elif highway == "pedestrian" and tags.get("area") == "yes" and is_closed(way):
            add_area(plazas, Polygon(pts).buffer(0), tags)
        elif highway in ROAD_WIDTHS and not is_underground(tags) and tags.get("area") != "yes":
            line = LineString(pts)
            width = road_width_m(tags)
            target = plazas if highway == "pedestrian" else roads
            add_area(target, line.buffer(width / 2.0, cap_style="flat", join_style="round"), tags)
            if highway in TRAFFIC_SPEEDS:
                carriageways.append(line.buffer(width / 2.0 - CARRIAGEWAY_TRIM, cap_style="flat"))
            if highway != "pedestrian":
                clipped = line.intersection(clip)
                for part in getattr(clipped, "geoms", [clipped]):
                    if isinstance(part, LineString) and len(part.coords) >= 2:
                        centerlines.append({
                            "id": "w" + wid,
                            "highway": highway,
                            "name": tags.get("name", ""),
                            "oneway": tags.get("oneway", "no"),
                            "width": round(width * 100),
                            "points": [c for x, y in part.coords for c in to_ue(x, y)],
                        })
        elif railway == "rail" and not is_underground(tags):
            add_area(rails, LineString(pts).buffer(TRACK_WIDTH[railway] / 2.0, cap_style="flat"), tags)

    for poly in baross["plazas"]:
        add_area(plazas, poly, {})
    for poly in baross["parks"]:
        add_area(parks, poly, {})

    for rel in relations:
        tags = rel["tags"]
        if tags.get("type") != "multipolygon" or is_underground(tags):
            continue
        is_building = tags.get("building") not in (None, "no")
        is_park = any((k, tags.get(k)) in PARK_TAGS for k in ("leisure", "landuse"))
        if not (is_building or is_park):
            continue
        geom = multipolygon_geometry(rel, ways, nodes, proj)
        if geom is None:
            continue
        geom = geom.buffer(0)
        if is_building:
            if "r" + rel["id"] in HERO_BUILDINGS | MODERN_BUILDINGS or geom.area < MIN_BUILDING_AREA:
                continue
            add_area(buildings, geom, dict(tags, id="r" + rel["id"]))
        else:
            add_area(parks, geom, tags)

    def merged_layer(name, items):
        merged = unary_union([p for p, _ in items]) if items else None
        records = [polygon_record(p) for p in polygons_of(merged)]
        return {"layer": name, "polygons": [r for r in records if r]}

    # Cut away building corners that stick out into a street cars drive on.
    # Bridges and canopies over the street stay whole.
    carriageway = unary_union(carriageways) if carriageways else None
    trimmed = []
    for poly, tags in buildings:
        if carriageway is not None and building_base_m(tags) < DRIVE_UNDER_HEIGHT and poly.intersects(carriageway):
            for part in polygons_of(poly.difference(carriageway)):
                if part.area > 2.0:
                    trimmed.append((part, tags))
        else:
            trimmed.append((poly, tags))
    buildings = trimmed

    # Walls that face a street or square get a dressed facade in Unreal.
    street_area = prep(unary_union([p for p, _ in roads + plazas]).buffer(FRONT_SIDEWALK)) if roads else None

    out_buildings = []
    for poly, tags in buildings:
        rec = polygon_record(poly)
        if rec is None:
            continue
        rec["id"] = tags["id"]
        rec["name"] = tags.get("name", "")
        rec["height"] = round(building_height_m(tags) * 100)
        base = round(building_base_m(tags) * 100)
        if 0 < base < rec["height"]:
            rec["base"] = base
        levels = building_levels(tags)
        if levels:
            rec["levels"] = levels
        if street_area is not None and base == 0 and rec["height"] >= FRONT_MIN_HEIGHT * 100:
            front = street_edges(rec, poly, street_area)
            if front:
                rec["front"] = front
        out_buildings.append(rec)

    walls = unary_union([p for p, tags in buildings if building_base_m(tags) < DRIVE_UNDER_HEIGHT]) if buildings else None

    # Today's tram tracks are left out: the 1932 network was different.
    tram_routes, tram_tracks = build_1932_tram_routes(ways, nodes, proj, clip, to_ue)
    for track in tram_tracks + baross["trams"]:
        add_area(trams, track.buffer(TRACK_WIDTH["tram"] / 2.0, cap_style="flat"), {})

    gx0, gy0 = to_ue(x0, y1)  # north-west corner becomes min X, min Y
    gx1, gy1 = to_ue(x1, y0)
    return {
        "version": 1,
        "source": "OpenStreetMap contributors, ODbL",
        "origin": {"lat": origin[0], "lon": origin[1]},
        "units": "cm, X east, Y south, Z up",
        "bounds": {"minX": gx0, "minY": gy0, "maxX": gx1, "maxY": gy1},
        "areas": [
            merged_layer("road", roads),
            merged_layer("plaza", plazas),
            merged_layer("park", parks),
            merged_layer("tram", trams),
            merged_layer("rail", rails),
        ],
        "buildings": out_buildings,
        "centerlines": centerlines,
        "lanes": build_lanes(ways, nodes, proj, clip, walls),
        "trams": tram_routes,
    }


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("osm")
    ap.add_argument("out")
    ap.add_argument("--origin", nargs=2, type=float, default=DEFAULT_ORIGIN, metavar=("LAT", "LON"))
    args = ap.parse_args()
    data = convert(args.osm, tuple(args.origin))
    with open(args.out, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, separators=(",", ":"))
    print(
        f"{len(data['buildings'])} buildings, "
        + ", ".join(f"{a['layer']}: {len(a['polygons'])}" for a in data["areas"])
        + f", {len(data['centerlines'])} centerlines, {len(data['lanes'])} traffic lanes"
        + f", {len(data['trams'])} tram routes"
    )


if __name__ == "__main__":
    main()
