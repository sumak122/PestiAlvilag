"""Bus lines of 1932, driven along the AI traffic lanes.

The lines come from the map of the Székesfővárosi Autóbusz Üzem (city bus
company), 1932, which Daniel found. Inside the prototype area it shows:

- 12 along the Nagykörút (Erzsébet körút - József körút), through Blaha Lujza tér;
- 20 from its terminus by Keleti pályaudvar up Thököly út;
- 5 / 5A along Dohány utca and up Rottenbiller utca (which it meets at the map edge).

(9 only touches the southern map edge at Horváth Mihály tér and is left out.)

Buses keep LEFT with the rest of the traffic, so each route is a chain of
the traffic lanes (already offset to the left of travel) from one map edge
to another, following the line's streets. A line with both ends at the map
edge runs both ways as two routes; a line with its terminus inside the map
comes in, turns round the block at the terminus and goes back out, as one
route. Stops are placed before the main crossings, named after the street
crossed (by its 1932 name).
"""

import heapq
import math

from trams_1932 import NAMES_1932

LINES_1932 = [
    {"ref": "12", "name": "Nagykörút", "streets": ["Erzsébet körút", "József körút"],
     "ends": ["north", "south"]},
    # Dohány utca meets Rottenbiller utca right by the northern map edge,
    # so inside the map line 5 is Dohány utca end to end (across Rózsák tere).
    {"ref": "5", "name": "Dohány utca - Rottenbiller utca", "streets": ["Dohány utca", "Rózsák tere"],
     "ends": ["west", "north"]},
    # The terminus in front of Keleti's main front (UE cm), like the tram stop there.
    {"ref": "20", "name": "Keleti pályaudvar - Thököly út", "streets": ["Thököly út", "Baross tér"],
     "ends": ["north"], "terminus": ("Keleti pályaudvar", (86166.0, -39178.0))},
]

# Which way each map edge lies, in UE (X east, Y south).
EDGE = {"north": (0.0, -1.0), "south": (0.0, 1.0), "east": (1.0, 0.0), "west": (-1.0, 0.0)}
# Cost per cm of streets not on the line (the line's own cost 1).
OFF_LINE_COST = 4.0
# Buses cruise at this speed (cm/s, about 22 km/h).
BUS_SPEED = 600
# Corners are rounded: resample every this many cm, then average neighbours this many times.
RESAMPLE_STEP = 200.0
SMOOTH_PASSES = 4
# Stops: at least this far apart and from the route's ends, and this far before the crossing (cm).
STOP_SPACING = 30000.0
STOP_END_ROOM = 6000.0
STOP_BEFORE_CROSSING = 2500.0
# Crossing streets get a stop, main roads first (in this order), then any other named street.
STOP_ROADS = ["primary", "secondary", "tertiary"]
# Streets of the line mapped one-way today were two-way for the buses: they
# get a lane back the other way, this far left of the one-way lane (cm).
BACK_LANE_OFFSET = 240.0


def lane_points(lane):
    p = lane["points"]
    return [(p[i], p[i + 1]) for i in range(0, len(p), 2)]


def length_of(pts):
    return sum(math.dist(pts[i], pts[i + 1]) for i in range(len(pts) - 1))


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1]


class LaneGraph:
    def __init__(self, lanes, centerlines, two_way=()):
        self.names = {c["id"]: c["name"] for c in centerlines}
        self.kinds = {c["id"]: c["highway"] for c in centerlines}
        self.lanes = list(lanes) + back_lanes(lanes, self.names, two_way)
        lanes = self.lanes
        self.pts = [lane_points(l) for l in lanes]
        self.lengths = [max(length_of(p), 1.0) for p in self.pts]
        self.leaving = {}
        self.arriving = {}
        for i, l in enumerate(lanes):
            self.leaving.setdefault(l["from"], []).append(i)
            self.arriving.setdefault(l["to"], []).append(i)

    def street(self, i):
        return self.names.get(self.lanes[i]["way"], "")

    def edge_lane(self, streets, edge, inward):
        """The lane of these streets nearest the map edge, heading in (or out)."""
        e = EDGE[edge]
        best, best_key = None, None
        for i, pts in enumerate(self.pts):
            if self.street(i) not in streets:
                continue
            heading = (pts[-1][0] - pts[0][0], pts[-1][1] - pts[0][1])
            if (dot(heading, e) < 0) != inward:
                continue
            key = dot(pts[0] if inward else pts[-1], e)
            if best_key is None or key > best_key:
                best, best_key = i, key
        return best

    def nearest_lane(self, point, streets):
        best, best_d = None, None
        for i, pts in enumerate(self.pts):
            if self.street(i) not in streets:
                continue
            d = min(math.dist(point, p) for p in pts)
            if best_d is None or d < best_d:
                best, best_d = i, d
        return best

    def path(self, start, goal, streets):
        """Lanes from start to goal (both included), never doubling back on the same street piece."""
        def cost(i):
            return self.lengths[i] * (1.0 if self.street(i) in streets else OFF_LINE_COST)

        best = {start: 0.0}
        came = {}
        queue = [(0.0, start)]
        while queue:
            c, i = heapq.heappop(queue)
            if i == goal:
                out = [goal]
                while out[-1] != start:
                    out.append(came[out[-1]])
                return out[::-1]
            if c > best[i]:
                continue
            here = self.lanes[i]
            for j in self.leaving.get(here["to"], []):
                there = self.lanes[j]
                if there["way"] == here["way"] and there["to"] == here["from"]:
                    continue
                nc = c + cost(j)
                if nc < best.get(j, math.inf):
                    best[j] = nc
                    came[j] = i
                    heapq.heappush(queue, (nc, j))
        return None


def offset_left(pts, d):
    """The line d to the left of travel (UE: X east, Y south, so left of (dx, dy) is (dy, -dx))."""
    out = []
    for k, p in enumerate(pts):
        a = pts[max(k - 1, 0)]
        b = pts[min(k + 1, len(pts) - 1)]
        dx, dy = b[0] - a[0], b[1] - a[1]
        n = math.hypot(dx, dy) or 1.0
        out.append((p[0] + dy / n * d, p[1] - dx / n * d))
    return out


def back_lanes(lanes, names, two_way):
    """A lane back the other way for every one-way piece of these streets."""
    pieces = {}
    for l in lanes:
        pieces.setdefault((l["way"], min(l["from"], l["to"]), max(l["from"], l["to"])), []).append(l)
    out = []
    for (way, _, _), found in pieces.items():
        if len(found) != 1 or names.get(way, "") not in two_way:
            continue
        l = found[0]
        pts = offset_left(lane_points(l)[::-1], BACK_LANE_OFFSET)
        out.append({"from": l["to"], "to": l["from"], "way": way, "speed": l["speed"],
                    "points": [c for p in pts for c in p]})
    return out


def resample(pts, step):
    out = [pts[0]]
    carry = 0.0
    for a, b in zip(pts, pts[1:]):
        seg = math.dist(a, b)
        t = step - carry
        while t <= seg:
            out.append((a[0] + (b[0] - a[0]) * t / seg, a[1] + (b[1] - a[1]) * t / seg))
            t += step
        carry = seg - (t - step)
    if math.dist(out[-1], pts[-1]) > 1.0:
        out.append(pts[-1])
    return out


def smooth(pts, passes):
    for _ in range(passes):
        pts = [pts[0]] + [((a[0] + 2 * b[0] + c[0]) / 4.0, (a[1] + 2 * b[1] + c[1]) / 4.0)
                          for a, b, c in zip(pts, pts[1:], pts[2:])] + [pts[-1]]
    return pts


def chain(graph, lanes):
    pts = []
    for i in lanes:
        for p in graph.pts[i]:
            if not pts or math.dist(pts[-1], p) > 1.0:
                pts.append(p)
    return pts


def distances(pts):
    out = [0.0]
    for a, b in zip(pts, pts[1:]):
        out.append(out[-1] + math.dist(a, b))
    return out


def project(pts, along, point):
    best, best_d = 0.0, None
    for i in range(len(pts) - 1):
        a, b = pts[i], pts[i + 1]
        dx, dy = b[0] - a[0], b[1] - a[1]
        seg2 = dx * dx + dy * dy
        t = 0.0 if seg2 == 0 else max(0.0, min(1.0, ((point[0] - a[0]) * dx + (point[1] - a[1]) * dy) / seg2))
        q = (a[0] + dx * t, a[1] + dy * t)
        d = math.dist(q, point)
        if best_d is None or d < best_d:
            best, best_d = along[i] + math.sqrt(seg2) * t, d
    return best


def crossings(graph, lanes, streets):
    """(junction point, name of the main street crossed there) along the lanes, in order."""
    out = []
    for i in lanes[:-1]:
        node = graph.lanes[i]["to"]
        names = []
        for j in graph.leaving.get(node, []) + graph.arriving.get(node, []):
            name = graph.street(j)
            kind = graph.kinds.get(graph.lanes[j]["way"])
            if name and name not in streets:
                names.append((STOP_ROADS.index(kind) if kind in STOP_ROADS else len(STOP_ROADS), name))
        if names:
            out.append((graph.pts[i][-1],) + min(names))
    return out


def make_route(graph, line, lanes, terminus=None):
    raw = chain(graph, lanes)
    pts = smooth(resample(raw, RESAMPLE_STEP), SMOOTH_PASSES)
    along = distances(pts)
    total = along[-1]
    stops = []
    fixed = []
    if terminus is not None:
        name, point = terminus
        fixed.append((project(pts, along, point), name))
    for point, _, name in sorted(crossings(graph, lanes, set(line["streets"])), key=lambda c: c[1]):
        at = project(pts, along, point) - STOP_BEFORE_CROSSING
        if at < STOP_END_ROOM or at > total - STOP_END_ROOM:
            continue
        if any(abs(at - s) < STOP_SPACING for s, _ in stops + fixed):
            continue
        stops.append((at, NAMES_1932.get(name, name)))
    stops = sorted(stops + fixed)
    return {
        "ref": line["ref"],
        "name": line["ref"] + " " + line["name"],
        "speed": BUS_SPEED,
        "points": [round(c) for p in pts for c in p],
        "stops": [{"at": round(at), "name": name} for at, name in stops],
    }


def build_1932_bus_routes(lanes, centerlines):
    routes = []
    for line in LINES_1932:
        graph = LaneGraph(lanes, centerlines, set(line["streets"]))
        streets = set(line["streets"])
        if "terminus" in line:
            edge = line["ends"][0]
            start = graph.edge_lane(streets, edge, True)
            end = graph.edge_lane(streets, edge, False)
            turn = graph.nearest_lane(line["terminus"][1], streets)
            there = graph.path(start, turn, streets)
            back = graph.path(turn, end, streets)
            if there and back:
                routes.append(make_route(graph, line, there + back[1:], line["terminus"]))
            else:
                print(f"bus {line['ref']}: no way found")
            continue
        for a, b in (line["ends"], line["ends"][::-1]):
            start = graph.edge_lane(streets, a, True)
            end = graph.edge_lane(streets, b, False)
            lanes_on = graph.path(start, end, streets) if start is not None and end is not None else None
            if lanes_on:
                routes.append(make_route(graph, line, lanes_on))
            else:
                print(f"bus {line['ref']} {a}->{b}: no way found")
    return routes


if __name__ == "__main__":
    import json
    import sys
    data = json.load(open(sys.argv[1], encoding="utf-8"))
    for r in build_1932_bus_routes(data["lanes"], data["centerlines"]):
        print(r["name"], len(r["points"]) // 2, "points", round(distances(lane_points(r))[-1] / 100), "m", [(s["at"] // 100, s["name"]) for s in r["stops"]])
