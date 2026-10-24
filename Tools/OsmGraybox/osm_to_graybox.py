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
        ../../Content/Data/OsmGraybox/blaha_test_area.json
"""

import argparse
import json
import math
import xml.etree.ElementTree as ET

import mapbox_earcut
import numpy as np
from shapely.geometry import LineString, MultiPolygon, Polygon, box
from shapely.geometry.polygon import orient
from shapely.ops import polygonize, unary_union

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


def convert(osm_path, origin):
    nodes, ways, relations, (minlat, minlon, maxlat, maxlon) = parse_osm(osm_path)
    proj = LocalProjection(*origin)
    x0, y0 = proj.to_metres(minlat, minlon)
    x1, y1 = proj.to_metres(maxlat, maxlon)
    clip = box(x0, y0, x1, y1)

    buildings, roads, plazas, parks, trams, rails, centerlines = [], [], [], [], [], [], []

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
            add_area(buildings, Polygon(pts).buffer(0), dict(tags, id="w" + wid))
        elif any((k, tags.get(k)) in PARK_TAGS for k in ("leisure", "landuse")) and is_closed(way):
            add_area(parks, Polygon(pts).buffer(0), tags)
        elif highway == "pedestrian" and tags.get("area") == "yes" and is_closed(way):
            add_area(plazas, Polygon(pts).buffer(0), tags)
        elif highway in ROAD_WIDTHS and not is_underground(tags) and tags.get("area") != "yes":
            line = LineString(pts)
            width = road_width_m(tags)
            target = plazas if highway == "pedestrian" else roads
            add_area(target, line.buffer(width / 2.0, cap_style="flat", join_style="round"), tags)
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
        elif railway in TRACK_WIDTH and not is_underground(tags):
            target = trams if railway == "tram" else rails
            add_area(target, LineString(pts).buffer(TRACK_WIDTH[railway] / 2.0, cap_style="flat"), tags)

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
            add_area(buildings, geom, dict(tags, id="r" + rel["id"]))
        else:
            add_area(parks, geom, tags)

    def merged_layer(name, items):
        merged = unary_union([p for p, _ in items]) if items else None
        records = [polygon_record(p) for p in polygons_of(merged)]
        return {"layer": name, "polygons": [r for r in records if r]}

    out_buildings = []
    for poly, tags in buildings:
        rec = polygon_record(poly)
        if rec is None:
            continue
        rec["id"] = tags["id"]
        rec["name"] = tags.get("name", "")
        rec["height"] = round(building_height_m(tags) * 100)
        out_buildings.append(rec)

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
        + f", {len(data['centerlines'])} centerlines"
    )


if __name__ == "__main__":
    main()
