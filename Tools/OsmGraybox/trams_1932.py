"""Tram routes of 1932, laid on today's OSM streets.

The lines come from the BSzKRt network schematic of 1932 (print number
M.92/932, Budapest City Archives, kozlekedes.bparchiv.hu). That map names the
streets with tram tracks but not the stops, so stops are placed at the main
crossings along each line.

Each line is a list of streets in running order. The track follows those
streets on the OSM street network, sits in the middle of streets mapped as two
one-way halves, and each direction gets its own track, keeping left.
"""

import heapq
import math

from shapely.geometry import LineString, Point

# Streets with trams in 1932 inside the prototype area, each line from one map
# edge to another. "from"/"to" say which end of the first/last street to use.
# Karpfenstein utca of 1932 is today's Dobozi utca.
LINES_1932 = [
    {"name": "Nagykörút", "streets": ["Erzsébet körút", "József körút"], "from": "north", "to": "south"},
    {"name": "Rákóczi út - Kerepesi út", "streets": ["Rákóczi út", "Baross tér", "Kerepesi út"], "from": "west", "to": "east"},
    {"name": "Népszínház utca - Salgótarjáni utca",
     "streets": ["Rákóczi út", "Népszínház utca", "Salgótarjáni utca"], "from": "west", "to": "east"},
    {"name": "Bezerédj utca",
     "streets": ["Rákóczi út", "Népszínház utca", "Csokonai utca", "Bezerédj utca", "II. János Pál pápa tér",
                 "Baross tér", "Kerepesi út"],
     "from": "west", "to": "east"},
    {"name": "Karpfenstein utca", "streets": ["Rákóczi út", "Népszínház utca", "Dobozi utca"], "from": "west", "to": "south"},
    {"name": "Fiumei út", "streets": ["Kerepesi út", "Baross tér", "Fiumei út"], "from": "east", "to": "south"},
]

# Each track sits this far left of the street's middle (m): double track, 1435 mm gauge.
TRACK_OFFSET = 1.6
# Two halves of one street are at most this far apart (m) to share a middle.
PAIR_DISTANCE = 30.0
# Points either side whose shifts to the middle are compared (resampled points).
MEDIAN_WINDOW = 4
# Cost per metre of other streets, to get from one street of a line to the
# next (the street being followed costs 1).
OFF_LINE_COST = 4.0
# A line within this distance (m) of a line built before it shares its track.
SHARE_DISTANCE = 16.0
# Resample the track every this many metres before offsetting.
RESAMPLE_STEP = 4.0
# Stops: at least this far apart and from the route ends (m), and this far
# before the crossing they serve.
STOP_SPACING = 280.0
STOP_END_ROOM = 60.0
STOP_BEFORE_CROSSING = 25.0
STOP_SHARE_DISTANCE = 60.0
STOP_ROADS = {"primary", "secondary", "tertiary"}
# Stops the story needs, (name, map metres x east, y north): a track passing
# within EXTRA_STOP_REACH gets a stop at its nearest point, and the automatic
# stops too close to it are dropped.
EXTRA_STOPS = [
    ("Teleki tér", (1057.15, -293.55)),
    ("Népszínház utca (Bözsi néni)", (583.92, -212.68)),
]
EXTRA_STOP_REACH = 100.0
# Stops are named after the street crossed, by its 1932 name.
NAMES_1932 = {
    "II. János Pál pápa tér": "Tisza Kálmán tér",
    "Teleki László tér": "Teleki tér",
    "Dobozi utca": "Karpfenstein utca",
}
EXTRA_STOP_ROOM = 150.0

TRAM_HIGHWAYS = {
    "primary", "primary_link", "secondary", "secondary_link", "tertiary", "tertiary_link",
    "unclassified", "residential", "living_street", "service",
}

SIDE_KEY = {
    "north": lambda p: -p[1],
    "south": lambda p: p[1],
    "east": lambda p: -p[0],
    "west": lambda p: p[0],
}


class StreetGraph:
    def __init__(self, ways, nodes, proj, clip):
        self.pos = {}
        self.edges = {}
        self.street_nodes = {}
        self.street_lines = {}
        for way in ways.values():
            tags = way["tags"]
            if tags.get("highway") not in TRAM_HIGHWAYS:
                continue
            name = tags.get("name", "")
            refs = [r for r in way["refs"] if r in nodes]
            for r in refs:
                self.pos[r] = proj.to_metres(*nodes[r])
            pts = [self.pos[r] for r in refs]
            if len(pts) >= 2:
                self.street_lines.setdefault(name, []).append(LineString(pts))
            for a, b in zip(refs, refs[1:]):
                length = math.dist(self.pos[a], self.pos[b])
                self.edges.setdefault(a, []).append((b, length, name))
                self.edges.setdefault(b, []).append((a, length, name))
            if name:
                self.street_nodes.setdefault(name, set()).update(
                    r for r in refs if clip.contains(Point(self.pos[r])))

    def path(self, start, goals, street):
        """Cheapest node path from start to the nearest of the goal nodes,
        preferring the named street."""
        best = {start: 0.0}
        came = {}
        heap = [(0.0, start)]
        reached = None
        while heap:
            cost, node = heapq.heappop(heap)
            if node in goals and node != start:
                reached = node
                break
            if cost > best[node]:
                continue
            for nxt, length, name in self.edges.get(node, ()):
                c = cost + length * (1.0 if name == street else OFF_LINE_COST)
                if c < best.get(nxt, math.inf):
                    best[nxt] = c
                    came[nxt] = node
                    heapq.heappush(heap, (c, nxt))
        if reached is None:
            return None
        out = [reached]
        while out[-1] != start:
            out.append(came[out[-1]])
        return out[::-1]


def line_path(graph, line):
    streets = line["streets"]
    for s in streets:
        if not graph.street_nodes.get(s):
            raise ValueError(f"tram line {line['name']}: street {s!r} is not in the map")
    start = min(graph.street_nodes[streets[0]], key=lambda r: SIDE_KEY[line["from"]](graph.pos[r]))
    goal = min(graph.street_nodes[streets[-1]], key=lambda r: SIDE_KEY[line["to"]](graph.pos[r]))
    nodes = [start]
    for i, street in enumerate(streets):
        # Follow this street until the first node of the next one.
        goals = graph.street_nodes[streets[i + 1]] if i + 1 < len(streets) else {goal}
        leg = graph.path(nodes[-1], goals, street)
        if leg is None:
            raise ValueError(f"tram line {line['name']}: no way along {street!r}")
        nodes.extend(leg[1:])
    # Drop back-and-forth spikes where two legs met.
    clean = []
    for n in nodes:
        if len(clean) >= 2 and clean[-2] == n:
            clean.pop()
            continue
        if not clean or clean[-1] != n:
            clean.append(n)
    return clean


def middle_of_street(graph, pts, streets):
    """Move points on one half of a street mapped as two one-way halves to
    the middle between them (where the tracks run)."""
    shifts = []
    for i, p in enumerate(pts):
        a = pts[max(0, i - 1)]
        b = pts[min(len(pts) - 1, i + 1)]
        heading = math.atan2(b[1] - a[1], b[0] - a[0])
        best = None
        for name in streets:
            for other in graph.street_lines.get(name, ()):
                d = other.distance(Point(p))
                if d < 3.0 or d > PAIR_DISTANCE or (best and d >= best[0]):
                    continue
                at = other.project(Point(p))
                q0 = other.interpolate(max(0.0, at - 2.0))
                q1 = other.interpolate(min(other.length, at + 2.0))
                other_heading = math.atan2(q1.y - q0.y, q1.x - q0.x)
                if abs(math.cos(other_heading - heading)) < 0.95:
                    continue
                # Only a twin half: the gap between must be across the street.
                q = other.interpolate(at)
                across = math.atan2(q.y - p[1], q.x - p[0])
                if abs(math.cos(across - heading)) > 0.3:
                    continue
                best = (d, (q.x, q.y))
        shifts.append(((best[1][0] - p[0]) / 2.0, (best[1][1] - p[1]) / 2.0) if best else (0.0, 0.0))
    # A point or two that missed (or wrongly found) its twin would kink the
    # track: take the median shift of the neighbours instead.
    out = []
    for i, p in enumerate(pts):
        near = shifts[max(0, i - MEDIAN_WINDOW):i + MEDIAN_WINDOW + 1]
        dx = sorted(s[0] for s in near)[len(near) // 2]
        dy = sorted(s[1] for s in near)[len(near) // 2]
        out.append((p[0] + dx, p[1] + dy))
    return out


def share_tracks(pts, centres):
    """Lines that run along the same street share one pair of tracks: snap
    points close to an earlier line onto it."""
    out = []
    for p in pts:
        pt = Point(p)
        best = min(centres, key=lambda c: c.distance(pt), default=None)
        if best is not None and best.distance(pt) < SHARE_DISTANCE:
            out.append(best.interpolate(best.project(pt)).coords[0])
        else:
            out.append(p)
    return out


def resample(line, step):
    n = max(2, int(line.length / step) + 1)
    return [line.interpolate(line.length * i / (n - 1)).coords[0] for i in range(n)]


def smooth(pts, passes=2):
    for _ in range(passes):
        pts = [pts[0]] + [
            ((a[0] + 2 * b[0] + c[0]) / 4.0, (a[1] + 2 * b[1] + c[1]) / 4.0)
            for a, b, c in zip(pts, pts[1:], pts[2:])
        ] + [pts[-1]]
    return pts


def stop_points(graph, path_nodes, track, own_streets):
    """The stops as (distance along the track in m, name): the story's
    stops (EXTRA_STOPS), then just before crossings, main roads first,
    spaced out. A stop is named after the street it crosses."""
    on_path = set(path_nodes)
    main, minor = [], []
    for n in path_nodes:
        names = {name for nxt, _, name in graph.edges.get(n, ()) if nxt not in on_path and name} - own_streets
        names = {NAMES_1932.get(name, name) for name in names}
        if names:
            at = track.project(Point(graph.pos[n])) - STOP_BEFORE_CROSSING
            mains = sorted(names & {NAMES_1932.get(m, m) for m in graph.main_streets})
            if mains:
                main.append((at, mains[0]))
            else:
                minor.append((at, sorted(names)[0]))
    stops = []
    for name, (x, y) in EXTRA_STOPS:
        if track.distance(Point(x, y)) < EXTRA_STOP_REACH:
            stops.append((track.project(Point(x, y)), name))
    extras = list(stops)
    for at, name in sorted(main) + sorted(minor):
        if (STOP_END_ROOM < at < track.length - STOP_END_ROOM and all(abs(at - s) > STOP_SPACING for s, _ in stops)
                and all(abs(at - s) > EXTRA_STOP_ROOM for s, _ in extras)):
            stops.append((at, name))
    return sorted(stops)


def share_stops(stops, track, stop_places):
    """Lines sharing a track share its stops (and their names): move a stop
    onto an earlier line's stop nearby on the same track."""
    out = []
    for at, name in stops:
        here = track.interpolate(at)
        for place, place_name in stop_places:
            if track.distance(place) < 1.5 and place.distance(here) < STOP_SHARE_DISTANCE:
                at, name = track.project(place), place_name
                break
        else:
            stop_places.append((here, name))
        if all(abs(at - s) > 1.0 for s, _ in out):
            out.append((at, name))
    return sorted(out)


def build_1932_tram_routes(ways, nodes, proj, clip, to_ue):
    """Routes in the JSON's "trams" format plus the track lines (map metres)
    for drawing the rails."""
    graph = StreetGraph(ways, nodes, proj, clip)
    graph.main_streets = {
        w["tags"]["name"] for w in ways.values()
        if w["tags"].get("highway") in STOP_ROADS and w["tags"].get("name")
    }
    inner = clip.buffer(-5.0)
    routes, tracks, centres, stop_places = [], [], [], []
    for line in LINES_1932:
        path_nodes = line_path(graph, line)
        centre = LineString([graph.pos[n] for n in path_nodes])
        pts = middle_of_street(graph, resample(centre, RESAMPLE_STEP), set(line["streets"]))
        pts = share_tracks(pts, centres)
        centre = LineString(smooth(pts, passes=3)).simplify(0.2)
        centres.append(centre)
        for forward in (True, False):
            axis = centre if forward else LineString(centre.coords[::-1])
            # Positive offset is to the left in map metres (x east, y north).
            track = axis.offset_curve(TRACK_OFFSET)
            if track.geom_type != "LineString":
                track = max(track.geoms, key=lambda g: g.length)
            clipped = track.intersection(inner)
            if clipped.geom_type != "LineString":
                clipped = max(clipped.geoms, key=lambda g: g.length)
            if clipped.length < 1.0:
                continue
            if track.project(Point(clipped.coords[0])) > track.project(Point(clipped.coords[-1])):
                clipped = LineString(clipped.coords[::-1])
            stops = stop_points(graph, path_nodes if forward else path_nodes[::-1], clipped, set(line["streets"]))
            stops = share_stops(stops, clipped, stop_places)
            tracks.append(clipped)
            routes.append({
                "ref": "",
                "name": line["name"],
                "points": [c for x, y in clipped.coords for c in to_ue(x, y)],
                "stops": [{"at": round(s * 100), "name": name} for s, name in stops],
            })
    return routes, tracks
