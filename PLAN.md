# Knights of Camelot - Roguelike Game Implementation Plan

## Implementation Checklist

- [x] **Phase 1** -- Skeleton: Build System, ncurses, Game Loop
- [x] **Phase 2** -- Overworld Map of England (terrain, towns, landmarks, castles, cottages, caves)
- [x] **Phase 3** -- Day/Night Cycle, Weather, Horses & Overworld Encounters
- [x] **Phase 4** -- Towns: Shops, Inns (beer!), Mystics, Churches, Banks, Pawn Shops, Wells
- [x] **Phase 5** -- Quest System (10-15 side quests, journal)
- [x] **Phase 6** -- Dungeon Generation (BSP, traps, magic circles, multiple dungeons + volcano)
- [x] **Phase 7** -- Field of View & Fog of War
- [x] **Phase 8** -- Enemies, Combat, Message Log (80+ monsters, werewolves, vampires, mad knights)
- [x] **Phase 9** -- Items & Inventory (100+ items, weight system, 50 potions, 22 rings, 12 artifacts, 20 gems, 40 food)
- [x] **Phase 10** -- Enemy AI & Pathfinding (A*, FSM, type-specific behaviours)
- [x] **Phase 11** -- Character Creation, Classes, Spells (50 spells, affiliations, chivalry, spell scrolls)
- [x] **Phase 12** -- Dungeon Bosses & The Holy Grail (Round Table membership)
- [x] **Phase 13** -- Save/Load, Permadeath, Death Screen & High Scores (bank, graveyard, fallen heroes)
- [x] **Phase 14** -- Help System, Settings, Title Screen, Special Events & Polish

### Data Files Checklist
- [x] `data/monsters.csv` -- 85+ monsters with stats, AI flags, drops
- [x] `data/items.csv` -- 150+ items: weapons, armor, rings, amulets, gems, treasures, potions, food, scrolls, spell scrolls, tools
- [x] `data/quests.csv` -- 15 side quests (delivery, fetch, kill)
- [x] `data/towns.csv` -- 43 towns, castles, abbeys with services
- [x] `data/locations.csv` -- 70+ overworld locations with coordinates
- [x] `data/creatures.csv` -- 12 overworld creature types
- [x] `data/spells.csv` -- 50 spells across 4 schools
- [x] `data/names.csv` -- 48 random character names
- [ ] `data/traps.csv` -- 12 trap types with damage, effects, messages (currently hardcoded in game.c)
- [ ] `data/weather.csv` -- 6 weather types with regional weights (currently hardcoded in game.c)
- [ ] `data/events.csv` -- 11 lunar cycle events (currently hardcoded in game.c)
- [ ] `data/loot_tables.csv` -- monster drop ranges (currently hardcoded in game.c)
- [ ] `data/dialogue.csv` -- NPC dialogue lines (feature not yet built)
- [ ] `data/ammo.csv` -- ranged ammo types (feature not yet built)
- [ ] `data/witch_tasks.csv` -- witch encounter tasks (feature not yet built)
*Note: potions, food, rings, amulets, gems, and treasures are all rows in `data/items.csv` with appropriate type columns. Terrain and time-of-day are simple enums that don't warrant separate CSV files.*

## Context
Building "Knights of Camelot" -- a full-featured terminal ASCII roguelike in C using ncurses. Theme: Knights of Camelot and the Quest for the Holy Grail. Features an overworld map of England with towns/villages, and multiple dungeons. The repo is empty (just README/LICENSE/.gitignore).

## Project Structure
```
camelot/
  Makefile
  include/common.h              (shared constants, Vec2, Tile, enums)
  src/main.c                    (entry point)
  src/game.c / game.h           (GameState, core loop, turn sequencing)
  src/map.c / map.h             (BSP dungeon generation)
  src/overworld.c / overworld.h (fixed overworld map, towns, travel)
  src/town.c / town.h           (town interiors: shops, inns, mystics)
  src/castle.c / castle.h       (castle interiors: active and abandoned)
  src/entity.c / entity.h       (player, enemies, NPCs)
  src/player.c / player.h       (classes, leveling)
  src/item.c / item.h           (item definitions, loot tables)
  src/inventory.c / inventory.h (inventory, equipment)
  src/combat.c / combat.h       (bump-to-attack damage calc)
  src/fov.c / fov.h             (recursive shadowcasting)
  src/ai.c / ai.h               (A* pathfinding, enemy FSM)
  src/spell.c / spell.h         (spell system)
  src/npc.c / npc.h             (shop/inn/mystic NPCs, dialogue)
  src/quest.c / quest.h         (quest system, journal, tracking)
  src/ui.c / ui.h               (ncurses rendering, panels)
  src/log.c / log.h             (message log ring buffer)
  src/save.c / save.h           (binary save/load)
  src/score.c / score.h         (persistent high scores)
  src/config.c / config.h       (CSV parser, data loader)
  src/rng.c / rng.h             (seeded PRNG)
  data/                         (all game data as editable CSV files)
    monsters.csv                (enemy types, stats, glyphs, colors, depth ranges)
    items.csv                   (weapons, armor, potions, scrolls, quest items)
    spells.csv                  (spell names, MP cost, damage, effects, class restrictions)
    quests.csv                  (quest descriptions, types, objectives, rewards)
    npcs.csv                    (NPC names, locations, dialogue IDs, roles)
    shops.csv                   (shop inventories per town, item IDs, prices)
    towns.csv                   (town names, services available, coordinates)
    castles.csv                 (castle names, type active/abandoned, floors, loot)
    encounters.csv              (overworld encounter rates by terrain, enemy groups)
    classes.csv                 (player classes, starting stats, starting gear)
    dialogue.csv                (dialogue lines keyed by NPC/dialogue ID)
    loot_tables.csv             (loot drop chances per dungeon depth)
```

## Data-Driven Architecture
All game entities and content are defined in CSV files under `data/`, not hardcoded.
- CSV format: first row is header, `#` lines are comments, fields are comma-separated
- `config.c` provides a generic CSV parser: `csv_load(path)` returns rows as arrays of strings
- Each module (entity, item, spell, etc.) has a `_load_from_csv()` function that parses its CSV into the appropriate structs at startup
- Game loads all CSV data at init before entering the main loop
- Modders/developers can edit CSV files to rebalance, add enemies, create new items, etc. without recompiling
- Example `monsters.csv`:
  ```
  # name,glyph,color,hp,str,def,spd,intel,min_depth,max_depth,ai_type
  Giant Rat,r,brown,6,3,1,3,1,1,3,chase
  Wolf,w,gray,10,5,2,4,2,1,4,chase
  Black Knight,K,red,30,10,8,5,3,5,8,aggressive
  Red Dragon,D,red,50,15,10,3,5,9,11,ranged
  ```

## Game Modes
The game has three distinct modes, each with its own rendering and input handling:
- **Overworld** -- top-down ASCII map of England, player moves between towns/dungeons
- **Town** -- enter a town to visit shops, inns, mystics; interior map or menu-driven
- **Dungeon** -- classic roguelike dungeon crawl with FOV, combat, items

## Color Scheme
All ASCII graphics use ncurses color pairs. Requires 256-color terminal support (`TERM=xterm-256color`), with fallback to 8-color mode.

- **Terrain:** `#` walls (gray), `.` floors (dark white), `~` water (blue), `"` grass (green), `^` mountains (white bold), `T` forest (dark green), `.` roads (yellow)
- **Player:** `@` (white bold on black)
- **Enemies:** each type has a distinct color -- rats (brown), wolves (gray), bandits (yellow), skeletons (white), Black Knight (red bold), Dragon (red on orange), Witch (green), Ghost (cyan), Mordred (magenta bold)
- **Items:** weapons `/` (cyan), armor `[` (blue), potions `!` (varies by type: red=health, blue=mana, green=strength), scrolls `?` (yellow), food `%` (brown), gold `$` (yellow bold), Holy Grail `*` (yellow bold on white)
- **NPCs:** shopkeeper `$` (green), innkeeper `I` (brown), mystic `?` (magenta), quest giver `!` (yellow bold), famous characters (white bold)
- **Landmarks:** stairs `>` `<` (white), doors `+` (brown), wells `O` (blue), cathedral `^` (white bold)
- **UI:** status bar (white on blue), message log (white on black), highlighted text (yellow bold)

## Phases

### Phase 1 -- Skeleton: Build System, ncurses, Game Loop
- Create Makefile (clang, -std=c11, -lncurses)
- `common.h` with constants, Vec2, Tile structs, GameMode enum (OVERWORLD, TOWN, DUNGEON)
- `main.c` -> `ui_init()` -> `game_run()` -> `ui_shutdown()`
- Core loop: render -> getch -> handle_input -> update
- Initialize color pairs in `ui_init()` -- detect 256-color vs 8-color support, define all color pairs
- Player `@` moves with hjklyubn / arrow keys, `q` quits
- **Result:** Compiles, runs, colored `@` moves on blank screen

### Phase 2 -- Overworld Map of England
- Hand-crafted 80x22 ASCII map of England with terrain:
  - `~` water/sea (impassable), `"` grassland (normal speed), `^` hills (slow, +encounter chance), `T` forest (slow, nature encounters, reduced visibility), `.` roads (fast, safest travel), `,` marsh (very slow, chance of getting stuck for a turn, snake encounters), `%` swamp (very slow, poison damage risk, swamp creature encounters), `&` dense woods (impassable without Ranger class or axe item)
  - `=` **river** (impassable, flows across map -- can only cross at `H` bridge tiles or with Walk on Water spell)
  - `o` **lake** (impassable -- can cross with Walk on Water spell or by finding a boat `B` on the shore. Fishing possible at lake edges for food items)
  - Some lakes contain **islands** (small land tiles `"` surrounded by lake water):
    - Islands may hold hidden treasure, a hermit NPC, a shrine, or a dungeon entrance
    - Reach islands by boat (interact with `B` boat tile on the shore to sail to the island) or Walk on Water spell
    - **Notable lakes with islands:**
      - **Lake of the Lady** -- the Lady of the Lake's home. Island holds a sacred grove (the Lady appears here or at the shore to offer Excalibur -- see Famous Characters)
      - **Lake Bala** (Wales) -- island with a druid hermit who teaches Nature spells
      - **Windermere** (north) -- island with an abandoned watchtower containing loot and a ghost encounter
      - **Loch Ness** (Scotland) -- home of **Nessie** (`N` dark green), a legendary sea monster. Sailing across triggers a boss encounter. Nessie is extremely tough (high HP, powerful water attacks) but defeating her yields a unique scale armor (best water resistance) and massive XP. Alternatively, offering fish from your inventory appeases her and she grants safe passage + a hidden treasure from the loch bed
      - **Loch Lomond** (Scotland) -- island with an ancient cairn, grants a vision revealing a hidden dungeon location
    - **Lake encounters**: while sailing by boat, random chance of a water monster attack. Fight takes place on the boat (limited movement, 3x1 tiles). Enemies: Water Serpent, Kelpie (shapeshifter horse-demon), Giant Pike, Nixie. Falling overboard (knocked off edge) ends the fight but you lose an inventory item to the depths
    - **Ghost Ship** -- rare encounter while sailing on any large lake (5% chance per crossing). A spectral ship (`=` cyan dim) emerges from the mist:
      - Player is pulled aboard the ghost ship -- a small explorable deck (8x5 map) with ghostly crew
      - **Crew**: 3-5 Drowned Knights and a Ghost Captain (mini-boss). All undead, weak to holy/Light attacks
      - **Loot**: the ship's hold contains a treasure chest with high-value items -- gems, gold, and a chance at a unique item (Ghost Captain's Cutlass: +3 damage, drains enemy HP on hit)
      - If player defeats the Ghost Captain, the ship fades and player returns to their boat with the loot
      - If player is defeated, they wash ashore at the nearest lake edge with 1 HP and lose half their gold to the depths
      - Holy weapons and Light spells are especially effective on the ghost ship
      - The ghost ship encounter can only happen once per lake per game
  - `H` **bridge** (road speed, the only way to cross rivers without magic)
  - Rivers and lakes act as natural barriers forcing players to find bridges or learn water-walking magic, creating meaningful exploration and spell progression
  - Terrain affects: movement speed, encounter type/rate, visibility, and hazards
  - Terrain types defined in `data/terrain.csv` (glyph, color, speed_modifier, encounter_rate, hazard_type, passable, pass_condition)
- Named towns and landmarks at fixed locations:
- **Towns** (enter to visit shops, inns, etc.):
  - **Camelot** (starting location, central) -- castle icon `#`
  - **London** -- major city, best equipment shops
  - **Glastonbury** -- mystics and holy site
  - **Tintagel** -- coastal castle, dungeon nearby
  - **Sherwood** -- forest town, ranger gear
  - **York** -- northern stronghold
  - **Winchester** -- old capital, good armor
  - **Cornwall** -- remote, coastal village
  - **Wales** -- dragon territory. Welsh villages under threat. Home of **Mount Draig**, an active volcano (`V` red) where the Red Dragon (Y Ddraig Goch) lairs. Smoke visible from several tiles away
  - **Canterbury** -- cathedral city with a **Shrine of St Thomas Becket** and a **Graveyard**. Pray at the shrine: full HP/MP restore, holy blessing (+1 DEF vs undead), cure all curses. Buy holy water and sacred scrolls. Pilgrims gather here offering rumours and quest leads.
    - **Shrine looting**: at any shrine, player can choose to **loot** instead of pray. Shrines contain valuable relics (gems, gold, sacred items worth 100-300g). However: **-15 chivalry** (severe sacrilege), shrine is permanently desecrated (can no longer pray there), and nearby NPCs turn hostile. If chivalry drops below 15 from looting, Camelot gates close to you. A desperate but tempting option for gold-starved players
    - **Canterbury Graveyard**: rows of gravestones (`+` gray) behind the cathedral. When the player dies, their character's name, class, cause of death, and epitaph are added to a gravestone, persisted in `~/.camelot/graveyard.dat`. On future games, visiting the graveyard shows all your previous fallen characters on their stones:
      ```
      Here lies Sir Galahad
      Knight, Level 7
      Slain by a Red Dragon
      "He sought the Grail but found only fire"
      ```
    - Gravestones accumulate across all playthroughs -- a permanent memorial of your failures
    - Interacting with a gravestone shows the full death stats of that character
    - Ghosts of previous characters may occasionally appear in the graveyard (harmless, flavour only)
    - If you visit the graveyard on your current run, it grants +1 chivalry (paying respects to the fallen)
  - **Llanthony** -- remote priory town in the Welsh hills with a **Shrine of the Virgin Mary**. Pray at the shrine: large Light affinity boost, learn a Light spell, permanent +1 INT. Monks sell rare sacred texts (spell scrolls). Peaceful refuge -- no enemy encounters in town
  - **Carbonek** -- the hidden Grail Castle with the **Shrine of the Holy Grail**. Only accessible after finding clues from Merlin and King Pellam. The shrine holds a vision of the Grail that grants a major blessing (+2 all stats) and reveals the final path to Glastonbury Tor. Guarded by holy knights who test the player's worthiness
  - **Whitby** -- a gloomy coastal town on the northeast coast. It is **always raining** here (permanent weather override). At night, there is a chance to encounter a **Vampire** (`V` dark red):
    - Vampire is a powerful undead enemy (high STR, fast, drains HP on hit). Weak to holy weapons, Light spells, and garlic (if carried, reduces vampire STR by 3)
    - If bitten (10% chance per vampire hit), player contracts **Vampirism**:
      - **Vampirism effects**: player can only travel at night. During daytime, if outdoors, player takes 3 HP damage per 10 turns from sunlight. Must rest indoors during the day
      - **Rest options updated**: when resting at an inn or camping, player can choose "Rest until morning" or "Rest until nightfall" (vampires choose nightfall)
      - **Benefits of vampirism**: no longer need to eat (hunger system disabled), +3 STR, +2 SPD, HP drain on melee attacks (steal 2 HP per hit), can see in the dark (FOV not reduced at night)
      - **Drawbacks**: sunlight damage, cannot enter churches or pray at shrines (burns on holy ground, -5 HP), -10 chivalry immediately on transformation, NPCs react with fear (some refuse to trade), holy water damages you
      - **Cure**: visit Merlin (he knows an ancient cure), or find the rare Potion of Cure Disease, or pray at Carbonek shrine (painful but effective -- costs 20 HP but removes vampirism)
    - Whitby has a ruined abbey (explorable like a small dungeon, 2 floors) with vampire-themed undead and a Vampire Lord boss
    - Town still has shops and inn, but the innkeeper charges double ("We don't get many visitors...")
  - **Bath** -- Roman ruins, hot springs restore status effects and cure curses
- **Landmarks** (special locations on the overworld, not full towns):
  - **Stonehenge** -- ancient druid circle. Visit to commune with ancient spirits: grants a permanent +1 to a random stat once per game, or reveals the location of a hidden dungeon. Mystic atmosphere with special encounter chance (druids)
  - **Hadrian's Wall** (far north) -- marks the edge of the map. Roman ruins to explore, chance to find ancient Roman weapons/armor
  - **The White Cliffs of Dover** (southeast coast) -- scenic overlook. Hidden cave entrance to a small bonus dungeon with sea-themed enemies (merfolk, sea serpents)
  - **Avalon** (hidden island, accessible only after obtaining a quest item or Merlin's guidance) -- legendary isle where Excalibur was forged. Unique high-tier equipment available
  - **Faerie Ring** (hidden clearings in forests, marked as `o` glowing green) -- rare locations where faeries dwell:
    - Stepping into a faerie ring triggers a faerie encounter -- mischievous, unpredictable beings
    - **Friendly faeries**: may grant a random boon -- restore MP, enchant a weapon, teach a Nature spell, reveal a secret, or give a faerie charm (lucky item, +1 to all rolls)
    - **Trickster faeries**: may play pranks -- teleport you to a random location, swap two of your stats, turn your gold into mushrooms (temporary), confuse your map for 20 turns
    - **Faerie Queen** (rare): appears once per game, offers a powerful pact -- permanent Nature affinity boost and a unique faerie blade, but you owe a favour (random future quest obligation)
    - Faeries appear as `f` (green/pink) -- not hostile, cannot be attacked. Interaction is dialogue-driven with random outcomes
    - Higher Nature affinity = better chance of friendly outcomes
    - Faerie-themed enemies in nearby forest areas: Will-o'-the-Wisps (`w` cyan, lure player off paths), Pixie Swarms (`p` pink, steal items), Redcaps (`r` red, aggressive goblin-fae)
- **Castles** (explorable multi-room interiors, shown as `#` on overworld):
  - **Active castles** -- functioning like towns but grander. Have throne rooms, armories, barracks, and dungeons beneath. Each ruled by a king who may be home or travelling (see Kings below):
    - **Camelot Castle** (Logres) -- **King Arthur**'s seat. Grand hall, royal armory (best knight gear), barracks with training NPCs (+1 STR for gold), dungeon entrance to Catacombs below
    - **Your Home** (Camelot town) -- a small house (`h` white) in Camelot that belongs to the player:
      - Contains a **storage chest** with **unlimited capacity** (no weight limit). Press `s` to stash items, `t` to take items
      - Items in the chest **persist across deaths** -- stored in `~/.camelot/home_chest.dat` separately from the save file
      - When you start a new game, visit your home in Camelot to retrieve items left by previous characters
      - This makes the game progressively easier the more you play -- stash good weapons, potions, rings for future runs
      - Combined with the bank (cross-game gold), this creates a meta-progression loop
      - Home also has a bed (free rest, advances to morning) and a mirror (view full character sheet)
      - The chest shows whose items they were: "Longsword (left by Sir Galahad, Day 12)"
    - **Castle Surluise** -- **King Galahaut**'s domain. A powerful allied king, offers quests to prove allegiance. Good weapon shop
    - **Castle Lothian** (Scotland) -- **King Lot**'s stronghold. Northern fortress, strong armor and cold-weather gear. Lot is suspicious of outsiders until you prove yourself
    - **Castle Northumberland** -- **King Clarivaus**'s seat. Remote northern castle, sparse but has unique northern weaponry
    - **Castle Gore** -- **King Uriens**'s domain. Well-stocked armory, Uriens offers martial training (+1 STR or DEF for gold)
    - **Castle Strangore** -- **King Brandegoris**'s seat. Trade hub, best general shop with wide item selection
    - **Castle Cradelment** (North Wales) -- **King Royns** rules here. A fierce king, will challenge you to a duel before granting audience. Rewards: rare Welsh equipment
    - **Castle Cornwall** -- **King Mark**'s seat. Coastal castle, naval supplies, King Mark is treacherous (may betray you -- steal gold or set a trap, but also sells rare items)
    - **Castle Listenoise** -- **King Pellam** (the Maimed King). A wounded, cursed king in a desolate castle. Healing him is a side quest; reward: access to the castle's holy relics and a Light affinity boost
    - **Castle Ireland** -- **King Agwisance**'s domain. Across the sea (requires a boat or Walk on Water). Unique Irish weapons and Celtic magic scrolls
    - **Castle Benwick** -- **King Ban**'s seat (Lancelot's father). Allied to Arthur, offers Lancelot-related quests and fine French equipment
    - **Castle Gaul** -- **King Bohrs**'s domain. Across the channel, requires travel. Continental weapons and armor, exotic goods
    - **Castle Brittany** -- **King Howell**'s seat. Coastal, allied to Arthur. Offers naval quests and sea-related gear
    - **Castle Carados** (Scotland) -- **King Carados**'s fortress. Scottish highland castle, rugged gear, whisky at the inn (extra HP restore)
  - **King travel system**: Kings are not always home. Each king has a chance to be travelling between their castle and Camelot (or another allied castle). When travelling, they appear as `K` (white bold) on overworld roads. Bumping into a travelling king triggers dialogue and possibly a quest or trade. Their castle still functions (shops, inn) but the throne room is empty when they're away. King locations update every ~50 turns
  - **Abandoned castles** -- ruined, overrun with enemies. Explorable like small dungeons (2-3 floors) with fixed layouts:
    - **Castle Dolorous Garde** -- haunted ruin, ghosts and skeletons. Hidden treasure rooms behind crumbling walls. Lancelot's lost sword somewhere inside
    - **Castle Perilous** -- dark sorcerer's lair, traps and magic enemies. Enchanted items as rewards
    - **Bamburgh Castle** -- northern ruin, occupied by bandits and a bandit king (mini-boss). Clearing it could unlock it as a safe rest point
  - Abandoned castles use the dungeon rendering mode (FOV, enemies, items) but with castle-themed tile sets: stone corridors, throne rooms, collapsed walls, arrow slits
  - Active castles use the town rendering mode with more elaborate layouts
- Towns shown as colored `*`, landmarks as colored `+`, castles as colored `#` on the map
- Player `@` moves on roads and terrain (roads = fast, forest/hills = slow or same speed)
- Entering a town tile triggers transition to town mode
- Dungeon entrances `>` near certain towns
- **Random cottages** -- scattered across the overworld (`n` brown), found along roads and in clearings:
  - 8-12 cottages placed on the overworld map, some fixed, some randomized per game
  - Entering a cottage is a single-room interior (small 10x8 map)
  - **Cottage states** (random per visit):
    - **Empty/abandoned** (30%) -- can rest for free (restore 25% HP/MP), sometimes contains a chest with minor loot (food, potion, a few coins)
    - **Friendly occupant** (35%) -- a peasant, hermit, retired knight, or healer lives here:
      - Hermit: shares rumours about nearby dungeons or treasure map locations
      - Healer: restores HP for free, cures poison (once per visit)
      - Retired knight: offers combat tips (+1 STR or DEF if you listen to his stories, once per game)
      - Peasant: offers food and shelter (free rest), may ask for help with a small favour (kill nearby wolves = small gold reward)
    - **Hostile occupant** (20%) -- a bandit hideout, witch's hut, or squatters:
      - Bandits: 2-3 bandits attack on entry, loot their stash if you win
      - Witch: offers a dark bargain (like Morgan le Fay but smaller scale) OR attacks with magic
      - Squatters: hostile vagrants, weak fight, minimal loot
    - **Special cottage** (15%) -- rare unique encounters:
      - Wise woman who teaches a Nature spell
      - Wounded knight who gives you a quest (rescue his squire from nearby dungeon)
      - Mysterious stranger who offers to trade a random item from their pack for one of yours (blind trade -- could be great or terrible)
      - Abandoned alchemist's lab with 2-3 random potions
  - Cottage state may change between visits (hermit might leave, bandits might move in)
  - Can camp in empty cottages safely (no ambush chance, unlike open overworld camping)
- **Random caves** -- scattered in hills and mountains on the overworld (`(` gray):
  - 5-8 cave entrances per game, mostly in hilly/mountainous terrain
  - Single-room interiors (small 8x6 map), dark (FOV applies)
  - **Cave states** (random):
    - **Empty cave** (25%) -- shelter from weather, safe camping spot. May contain a small chest (minor loot)
    - **Monster lair** (30%) -- inhabited by a bear, troll, or spider. Kill for loot (pelts, fangs, small gold stash)
    - **Hermit's cave** (35%) -- a hermit (`h` brown) lives here in solitude:
      - Hermits are peaceful NPCs. They may offer: wisdom (quest hint), a blessing (+1 random stat), a potion, or trade items
      - Hermits often have a hidden stash: gold (20-80g) and 1-2 useful items
      - **You can kill the hermit** to take their belongings -- but: **-8 chivalry** (killing a defenseless holy man), and there's a 25% chance the hermit was actually a disguised saint, causing **-15 chivalry** and a curse instead
      - If you leave the hermit in peace and are polite, they may give you a gift freely (+2 chivalry for respecting their solitude)
    - **Bandit hideout** (10%) -- 2-3 bandits, kill for their stash (gold, stolen goods). No chivalry penalty
  - Caves marked on treasure maps sometimes lead to larger underground areas (2-3 rooms)
- **Result:** Player navigates a map of England, sees towns, cottages, caves, and terrain

### Phase 3 -- Day/Night Cycle, Weather & Overworld Encounters

**Day/night cycle**:
- Time advances with each turn: 1 turn = ~1 minute of game time. A full day = 1440 turns (24 hours)
- Time of day shown on status bar: Dawn (5-7), Morning (7-12), Afternoon (12-17), Dusk (17-19), Night (19-5)
- **Night effects**:
  - Overworld visibility reduced (FOV radius shrinks from full to 5 tiles)
  - Different encounter table: wolves and ghosts more common, bandits less common
  - **Wandering spirits** (`g` dim cyan) -- ethereal ghosts that drift across the overworld at night. They are non-hostile and cannot be attacked (weapons pass through them). However, passing within 2 tiles of a wandering spirit **drains MP** (3-5 MP per turn in proximity). They drift slowly in random directions and fade at dawn. Best to avoid them. If MP reaches 0 from spirit drain, player feels "a terrible chill" (-1 SPD for 20 turns). Holy items (Amulet of Light, holy water in inventory) reduce drain to 1 MP per turn. Sanctuaries and churches repel them
  - Some shops close at night (equipment shops, potion shops). Inns and mystics stay open
  - Dungeons unaffected (always dark underground)
  - Castle gates close at night -- must wait until dawn or have high chivalry to gain entry
- **Dawn/dusk**: transitional lighting, brief visual effect
- **Resting at an inn**: choose "Rest until morning" (advances to 6 AM next day) or "Rest until nightfall" (advances to 7 PM). Restores HP/MP either way. Vampiric players will prefer nightfall
- **Day counter**: the game tracks the current day number (Day 1, Day 2, etc.) displayed on the status bar alongside the time. Day advances when the clock passes midnight (turn 1440+). Death screen shows "Survived X days". Allows players to track how long their quest has taken
- **30-day lunar cycle** -- the game runs on a repeating 30-day cycle. The current moon phase is shown on the status bar. Events trigger on specific days each cycle, announced by a herald or message the day before:
  - **Day 1: New Moon** -- darkest night. Visibility reduced even further at night. Undead are stronger (+2 STR). Ghost ship encounter chance doubled. Good night to stay indoors
  - **Day 5: Feast Day** -- grand feast at Camelot Castle. Free food (full hunger restore), +2 chivalry for attending, King Arthur gives a speech (quest hints), travelling merchants sell rare items
  - **Day 8: First Quarter Moon** -- wolves and forest creatures are more active. Rangers get +1 SPD bonus (in tune with nature)
  - **Day 10: Market Day** -- travelling merchants in Camelot and London. Rare items, 20% lower prices, exotic goods
  - **Day 12: Druid Gathering** -- all druids converge on Stonehenge for a grand ritual. Visit Stonehenge on this day for: a powerful blessing (+2 to a stat of your choice), free Nature spell teaching, and a vision of a hidden dungeon. Druids will not steal from you on this day. Magic circles across the map glow brighter (guaranteed beneficial)
  - **Day 15: Full Moon** -- werewolves can appear **anywhere** on the overworld at night, not just moors. Lycanthropic players transform involuntarily. Faerie rings are especially active (double encounter chance, better outcomes). Lady of the Lake is more likely to appear. Moonsilver Dress glows with extra power (+2 all stats on full moon night)
  - **Day 18: Tournament Day** -- jousting tournament at a random active castle (rotates each cycle). Free entry, unique prizes. Best knights compete
  - **Day 20: Holy Day** -- pilgrims flock to Canterbury, Glastonbury, and Llanthony. Shrines grant double blessings. Churches donate food. +3 chivalry for attending a service. Undead are weaker (-2 STR)
  - **Day 22: Witching Hour** -- witch encounters are 3x more frequent this day. The veil between worlds is thin. Dark spells are more powerful (+25% damage). Vampires are more active. Cursed items have double negative effect
  - **Day 25: King's Court** -- Arthur holds court at Camelot (returns there specifically for this). Special quests available only on this day. Round Table members get a bonus reward. Knights report their progress
  - **Day 27: Night of Spirits** -- ghosts and undead surge across the overworld. Abandoned castles spawn extra enemies. Graveyards glow. But: rare spectral loot drops and ghosts may reveal hidden treasure. Ghost ship guaranteed to appear if you sail. Canterbury graveyard ghosts of previous characters are especially active
  - **Day 30: Harvest Moon** -- food is abundant. Apple trees give double yield. Inn meals cost half. Farmers offer free bread. A peaceful day before the cycle restarts
  - Moon phase affects certain mechanics continuously: werewolf strength scales with moon fullness, some spells are stronger/weaker by phase, faerie encounters vary
  - Events cycle every 30 days. Player is notified: "Tomorrow is the Full Moon" / "The Druid Gathering at Stonehenge is in 2 days"
  - All events defined in `data/events.csv` for easy modification
- **Camping on overworld**: press `c` to camp (only in safe terrain -- grassland, road). Advances time 8 hours. Restores 50% HP/MP. Chance of night ambush (higher off-road)
- Time of day defined in `data/time.csv` for tweaking encounter modifiers

**Weather system**:
- Weather changes randomly every 100-300 turns, with seasonal/regional tendencies
- Weather types:
  - **Clear** -- normal conditions, no effects
  - **Rain** -- reduces visibility by 2 tiles on overworld, -1 SPD, ranged attacks less accurate. Rivers swell (some shallow crossings become impassable)
  - **Heavy Rain/Storm** -- reduces visibility by 4, -2 SPD, chance of lightning strike (small HP damage), cannot camp. Thunder sound effects in message log
  - **Fog** -- drastically reduced visibility (FOV 3 on overworld), encounters happen at close range (harder to flee), but also harder for enemies to spot you
  - **Snow** (northern regions, winter) -- -1 SPD, marsh/swamp becomes frozen (walkable!), lakes freeze over (can walk across without boat). Cold damage over time without warm clothing or fire resistance
  - **Wind** -- ranged attacks affected, sailing on lakes faster in one direction, slower in opposite
- Weather shown on status bar with icon: `☀` clear, `🌧` rain, `⛈` storm, `🌫` fog, `❄` snow, `💨` wind (or ASCII alternatives: `*` clear, `/` rain, `!` storm, `~` fog, `+` snow, `>` wind)
- Some spells interact with weather: Lightning is stronger in storms, Fireball weaker in rain, Fog spell creates local fog
- **Rainbow event**: after rain clears during daytime, there is a 25% chance a rainbow appears on the overworld. A colourful arc (`=` multicolored) is drawn across a section of the map for 30-50 turns before fading:
  - One end of the rainbow touches a specific overworld tile. If the player reaches that tile before the rainbow fades, they find a **pot of gold** (100-500 gold!) guarded by a Leprechaun (`l` green, passive unless attacked)
  - The Leprechaun can be: bargained with (take half the gold peacefully), fought for all the gold (easy fight but -2 chivalry for greed), or ignored
  - If the rainbow fades before you reach it: "The rainbow shimmers and vanishes..." -- gold is gone
  - Rainbow direction is random. Message: "A rainbow appears in the sky to the [direction]!" -- player must navigate there quickly
  - Only one rainbow per rain cycle. Rare and rewarding event that encourages exploration
- Weather defined in `data/weather.csv` with effects on visibility, speed, encounter rates, hazards

**Horses & mounts**:
- Horses can be bought at **stables** (found in larger towns: Camelot, London, York, Winchester)
- Horse cost: 100-200 gold depending on quality
- Horse types:
  - **Palfrey** (100g) -- standard riding horse, 2x overworld movement speed
  - **Destrier** (200g) -- war horse, 2x speed + +2 DEF in overworld encounters (mounted combat bonus)
  - **Pony** (50g) -- 1.5x speed, can traverse hills without speed penalty
- While mounted:
  - Overworld movement is faster (2x tiles per turn)
  - Cannot enter dungeons while mounted -- horse waits at the dungeon entrance. There is a small chance (5% per 100 turns inside) the horse wanders off while you're in the dungeon ("Your horse got bored and trotted away...")
  - Cannot enter buildings (shops, inns, castles) while mounted -- auto-dismount, horse waits outside. If you spend more than 200 turns inside, 10% chance the horse runs away
  - Mounted combat: +2 STR for charge attacks on first encounter turn, but horse can be injured
- Horse can be **lost**: if player is defeated in overworld combat and flees, horse may bolt (50% chance). If horse takes damage in combat, it can die. Horse left outside too long may wander off
- Horses bought at stables in towns (Camelot, London, York, Winchester, and larger castles)
- Horse shown as `h` following player on overworld, player glyph changes to `±` (mounted) on overworld
- **Stables in towns**: buy, sell, heal injured horses. Can only own one horse at a time
- Dismount with `d` on overworld, auto-dismount entering towns/dungeons

**Overworld random encounters**:
- While moving on the overworld, random chance of encounter (higher off-road, higher at night, affected by weather)
- Encounter types by terrain: wolves in forests, bandits on roads, wild boars in hills
- Encounters spawn a small combat map (20x15) with the enemy group
- After combat, return to overworld at same position
- Option to flee (chance-based, easier on roads, harder at night/in fog)
- Mounted players get flee bonus (+20% chance)
- **Result:** Dynamic overworld with time, weather, mounts, and encounters

### Phase 4 -- Towns: Shops, Inns, Mystics
- Entering a town shows a town interior map (small fixed layout per town) OR a menu:
  - **Equipment Shop** (`$`) -- buy/sell weapons and armor, stock varies by town
  - **Potion Shop** (`!`) -- buy healing potions, mana potions, antidotes
  - **Shop interaction system** (applies to all shops):
    - **Haggling**: press `h` when buying/selling to negotiate price. Success based on chivalry + INT check. High chivalry (60+): 10-20% discount. Low chivalry: shopkeeper offended, prices increase 10%. Failed haggle: "The shopkeeper frowns at your offer"
    - **Stealing**: press `t` while in a shop to attempt theft of a displayed item. SPD check to grab and run. Success: free item, but **-8 chivalry**, shopkeeper screams and town guards pursue for 50 turns. Failure: shopkeeper attacks (shopkeepers are tough fighters!), **-5 chivalry**, banned from that shop permanently. Guards alerted
    - **Reputation per shop**: each shop tracks whether you've stolen from it. Stolen from = banned permanently. Other shops in the same town hear about it (prices +20% in that town)
    - High chivalry (70+) unlocks "loyal customer" status at frequently visited shops: 15% permanent discount
  - **Church** (`C` white) -- found in most towns. A priest (`p` white) tends the church:
    - **Pray**: restore some HP/MP (less than a shrine, but free and repeatable)
    - **Donate gold**: leave gold at the church altar. +1 chivalry per 20 gold donated. Message: "The priest blesses you for your generosity"
    - **Confession**: if chivalry is low, confess sins to the priest to regain +3 chivalry (once per church per visit). "Your sins are forgiven, go forth and do good"
    - **Buy holy water** and simple healing herbs from the priest (cheap prices)
    - **Loot the church**: steal the collection plate and altar items (50-100g). **-12 chivalry**, priest turns hostile and calls town guards. Church is permanently locked to you in that town. Other town NPCs charge you higher prices if they hear about it
    - **Attacking a priest**: **-20 chivalry** (the gravest sin). All town NPCs turn hostile. You are branded "Priest-Slayer" -- visible on status bar until chivalry is restored above 50. Priests in other towns will refuse to help you until the brand is lifted
    - Priests can also cure poison and remove minor curses (for a donation of 30+ gold)
  - **Pawn Shop** (`P`) -- buy and sell ANY item type regardless of category. Buys everything you want to sell (weapons, armor, potions, scrolls, junk, quest items you no longer need). Sells at 40% of item value (worse than specialist shops which give 50%), buys at 80% of value (more expensive than specialists). Found in most towns. Good for offloading misc loot quickly
  - **Bank** (`B`) -- deposit and withdraw gold. Found in London, Camelot, Winchester, and York:
    - Deposit any amount of gold for safekeeping
    - Gold in the bank persists **across deaths** -- stored in `~/.camelot/bank.dat` separately from save file
    - When you start a new game after dying, visit any bank to withdraw your previously deposited gold
    - A small fee (5%) is charged on deposits (the banker needs to eat too)
    - No fee on withdrawals
    - Interest: gold in the bank earns 1% interest per 500 game turns (very slow, but rewards long-term saving)
    - Cross-game persistence systems: bank (gold), home chest (items), Canterbury graveyard (memorial), fallen heroes (records), high scores
    - Bank balance shown when interacting with the banker
  - **Inn** (`I`) -- rest to restore full HP/MP, costs gold, flavor text about beer/food. Also where quest-giving NPCs hang out (see Phase 5).
    - **Beer & drinking**: buy beer at the inn bar (2g per pint). Each beer drunk adds to a **drunkenness counter**:
      - 1 beer: "You feel merry" -- no gameplay effect, +1 hunger
      - 2 beers: "You feel tipsy" -- occasional random movement (10% chance per turn, lasts 30 turns)
      - 3 beers: "You are drunk" -- random movement 25% of turns, -1 INT, +1 STR (liquid courage), lasts 50 turns
      - 4 beers: "You are very drunk" -- random movement 40% of turns, -2 INT, -1 SPD, +2 STR, vision blurs (FOV reduced by 2), lasts 70 turns
      - 5+ beers: "You pass out" -- black screen, wake up outside the inn 8 hours later with 50% HP, all gold intact but 1 random item missing ("Where did your sword go?"), massive hangover (-2 all stats for 100 turns)
    - Beer types vary by town (flavour text): Camelot Ale, York Bitter, Sherwood Stout, Welsh Mead, London Porter, Canterbury Holy Water Ale
    - Drinking is a fun risk/reward: cheap hunger restoration but dangerous if overdone
    - Drunkenness wears off over time (counter decreases 1 per 20 turns when not drinking)
  - **Mystic** (`?`) -- pay gold to have fortune told; random stat boost OR penalty (+1/-1 to a random stat), with cryptic dialogue ("The stars favor your strength..." or "A dark cloud hangs over your wisdom...")
  - **Well** (`O`) -- appears randomly in some towns (not all). Player can choose to climb down:
    - Chance of finding random treasure (gold, potion, rare equipment, or even a unique item)
    - Chance of encountering a monster (rat, snake, skeleton) -- fight in a small confined space
    - Chance of finding nothing ("The well is dry and empty...")
    - Risk/reward mechanic: better loot possible but tougher monsters in later-game towns
    - Each well can only be explored once per visit (resets when you leave and return to town)
- Each town has a different mix (not all have every service)
- Town NPCs with dialogue for flavor and quest hints
- **Famous characters** at fixed locations (unique named NPCs, interact by bumping):
  - **King Arthur** (Camelot Castle, throne room) -- the first thing the player must do is visit Arthur to receive the Holy Grail quest. Arthur grants a knighthood (+2 DEF, +1 STR) and bestows the main quest. **The Grail will not spawn in Glastonbury Tor until this quest is accepted.** Arthur waits in Camelot Castle until the quest is collected -- he will not leave. Once the quest is given, Arthur begins travelling his kingdom, visiting allied castles (Warwick, Windsor, Benwick, etc.) on a rotation (~100 turns per castle). Finding Arthur again later: he can grant a royal sword if you've completed 3+ quests, and offers counsel about your progress. His current location can be learned by asking castle guards or innkeepers ("The King rode north toward York three days past...").
  - **Round Table membership** -- after accepting the Grail quest, Arthur offers a second quest to prove yourself worthy of joining the Knights of the Round Table:
    - Quest: "Prove your valor -- clear the Camelot Catacombs and return the Dark Monk's sigil to me"
    - Requires: chivalry 40+, Catacombs boss defeated, sigil item returned to Arthur
    - **Reward**: knighted as a Knight of the Round Table. Permanent title displayed on status bar. Benefits:
      - +3 chivalry permanently
      - All allied castle kings treat you as an equal (best prices, best quests, access to royal vaults)
      - Can command allied knights on the road to join you temporarily (ask for aid)
      - Unlocks the Round Table chamber in Camelot Castle: a special room where you can sit at the table and receive visions (hints about artifact locations, quest solutions, or boss weaknesses -- one vision per dungeon cleared)
      - Arthur's personal squire follows you for 100 turns carrying extra supplies (acts as extra inventory space)
  - **Merlin** (Glastonbury) -- teaches the player a spell (class-appropriate) or buffs INT/MP. Offers cryptic guidance about dungeon dangers and the Grail's location.
  - **Morgan le Fay** (Cornwall) -- offers a dark bargain: a powerful boon (large stat boost or rare enchanted item) at a cost (permanent HP reduction or cursed debuff). Player can accept or decline.
  - **Queen Guinevere** (Camelot Castle, throne room) -- Arthur's queen. Gives quests related to courtly intrigue and diplomacy. Rewards: royal favour (shop discounts across all towns), a blessed amulet (+2 INT), or key information about castle secrets. If player has high Light affinity, she grants access to the royal vault.
  - **Sir Lancelot** (Castle Benwick -- his father King Ban's castle -- or wandering the overworld) -- the greatest knight. Can be encountered as a sparring partner: defeat him in a duel for a massive XP reward and his respect (+2 STR, +2 DEF). If player has recovered his lost sword from Castle Dolorous Garde, he becomes a temporary ally who fights alongside you for 50 turns. Also offers knightly advice on dungeon bosses.
  - **Breunis sans Pitie** (roaming the overworld as a hostile encounter, or lurking in an abandoned castle) -- the merciless knight, a recurring villain. A dangerous enemy who ambushes the player on roads and open terrain. Tough fight (high STR/DEF, aggressive AI, never flees). Defeating him yields a unique dark sword and large gold reward. He can appear multiple times, growing stronger each encounter -- a persistent rival throughout the game.
  - **Mad Knights** -- rare random overworld encounters (`k` red, flashing). Knights who have lost their minds from battle, curses, or failed quests:
    - Appear randomly on roads and near castles (~1 per 400 overworld turns)
    - **Unpredictable behaviour** (50/50 each encounter):
      - **Aggressive**: charges at you screaming gibberish, attacks immediately. Moderate stats (mid-tier fighter). Drops random equipment (whatever they were wearing/wielding) and a small amount of gold
      - **Passive**: mutters to themselves and wanders off in a random direction. Harmless, ignores you completely. May drop an item as they stumble away
    - Killing a mad knight: no chivalry penalty (defending yourself) but no bonus either. It's a tragic encounter
    - Rarely (10%), a mad knight is actually a **cursed knight** -- if you have Purify spell or holy water, you can cure them instead of fighting. Cured knight: **+5 chivalry**, gives you their sword in gratitude, and tells you about a hidden treasure or quest. Major reward for mercy
    - Named mad knights (very rare): occasionally encounter a mad version of a famous knight (mad Sir Pellinore, mad Sir Balin) with unique loot
  - **Lady of the Lake** (found at the Lake of the Lady on the overworld) -- a mystical figure who rises from the water when the player steps onto the lake shore tile. She offers **Excalibur**, the legendary sword (highest damage weapon in the game, +Light affinity, bonus damage vs evil). To receive it, player must prove worthy: chivalry 40+ AND either completed 5+ quests or have high Light affinity (15+). If unworthy, she tells you to return when you have proven yourself. Excalibur glows (`!` yellow bold) and also grants +2 STR while wielded.
- **Castle cat** -- in any active castle, there's a chance a cat (`c` yellow) is lounging around. Interact to pick it up as a pet:
  - The cat follows you (`c` trailing behind `@`) on the overworld and in dungeons
  - While you have the cat, **no rat encounters** spawn (Giant Rats, rats in wells, etc.)
  - The cat provides a small morale boost (+1 SPD)
  - After a random duration (100-300 turns), the cat wanders off on its own ("The cat loses interest and saunters away...") -- because cats
  - Only one cat at a time. You cannot force the cat to stay
  - Rare chance the cat brings you a small gift before leaving (mouse corpse = food item, shiny coin = 5 gold, a hairball = nothing useful)
- **Witch encounters** -- random bad luck event on the overworld (especially near swamps, forests, and moors):
  - A witch (`W` green) appears and curses you with a **geas** (a compulsory task). You have a limited number of turns to complete it or be automatically cursed
  - **Witch tasks** (randomly selected):
    - "Bring me 3 mushrooms from the forest" (50 turns) -- forage mushrooms and return to the witch's location
    - "Slay the wolf that stole my cat" (80 turns) -- a special wolf spawns nearby, must kill it
    - "Deliver this cursed amulet to the monk in Canterbury" (120 turns) -- carry a cursed item to Canterbury (it weighs you down and drains MP while held)
    - "Fetch me a gem from the nearest dungeon" (100 turns) -- bring any gem back to her
    - "Stand in the stone circle at midnight" (wait for nightfall, go to Stonehenge or faerie ring) (60 turns)
    - "Brew me a potion" (80 turns) -- bring her 2 specific potion ingredients (random)
  - **If completed in time**: witch is appeased, lifts the geas, and may offer a small reward (a potion, a dark spell scroll, or a cryptic hint). +2 Dark affinity, **-2 chivalry** (doing a witch's bidding is never honourable)
  - **If failed (timer runs out)**: witch curses you -- permanent stat reduction (-2 to a random stat) until cured at a shrine, with Purify spell, or by completing a harder witch task later. Curse shown on status bar. **-3 chivalry** (you failed and were cursed -- shameful)
  - Witch tasks always cost chivalry regardless of outcome -- they are never positive for your honour. A true dilemma: complete the task (small chivalry loss, avoid curse) or refuse/fail (bigger chivalry loss + curse)
  - Witches cannot be attacked (they vanish in smoke if you try, and the curse is applied immediately)
  - Encounter frequency: ~1 witch encounter per 500 overworld turns (bad luck). Higher in swamps/forests
  - Some witches are recurring -- fail one and she may appear again with a harder task
  - Witch encounters defined in `data/witch_tasks.csv`
- **Damsel in distress** -- random event that can trigger on the overworld or in dungeons:
  - A damsel (`d`) appears surrounded by enemies (bandits, wolves, or a monster)
  - Player must defeat the enemies to rescue her
  - Rewards: gold, a kiss (restores full HP/MP), a lucky charm (temporary stat buff), or she reveals a hidden treasure location
  - Ignoring or failing to save her has no penalty but you miss the reward
  - Multiple damsel scenarios: kidnapped by bandits on the road, trapped in a dungeon room, cornered by wolves in the forest
- **Jousting tournaments** -- held at active castles (Camelot, Warwick, Castle Strangore, Castle Gore):
  - Enter by speaking to the tournament herald (`H` yellow) in the castle courtyard
  - Entry fee: 20-50 gold. Must own a horse (Destrier gives a bonus)
  - **Tournament format**: 3 rounds of jousting, each against a progressively tougher knight
    - Round 1: Local squire (easy), Round 2: Rival knight (medium), Round 3: Castle champion (hard)
  - **Jousting mechanic**: timed button press mini-game -- press `space` at the right moment as your opponent charges (shown as ASCII animation). Timing determines hit accuracy:
    - Perfect: unhorse opponent instantly (win round)
    - Good: solid hit, opponent takes damage, may fall in 2-3 passes
    - Miss: you take damage or get unhorsed
  - Each pass: choose aim point -- head (risky, instant KO but easy to miss), chest (balanced), shield (safe, low damage)
  - **Rewards**: Round 1 win: 30 gold. Round 2: 80 gold + chivalry. Round 3 (champion): 200 gold, +5 chivalry, a unique prize item (varies by castle -- fine weapon, enchanted lance, etc.)
  - **Grand Tournament at Camelot**: available after collecting the Grail quest. Win all 3 rounds against the best knights in the land. Grand prize: a legendary jousting lance and the title "Tournament Champion" (displayed on status bar, +3 chivalry permanently)
  - Losing a round: take damage, lose entry fee, small chivalry loss (-1). Can retry next visit
  - Tournaments reset each time you visit a castle

- **Treasure maps** -- rare items found in dungeons, bought from shady NPCs in inns, or dropped by bandits:
  - Appear in inventory as "Tattered Map" (`&` brown) -- use (`u`) to view the map
  - Map shows a crude ASCII drawing of a section of the overworld with an `X` marking the treasure location
  - Player must navigate to the spot on the overworld and dig (`x` on the tile) to unearth the treasure
  - **Treasure types** (randomized):
    - Small cache: 50-100 gold + a potion or two
    - Medium cache: 150-300 gold + a decent weapon/armor
    - Large cache: 500+ gold + a rare/unique item, possibly a Spell Scroll
    - Trapped cache: treasure is guarded by a buried undead (Barrow-Wight, Skeleton Knight) who emerges when you dig
  - ~5-8 treasure maps exist per game, scattered across dungeons and NPC inventories
  - Maps can be sold to collectors in London for gold if you don't want to hunt for treasure
  - Some maps are fake (drawn by bandits to lure you to an ambush spot -- triggers a bandit encounter with no treasure)
  - Legendary map: one rare map per game leads to the location of a Legendary Artifact that isn't in a fixed location

- **Result:** Towns are functional hubs for resupply, risk/reward stat gambling, jousting fame, and encounters with legendary characters

### Phase 5 -- Quest System
- 10-15 side quests picked up from NPCs in inn taverns, in addition to the main Grail quest
- Press `J` (capital J) to open quest journal showing active/completed quests
- Quest types (mix of all three):
  - **Fetch quests** -- retrieve an item from a dungeon and return it to the quest giver
    - "Find my grandfather's shield in the Catacombs" (Camelot inn)
    - "Retrieve the ancient tome from Tintagel Caves" (Tintagel inn)
    - "Bring me a Dragon scale from the Welsh mountains" (Winchester inn)
  - **Kill quests** -- slay a specific enemy or clear a threat
    - "A ghost haunts Sherwood Depths, put it to rest" (Sherwood inn)
    - "Bandits have a hideout in the Catacombs, clear them out" (Camelot inn)
    - "The Black Knight blocks the road to York, defeat him" (York inn)
  - **Delivery quests** -- carry an item between towns
    - "Deliver this holy water to the monk in Glastonbury" (Winchester inn)
    - "Bring this letter to the blacksmith in London" (York inn)
    - "Carry this healing herb to the mystic in Cornwall" (Sherwood inn)
- Quest rewards: gold, unique equipment, permanent stat boosts (+1 STR, +1 DEF, etc.)
  - Harder quests (deeper dungeons, longer deliveries) give better rewards
  - Some quests reward unique named items not found elsewhere
- Quest NPCs shown as `!` in inns, dialogue triggers quest offer (accept/decline)
- Quest state tracked per quest: NOT_STARTED, ACTIVE, COMPLETE, TURNED_IN
- Completing quests also grants XP
- **Rescue the Princess** -- a special major side quest, the game's **true ending**:
  - Picked up from an inn NPC or a distraught king in a random active castle: "My daughter has been kidnapped by an Evil Sorcerer! She is locked in a tower!"
  - The princess belongs to a **random king** (different each playthrough). The tower is a special dungeon location: a standalone tower (`|` white on overworld) that appears after accepting the quest, 3-5 levels tall
  - **Tower dungeon**: vertical layout (narrow, spiraling stairs), guarded by magical enemies (imps, enchantresses, golems). The Evil Sorcerer is the boss at the top
  - Defeat the Evil Sorcerer, free the princess. She thanks you and **disappears, returning to her father's castle**
  - Visit the princess at the castle. She offers to **marry you** (regardless of player gender -- "we are liberal"):
    - **Accept**: the game **ends**. This is the **one true ending** of the game
      - Victory screen: "You married Princess [name] and became King/Queen of [kingdom]. Your quest is truly complete."
      - Title changes from Knight to **King/Queen**
      - **+10000 score** (the highest possible score bonus)
      - Canterbury graveyard shows you as a **hero** with a golden tombstone: "[Name], King/Queen of [kingdom], who found love and glory"
      - Morgue file records the marriage ending
    - **Decline**: princess is disappointed but understanding. Quest marked as complete. +5 chivalry, +500 gold reward from the king. You continue playing
  - The quest requires chivalry 50+ to be offered (the king won't trust a dishonourable knight with his daughter)
  - The princess quest and the Grail quest are independent -- you can complete both, one, or neither
- **Result:** Rich side content, reasons to explore all towns and dungeons, and a true ending for those who seek it

### Phase 6 -- Dungeon Generation (BSP Rooms & Corridors)
- BSP algorithm: recursively split map, carve rooms in leaves, L-shaped corridors between siblings
- Min leaf 6x6, min room 4x3, depth ~5 -> 8-15 rooms per level
- Place stairs down `>` and stairs up `<`
- Glyphs: `#` wall, `.` floor, `+` door, `>` / `<` stairs
- **Interactive doors**:
  - `+` closed door, `/` open door. Press `o` adjacent to open, `c` to close
  - **Locked doors**: some doors are locked (`+` red). Require a key, Lock Pick item, or force (bash with STR check -- may break the door but makes noise alerting enemies). Knights get +3 to bash, Rangers can pick locks with higher success rate
  - **Secret doors**: hidden in walls, appear as `#` until found. Press `s` to search adjacent walls (INT check, 20% base chance per search). Rangers: 35% chance. Some secret doors revealed by Map spell or Scroll of Mapping
  - Doors block FOV when closed. Open doors allow sight through
  - Enemies can open unlocked doors but not locked ones (except bosses who bash them down)
- **Special dungeon rooms** -- BSP generator occasionally creates special room types (10-15% chance per room):
  - **Treasure vault**: walled-off room with locked door, contains a chest with good loot and gold. May be trapped
  - **Dungeon chests** (`=` brown) -- found in various rooms throughout dungeons (1-3 per dungeon level):
    - **Unlocked chests**: interact to open, contains 1-3 random items (potions, gold, food, scrolls, gems)
    - **Locked chests** (`=` red): require one of: Ranger lockpick skill (automatic for Rangers), Lockpick tool item, Unlock spell (Universal, 3 MP), or bashing (STR check, may destroy contents 30% chance). Knights and Wizards without tools/spells must bash and risk losing loot
    - **Trapped chests**: 20% of chests are trapped. Trap triggers on opening: poison dart, explosion (5 damage + destroys 1 item inside), gas cloud (confusion), or alarm (alerts enemies). Detect Traps spell or Ranger passive reveals chest traps. Can be disarmed with `D` before opening
    - **Mimic chest** (rare, 5%): the chest is actually a Mimic monster (`=` that attacks when you try to open). Moderate difficulty enemy, drops gold on defeat. A classic roguelike surprise
    - Chest contents scale with dungeon depth -- deeper chests contain better loot
  - **Monster lair**: room filled with 3-5 enemies of the same type and their loot hoard
  - **Underground temple**: an altar in the center. Pray to identify BUC status of all items, or receive a blessing (+1 random stat). Desecrating: -5 chivalry, summons hostile guardian
  - **Armoury**: weapon racks and armour stands. 3-5 equipment items, some may be cursed
  - **Library**: bookshelves with 1-3 spell scrolls and lore text (quest hints)
  - **Crypt**: coffins that may contain undead enemies or treasure. Search coffins for loot but risk waking a Wight or Revenant
  - **Flooded room**: shallow water tiles (`.` blue), -1 SPD to move through, water creatures may spawn. Some items submerged (search to find)
  - Special rooms defined in `data/special_rooms.csv`
- **Dungeon terrain hazards** -- beyond walls and floors:
  - `~` **shallow water**: -1 SPD, can extinguish fire effects, rusts unprotected iron equipment (10% chance per step)
  - `~` **deep water** (blue bold): impassable without Levitation or Walk on Water. Items can be thrown in (lost forever)
  - `^` **lava**: 10 fire damage per step without Fire Resistance. Items dropped in lava are destroyed
  - `_` **ice**: slippery, 30% chance to slide an extra tile in movement direction. -2 SPD. Can crack and become deep water if heavy (plate armour + heavy load)
  - `"` **fungal growth**: releases spores when stepped on (20% poison chance). Can be burned with fire
  - `%` **rubble**: destructible with pickaxe or force. May reveal secret passages. Blocks movement
  - `*` **crystal formation**: glows, increases FOV by 1 in that room. Can be mined for gems (pickaxe needed)
  - Terrain types defined in `data/dungeon_terrain.csv`
- Multiple dungeons across the overworld (7+ dungeons), each with different depth:
  - **Camelot Catacombs** (3-6 levels, beginner)
  - **Tintagel Caves** (5-10 levels, medium)
  - **Sherwood Depths** (5-10 levels, medium)
  - **Mount Draig Volcano** (Wales) -- an active volcano (`V` red on overworld) in the Welsh mountains. 4-8 dungeon levels descending into the caldera. Lava tiles (`.` red/orange) deal fire damage if stepped on without Fire Resistance. Fire-themed enemies (Fire Drakes, Hellhounds, Imps, lava golems). The **Red Dragon (Y Ddraig Goch)** lairs in the deepest magma chamber. Extreme heat: HP drains slowly without fire resistance potion/ring/spell
  - **Glastonbury Tor** (8-15 levels, hardest -- default Grail location, but Grail is randomly placed each game, see Phase 12)
  - **White Cliffs Cave** (3-6 levels, sea-themed)
  - **Whitby Abbey** (2-4 levels, vampire-themed)
  - Additional random dungeon entrances found via treasure maps or NPC hints (1-20 levels, randomized depth)
- **Dungeon depth is randomized** within ranges each playthrough -- you never know exactly how deep a dungeon goes. The number of levels is determined when you first enter
- Exiting a dungeon (ascending from level 0) returns to overworld
- **Persistent dungeon levels**: once generated, dungeon levels are stored in memory and persist:
  - Items dropped on a floor stay there. Killed monsters stay dead. Opened doors remain open
  - Players can stash items on cleared floors and return for them later
  - Returning to a previously visited level loads it exactly as left (no regeneration)
  - This allows strategic play: leave a stash of potions near the entrance, clear a level as a safe retreat point
  - Persistence is per-dungeon, saved with the game state
- **Dungeon branches & shafts** -- dungeons are not purely linear:
  - **Branches**: some levels have 2 sets of stairs down, leading to different sub-branches. One branch may be the main path, the other a side vault or challenge area with unique loot. Branches rejoin after 1-2 levels or dead-end with a portal back
  - **Shafts**: rare tiles (`>` red) that drop the player 2-3 levels at once. One-way -- must find stairs back up. Higher risk (skip to harder levels underequipped) but skip cleared content
  - **One-way drops**: crumbling floor tiles that collapse when stepped on, dropping to the level below. Cannot return the same way -- must find stairs. Creates tension and prevents easy retreat
  - **Secret levels**: very rare hidden stairs (behind secret doors) leading to small bonus levels with unique treasure and tough enemies. One per dungeon at most
- **Exit portal**: on the deepest level of every dungeon, a **portal** (`0` cyan glow) is placed that teleports the player instantly back to the dungeon entrance on the overworld. This saves backtracking through cleared levels
- **Scroll of Recall**: a rare and valuable scroll that can be found in dungeons, bought from mystics, or rewarded by NPCs:
  - When read, it begins a **recall countdown** (random 5-15 turns). A message displays: "You feel the magic of recall building... (X turns remaining)"
  - After the countdown, the player is teleported to the entrance of the current dungeon (overworld tile)
  - During the countdown, the player can still move and fight normally -- but if they take a hit that reduces HP below 10%, the recall is disrupted ("The recall spell fizzles!")
  - If used on the overworld, recall teleports to the nearest town instead
  - Extremely useful for escaping deep dungeons when low on resources. Stackable, moderate weight (0.5)
- **Dungeon traps** -- scattered on dungeon floors, hidden until triggered or detected:
  - Traps are invisible by default (appear as normal floor `.`). Revealed by: Detect Traps spell, Ranger passive (50% chance to spot within FOV), or stepping on one (ouch)
  - Once revealed, shown as `^` (red) and can be stepped over carefully or disarmed
  - **Trap types** (defined in `data/traps.csv`):
    - **Pit Trap** -- fall into a pit, take 5-10 damage, stuck for 1 turn
    - **Poison Dart** -- dart shoots from wall, 3 damage + poison DOT for 10 turns
    - **Tripwire** -- stumble, lose 1 turn, items may scatter from inventory (drop 1 random item)
    - **Pressure Plate -- Arrow** -- triggers arrow volley, 8 damage
    - **Pressure Plate -- Alarm** -- alerts all enemies on the floor to your position (all enter CHASE state)
    - **Bear Trap** -- immobilized for 3 turns, 4 damage, must spend a turn to free yourself
    - **Fire Trap** -- burst of flame, 10 damage, burns scrolls in inventory (10% chance per scroll)
    - **Teleport Trap** -- teleported to random tile on current floor
    - **Gas Trap** -- confusion gas, random movement for 10 turns
    - **Collapsing Floor** -- fall to the next dungeon level (or take 15 damage if on the deepest level)
    - **Spike Trap** -- 6 damage, -1 SPD for 20 turns (leg injury)
    - **Magic Drain Trap** -- lose 10-20 MP
  - Trap density: 3-8 per dungeon level, more on deeper levels
  - Rangers spot traps passively, Detect Traps spell reveals all on current floor
  - Disarming: step adjacent and press `D` -- success based on INT + SPD check. Failure triggers the trap. Rangers get +5 bonus to disarm
  - Abandoned castles have castle-themed traps: portcullis drops (blocks corridor), arrow slits (continuous damage zone), collapsing masonry
- **Magic circles** -- glowing arcane circles found randomly in dungeons and on the overworld:
  - Appear as `()` (various colors: blue, red, green, purple) on the ground in dungeon rooms or overworld clearings
  - Stepping into a magic circle triggers a random magical effect -- can be beneficial or dangerous:
  - **Beneficial circles**:
    - **Circle of Healing** (white glow) -- restores full HP
    - **Circle of Mana** (blue glow) -- restores full MP
    - **Circle of Enchantment** (gold glow) -- randomly enchants one equipped item (+1 to its primary stat)
    - **Circle of Knowledge** (purple glow) -- identifies all items in inventory
    - **Circle of Warding** (silver glow) -- grants temporary invulnerability for 10 turns
    - **Circle of Teleportation** (green glow) -- teleports to a random magic circle elsewhere in the world (fast travel!)
    - **Circle of Summoning** (cyan glow) -- summons a friendly elemental ally for 30 turns
  - **Dangerous circles**:
    - **Circle of Fire** (red glow) -- burst of flame, 15 damage, may destroy scrolls
    - **Circle of Binding** (dark red) -- enemy summoned and must be defeated before you can leave the room
    - **Circle of Draining** (black glow) -- drains 50% of current MP
    - **Circle of Confusion** (swirling colors) -- confusion effect for 20 turns
    - **Circle of the Void** (no glow, invisible) -- teleports a random equipped item away (lost forever!)
  - Circles are single-use (fade after activation) except Circle of Teleportation (permanent, reusable, acts as a waypoint network)
  - Wizards have a 30% chance to sense a circle's type before stepping in. Rangers can detect dangerous circles
  - ~1-2 circles per dungeon level, ~5-8 scattered across the overworld
  - Circle of Teleportation network: each one you discover is added to your known circles list. Using any teleport circle lets you choose which discovered circle to travel to -- powerful fast travel that rewards exploration
  - **Druids** (`D` green) -- found near overworld magic circles and at Stonehenge:
    - Harmless -- cannot be attacked (they phase away if you try)
    - When encountered, there is a 30% chance a druid pickpockets some of your gold (10-30g stolen, "A druid's hand brushes your coin purse...")
    - Otherwise they are friendly and may: teach a Nature spell, share lore about nearby circles, offer to identify a potion for free, or give a cryptic prophecy about your quest
    - If you have high Nature affinity (20+), druids never steal from you and always offer help
    - Druids sometimes tend to magic circles, keeping them charged -- if you see a druid at a circle, the circle is guaranteed to be beneficial
    - 2-4 druids wander near circles and Stonehenge at any given time
- **Result:** Multiple dungeons with varying difficulty, traps and magic circles add tactical danger, connected to overworld

### Phase 7 -- Field of View & Fog of War (Dungeons)
- Recursive shadowcasting in 8 octants, radius 8
- `visible` = currently in FOV, `revealed` = ever seen
- Visible tiles: full color. Revealed: `A_DIM`. Unknown: blank
- Full shadowcasting applies in dungeon mode. Overworld uses simpler radius-based visibility (full during day, reduced at night/weather -- see Phase 3)
- **Result:** Only nearby dungeon area visible, fog of war on explored areas

### Phase 8 -- Enemies, Combat, Message Log
- 80+ monster types across all environments, defined in `data/monsters.csv`. Categories:
  - **Beasts** (overworld, early dungeons): Giant Rat, Wolf, Wild Boar, Stag (passive unless provoked), Bear, Giant Spider, Bat Swarm, Adder, Rabid Dog, Giant Beetle
  - **Bandits & Outlaws** (roads, forests, early dungeons): Bandit, Bandit Chief, Highwayman, Poacher, Rogue Archer, Cutthroat, Smuggler, Deserter
  - **Undead** (dungeons, abandoned castles, graveyards): Skeleton, Skeleton Knight, Zombie, Ghoul, Wraith, Spectre, Ghost, Revenant, Bone Dragon, Wight, Barrow-Wight, Death Knight
  - **Dark Knights & Warriors** (mid-late dungeons, overworld): Black Knight, Dark Squire, Fallen Paladin, Mercenary, Rogue Knight, Iron Golem, Breunis sans Pitie (recurring)
  - **Magical Creatures** (varied): Witch, Evil Sorcerer, Dark Monk, Warlock, Necromancer, Enchantress, Imp, Hellhound, Shadow Fiend, Demon
  - **Dragons** (Wales, deep dungeons): Young Drake, Fire Drake, Ice Drake, Red Dragon (Y Ddraig Goch), Black Dragon, Dragon Whelp, Wyvern
  - **Faerie & Forest** (forests, faerie rings): Will-o'-the-Wisp, Pixie Swarm, Redcap, Spriggan, Green Man, Treant, Bog Hag, Forest Troll, **Wood Nymph** (`n` light green)
    - **Wood Nymph** -- encountered randomly in forests. Not hostile (cannot be attacked -- she vanishes if you try). She appears, giggles, and **steals one random item from your inventory** before disappearing into the trees ("A wood nymph snatches your Longsword and vanishes!"). The stolen item is gone permanently. Higher SPD gives a chance to resist the theft (SPD check: if SPD > 6, 30% chance to hold onto your item). Ring of Free Action also prevents nymph theft. Rare chance (10%) she drops a **Nymph's Kiss** item instead of stealing -- a charm that grants +2 SPD for 100 turns
  - **Water Creatures** (lakes, rivers, coastal): Water Serpent, Kelpie, Giant Pike, Nixie, Nessie (boss), Merfolk Warrior, Sea Serpent, Giant Crab, Drowned Knight
  - **Giants & Trolls** (hills, mountains, deep dungeons): Hill Troll, Cave Troll, Stone Giant, Frost Giant, Ogre, Ettins, Cyclops
  - **Mythical & Boss** (unique encounters): Mordred (final boss), Dragon (Wales boss), Questing Beast, Green Knight, Pellinore's Beast, Manticore, Griffin, Basilisk, Chimera
  - **Werewolf** (`W` gray/red) -- encountered **only at night** on moors, marshes, and hills:
    - Fast, aggressive, high STR. Regenerates HP each turn (2 HP/turn). Only **Silver Dagger** or **Holy weapons** deal full damage -- all other weapons deal half damage
    - Bite attack has a 10% chance to inflict **Lycanthropy curse**:
      - **Permanent buff**: +3 STR, +2 SPD while cursed (the wolf within strengthens you)
      - **Full Moon transformation** (Day 15 of each 30-day cycle): at nightfall, the player involuntarily transforms into a werewolf. Screen goes red, message: "The moon rises full... you feel the change coming... AWOOOO!"
      - During transformation: player loses control. Screen blacks out. When you "wake up" it is morning, and you are at a **random location** on the overworld map
      - You have **lost 1-2 random items** from your inventory (dropped somewhere during the rampage)
      - There is a chance (20%) you killed an innocent NPC during the night -- if so, **-5 chivalry** and a bounty in the nearest town
      - Hunger is fully restored (you... ate something)
      - The transformation happens every 30 days on the full moon, unavoidable while cursed
      - **Cure**: Purify spell, Canterbury shrine, Potion of Cure Disease, or Silver Dagger self-inflicted ritual (costs 10 HP, cures lycanthropy permanently)
    - Werewolf Pelt dropped on kill -- valuable, can be sold or used to craft warm clothing (cold resistance)
    - Rare variant: **Alpha Werewolf** (larger, tougher, 20% bite curse chance) -- found only on remote moors deep into the night
- Bump-to-attack: `damage = str + weapon - def - armor + rand(-2,2)`, min 0
- Enemy turns: move toward player (simple), attack if adjacent
- Message log: ring buffer of 50 strings, display 3-5 most recent at bottom
- **UI layout** -- uses ncurses windows/panels. Minimum terminal size: 100x30 (wider than classic 80x24 to fit sidebar):
  ```
  ┌─────────────────────────────────────────────────────────┬──────────────────┐
  │                                                         │ Sir Galahad      │
  │                    MAP AREA                              │ Knight Lv.7      │
  │                   (60x22 tiles)                          │ Male             │
  │                                                         │                  │
  │  Player moves here, dungeon/overworld/town rendered      │ HP: 25/30 ██▓░░ │
  │                                                         │ MP: 10/15 ██▓░░ │
  │                                                         │ STR: 6           │
  │                                                         │ DEF: 5           │
  │                                                         │ INT: 4           │
  │                                                         │ SPD: 5           │
  │                                                         │ Wt: 45/60       │
  │                                                         │ Gold: 120        │
  │                                                         │ Chivalry: 42     │
  │                                                         │  "Knight"        │
  │                                                         │                  │
  │                                                         │ Turn: 1523       │
  │                                                         │ Day 2 14:30      │
  │                                                         │ Weather: Clear   │
  │                                                         │                  │
  │                                                         │ Weapon: Longswd  │
  │                                                         │ Armor:  Chain    │
  │                                                         │ Rings:  Prot/Reg │
  ├─────────────────────────────────────────────────────────┴──────────────────┤
  │ Status: Poi:8 Hst:5 Hungry                                                │
  ├────────────────────────────────────────────────────────────────────────────┤
  │ The wolf bites you for 3 damage.                                           │
  │ You hit the wolf for 5 damage. The wolf dies.                              │
  │ You see a potion here.                                                     │
  └────────────────────────────────────────────────────────────────────────────┘
  ```
  - **Map area** (left, ~60x22): the main game view, renders dungeon/overworld/town
    - **Camera follows player, centered**: the player `@` is always kept in the center of the map viewport. The map scrolls around the player. When the player approaches the edge of the map (dungeon wall, overworld boundary), the player moves toward the edge of the viewport while the map stops scrolling. This keeps orientation intuitive and the player always visible
  - **Character sidebar** (right, ~18 wide): persistent display of character stats, equipment, and time info. Always visible
    - Press `@` to **expand sidebar** into a full-screen character sheet showing: all stats, all equipment with details, all known spells, affiliation scores, quest summary, kill count, complete inventory overview
  - **Status effect bar** (1 line, below map): active effects with colour-coded turn counters
    - Green (beneficial): `Hst:5` `Shd:12` `Reg:8` `Inv:15`
    - Red (harmful): `Poi:8` `Cnf:10` `Bld:5` `Crse`
    - Yellow (neutral): `Hgr:Hungry` `Enc:Burdened` `Vmp` `Lyc`
    - Most critical shown first (harmful > beneficial > neutral). Effects flash when applied
  - **Message log** (bottom, 3-5 lines): scrolling combat and event messages
    - Press `Ctrl+P` to **expand** into full-screen message history (200 messages with turn numbers, scrollable)
    - Important messages highlighted in colour (red=damage, green=healing, yellow=quest, cyan=loot)
  - **Adaptive layout**: detect terminal size with `getmaxyx()`. If terminal < 100 wide, collapse sidebar and show stats on top bar instead (classic roguelike layout). If very tall terminal (40+), show more message log lines
  - **Turn counter**: persists through save/load, drives all time-based systems (day/night, hunger, weather, king travel, witch timers, buff durations, etc.)
  - Death screen and high score table show total turns survived
- Works in both dungeon and overworld encounter modes
- **Result:** Enemies fight back, combat messages scroll

### Phase 9 -- Items & Inventory (Weight System)
- Item types: weapons, armor, potions, scrolls, food, quest items
- 100+ items defined in `data/items.csv`, across categories:
  - **Weapons -- Swords** (12): Rusty Sword, Short Sword, Longsword, Broadsword, Claymore, Bastard Sword, Falchion, Rapier, Flamberge, Lancelot's Sword (unique), Excalibur (unique), Dark Blade of Breunis (unique)
  - **Weapons -- Axes & Maces** (8): Hand Axe, Battle Axe, War Hammer, Mace, Flail, Morning Star, Great Axe, Holy Mace (bonus vs undead)
  - **Weapons -- Daggers & Short** (6): Rusty Dagger, Dagger, Stiletto, Poisoned Dagger (DOT), Silver Dagger (bonus vs werewolves), Faerie Knife (weightless)
  - **Weapons -- Ranged** (6): Short Bow, Longbow, Crossbow, Sling, Throwing Knives (stackable), Enchanted Bow (homing arrows)
  - **Weapons -- Staves** (5): Wooden Staff, Iron-Shod Staff, Wizard's Staff (+INT), Druid Staff (+Nature), Staff of Light (+Light spells)
  - **Armor -- Body** (10): Tattered Robes, Leather Armor, Studded Leather, Chainmail, Scale Mail, Banded Mail, Plate Armor, Enchanted Robe (+MP regen), Nessie Scale Armor (unique), Dragon Scale Armor (unique)
  - **Armor -- Shields** (6): Buckler, Round Shield, Kite Shield, Tower Shield, Enchanted Shield (magic resist), Arthur's Shield (unique, +chivalry)
  - **Armor -- Helmets** (5): Leather Cap, Iron Helm, Great Helm, Crown of Wisdom (+INT), Horned Helm (+STR)
  - **Armor -- Boots** (4): Leather Boots, Iron Boots (heavy, +DEF), Elven Boots (weightless, +SPD), Boots of Water Walking
  - **Armor -- Gloves** (4): Leather Gloves, Gauntlets, Gauntlets of Strength (+2 STR), Thief's Gloves (+steal chance)
  - **Enchanted Attire** (8) -- magical clothing and accessories, usable by any gender but with particular magical affinity:
    - **Enchantress's Gown** (body armor): +4 INT, +3 max MP, +10% spell damage. Light weight (3). Glows faintly purple. Found in Castle Perilous or gifted by Morgan le Fay
    - **Moonsilver Dress** (body armor): +3 DEF, +2 SPD, grants Invisibility at night. Weightless. Found on Avalon or faerie ring reward
    - **Sorceress Heels** (boots): +2 INT, +1 SPD, -1 noise (better stealth). Enchanted to never slip. Found in mystic shops or dungeon loot
    - **Crystal Hairpin** (helmet slot): +2 INT, +5 max MP, identifies potions automatically when picked up. Weightless. Rare drop from Enchantress enemies
    - **Faerie Tiara** (helmet slot): +1 all stats, +Nature affinity, faeries always friendly while worn. Found at faerie ring or Faerie Queen gift
    - **Silken Sash** (amulet slot): +2 DEF, reflects 10% of magic damage. Lightweight (0.5). Found in libraries or special dungeon rooms
    - **Witch's Shawl** (body armor): +3 INT, +Dark affinity, spells cost 1 less MP. Cursed: -3 chivalry while worn. Dropped by witches
    - **Morgan's Circlet** (helmet slot): +4 INT, +3 Dark affinity, learn Dark spells 1 threshold lower. Unique, gifted by Morgan le Fay as part of her bargain
  - **Potions** (50) -- **unidentified when first found**. Appear as colored liquids ("Bubbling Red Potion", "Murky Green Potion", etc.) until identified. Color-to-effect mapping randomized each game. Identification methods: (1) drink it and find out (risky!), (2) pay a potion shop to identify (10-30 gold), (3) Identify spell/scroll. Once one potion of a type is identified, all potions of that color are known for the rest of the game.
    - **Beneficial potions** (25):
      - Healing Potion (restore 15 HP), Greater Healing (restore 40 HP), Full Healing (restore all HP)
      - Mana Potion (restore 10 MP), Greater Mana (restore 30 MP), Full Mana (restore all MP)
      - Strength Potion (temp +3 STR, 30 turns), Speed Potion (temp +3 SPD, 30 turns)
      - Defense Potion (temp +3 DEF, 30 turns), Wisdom Potion (temp +3 INT, 30 turns)
      - Potion of Giant Strength (temp +6 STR, 15 turns), Potion of Haste (double speed, 10 turns)
      - Poison Antidote (cure poison), Potion of Cure Disease (cure all status effects)
      - Potion of Invisibility (invisible 20 turns), Potion of Fire Resistance (resist fire 50 turns)
      - Potion of Cold Resistance (resist cold/snow 50 turns), Potion of Levitation (float over traps/water 30 turns)
      - Potion of Chivalry (+5 chivalry), Potion of Experience (+50 XP)
      - Potion of True Sight (see invisible enemies 30 turns), Potion of Regeneration (HP regen 50 turns)
      - Potion of Light Affinity (+3 Light), Potion of Nature Affinity (+3 Nature)
      - Elixir of Life (permanent +2 max HP, very rare)
    - **Harmful potions** (15):
      - Poison (take 20 damage over 10 turns), Strong Poison (take 40 damage over 10 turns)
      - Potion of Weakness (-3 STR, 30 turns), Potion of Slowness (-3 SPD, 30 turns)
      - Potion of Confusion (random movement for 15 turns), Potion of Blindness (FOV = 1 for 20 turns)
      - Potion of Amnesia (forget 1 random learned spell!), Potion of Clumsiness (-3 DEF, 30 turns)
      - Potion of Decay (-1 permanent max HP, nasty), Potion of Curse (random cursed status)
      - Potion of Rage (attack everything including allies for 10 turns, +5 STR during)
      - Potion of Polymorph (random stat shuffle -- all stats randomly redistributed!)
      - Potion of Teleportation (teleport to random location on map -- could be dangerous)
      - Potion of Dark Affinity (+3 Dark, -2 chivalry), Potion of Hallucination (all enemy glyphs randomized for 30 turns)
    - **Mixed/situational potions** (10):
      - Potion of Exchange (swap HP and MP values), Potion of Mutation (random +2 to one stat, -2 to another)
      - Potion of Berserking (+5 STR, +3 SPD, but can't cast spells or use items for 20 turns)
      - Potion of Water Breathing (walk on water 100 turns), Potion of Etherealness (walk through walls 10 turns)
      - Potion of Growth (double size: +3 STR, +3 DEF, but can't fit through doors for 15 turns)
      - Potion of Shrinking (half size: -2 STR, -2 DEF, but +4 SPD and can dodge attacks for 15 turns)
      - Potion of Luck (+luck modifier for 50 turns: better loot, better encounter outcomes)
      - Potion of Mapping (reveal current dungeon floor), Potion of Monster Detection (show all enemies on floor for 30 turns)
    - All potions defined in `data/potions.csv` with columns: id, true_name, color_description, effect_type, magnitude, duration, rarity
    - Potion shops (`!` green) in towns: buy identified potions, pay to identify unknown ones, sell potions
  - **Scrolls** (10): Scroll of Fireball, Scroll of Teleport, Scroll of Mapping, Scroll of Identify, Scroll of Enchantment (upgrade weapon), Scroll of Protection, Scroll of Summoning, Scroll of Confusion, Scroll of Holy Word, Scroll of Darkness
  - **Food & Hunger System** (40 food types) -- hunger is tracked as a core mechanic:
    - **Hunger stat** (0-100, starts at 100 = full). Decreases by 1 every 10 turns. Displayed on status bar
    - **Hunger effects**:
      - 100-60: **Full** -- no effects
      - 59-30: **Hungry** -- message "You are getting hungry", no stat effects yet
      - 29-10: **Very Hungry** -- -1 STR, -1 SPD (stamina loss), warning messages
      - 9-1: **Starving** -- -2 STR, -2 SPD, -1 DEF, HP slowly drains (1 per 10 turns)
      - 0: **Death by starvation** -- game over
    - Eating food restores hunger points (amount varies by food type)
    - **Food sources**: buy from shops/inns, loot from dungeons, forage on overworld, fish from lakes, apple trees (`a` green on overworld -- interact to pick apples), hunt animals (kill a boar/stag = raw meat)
    - **Food types** (40, defined in `data/food.csv`):
      - **Basic rations** (10): Bread (+15 hunger), Cheese (+12), Dried Meat (+18), Hardtack (+10, never spoils), Oatcake (+8), Porridge (+14), Stew (+20), Gruel (+6, cheap), Salted Fish (+15), Trail Mix (+12)
      - **Fresh food** (10): Apple (+8, from trees), Roast Chicken (+25), Roast Boar (+30), Venison (+28), Fresh Fish (+18, from fishing), Mutton (+22), Pork Pie (+16), Meat Pasty (+18), Turnip (+6), Berries (+5, foraged)
      - **Inn meals** (8): Shepherd's Pie (+30), Roast Beef (+35), Fish and Chips (+25), Ploughman's Lunch (+20), Lamb Stew (+28), Royal Feast (+50, full HP too, expensive), Monk's Bread (+15, free at monasteries), Castle Banquet (+40, only at active castles)
      - **Special food** (7): Faerie Cake (+10, random magical side effect), Elvish Waybread (+40, weightless, very rare), Dragon Steak (+35, +temp fire resist, cook from dragon meat), Enchanted Apple (+20, +5 temp HP), Mead (+10, +temp +1 STR, -1 INT), Ale (+8, +temp +1 DEF, -1 SPD, bought at inns), Wine (+5, +temp +1 INT, -1 DEF)
      - **Raw/uncooked** (5): Raw Meat (+8, 20% chance of food poisoning), Raw Fish (+6, 15% chance food poisoning), Mushroom (+5, 10% chance poison, 5% chance hallucination), Raw Egg (+4), Acorn (+2, emergency food)
    - Cooking: campfire (`c` then choose "cook") converts raw meat/fish to cooked versions (better hunger value, no poisoning risk). Requires tinderbox item
    - Inn meals automatically available when resting at an inn
    - Food has weight (adds to inventory management decisions)
  - **Magic Rings** (22) -- equippable, player can wear up to 2 rings at once (one per hand). Rings glow softly (`o` various colors) when on the ground:
    - **Ring of Rebirth** -- when you die, you are revived at Stonehenge with all equipment intact. The ring is consumed on use. Multiple instances exist in the game (~3-4 scattered across dungeons and hidden locations). The most valuable ring to find
    - **Ring of Protection** (+2 DEF)
    - **Ring of Strength** (+2 STR)
    - **Ring of Wisdom** (+2 INT)
    - **Ring of Speed** (+2 SPD)
    - **Ring of Vitality** (+10 max HP)
    - **Ring of Sorcery** (+10 max MP)
    - **Ring of Invisibility** (permanent invisibility while worn, but attacking breaks it for 5 turns)
    - **Ring of Regeneration** (HP regen: +1 HP every 3 turns)
    - **Ring of Mana Regeneration** (MP regen: +1 MP every 3 turns)
    - **Ring of Fire Resistance** (immune to fire damage)
    - **Ring of Cold Resistance** (immune to cold/snow damage)
    - **Ring of Poison Resistance** (immune to poison)
    - **Ring of Free Action** (immune to paralysis, bear traps, entangle)
    - **Ring of See Invisible** (see invisible enemies and hidden traps)
    - **Ring of Teleport Control** (choose destination when teleported instead of random)
    - **Ring of Feather Fall** (immune to pit trap damage and falling damage)
    - **Ring of Light** (+3 FOV radius, Light affinity +1)
    - **Ring of Shadow** (+stealth, enemies detect you 3 tiles later, Dark affinity +1)
    - **Ring of the Wild** (animals won't attack you, Nature affinity +1)
    - **Cursed Ring of Binding** (stat drain: -1 to random stat every 100 turns, cannot be removed without Remove Curse/Purify spell or a shrine visit)
    - **Cursed Ring of Hunger** (doubles food consumption rate, cannot be removed without Remove Curse)
  - **Amulets** (5): Amulet of Light (Light affinity), Amulet of Darkness (Dark affinity), Nature Talisman, Guinevere's Amulet (+2 INT, unique), Amulet of Chivalry
  - **Gems & Trinkets** (20) -- valuable items with no gameplay effect, exist purely to be sold at pawn shops for gold. Found in dungeon chests, treasure caches, enemy drops, and wells. Shown as `*` (various colors):
    - **Gems**: Ruby (50g), Sapphire (60g), Emerald (55g), Diamond (100g), Amethyst (40g), Topaz (35g), Opal (70g), Pearl (45g), Garnet (30g), Black Pearl (80g, rare)
    - **Trinkets**: Gold Goblet (25g), Silver Chalice (20g), Jewelled Brooch (35g), Gold Chain (30g), Ivory Comb (15g), Crystal Vial (20g), Ornate Dagger (40g, decorative not a weapon), Ancient Coin (10g), Crown Fragment (60g), Golden Idol (90g, rare)
    - Lightweight (weight: 0.5 each) so they don't burden the player
    - Higher value gems/trinkets found in deeper dungeons and harder locations
    - Some quest NPCs may ask for specific gems as quest objectives
  - **Quest & Special** (7): Holy Grail (unique), Castle Keys (various), Dragon Scale, Lancelot's Lost Sword (quest), Holy Water, Faerie Charm, Merlin's Crystal
  - **Tools** (3): Tinderbox (required for cooking at campfires), Lockpick (Rangers start with one, others can buy -- used to pick locked doors), Pickaxe (mine crystal formations, break rubble in dungeons)
  - **Legendary Artifacts** (12) -- unique, extremely powerful items scattered across the world. Each has a backstory and special location. Only one of each exists per game. Glow golden (`*` yellow bold) when on the ground:
    - **Excalibur** -- the sword of King Arthur (from the Lady of the Lake). Highest damage, +Light affinity, +2 STR, bonus vs evil
    - **Excalibur's Scabbard** -- hidden in Avalon. While carried: HP regenerates 1 per 5 turns, bleed effects cannot kill you. More valuable than the sword itself according to legend
    - **The Round Table Fragment** -- piece of the Round Table, found in Camelot Catacombs. While carried: +10 chivalry cap (max becomes 110), all allied NPCs fight harder
    - **Merlin's Staff** -- in Glastonbury or given by Merlin after completing his quests. +5 INT, all spell MP costs reduced by 2, doubles spell damage
    - **The Siege Perilous** (enchanted seat fragment) -- found in a hidden room in Glastonbury Tor. While carried: immune to instant-death effects, +3 all stats, but only the worthy can hold it (chivalry 70+ or it damages you each turn)
    - **Lancelot's Shield** -- in Castle Dolorous Garde, near his sword. +5 DEF, reflects 25% of melee damage back at attackers
    - **Morgan le Fay's Mirror** -- offered by Morgan or found in Castle Perilous. Reveals all enemies and items on the current floor permanently. +3 INT, but -5 chivalry to use (dark artifact)
    - **The Green Knight's Girdle** -- dropped by the Green Knight after defeating him. Wearer cannot be killed in one hit (any blow that would kill leaves you at 1 HP instead, once per floor)
    - **Tristan's Harp** -- found on a lake island or in Cornwall. Playing it (`p`) pacifies all enemies in the room for 5 turns, can be used once per dungeon level
    - **The Questing Beast's Horn** -- dropped by the Questing Beast. Blowing it (`b`) summons a random allied creature to fight for 20 turns, usable once per 200 turns
    - **Galahad's Helm** -- found in Carbonek after the shrine vision. +4 DEF, immunity to fear/confusion/blindness, +5 Light affinity
    - **The Cauldron of Rebirth** -- hidden in a Welsh dungeon. One-time use: fully resurrects the player on death with full HP/MP at the dungeon entrance (consumed on use). Unlike the Ring of Rebirth (which teleports to Stonehenge), this keeps you in the dungeon. The ultimate insurance policy for deep dungeon runs
- 3-8 items per dungeon level, weighted by depth
- Inventory: 26 slots (a-z), `g` to pickup, `i` to open, equip/drop/use/throw/apply
- **Item stacking**: identical items stack in a single inventory slot (e.g., "12 Arrows", "3 Healing Potions"). Stacking applies to: ammo, potions of same identified type, food of same type, gems/trinkets of same type, scrolls of same type, gold coins. Non-stackable: weapons, armor, rings, amulets, unique items. This keeps the 26-slot limit manageable
- Items also purchasable in town shops
- **Class and gender item restrictions** -- defined per item in `data/items.csv` with `class_restrict` and `gender_restrict` columns:
  - **Class restrictions**:
    - **Knight only**: Plate Armor, Tower Shield, Great Helm, Holy Mace, Arthur's Shield. Heavy martial gear
    - **Wizard only**: Wizard's Staff, Enchanted Robe, Crown of Wisdom, Merlin's Staff, Staff of Light. Arcane focus items
    - **Ranger only**: Longbow, Enchanted Bow, Elven Boots, Druid Staff. Wilderness and ranged gear
    - **Knight cannot use**: Wizard's Staff, Enchanted Robe, Druid Staff, Sorceress Heels, Crystal Hairpin ("You lack the arcane skill to wield this")
    - **Wizard cannot use**: Plate Armor, Tower Shield, Great Helm, Claymore, Great Axe, War Hammer ("This armour is too heavy and restrictive for spellcasting")
    - **Ranger**: can use most items but not the heaviest armour (Plate Armor, Tower Shield) or pure arcane items (Wizard's Staff, Merlin's Staff)
  - **Gender-flavoured items** (usable by either gender but with bonus for one):
    - **Female bonus**: Enchantress's Gown (+1 extra INT if female), Moonsilver Dress (+1 extra SPD if female), Crystal Hairpin (+1 extra MP if female), Faerie Tiara (+1 extra to all if female)
    - **Male bonus**: Horned Helm (+1 extra STR if male), Gauntlets of Strength (+1 extra STR if male), Great Helm (+1 extra DEF if male)
    - Items can still be equipped by either gender, but the bonus stat only applies to the matching gender. Encourages thematic builds without hard-locking
  - Attempting to equip a class-restricted item: "You cannot use this item" message, item stays in inventory
- **Blessed/Cursed/Uncursed (BUC) item system** -- inspired by NetHack:
  - Every item has a BUC state: **Blessed**, **Uncursed** (default), or **Cursed**
  - BUC state is **unknown** until identified (items show as "a longsword" not "a blessed longsword")
  - **Identification methods**: pray at a church altar (priest identifies all carried items' BUC), Identify spell, or pay a shopkeeper
  - **Blessed items**: enhanced effects -- blessed potions heal more, blessed weapons deal +2 damage, blessed armor gives +1 DEF, blessed food restores more hunger
  - **Cursed items**: reduced effects and **cannot be unequipped** once worn/wielded. Cursed weapons deal -2 damage, cursed armor gives -1 DEF. Must use Remove Curse spell, Purify, or visit a shrine to uncurse
  - **Holy water** (buyable at churches): pour on an item to bless it. Unholy water (rare dark item): pour on an item to curse it
  - Some enemies can curse your equipped items (witches, evil sorcerers) during combat
  - BUC state tracked in `items.csv` as a column, default "uncursed". Dungeon loot has a % chance to be blessed (10%) or cursed (15%)
  - Fits the Arthurian theme: holy blessings from churches, dark curses from enemies

- **Ranged combat & ammunition**:
  - Ranged weapons (bows, crossbows, sling) require **ammunition** to fire
  - **Ammo types**: Arrows (for bows), Bolts (for crossbows), Stones (for sling), Throwing Knives (standalone). Stackable items (e.g., "24 Arrows")
  - **Quiver slot**: a dedicated equipment slot for ammo. Press `f` to fire the equipped ranged weapon using quiver ammo
  - **Targeting UI**: press `f`, a cursor appears on the map. Move with arrow keys to target an enemy within range and line-of-sight. Press Enter to fire. Range: Short Bow (6 tiles), Longbow (10), Crossbow (8), Sling (5), Throwing Knives (4)
  - **Damage**: weapon base damage + ammo modifier + STR/2 (bows) or INT/2 (sling). No DEF bonus for targets at range
  - **Ammo recovery**: 50% chance to recover arrows/bolts from the tile where the target was hit. Throwing knives: 75% recovery. Stones: 0% (consumed)
  - **Special ammo**: Silver Arrows (bonus vs werewolves/undead), Fire Arrows (fire damage, chance to ignite), Blessed Bolts (bonus vs evil), Poisoned Throwing Knives (DOT)
  - Ammo is bought in shops, found in dungeons, and dropped by archer enemies
  - Ammo defined in `data/ammo.csv` with columns: name, type, damage_mod, weight, special_effect, rarity

- **Weight system**:
  - Every item has a weight value (defined in `data/items.csv`, column: weight)
  - Player has a **carry capacity** based on STR and level: `capacity = 30 + (STR * 3) + (level * 2)` weight units
  - Current weight / max weight displayed on status bar (e.g., `Wt: 45/60`)
  - Cannot pick up items that would exceed carry capacity ("You are too burdened to carry that")
  - **Encumbrance tiers** when approaching the limit:
    - Below 75%: normal movement
    - 75-90%: **Burdened** -- movement speed reduced, -1 SPD
    - 90-100%: **Heavily Burdened** -- movement very slow, -2 SPD, cannot flee encounters
  - Typical weights: dagger (2), longsword (5), plate armor (15), chainmail (10), leather armor (5), shield (6), potion (1), scroll (0.5), food (1), gold coins (0.1 per coin), Excalibur (3 -- lighter than normal swords, magical)
  - **Weightless items**: all magical scrolls, quest items (Holy Grail, keys), faerie charms, enchanted rings/amulets -- marked with `weight: 0` in CSV
  - Leveling up increases capacity automatically (+2 per level)
  - STR buffs/debuffs temporarily affect capacity
  - Inventory screen shows each item's weight and total carried
- **Result:** Loot in dungeons, buy in towns, full inventory management with weight-based decisions

### Phase 10 -- Enemy AI & Pathfinding
- A* pathfinding with binary-heap priority queue
- AI states: IDLE -> CHASE -> FLEE (FSM per enemy)
- Type-specific: Ghost walks through walls, Dragon breathes fire, Witch casts at range, Black Knight always chases
- **Monster abilities framework** -- enemies have special abilities beyond basic melee, defined in `data/monster_abilities.csv`:
  - **Summoners**: Necromancer raises slain monsters as undead allies. Evil Sorcerer summons imps. Witch summons spiders
  - **Healers/Buffers**: Troll Shaman heals nearby trolls (+5 HP/turn to allies within 3 tiles). Dark Monk buffs adjacent undead (+2 STR)
  - **Debuffers**: Witch curses player (random stat -1 for 20 turns). Enchantress charms player (random movement 5 turns). Ghost chills player (-2 SPD for 10 turns)
  - **Ranged casters**: Evil Sorcerer casts dark bolt (8 damage, 6 tile range). Warlock casts fear. Enchantress casts sleep
  - **On-death effects**: Imp explodes on death (3 fire damage to adjacent tiles). Slime splits into 2 smaller slimes. Gas Spore releases confusion cloud
  - **Aura effects**: Death Knight radiates fear aura (enemies within 2 tiles must pass INT check or flee). Demon Lord radiates fire aura (2 damage/turn to adjacent player)
  - **Theft/special**: Pixie Swarm steals a random item. Kelpie lures player toward water. Basilisk gaze attack (stun 2 turns if player faces it, countered by looking away or mirror)
  - Abilities have cooldowns (e.g., dragon breath every 3 turns), MP costs for casters, and range limits
  - Each monster can have 0-3 abilities, making encounters varied and tactical
- **Result:** Smart enemy movement, diverse behaviors, tactical depth in combat

### Phase 11 -- Character Creation, Classes & Spells

**Character creation flow** (sequential screens at game start):

1. **Class selection screen** -- choose class with description and starting gear preview:
   - Knight (tanky), Wizard (magic), Ranger (balanced)

2. **Gender selection screen** -- choose Male or Female:
   - Male: +1 STR, +1 DEF
   - Female: +1 INT, +1 SPD
   - Gender affects NPC dialogue slightly (Sir/Lady, etc.) but not quest availability

3. **Name entry screen** -- enter a name or press `r` to generate a random name:
   - Random name generator pulls from a pool of period-appropriate names in `data/names.csv`
   - Male names: Galahad, Percival, Tristan, Gareth, Gawain, Bors, Bedivere, Kay, Ector, Owain, etc.
   - Female names: Elaine, Isolde, Lynette, Enid, Viviane, Nimue, Igraine, Morgause, Laudine, etc.
   - Player can re-roll random names as many times as they like

4. **Appearance screen** -- flavour traits that make the character feel real (no gameplay effect, purely cosmetic):
   - **Hair colour**: randomly generated or chosen -- Black, Brown, Auburn, Red, Blonde, Grey, White
   - **Eye colour**: Blue, Green, Brown, Grey, Hazel, Amber
   - **Build**: Lean, Average, Stocky, Tall, Short, Athletic, Broad-shouldered
   - **Distinguishing feature**: Scar across cheek, Braided hair, Missing tooth, Weathered face, Freckles, Piercing gaze, Noble bearing, Calloused hands
   - Press `r` to randomize all traits, or cycle through each option individually
   - Displayed on the full character sheet (`@` screen) and in the death screen / morgue file
   - NPCs occasionally reference appearance in dialogue flavour ("A [build] [hair] warrior approaches...")

5. **Stats screen** -- base stats are randomly generated (rolled), modified by class and gender:
   - Stats rolled: STR, DEF, INT, SPD (each 3-8 base, random) + class bonuses + gender bonuses
   - HP and MP derived from stats and class
   - Display all stats, starting gear, and starting spell clearly
   - Player can **re-roll** stats (press `r`) as many times as they want
   - Press Enter/Return to **accept** and proceed
   - Starting spell shown: Knight = Shield, Wizard = Magic Missile, Ranger = Detect Traps

6. **Cheat mode** (optional, hidden during character creation):
   - Press `C` (capital C) on the stats screen to open the cheat menu. Stats screen displays: "Press [r] to re-roll, [Enter] to accept, [C] for cheat options"
   - **Cheat options** (toggle on/off):
     - God Mode: HP set to 999, cannot die from damage
     - Infinite Mana: MP set to 999, never depleted
     - Rich Start: Gold set to 1000
     - Max Stats: all stats set to 15
     - Full Spellbook: all 50 spells learned immediately
     - Full Inventory: start with a complete set of magical items (best weapon, best armour, all rings, potions)
     - All Maps Revealed: overworld and dungeon maps fully visible (no fog of war)
     - Level Skip: press `]` to skip to next dungeon level at any time
     - Instant Kill: all attacks do 999 damage
     - No Hunger: hunger system disabled
   - **Cheat consequences**:
     - Score is always **0** regardless of achievements
     - High score table marks the entry as "CHEAT" and does not rank it
     - Canterbury graveyard shows **"CHEAT"** on the gravestone
     - Fallen Heroes screen marks cheated characters with a skull icon
     - Morgue file header shows "CHEAT MODE ENABLED"
     - Chivalry title replaced with "Cheater" on status bar
   - Flag stored in GameState: `bool cheat_mode`. Once enabled, cannot be disabled for that run
   - Primarily for developer testing but left in for players who want to explore freely

7. **Story screen** -- sets the scene and introduces the main quest:
   - Scrolling narrative text, atmospheric:
     ```
     The kingdom of Camelot is in peril.
     The Holy Grail, long hidden somewhere in the
     depths of England, holds the power to save the realm.
     
     King Arthur has summoned you to his court.
     Seek an audience with the King in the
     throne room of Camelot Castle to learn
     of your destiny.
     
     Your journey begins at the gates of Camelot...
     ```
   - Press any key to begin the game
   - Player starts on the overworld at Camelot's location

**Spell affiliation system**:
- Each spell belongs to an affiliation: **Light**, **Dark**, or **Nature** (or **Universal** for spells usable by all)
**Chivalry system**:
- Chivalry is a core stat (0-100, starting at 25) displayed on the status bar
- **Gain chivalry**: rescuing damsels (+5), completing quests (+3), helping NPCs (+2), defeating evil bosses (+5), giving gold to beggars (+1), praying at shrines (+2), sparing a defeated enemy (+3), returning lost items (+2)
- **Lose chivalry**: attacking friendly NPCs (-10), stealing from shops (-8), fleeing from a damsel encounter (-3), accepting dark bargains (-5), killing non-hostile creatures (-2), breaking a promise/quest (-5), **looting a shrine (-15, severe sacrilege)**, bad luck events: accused of a crime you didn't commit (-3), cursed by a witch (-2)
- **Chivalry effects on gameplay**:
  - **Camelot access**: below 15 chivalry, guards refuse entry. Below 30, King Arthur is cold and won't grant quests
  - **Castle reception**: high chivalry (60+) = kings greet you warmly, offer better prices and quests. Low chivalry (below 20) = refused entry or attacked on sight
  - **NPC reactions**: shopkeepers give discounts at high chivalry, charge more at low. Inn prices vary. Quest givers offer better quests to chivalrous players
  - **Knight encounters**: allied knights on the road help you if chivalry is high, challenge you to duels if low
  - **Lancelot**: won't ally with you unless chivalry is 50+
  - **Lady of the Lake**: requires chivalry 40+ to receive Excalibur
  - **Endgame**: chivalry factors into final score multiplier (1.0x at 50, up to 2.0x at 100, down to 0.5x at 0)
  - **Title**: displayed based on chivalry level: Knave (0-15), Squire (16-30), Knight (31-50), Noble Knight (51-70), Champion (71-85), Paragon of Virtue (86-100)

- Player gains affiliation through choices and actions during the game:
  - Helping NPCs, rescuing damsels, visiting Canterbury cathedral -> **Light** affinity
  - Accepting Morgan le Fay's bargains, using cursed items, dark choices -> **Dark** affinity
  - Exploring forests, communing at Stonehenge, Sherwood quests -> **Nature** affinity
- Affiliation is a spectrum (tracked as Light/Dark/Nature points), not a hard lock -- player can have mixed affinity
- Spells require a minimum affinity threshold to learn/cast. 50 spells total across affiliations:
  - **Light spells** (13):
    - Heal (restore HP), Greater Heal (large HP restore), Holy Strike (bonus vs undead/evil),
    - Divine Shield (temporary DEF boost), Purify (cure poison/curse), Smite (ranged light damage),
    - Sanctuary (enemies can't enter adjacent tiles for 3 turns), Resurrect (revive with 50% HP after death, one-time),
    - Turn Undead (fear effect on undead), Blessing (buff all stats +1 for 20 turns),
    - Holy Light (reveal entire floor), Sacred Flame (fire damage + heal self), Aura of Protection (party DEF aura)
  - **Dark spells** (13):
    - Drain Life (damage enemy, heal self), Shadow Step (short-range teleport behind enemy),
    - Curse (reduce enemy stats), Summon Undead (summon skeleton ally for 10 turns),
    - Fear (enemies flee for 5 turns), Dark Bolt (high damage single target), Wither (DOT poison),
    - Soul Steal (kill low-HP enemy, gain MP), Darkness (blind all enemies in room),
    - Animate Dead (turn killed enemy into ally), Hex (enemy takes double damage for 5 turns),
    - Nightmare (AoE sleep), Blood Pact (sacrifice HP for large MP restore)
  - **Nature spells** (13):
    - Entangle (root enemy in place 5 turns), Lightning (chain damage to 3 nearby enemies),
    - Beast Form (transform: +5 STR, +3 SPD, can't cast for 10 turns), Poison Cloud (AoE DOT),
    - Bark Skin (large DEF boost 15 turns), Earthquake (damage all enemies on floor),
    - Summon Wolf (wolf ally for 10 turns), Thorns (attackers take reflect damage),
    - Regrowth (heal over time 10 turns), Gust (push enemies back 3 tiles),
    - Camouflage (invisible in forest/grass terrain), Vine Whip (ranged pull enemy to you),
    - Walk on Water (cross rivers and lakes for 50 turns)
  - **Universal spells** (11):
    - Teleport (random safe tile on current floor), Magic Missile (reliable ranged damage),
    - Detect Traps (reveal traps on floor), Light (increase FOV radius for 20 turns),
    - Identify (reveal item properties), Slow (reduce enemy speed), Haste (double player speed 5 turns),
    - Shield (absorb next 15 damage), Blink (short random teleport 5 tiles),
    - Map (reveal current floor layout), Fireball (AoE fire damage, 3x3)
- Spell data defined in `data/spells.csv` with columns: name, affiliation, threshold, mp_cost, damage, effect, duration, range, aoe_size, class_restriction, level_required
- `z` to cast, targeting cursor for AoE where applicable
- **Mana (MP) system**:
  - Every spell costs MP to cast. Costs range from 2 (Light, Detect Traps) to 25 (Teleport, Earthquake, Resurrect). Defined per spell in `data/spells.csv`
  - **Starting MP by class**: Knight: 8, Ranger: 15, Wizard: 30
  - **MP gain on level-up**: Knight: +1, Ranger: +2, Wizard: +4
  - **MP restoration methods**:
    - Rest at an inn (full MP restore)
    - Mana Potions / Greater Mana Potions (restore 10/30 MP)
    - Stepping into a Circle of Mana (full restore, single use)
    - Camping on overworld (restores 50% MP)
    - Praying at a shrine (restores some MP alongside HP)
    - Natural regeneration: 1 MP per 20 turns (slow background regen)
    - Ring of Mana Regeneration (+1 MP per 3 turns)
  - **MP-boosting equipment**: Wizard's Staff (+5 max MP), Ring of Sorcery (+10 max MP), Enchanted Robe (+MP regen), Crown of Wisdom (+5 max MP via INT bonus), Merlin's Staff (+5 INT = +significant max MP)
  - Max MP = base (from class) + level-up gains + INT bonus (INT * 2) + equipment bonuses
  - Cannot cast a spell if insufficient MP ("Not enough mana..."). Strategic MP management is key, especially for Knights and Rangers with small pools
- **Leveling system** (20 levels max):
  - XP thresholds: 50, 120, 250, 450, 750, 1200, 1800, 2600, 3600, 5000, 6800, 9000, 12000, 15500, 20000, 25000, 31000, 38000, 46000, 55000
  - On level-up: HP increases (+3 Knight, +1 Wizard, +2 Ranger), MP increases (+1 Knight, +4 Wizard, +2 Ranger)
  - Player chooses **one stat to increase by +1** (STR, DEF, INT, or SPD) -- meaningful choice each level
  - Carry capacity increases automatically (+2 per level)
  - At specific levels, gain class-specific bonuses:
    - Level 3: unlock a class spell slot (learn one extra spell beyond the 15 max)
    - Level 5: class perk -- Knight: +2 max HP per level from now on; Wizard: -1 MP cost on all spells; Ranger: passive trap detection range doubles
    - Level 10: class ability -- Knight: "Shield Bash" (stun enemy 2 turns on melee, 20% chance); Wizard: "Mana Shield" (absorb damage with MP when HP < 25%); Ranger: "Double Shot" (ranged attacks hit twice, 30% chance)
    - Level 15: class mastery -- Knight: immunity to fear; Wizard: cast one spell per turn for free (0 MP) if INT > 10; Ranger: always act first in combat
  - Level-up screen: displays stat changes, lets player allocate their +1, shows any new abilities unlocked
  - XP sources: killing monsters (varies by monster), completing quests (+50-200), discovering new locations (+10), boss kills (+200-500)
- **Spell learning system**:
  - Player starts with only **1 spell** based on class: Knight = Shield, Wizard = Magic Missile, Ranger = Detect Traps
  - New spells are learned by **reading Spell Scrolls** -- consumable items found in dungeons, bought from shops, or rewarded by NPCs
  - Spell Scrolls are distinct from combat scrolls: `Scroll of Learn: Fireball` teaches the spell permanently, while `Scroll of Fireball` is a one-time use combat item
  - Spell Scrolls are rare loot -- roughly 1-2 per dungeon, also sold by mystics and Merlin
  - To learn a spell, the player must: (1) have the scroll, (2) meet the affiliation threshold, (3) have sufficient INT (some advanced spells require INT 6+)
  - If requirements aren't met: "You cannot comprehend the magic within this scroll..."
  - **Spell capacity varies by class**:
    - **Wizard**: max 15 spells. Can freely choose which to learn. The primary spellcaster
    - **Ranger**: max 6 spells. Learning a 7th spell **randomly replaces** one of the existing 6 ("The new magic pushes an old spell from your mind... you forget Detect Traps"). Player cannot choose which is lost
    - **Knight**: max 4 spells. Learning a 5th spell **randomly replaces** one of the existing 4. Knights are warriors first, magic is secondary and unreliable for them
    - This makes class choice meaningful: Wizards have spell mastery, Rangers have moderate magic, Knights must rely on steel and faith with only a few spells as backup
  - Some spells can only be learned from specific NPCs (not scrolls): Merlin teaches 3 exclusive spells, Faerie Queen teaches 1, Lake Bala druid teaches 2
  - Spell Scrolls added to `data/items.csv` with type `spell_scroll` and a `teaches_spell` column linking to `spells.csv`
- **Result:** Character creation with gender/class, meaningful affiliation choices affect available magic, spell progression through exploration and discovery

### Phase 12 -- Dungeon Bosses & The Holy Grail

**Dungeon bosses** -- each dungeon has a boss on its deepest level:
  - Camelot Catacombs: **Dark Monk** (mini-boss)
  - Tintagel Caves: **Black Knight**
  - Sherwood Depths: **Evil Sorcerer**
  - Mount Draig Volcano: **Red Dragon (Y Ddraig Goch)**
  - Glastonbury Tor: **Dark Templar** (native boss -- replaced by Mordred if Grail spawns here)
  - Whitby Abbey: **Vampire Lord**
  - White Cliffs Cave: **Sea Serpent King**

**The Holy Grail -- main quest flow**:
1. **Accept the quest**: visit King Arthur in Camelot Castle throne room. He bestows the quest and grants knighthood. The Grail does not exist in the world until this quest is accepted
2. **Find the Grail's location**: the Grail is **randomly placed** in one of the game's dungeons each playthrough (not always Glastonbury Tor). It could be on the deepest level of any dungeon, behind a locked door or in a hidden room
3. **Gather hints**: NPCs in inns give hints about the Grail's location when you talk to them:
   - "I heard a holy light was seen deep beneath Tintagel..." (could be true)
   - "A pilgrim told me the Grail lies under Glastonbury..." (could be a red herring)
   - "The druids whisper of a sacred chalice in the Welsh mountains..." (could be true)
   - Some hints are **red herrings** (30% chance an NPC hint is false). Cross-referencing multiple hints increases confidence
   - Merlin gives a **reliable** hint (always true): "The Grail rests in [dungeon name], brave [Sir/Lady]"
   - King Pellam at Castle Listenoise also gives a reliable hint if you heal him
   - The Carbonek shrine vision reveals the Grail's exact dungeon and level
4. **Recover the Grail**: find it on the deepest level of the correct dungeon, guarded by **Mordred** (who appears as final guardian wherever the Grail is placed). Defeat Mordred, pick up the Grail (`*` yellow bold glow)
5. **Return the Grail to Arthur**: carry the Grail back to Camelot and present it to King Arthur (find him in his castle or on his travels). **Arthur requires chivalry 30+ to accept the Grail** -- if your chivalry is too low, he refuses: "You have the Grail, but you are not worthy to present it. Prove your honour first, then return." Player must raise chivalry (pray, donate, do good deeds) before Arthur will accept. This prevents a low-chivalry "evil" playthrough from easily completing the main quest. Once accepted, Arthur rewards you:
   - Knighted as a **Knight of the Round Table** (if not already)
   - **5000 gold** reward
   - **+10 chivalry** permanently
   - Title: "Grail Knight" displayed on status bar
   - Victory fanfare screen with your character's stats and journey summary
6. **Continue playing**: after returning the Grail, the game **does not end**. You can continue exploring, completing side quests, clearing remaining dungeons, building your character, jousting, and exploring. The score records that you found the Grail (+5000 score bonus). Eventually you may die, but you've achieved the main quest
   - Post-Grail content: some new dialogue from NPCs congratulating you, Arthur offers additional elite quests, access to Avalon if not yet discovered

- Some dungeons may require items/keys from other dungeons to access deeper levels (optional progression gating)
- **Result:** Non-linear Grail quest with investigation, red herrings, and continued play after victory

### Phase 13 -- Save/Load, Permadeath & High Scores

**Title screen menu**:
- If a save file exists, menu shows: **Continue** / New Game / Fallen Heroes / High Scores / Settings / Quit
- If no save file, menu shows: New Game / Fallen Heroes / High Scores / Settings / Quit
- **"Fallen Heroes"** -- view all your previous dead characters from past games:
  - Data stored in `~/.camelot/fallen_heroes.dat` (persists across all playthroughs, same data as graveyard but viewable from title screen)
  - Scrollable list showing each fallen character: name, class, gender, level, cause of death, turns survived, score, chivalry title
  - Select a hero to see their full death summary (same as the death screen: stats, combat record, quests, spells learned, etc.)
  - Sorted by most recent death first
  - Shows total number of heroes who have fallen ("12 brave souls have perished in the quest for the Grail")
  - Same data displayed on Canterbury Graveyard gravestones in-game
- "Continue" resumes from the single save file immediately
- "New Game" warns if a save exists ("This will overwrite your existing save. Are you sure?")
- "Settings" opens the settings menu (font size, colors, etc.) before starting

**Single save slot (permadeath roguelike)**:
- Only **1 save file** at `~/.camelot/save.dat` -- no multiple saves, no save scumming
- `S` to save at any point (overworld, town, or dungeon). Saves and continues playing
- Binary serialization of entire GameState, magic number "CMLT" + version byte
- Save is **deleted on death** -- permadeath, true roguelike style. No reloading after dying

**Death screen**:
- On player death, display a full-screen death summary:
  - Character name, gender, class, and title (chivalry rank)
  - Cause of death ("Slain by a Red Dragon in the Welsh Dragon Lair, level 2")
  - **Final stats**: HP, MP, STR, DEF, INT, SPD, Level, Chivalry
  - **Journey summary**: turns survived, dungeons explored, deepest level reached
  - **Combat record**: monsters slain (total + breakdown by type), times fled
  - **Quests**: completed / total accepted
  - **Gold earned** (lifetime, not just current)
  - **Spells learned**, best weapon wielded
  - **Final score**: kills*10 + gold + dungeons_cleared*200 + quests_completed*150 + (chivalry_multiplier)
  - Prompt for name entry for high score table (pre-filled with character name)
  - "Press any key to return to title screen"
- Save file deleted after death screen is shown

**High scores**: top 10 in `~/.camelot/scores.dat`, showing name, class, score, cause of death, and whether they found the Grail
- Victory score bonus: +5000 if the Grail was found
- **Result:** Tense permadeath gameplay, meaningful death screen, persistent leaderboard

### Phase 14 -- Help System, Settings, Title Screen & Polish

**In-game help system** (press `?` to open):
- Full-screen overlay with scrollable content, navigate with arrow keys, `q` to close
- **Commands tab** -- lists all keybindings organized by category:
  - Movement: hjklyubn / arrow keys, `>` `<` for stairs
  - Actions: `g` pickup, `i` inventory, `z` spells, `J` journal, `S` save, `f` fire ranged, `o` open door, `c` close door, `s` search walls, `D` disarm trap, `t` throw item, `a` apply/use item on another item (e.g., pour holy water on weapon), `x` dig
  - Context-sensitive keys: `c` = close door (dungeon) / camp (overworld). `s` = search walls (dungeon) / stash at home chest. `t` = throw (combat) / take from chest (home) / steal (shop). `h` = move left (normal) / haggle (shop UI). `b` = move down-left (normal) / blow horn (inventory use). These resolve by game mode (DUNGEON, OVERWORLD, TOWN, SHOP, HOME)
  - Movement: hjklyubn (8-dir) / arrow keys (4-dir) / numpad 1-9 (8-dir). `.` or `5` wait one turn. Shift+direction to auto-run until interrupted
  - UI: `?` help, `=` settings, `;` look/examine, `Ctrl+P` message history, `@` character sheet, `q` quit
- **Symbols tab** -- legend of all ASCII characters and their colors:
  - `@` You (the player), `#` Wall, `.` Floor, `~` Water, etc.
  - All enemy glyphs with names and colors
  - All item glyphs with descriptions
  - All NPC/landmark glyphs
- **Tips tab** -- gameplay tips: "Bump into enemies to attack", "Visit inns to rest", "Wells may contain treasure... or danger", etc.
- Context-sensitive: pressing `?` while looking at an entity shows info about that specific entity

**Look/examine mode** (press `;`):
- A cursor (`X` yellow) appears on the map at the player's position
- Move cursor with arrow keys / hjkl to any visible tile
- Status line shows what's on the highlighted tile: tile type, monster name and HP (if visible), item name, NPC name
- Press Enter on a tile to see a detailed description: monster stats and abilities, item properties, terrain effects
- Press `q` or Escape to exit look mode
- Works in all modes (dungeon, overworld, town)

**Message history** (press `Ctrl+P`):
- Opens a full-screen scrollable view of all messages from the current session
- Scroll with arrow keys, Page Up/Down
- Shows up to 200 most recent messages with turn numbers
- Press `q` to close and return to the game
- Important messages (combat, deaths, quest updates) highlighted in colour

**Morgue file / character dump**:
- On death (or victory), automatically save a text file to `~/.camelot/morgue/[name]_[date].txt`
- Contains complete character record:
  - Character name, class, gender, level, cause of death
  - All final stats, chivalry, affiliations
  - Equipment worn at death
  - Full inventory listing
  - Spells known
  - Quests completed and active
  - Monsters killed (by type and count)
  - Locations visited, dungeons explored
  - Turn count, days survived, final score
  - Last 20 messages (combat log leading to death)
  - **Game seed** number (for replay or sharing challenge runs)
- Shareable text format for comparing runs or posting online
- Morgue files persist indefinitely in `~/.camelot/morgue/`
- **Seed system**: game seed shown on death screen and in morgue file. Players can enter a seed at New Game for challenge runs or bug reproduction

**Settings menu** (press `=` to open):
- Stored in `~/.camelot/settings.cfg` (simple key=value text file)
- **Font size** -- adjusts terminal font size via escape sequences (OSC codes for supported terminals like iTerm2/Terminal.app: `\033]50;SetProfile=Large\007` or similar). Fallback: display instructions to change font manually
- **Color mode** -- toggle between 256-color, 8-color, and monochrome
- **Message log lines** -- configurable: 3, 4, or 5 lines
- **Auto-pickup gold** -- on/off (default: on)
- **Confirm before attacking** -- on/off (default: off) for accidental bumps
- **Show minimap** -- on/off (overworld only, shows player position on a small England outline)
- **Key rebinding** -- customise all keybindings via settings file (`~/.camelot/keys.cfg`). Supports remapping movement, actions, and UI keys. Essential for non-QWERTY keyboards and accessibility
- **Animation speed** -- fast / normal / off. Controls projectile animations, spell effects, and message display pacing
- **Numpad support** -- enable/disable numpad (1-9) for 8-directional movement as alternative to vi-keys. On by default

**Title screen and polish:**
- **Title screen** with elaborate ANSI art:
  - Large ASCII/ANSI art of the Holy Grail (coloured, using extended characters and colour pairs)
  - Game title: "KNIGHTS OF CAMELOT" in large ASCII block letters
  - Subtitle: "The Quest for the Holy Grail"
  - Atmospheric border with castle battlements and medieval flourishes
  - Menu: Continue (if save exists) / New Game / Fallen Heroes / High Scores / Settings / Quit
  - Background colour: dark blue/black with gold accents
- Class selection screen after New Game
- Death screen and victory screen with score entry
- Debug mode (`-d` flag): full map, stats visible, level skip with `]`
- **Game balance philosophy** -- the game should be challenging but fair, never frustrating:
  - **Level-scaled encounters**: overworld encounters scale with player level. A level 1 player encounters rats and wolves; a level 10 player encounters trolls and dark knights. Monsters from `data/monsters.csv` have `min_depth` and `max_depth` fields that determine when they appear
  - **Dungeon difficulty curves**: each dungeon has a difficulty rating. Enemies on each floor are appropriate for the expected player level at that dungeon depth. Early dungeon floors should be beatable by a careful player
  - **Flee is always an option**: running away from combat always has at least a 30% base success rate (higher on roads, with horse, with high SPD). The player should never feel trapped with no escape. Even boss fights allow retreat (boss stays on its floor)
  - **Out-of-depth monsters**: rare (5% chance) encounters with monsters above the player's level. These are signalled: "You sense a powerful presence..." -- giving the player a chance to avoid. Defeating out-of-depth enemies gives bonus XP
  - **Difficulty tuning knobs** (all in CSV files for easy adjustment):
    - Monster HP, STR, DEF per depth level
    - Item drop rates and quality per dungeon depth
    - Encounter frequency per terrain
    - Potion/food spawn rates
    - Gold economy (shop prices, quest rewards, gold drops)
    - XP curve and level-up bonuses
  - **Intended progression curve**: 
    - Levels 1-3: Camelot area, Catacombs. Learning the basics
    - Levels 4-7: expand to nearby towns, Tintagel/Sherwood dungeons
    - Levels 8-12: deeper dungeons, volcano, difficult castles, most quests completable
    - Levels 13-17: endgame content, Glastonbury, Grail hunt, boss encounters
    - Levels 18-20: post-Grail elite content, princess quest, Avalon
  - **Death should feel fair**: when the player dies, they should feel "I could have survived if I'd played differently" rather than "that was unfair". No unavoidable instant kills (except Basilisk with save via Ring of Free Action). Traps are detectable. Ambushes give 1 turn to react
- Inn flavor text and beer names for atmosphere
- **Humour system** -- occasional funny messages and easter eggs to keep the game fun:
  - **Funny combat messages** (10% chance to replace standard message):
    - "You swing your sword heroically... and miss. The wolf looks unimpressed."
    - "The skeleton rattles menacingly. You wonder who dressed it this morning."
    - "You hit the bandit. He looks personally offended."
    - "The troll falls. The ground shakes. Your teeth rattle."
    - "You defeated the Giant Rat. Your mother would be so proud."
  - **Funny item descriptions** (shown in look/examine mode):
    - Rusty Sword: "It's seen better days. And better centuries."
    - Bread: "Stale enough to use as a weapon in a pinch."
    - Healing Potion: "Tastes like hope and elderberries."
    - Empty chest: "Someone got here first. Typical."
  - **Funny NPC dialogue** (random inn patron chatter):
    - "I used to be an adventurer like you, then I took an arrow to the... actually, never mind."
    - "Have you tried the stew? I think it moved."
    - "I saw a dragon once. Then I ran. Quickly."
    - "The Black Knight? Oh, he's armless. Literally."
  - **Funny death messages** (added to cause of death):
    - "Slain by a Giant Rat on level 1. The bards will not sing of this."
    - "Drowned in a puddle. A hero's end."
    - "Killed by a kitten. Wait, how?"
  - **Fun/joke items** (found rarely in dungeons, wells, or as joke shop inventory):
    - **Rubber Duck** (`d` yellow): no combat use. Squeaks when used ("Squeak!"). +1 morale (tiny HP regen, 1 per 50 turns). Floats on water. If thrown at an enemy, 1 damage and confuses them for 2 turns (they're so baffled). Beloved by speedrunners
    - **Pointy Hat of Silliness** (helmet): +0 stats but makes all NPCs laugh when they see you. Shopkeepers give 5% discount because you cheer them up. "You look ridiculous." -2 chivalry while worn
    - **Coconut Halves** (misc item): use to simulate horse sounds while walking. No actual speed boost. "You clop along pretending to ride a horse. A nearby peasant stares."
    - **Holy Hand Grenade** (rare, one-use): deals massive damage (50) to all enemies in the room. Must count to three first (takes 3 turns to use). "One... two... five!" "Three, sir!" "Three!"
    - **Shrubbery** (misc item): a nice shrubbery. Quest item for a joke quest from the Knights Who Say Ni (random overworld encounter, very rare). Reward: they stop saying "Ni" and give you a herring
    - **Comfy Chair** (misc item): when used, sit for 5 turns and restore 10 HP. "Nobody expects the comfy chair!"
  - Humorous messages stored in `data/dialogue.csv` with a `humour` tag for easy editing/expansion

## Key Algorithms
- **Map gen:** Binary Space Partition (BSP) for dungeons
- **FOV:** Recursive shadowcasting (8 octants)
- **Pathfinding:** A* with Manhattan heuristic on 80x22 grid
- **RNG:** Seeded PRNG for reproducible dungeons
- **Overworld:** Fixed ASCII map stored as const char arrays

## Verification
- Each phase produces a compilable, playable build (`make && ./camelot`)
- Debug mode for testing: `-d` flag shows full map, enemy stats, level skip
- Manual playtesting across overworld, all towns, and all 7+ dungeons
- Verify quest system: pick up quests in inns, complete objectives, return for rewards
- Verify Grail quest: accept from Arthur, gather hints, find randomized Grail location, defeat Mordred, return to Arthur
- Verify princess quest: rescue from tower, visit at castle, marriage ending
- Verify post-Grail gameplay continues
