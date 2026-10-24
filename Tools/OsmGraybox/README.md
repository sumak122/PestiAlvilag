# OSM graybox

Turns an OpenStreetMap export into the JSON that the `AOsmGraybox` actor
(`Source/PestiAlvilag/OsmGraybox.*`) builds into gray blocks in Unreal.

- `source/blaha_test_area.osm` is the prototype area: Blaha Lujza tér,
  Népszínház utca, Teleki tér and Keleti, bounding box
  19.066,47.490 to 19.090,47.503. Downloaded 2026-10-24 from
  `https://www.openstreetmap.org/api/0.6/map?bbox=19.066,47.490,19.090,47.503`.
- `Content/Data/OsmGraybox/blaha_test_area.json` is the output the actor reads.

To regenerate:

```
pip install -r requirements.txt
python osm_to_graybox.py source/blaha_test_area.osm ../../Content/Data/OsmGraybox/blaha_test_area.json
```

Then press **Rebuild** on the OsmGraybox actor in the editor.

Notes:

- The origin (0, 0) is Blaha Lujza tér. X points east, Y south, units are cm.
- Building heights use `height`, then `building:levels` × 3.8 m, then 4 storeys.
- This is today's map. Buildings that did not exist in 1932 (for example
  around Corvin köz and Keleti's newer blocks) are still in it; fixing the
  1932 street plan comes later, from the MAPIRE 1930s maps.
- `lanes` are the AI traffic routes: every drivable street piece gets one
  lane each way, offset to the LEFT (Budapest drove on the left until 1941).
  Today's one-way tags are ignored because they describe right-hand traffic.
- Map data © OpenStreetMap contributors, ODbL 1.0. The game must credit it.
