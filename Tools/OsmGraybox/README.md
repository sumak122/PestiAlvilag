# OSM graybox

Turns an OpenStreetMap export into the JSON that the `AOsmGraybox` actor
(`Source/PestiAlvilag/OsmGraybox.*`) builds into gray blocks in Unreal.

- `source/blaha_test_area.osm` is the prototype area: Blaha Lujza tér,
  Népszínház utca, Teleki tér and Keleti, bounding box
  19.066,47.490 to 19.090,47.503. Downloaded 2026-10-24 from
  `https://www.openstreetmap.org/api/0.6/map?bbox=19.066,47.490,19.090,47.503`.
- `Data/OsmGraybox/blaha_test_area.json` is the output the actor reads.

To regenerate:

```
pip install -r requirements.txt
python osm_to_graybox.py source/blaha_test_area.osm ../../Data/OsmGraybox/blaha_test_area.json
```

Then press **Rebuild** on the OsmGraybox actor in the editor.

Notes:

- The origin (0, 0) is Blaha Lujza tér. X points east, Y south, units are cm.
- Building heights use `height`, then `building:levels` × 3.8 m, then 4 storeys.
  Bridges between buildings get a `base` from `min_height` or
  `building:min_level`, so streets stay open underneath.
  Canopies (`building=roof`, `carport`) are a thin roof 3-4 m up. Building
  corners that stick out into a street cars drive on are cut back to the
  carriageway edge.
- Buildings get `front`: the wall edges (counted through all rings, edge i
  from point i to i+1) whose outside is within 3 m of a road or square
  widened by a 6 m pavement. The actor dresses those walls as Pest tenement
  facades (windows, ledges, cornice, shops, gates); courtyard walls stay
  plain. `levels` is passed on from OSM `building:levels` when mapped.
  Facades are made when the game starts and are not shown in the editor.
- This is today's map. Buildings that did not exist in 1932 (for example
  around Corvin köz and Keleti's newer blocks) are still in it; fixing the
  1932 street plan comes later, from the MAPIRE 1930s maps.
- `lanes` are the AI traffic routes, keeping LEFT (Budapest drove on the
  left until 1941). Main roads mapped as one-way pairs stay one-way with the
  direction flipped; side streets narrower than 6 m keep their one-way rule;
  everything else is two-way with a lane each side. A one-way lane that would
  trap cars at a dead end gets a lane back too. Lanes are nudged to stay
  1.3 m clear of walls where buildings crowd the street.
- `trams` are the 1932 tram lines, not today's: `trams_1932.py` lists them as
  streets in running order, taken from the BSzKRt network schematic of 1932
  (print M.92/932, Budapest City Archives, kozlekedes.bparchiv.hu). The
  track follows those streets, sits in the middle of streets mapped as two
  one-way halves, and each direction keeps left. Lines on the same street
  share tracks and stops. The 1932 map has no stops, so stops are placed
  just before crossings, main roads first, about 300 m apart. The `tram`
  area layer draws these tracks; today's OSM tram tracks are ignored.
  Karpfenstein utca (1932) is today's Dobozi utca.
- Map data © OpenStreetMap contributors, ODbL 1.0. The game must credit it.

## Street furniture and Tisza Kálmán tér

`street_furniture.py` reads the graybox JSON and writes
`Data/OsmGraybox/street_furniture.json`. The `AStreetFurniture` actor (the game
mode spawns it) reads that file and places everything on the ground:

- the period street lamps along the pavements (after Fortepan 100450);
- round advertising columns near street corners;
- benches in the parks;
- litter bins beside some street lamps and next to the park benches;
- walking lines along both pavements of every street and down the park
  paths, which the people out walking (APedestrians) follow;
- the 1932 layout of Tisza Kálmán tér: paths, flower beds, trees, benches,
  lamps and a railing, with knee-high wire fences along the lawns and low
  clipped hedges round the round places (after Fortepan 70268), and for
  autumn raked leaf piles and the routes of two park keepers who rake them.

Run it again whenever the graybox JSON changes:

    python street_furniture.py ../../Data/OsmGraybox/blaha_test_area.json \
        ../../Data/OsmGraybox/street_furniture.json
