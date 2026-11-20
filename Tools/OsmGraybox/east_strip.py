"""Cut the east strip of the map (Kerepesi út out to the trotting track) from
raw OpenStreetMap downloads and keep only what stood there in 1932.

The prototype area (`source/blaha_test_area.osm`) ends at 19.090 E. The
strip east of it, out to the railway at 19.102 E, holds the Kerepesi út
trotting track, the Keleti engine yard and the old gallop course, which the
game builds itself (city_details.py). From today's map only these are kept:

- the old streets: Kerepesi út, Aréna út (today Dózsa György út), Verseny
  utca, Jobbágy utca, Salgótarjáni utca, Fiumei út and the road past the
  track's stables down to Salgótarjáni utca (today Asztalos Sándor út);
- the Kerepesi cemetery's paths and tombs and the Salgótarjáni utca Jewish
  cemetery (1874) with its two 1908 buildings;
- the railway north of Kerepesi út (the Keleti yard) and the line along the
  east edge.

Everything else there (the shopping mall, sports halls, offices) is newer and
is left out. The output is read by osm_to_graybox.py next to the main area:

    python east_strip.py raw1.osm raw2.osm ... source/east_strip.osm

The raw files were downloaded on 2026-11-20 from
https://api.openstreetmap.org/api/0.6/map?bbox=... (three boxes covering
19.088-19.112 E, 47.488-47.5035 N).
"""

import argparse
import math
import xml.etree.ElementTree as ET

# The strip, as an OSM bounds box.
STRIP = {"minlat": 47.4900, "minlon": 19.0900, "maxlat": 47.5030, "maxlon": 19.1020}

# Same origin and projection as osm_to_graybox.py, in metres (x east, y north).
LAT0, LON0 = 47.49655, 19.07040

KEEP_STREETS = {
    "Kerepesi út", "Dózsa György út", "Verseny utca", "Jobbágy utca",
    "Salgótarjáni utca", "Fiumei út", "Asztalos Sándor út",
}
STREET_KINDS = {"primary", "secondary", "tertiary", "residential", "unclassified", "living_street"}
PATH_KINDS = {"service", "footway", "path", "track", "pedestrian"}
# Cemeteries whose paths and buildings are kept: the Kerepesi cemetery
# (Fiumei úti sírkert) and the Salgótarjáni utca Jewish cemetery.
CEMETERIES = {"228345157", "228380750"}
# Rails south of Kerepesi út, except the line along the east edge, are newer
# sidings through the track's ground.
KEREPESI_Y = 330.0
EAST_LINE_X = 2290.0


def project(lat, lon):
    phi = math.radians(LAT0)
    m_lat = 111132.92 - 559.82 * math.cos(2 * phi) + 1.175 * math.cos(4 * phi)
    m_lon = 111412.84 * math.cos(phi) - 93.5 * math.cos(3 * phi)
    return ((lon - LON0) * m_lon, (lat - LAT0) * m_lat)


def inside(pt, ring):
    x, y = pt
    hit = False
    for (x1, y1), (x2, y2) in zip(ring, ring[1:] + ring[:1]):
        if (y1 > y) != (y2 > y) and x < x1 + (y - y1) * (x2 - x1) / (y2 - y1):
            hit = not hit
    return hit


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("raw", nargs="+")
    ap.add_argument("out")
    args = ap.parse_args()

    nodes, ways = {}, {}
    for path in args.raw:
        root = ET.parse(path).getroot()
        for n in root.iter("node"):
            nodes[n.get("id")] = n
        for w in root.iter("way"):
            ways[w.get("id")] = w

    def tags_of(w):
        return {t.get("k"): t.get("v") for t in w.iter("tag")}

    def points(w):
        return [project(float(nodes[r].get("lat")), float(nodes[r].get("lon")))
                for r in (nd.get("ref") for nd in w.iter("nd")) if r in nodes]

    cemeteries = [points(ways[c]) for c in CEMETERIES if c in ways]
    x0, _ = project(STRIP["minlat"], STRIP["minlon"])

    def in_cemetery(pts):
        mid = (sum(p[0] for p in pts) / len(pts), sum(p[1] for p in pts) / len(pts))
        return any(inside(mid, ring) for ring in cemeteries)

    kept = []
    for wid, w in ways.items():
        t = tags_of(w)
        pts = points(w)
        if not pts or max(p[0] for p in pts) < x0:
            continue
        hw, rail = t.get("highway"), t.get("railway")
        keep = False
        if wid in CEMETERIES:
            keep = True
        elif hw in STREET_KINDS and t.get("name") in KEEP_STREETS:
            keep = True
        elif hw in PATH_KINDS and in_cemetery(pts):
            keep = True
        elif t.get("building") and in_cemetery(pts):
            keep = True
        elif rail == "rail" and t.get("tunnel") != "yes":
            keep = min(p[1] for p in pts) > KEREPESI_Y or min(p[0] for p in pts) > EAST_LINE_X
        if keep:
            if hw in PATH_KINDS:
                # Cemetery paths carry today's names (after people of later
                # times); in 1932 they had none on any map.
                for tag in [tg for tg in w.findall("tag") if tg.get("k") == "name"]:
                    w.remove(tag)
            kept.append(w)

    out = ET.Element("osm", {"version": "0.6", "generator": "east_strip.py"})
    ET.SubElement(out, "bounds", {k: f"{v:.7f}" for k, v in STRIP.items()})
    refs = sorted({nd.get("ref") for w in kept for nd in w.iter("nd") if nd.get("ref") in nodes}, key=int)
    for r in refs:
        n = nodes[r]
        ET.SubElement(out, "node", {"id": r, "lat": n.get("lat"), "lon": n.get("lon")})
    for w in sorted(kept, key=lambda w: int(w.get("id"))):
        e = ET.SubElement(out, "way", {"id": w.get("id")})
        for nd in w.iter("nd"):
            if nd.get("ref") in nodes:
                ET.SubElement(e, "nd", {"ref": nd.get("ref")})
        for k, v in tags_of(w).items():
            ET.SubElement(e, "tag", {"k": k, "v": v})
    ET.indent(out, space=" ")
    ET.ElementTree(out).write(args.out, encoding="utf-8", xml_declaration=True)
    print(f"{len(kept)} ways, {len(refs)} nodes")


if __name__ == "__main__":
    main()
