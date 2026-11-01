# PestiAlvilag code map

A 1932 Budapest GTA 3 style game. Unreal 5.8.4, C++ only: almost everything is built in code at runtime, the levels are nearly empty.
Read this first. Open only the files your task needs, and grep big files instead of reading them whole.

## Where things start
- `Source/PestiAlvilag/PestiAlvilagGameModeBase.cpp` spawns every system (world, traffic, people, missions, landmarks). Start here to find who owns what. `PestiStart` holds start spots and yaws.
- Map data comes from `Tools/OsmGraybox/` (Python): `osm_to_graybox.py` (streets, blocks), `street_furniture.py` (lamps, benches, pavements, shops, walker lines), `trams_1932.py` (tram lines). Output JSON lives under `Data/` and is read by `OsmGraybox.cpp`, `StreetFurniture.cpp`, `Pedestrians.cpp`, `Knockables.cpp`.

## Naming pattern
- `*Model.h` (namespace, header only): builds the boxes/shapes of one thing (CarModel, PersonKit, TramModel, ShopModel, MarketHallModel...). Change looks here.
- `*Synth.h` + `*Sound.cpp`: sounds made in code (EngineSound, TramSound, HorseSound, FightSound).
- The matching `.cpp/.h` actor holds behaviour.

## Areas (files in Source/PestiAlvilag)
- Player on foot: `PestiPerson` (big, ~1400 lines: moves, camera, F/T keys), `FreeRun` (vault/slide), `Brawl.h` (fist fighting).
- Player car: `PestiCar`, `CarBody`, `CarVisuals`, `CarModel.h`, `ExhaustSmoke`, `EngineSound`, `Wreck.h`.
- Traffic and police: `TrafficManager`, `TrafficCar`, `PoliceCar`, `PoliceModel.h`, `PolicePatrols` (constables, phone boxes), `CarjackedDriver`.
- Taxis: `PestiTaxi`, `TaxiCompany` (job), `TaxiRides` (ride as passenger), `TaxiPassenger`.
- Trams: `Tram`, `TramModel.h`, `TramRides`, `TramSound`/`TramSynth.h`.
- Horse cart: `HorseCart`, `HorseCartModel.h`, `HorseSound`/`HorseSynth.h`.
- People: `PersonKit.h` + `PersonModel` (bodies, clothes), `Pedestrians` + `WalkingPerson` (walkers), `StoryPerson` (named characters), `ParkKeeper`, `Pigeons`/`PigeonModel.h`, `ChaseThief`/`ChaseModel.h`.
- World and look: `OsmGraybox` (streets, buildings, facades), `StreetFurniture` (big: lamps, benches, shops), `ShopModel.h`, `GroundSurfaces` + `SurfaceModel.h` (cobbles, kerbs, market earth), `DayNight` (sun, lamps, glass at night), `FallingLeaves`, `ParkModel.h`, `Knockables`/`KnockModel.h`.
- Landmarks: `Nepszinhaz`, `KeletiStation` (big), `KeletiEagle`/`EagleModel.h`, `MarketHall` + `MarketHallModel.h` + `HallMarketModel.h`, `FleaMarket` (biggest, ~1600 lines: Teleki and hall markets) + `MarketModel.h`, `Cafe`, `FoodVendor`, `FoodMenus`, `HomeDoor`.
- Missions and saving: `Mission` (base), `MissionPrologue`, `MissionUzenet`, `MissionZsebtolvaj`, `MissionZsibvasar`, `PestiSaveGame`, `Lighters`/`LighterModel.h` (collectibles), `GoalMarkerComponent`, `RouteGuideComponent`.
- Screen text: `PestiHud` (art deco HUD, menus), `PestiText.h` (Hungarian/English strings).

## Rules for changes
- Claude can't build Unreal; Daniel does, and each mistake costs him a failed build. Check carefully before pushing.
- Known build traps (more in project memory unreal-build-gotchas): include the full header of any component type you convert or call; never name locals after AActor members (Children, Tags, Owner...) or after names in visible namespaces (Brawl, FreeRun); FString::Printf format must be one plain TEXT literal; use \uXXXX escapes for Hungarian letters inside TEXT("...").
- Never push binaries (.uasset, .umap, most .png): Git LFS is blocked from the cloud.
- When you add a new file, add one line for it in this map.
