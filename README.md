# Knights of Camelot

**The Quest for the Holy Grail** -- a terminal-based ASCII roguelike set in Arthurian England.

## About

Knights of Camelot is a full-featured roguelike game written in C using ncurses. You play as a knight, wizard, or ranger on a quest to find the Holy Grail. Explore an overworld map of medieval England, visit towns and castles, delve into dangerous dungeons, and encounter legendary characters from Arthurian myth.

The game features permadeath -- when you die, your character is gone forever. But your legacy lives on: gold stored at the bank, items left at home, and your gravestone in Canterbury cathedral carry forward to future characters.

## Current State

**Implemented (Phases 1-4):**
- Overworld map of England (500x250 tiles) with procedural terrain, coastlines, rivers, lakes, mountains, islands
- 18 towns and 14 active castles with walkable interior maps
- 3 abandoned castles, 7 islands with boats, 10 cottages, 6 caves
- Landmarks: Stonehenge (stat boost), Faerie Rings (random effects), Avalon, Holy Island, Hadrian's Wall, White Cliffs of Dover
- Day/night cycle with dimming and light radius around the player
- 30-day lunar cycle with 12 event notifications
- Regional weather system (6 types, varies by location)
- Town services: Inn (rest/beer), Church (pray/donate/confess/loot), Mystic, Bank, Well
- Castle throne rooms with Kings, Queens, and Guards
- Wandering NPCs and animals (overworld and towns)
- Boat sailing across lakes
- BSP-generated dungeons (160x80 tiles, scrollable) with multiple levels, 2 staircases per level
- 10 room shapes (rectangles, caverns, pillared halls, temples, L-shaped, cross, jagged caves, multi-chamber)
- 3 corridor styles (L-shaped, winding/twisting, S-curve)
- Interactive doors (open/close, auto-open on walk), locked doors (bash with STR), secret doors (search walls)
- 12 trap types (hidden, passive detection, disarm with D key)
- 7 terrain hazards: shallow water, deep water, lava, ice, fungal growth, rubble, crystal formations
- Special rooms: temples with altars, libraries with bookshelves, crypts with coffins, treasure rooms
- Secret rooms (golden floor, accessible via secret doors or teleportation)
- 1-3 dungeon chests per level (20% trapped)
- Magic circles in dungeons and overworld (random effects, single use)
- Druids near magic circles and Stonehenge
- Exit portal on deepest dungeon level
- Quest system with 16 quests (delivery, fetch, kill), quest journal
- Camping system, drunkenness system, terrain-based travel time
- Minimap overview (overworld and dungeons)
- Message history (Shift+P)
- Atmospheric flavour messages in dungeon rooms
- Field of View with raycasting (torch-based lighting, fog of war)
- Torch system (T to toggle, yellow glow, radius 10 with torch, radius 2 without)
- 7 historical abbeys with monks or nuns (never mixed), extra strong beer
- Edinburgh Castle and Dover Castle
- Magical deer (10% chance of +1 stat on touch)
- Rainbow event after rain (race to find pot of gold, 100-500g)
- Cottages and caves reset after 5-20 days for revisiting
- 85 monster types loaded from data/monsters.csv (beasts, bandits, undead, knights, magical, dragons, trolls, bosses)
- Bump-to-attack combat with hit/miss rolls and 10% critical hits
- Monster AI: energy/speed system, flee at low HP, erratic movement, ranged attacks, door opening
- Monster gold drops on kill, XP tracking
- Level feelings on dungeon entry ("This seems quiet" to "DEADLY!")
- Timed monster respawning (every 80 turns outside FOV)
- Overworld enemies: bandits on roads, wolves in forests, boars in hills, skeletons
- Maze regions in deeper dungeon levels
- Reachability validation (flood-fill ensures all stairs are connected)

**Coming next:** Items/inventory, spells, character classes, save/load

## Features (Planned)

### World
- Hand-crafted overworld map of England with 10+ towns, 17 castles (14 active + 3 abandoned), landmarks, and 7+ dungeons
- Towns with equipment shops, potion shops, inns (with beer!), churches, mystics, pawn shops, banks, and wells
- Active castles with throne rooms ruled by Arthurian kings
- Landmarks: Stonehenge, Hadrian's Wall, White Cliffs of Dover, Avalon, Faerie Rings
- Lakes with islands, boats, and the occasional ghost ship or Nessie encounter
- Random cottages, caves with hermits, and overworld encounters
- Day/night cycle with a 30-day lunar calendar and recurring events (feasts, tournaments, holy days)
- Weather system: rain, storms, fog, snow, wind -- regional variation across the map

### Character
- Three classes: Knight (warrior), Wizard (spellcaster), Ranger (balanced)
- Male or female with cosmetic appearance traits (hair, eyes, build)
- Chivalry system (0-100) that affects NPC reactions, castle access, quest availability, and endgame score
- Spell affiliation: Light, Dark, and Nature magic paths shaped by your choices
- 20 character levels with stat choices and class-specific perks

### Combat & Magic
- Classic bump-to-attack melee combat
- Ranged combat with bows, crossbows, and thrown weapons (ammunition system)
- 50 spells across Light, Dark, Nature, and Universal schools
- Spell scrolls for learning new magic (class-limited spell capacity)
- 80+ monster types including werewolves (night only), vampires, dragons, faerie creatures, and undead
- Monsters with special abilities: summoning, healing allies, ranged attacks, aura effects

### Items & Inventory
- 100+ items: weapons, armour, shields, helmets, boots, gloves, enchanted attire
- 50 unidentified potions (randomised colours each game -- drink to discover, or pay to identify)
- 22 magic rings, 12 legendary artifacts, 20 gems & trinkets
- 40 food types with a hunger system
- Weight-based inventory with encumbrance tiers
- Blessed/Cursed/Uncursed item system (holy water to bless, shrines to identify)
- Class and gender item restrictions and bonuses
- Fun items: Rubber Duck, Holy Hand Grenade, Coconut Halves, Shrubbery

### Quests
- Main quest: find the Holy Grail (randomly hidden each game) and return it to King Arthur
- 10-15 side quests from inn NPCs: fetch, kill, and delivery quests
- Rescue the Princess quest -- the game's true ending (marriage!)
- Round Table membership quest from King Arthur
- Witch encounters with timed tasks (geas)
- Treasure maps leading to buried loot (some are fake!)

### Dungeons
- BSP-generated rooms and corridors with 7+ unique dungeons
- Randomised dungeon depth (varies each playthrough)
- Field of view with recursive shadowcasting and fog of war
- 12 trap types (hidden, detectable, disarmable)
- Magic circles (teleportation network for fast travel)
- Special rooms: vaults, temples, armouries, libraries, crypts
- Dungeon terrain: water, lava, ice, fungal growth, crystal formations
- Locked and trapped chests (with rare mimic surprises)
- Interactive doors: open, close, locked, secret
- Persistent levels: items you drop stay where you left them
- Branches, shafts, and secret levels
- Exit portal on the deepest level and Scroll of Recall for escape

### Famous Characters
- King Arthur, Queen Guinevere, Merlin, Morgan le Fay, Sir Lancelot, Lady of the Lake
- Breunis sans Pitie (recurring villain), the Faerie Queen
- 14 Arthurian kings in their castles, travelling the realm
- Damsels in distress, mad knights, druids, hermits

### Persistence
- Single save slot with permadeath (save deleted on death)
- Bank: deposit gold that persists across deaths
- Home chest in Camelot: store items for future characters
- Canterbury graveyard: gravestones for all fallen characters
- Fallen Heroes screen on the title menu
- High score table with morgue file character dumps
- Game seed for challenge runs and sharing

## Terrain Guide

### Overworld Terrain
| Symbol | Colour | Terrain | Passable | Travel Time | Notes |
|--------|--------|---------|----------|-------------|-------|
| `.` | Yellow | Road | Yes | 5 min | Fast travel, safest route |
| `H` | Brown | Bridge | Yes | 5 min | Only way to cross rivers |
| `"` | Green | Grassland | Yes | 10 min | Normal open ground |
| `T` | Bright green | Forest | Yes | 20 min | Dense undergrowth, nature encounters |
| `^` | White | Hills | Yes | 25 min | Steep climbs, increased encounter chance |
| `,` | Green | Marsh | Yes | 30 min | Boggy, chance of getting stuck |
| `%` | Green | Swamp | Yes | 35 min | Exhausting, poison damage risk |
| `A` | Bright white | Mountains | **No** | -- | Impassable -- go around or find a pass |
| `&` | Bright green | Dense woods | **No** | -- | Impassable without Ranger/axe |
| `~` | Blue | Sea | **No** | -- | Impassable |
| `=` | Blue | River | **No** | -- | Impassable -- find a bridge (`H`) |
| `o` | Blue | Lake | **No** | -- | Use boat (`B`) or Walk on Water |
| `B` | Brown | Boat | Yes | -- | Board to sail across water |

*Weather adds extra time: Rain +2, Storm +5, Snow +4, Fog +1, Wind +1 min per step.*

### Overworld Locations
| Symbol | Colour | Location Type |
|--------|--------|---------------|
| `*` | Various | Town (press Enter to enter) |
| `#` | White/yellow | Castle (press Enter, locked at night) |
| `+` | Various | Landmark (press Enter to interact) |
| `>` | White | Dungeon entrance (press `>` to descend) |
| `V` | Red | Volcano (dungeon entrance) |
| `n` | Brown | Cottage (press Enter to explore) |
| `O` | Gray | Cave (press Enter to explore) |
| `(` | Various | Magic circle (press Enter to activate) |
| `A` | White | Abbey (press Enter to enter) |
| `D` | Bright green | Druid (bump to interact) |
| `@` | Bright white | You (the player) |

### Overworld Creatures
| Symbol | Colour | Creature | Behaviour |
|--------|--------|----------|-----------|
| `@` | White | Traveller | Walks roads, shares tips |
| `@` | Yellow | Pilgrim | Walks roads, heading to Canterbury |
| `@` | Green | Merchant | Walks roads, advertises wares |
| `@` | Brown | Peasant | Wanders grassland |
| `d` | Brown | Deer | In forests, flees when approached |
| `s` | White | Sheep | On grassland, bleats |
| `r` | Brown | Rabbit | On grassland, darts away |
| `v` | Gray | Crow | Anywhere, flies away |
| `D` | Bright green | Druid | Near magic circles/Stonehenge. 30% pickpocket, or blessing/lore |
| `p` | Red | Bandit | HOSTILE -- roads/grassland. Bump to fight |
| `w` | Red | Wolf | HOSTILE -- forests. Bump to fight |
| `B` | Red | Wild Boar | HOSTILE -- hills. Bump to fight |
| `z` | Red | Skeleton | HOSTILE -- scattered. Bump to fight |

### Town/Castle Interior
| Symbol | Colour | Meaning |
|--------|--------|---------|
| `#` | Gray/brown | Wall |
| `#` | Yellow | Throne room wall (castles) |
| `.` | Green | Courtyard |
| `.` | Yellow | Path / floor |
| `/` | Brown | Open door / gate |
| `I` | Brown | Innkeeper |
| `P` | White | Priest |
| `$` | Yellow | Blacksmith |
| `!` | Magenta | Apothecary |
| `P` | Green | Pawnbroker |
| `?` | Magenta | Mystic |
| `B` | Yellow | Banker |
| `O` | Blue | Well |
| `K` | Yellow | King (castles) |
| `Q` | Magenta | Queen (castles) |
| `G` | White | Guard (castles) |
| `@` | White | Townfolk (wanders) |
| `d` | Brown | Dog (wanders) |
| `c` | Yellow | Cat (wanders) |
| `k` | White | Chicken (wanders) |
| `M` | Brown | Monk (abbeys, wanders) |
| `N` | White | Nun (abbeys, wanders) |

### Dungeon Terrain
| Symbol | Colour | Terrain | Notes |
|--------|--------|---------|-------|
| `#` | Gray | Wall | Impassable, blocks line of sight |
| ` ` | Black | Solid rock | Unexplored, empty space |
| `.` | White | Floor | Standard walkable tile |
| `.` | Yellow | Secret room floor | Golden floor indicates a special hidden room |
| `+` | Brown | Closed door | Walk into to auto-open |
| `+` | Red | Locked door | Walk into to bash (STR check) |
| `/` | Brown | Open door | Press `c` + direction to close |
| `>` | White | Stairs down | Press `>` to descend (2 per level from level 2+) |
| `<` | White | Stairs up | Press `<` to ascend (2 per level from level 2+) |
| `^` | Red | Revealed trap | Safe to walk over. Press `D` adjacent to disarm |
| `0` | Cyan | Exit portal | Deepest level only -- teleports to overworld |
| `(` | Various | Magic circle | Step on for random effect (single use) |
| `~` | Blue | Shallow water | Passable, -1 HP per step (cold and filthy) |
| `~` | Blue | Deep water | **Impassable** -- too deep to cross |
| `^` | Bright red | Lava | 5-10 HP per step! Deeper levels only |
| `_` | Cyan | Ice | 30% chance to slip and slide |
| `"` | Green | Fungal growth | 20% chance of poison spores (1-3 HP) |
| `%` | Brown | Rubble | **Impassable** -- needs pickaxe to clear |
| `*` | Bright cyan | Crystal formation | Glows softly, atmospheric |
| `,` | Gray | Scattered rocks | Decorative, passable |

### Dungeon Room Features
| Symbol | Colour | Feature | Interaction |
|--------|--------|---------|-------------|
| `_` | Yellow | Altar | Walk on: +1 stat, full HP (single use) |
| `=` | Yellow bold | Treasure chest | Walk on: 20-60 gold |
| `=` | Brown | Dungeon chest | Walk on: 10-40 gold (20% trapped!) |
| `$` | Yellow | Gold pile | Walk on: 5-20 gold |
| `\|` | Brown | Bookshelf | Walk on: 30% chance of lore text |
| `-` | Gray | Coffin | Walk on: 40% gold, 30% attack, 30% empty |
| `O` | Gray | Pillar/stalagmite | Impassable, blocks movement |

### Dungeons
There are 9 named dungeons spread across England, each with a randomised depth:

| Dungeon | Location | Depth Range |
|---------|----------|-------------|
| Camelot Catacombs | Near Camelot | 3-6 levels |
| Tintagel Caves | Near Tintagel | 5-10 levels |
| Sherwood Depths | Near Sherwood | 5-10 levels |
| Mount Draig | Wales volcano | 4-8 levels |
| Glastonbury Tor | Near Glastonbury | 8-15 levels |
| White Cliffs Cave | Dover coast | 3-6 levels |
| Whitby Abbey | Near Whitby | 2-4 levels |
| Avalon Shrine | Avalon island | 3-5 levels |
| Orkney Barrows | Orkney island | 2-4 levels |

Each level is 160x80 tiles (scrollable), generated with BSP rooms and corridors. Levels are persistent -- items you drop stay where you left them.

## Keyboard Commands

### Movement (all map modes)
| Key | Action |
|-----|--------|
| `h` `j` `k` `l` `y` `u` `b` `n` | Move (vi-keys, 8-directional) |
| Arrow keys | Move (4-directional) |
| Numpad 1-9 | Move (8-directional) |
| `.` or `5` | Wait one turn |

### Overworld
| Key | Action |
|-----|--------|
| `Enter` | Enter a town, castle, or interact with landmark |
| `>` | Descend into a dungeon entrance |
| `c` | Camp (rest 8 hours, restore 50% HP/MP) |
| `M` | Minimap (full overworld overview) |
| `q` | Quit game |

### Town / Castle Interior
| Key | Action |
|-----|--------|
| Movement keys | Walk around, bump into NPCs to interact |
| `q` | Leave through the gate |

### Town Services

#### Inn
| Key | Action |
|-----|--------|
| `r` | Rest until morning (costs gold, full HP/MP) |
| `n` | Rest until nightfall (costs gold, full HP/MP) |
| `b` | Buy a beer (local brew, risk drunkenness!) |
| `q` | Leave the inn |

#### Church
| Key | Action |
|-----|--------|
| `p` | Pray (restore 25% HP/MP, free) |
| `d` | Donate gold (+1 chivalry per 20g) |
| `c` | Confession (+3 chivalry, once per visit) |
| `l` | Loot the church (-12 chivalry! Permanently banned) |
| `q` | Leave the church |

#### Bank
| Key | Action |
|-----|--------|
| `d` | Deposit 50 gold (5% fee) |
| `D` | Deposit all gold (5% fee) |
| `w` | Withdraw 50 gold |
| `W` | Withdraw all gold |
| `q` | Leave the bank |

#### Mystic
Pay 5 gold for a fortune. 60% chance of +1 to a random stat, 40% chance of -1.

#### Well
Climb down for a random outcome: treasure (40%), rat attack + loot (25%), or empty (35%).

### Dungeon
| Key | Action |
|-----|--------|
| `<` | Ascend stairs (return to overworld from level 1) |
| `>` | Descend stairs / use exit portal |
| `g` | Pick up item on the ground |
| `i` | Open inventory (works in all modes) |
| `;` | Look/identify mode -- examine tiles, monsters, items |
| `o` + direction | Open a door |
| `c` + direction | Close a door |
| `s` | Search adjacent walls for secret doors |
| `D` | Disarm an adjacent revealed trap (INT+SPD check) |
| `R` | Rest to recover HP/MP (interrupted if monster approaches) |
| `M` | Dungeon minimap (full level overview) |
| `T` | Toggle torch on/off (light radius 10 vs 2) |
| `q` | Quit game |

- Walking into a closed door `+` (brown) auto-opens it.
- Locked doors `+` (red) require bashing -- walk into them for a STR check.
- Walking into a monster attacks it (bump-to-attack).
- Traps are hidden until triggered or passively detected (10% within 2 tiles). Revealed traps `^` (red) are safe to walk over.
- Walk onto chests `=`, gold piles `$`, altars `_`, coffins `-`, and bookshelves `|` to interact.
- Stepping on an item shows "You see here: [item name]" -- press `g` to pick up.
- Weapons show as `|`, armor as `[`, potions as `!`, food as `%`, scrolls as `?`, tools as `(`.

### UI (all modes)
| Key | Action |
|-----|--------|
| `M` | Minimap (overworld only) |
| `P` | Message history (scroll with Up/Down, q to close) |
| `q` | Quit game / leave town |

## Player Guide

### Getting Started
1. You start at Camelot on the overworld. Walk to the yellow `*` and press **Enter** to visit the town.
2. Inside the town, **bump into NPCs** to interact with them -- the Innkeeper, Priest, Blacksmith, etc.
3. Walk through the **gate** (`/` at the bottom) to leave and explore the world.
4. Visit **Camelot Castle** `#` to meet the King in the Throne Room.

### Time & Travel
- Time passes with every action. Each step you take advances the game clock.
- **Dungeon movement**: 1 minute per step -- corridors are short.
- **Overworld movement**: varies by terrain. Roads are fast (5 min/step), grassland is normal (10 min), and swamps are exhausting (35 min/step). Plan your route!
- **Stick to roads** when possible -- they are 7x faster than swamps and have fewer encounters.
- **Day/night cycle**: shops close at night, castles lock their gates, visibility drops. The time of day is shown on your status bar: [Dawn], [Morning], [Midday], [Afternoon], [Dusk], [Evening], [Night].
- **Weather** changes as you travel. Scotland is colder and rainier, Wales is foggy, Whitby is always raining. Weather affects travel speed.
- **30-day lunar cycle**: special events happen on specific days. Watch for notifications at dawn: Feast Day, Market Day, Full Moon, Tournament Day, and more.

### Camping
Press `c` on grassland, road, or forest to camp for 8 hours. Restores 50% HP/MP. There's a small chance of a night ambush.

### Boats & Lakes
Walk onto a `B` tile to board a boat. You can then sail across lake water freely. When you step onto land, the boat is left at the shore behind you.

### Dungeons
- Walk onto a dungeon entrance `>` on the overworld and press `>` to enter.
- Each dungeon is randomly generated with BSP rooms and corridors (160x80 tiles, scrollable).
- The depth is randomised each playthrough (e.g. Camelot Catacombs: 3-6 levels).
- **From level 2 onwards**, each level has **2 sets of stairs** up and down, giving you multiple routes.
- Walk into closed doors `+` to auto-open them, or press `o` + direction. Press `c` + direction to close.
- Press `s` to search adjacent walls for **secret doors** (5% chance per wall tile).
- **Traps** are hidden on floor tiles. Stepping on one triggers it (damage, teleport, mana drain, etc.). Revealed traps show as `^` red. You have a 10% passive chance to spot traps within 2 tiles. Press `D` adjacent to a revealed trap to disarm it (INT + SPD check).
- The **deepest level** has an **exit portal** `0` (cyan) -- step on it to teleport back to the overworld instantly.
- Press `<` on stairs up to ascend. On level 1, ascending returns you to the overworld.
- Press `M` for a **dungeon minimap** showing the full level layout.
- Levels are **persistent** -- if you go back up, everything is exactly as you left it.

#### Room Variety
Dungeons contain 10 different room shapes: rectangles, pillared halls, circular caverns, cross-shaped rooms, L-shaped rooms, jagged caverns with rubble, multi-chamber caves, and pillared temples with altars. Corridors come in 3 styles: L-shaped, winding/twisting, and S-curves.

#### Hazards
- **Shallow water** (`~` blue): -1 HP per step. Cold and filthy.
- **Lava** (`^` bright red): 5-10 HP per step! Appears on deeper levels (3+).
- **Ice** (`_` cyan): 30% chance to slip and slide an extra tile. May crash into walls.

#### Magic Circles
Coloured `(` glyphs on the floor. Step on one for a random effect (single use):
- Healing (full HP), Mana (full MP), Blessing (+1 stat), Gold, Teleportation
- Dangerous: Fire damage, Mana drain, Confusion

#### Secret Rooms
Some levels contain hidden rooms with golden floors and treasure chests. They are not connected to any corridor -- you cannot walk to them normally. Three ways to find them:
1. **Search walls** (`s`) -- if a secret door is nearby in the wall, you'll reveal it
2. **Teleport trap** -- a trap may randomly land you inside a secret room
3. **Magic circle teleportation** -- a teleport circle may drop you there

Secret rooms appear ~30% of the time, more often on deeper levels. Keep searching those walls!

### Towns & Castles
- **Towns** (`*`) are open at all hours. Press Enter to enter.
- **Castles** (`#`) are locked at night (22:00-04:59). Return at dawn, or have high chivalry (60+) to convince the guards.
- Inside, walk around and **bump into NPCs** to use services.
- Castles have a **Throne Room** with the King, possibly the Queen, and Guards.

### Landmarks
Press Enter on a landmark to interact:
- **Stonehenge**: +1 to a random stat (once per game)
- **Faerie Ring**: random effect -- stat boost, gold, MP restore, stat swap, or teleport!
- **Avalon**: full HP/MP restore
- **Holy Island**: +2 chivalry, full HP restore
- **Magic Circles**: random effect -- healing, mana, blessing, gold, teleport, or fire/drain/confusion
- **Rainbow** (`=` cycling colours): appears after rain clears. Race to its end for 100-500 gold!

### Abbeys
Seven historical abbeys (`A` white) are scattered across England. Press Enter to enter.
- Westminster Abbey, Whitby Abbey, Rievaulx Abbey, Bath Abbey, St Mary's Abbey, Cleeve Abbey, Mount Grace Priory
- Each has an **inn with extra strong beer** (4g per pint!), a **potion shop**, and a **church**
- Populated by **monks** (`M` brown) or **nuns** (`N` white) -- never both in the same abbey
- Abbeys are **always open** -- no night lockout
- Bump into monks/nuns for prayers, blessings, and advice about the abbey's ale

### Cottages & Caves
- **Cottages** (`n` brown) and **caves** (`O` gray) are scattered across the map. Press Enter to explore.
- Random encounters inside: friendly NPCs, empty shelters, bandits, hermits, alchemist labs
- After visiting, they become abandoned -- but **reset after 5-20 days** with new occupants

### Magical Deer
Deer (`d` brown) roam the forests. When you bump into one, there's a **10% chance** it glows with golden light and grants **+1 to a random stat**!

### Items & Inventory
- Press **`i`** anywhere to open your inventory (works on overworld, in towns, and dungeons).
- **Equipment slots**: Weapon, Armor, Shield, Helmet, Boots, Gloves, Ring 1, Ring 2.
- Select an item with `a`-`z`, then: **`e`** equip, **`d`** drop, **`u`** use (potions/food).
- **Equipped weapon** adds power to your attack damage. **All armor** stacks for defense.
- Items spawn in dungeon rooms (`|` weapons, `[` armor, `!` potions, `%` food, `$` gold, `?` scrolls, `(` tools).
- Press **`g`** to pick up items on the ground. Gold goes straight to your wallet.
- Monsters drop items on death: knights drop weapons, warlocks drop potions, beasts drop food.
- **Shops** in towns sell items -- bump into the Blacksmith `$`, Apothecary `!`, or Pawnbroker `P`.
- Press **`S`** in a shop to sell items from your inventory.
- You start with: Rusty Sword (equipped), Leather Armor (equipped), 3x Bread.

### Look/Identify Mode
Press **`;`** in a dungeon to enter look mode:
- Move a cursor with hjkl/arrow keys across visible tiles.
- **Monsters** show: name, HP, STR, DEF, SPD, XP reward.
- **Items** show: name, type, power, value, weight.
- **Tiles** show: description (Wall, Shallow water, Lava, Magic circle, etc.)
- Press **`;`** again or **Escape** to exit. (`q` does NOT exit look mode.)

### Resting
Press **`R`** in a dungeon to rest and recover HP/MP:
- Recovers 1 HP every 5 turns, 1 MP every 8 turns.
- Rests until full HP or 100 turns max.
- **Monsters still move** while you rest -- they can reach you!
- **Interrupted** if a monster reaches an adjacent tile.
- **3% chance per turn** of a new monster spawning nearby.

### Combat
- **Bump-to-attack**: walk into an enemy to attack. Both overworld (red creatures) and dungeon monsters.
- **Hit chance**: 70% base + your STR - enemy DEF*2 (min 15%, max 95%). You can miss!
- **Critical hits**: 10% chance for double damage. "CRITICAL HIT!"
- **Damage**: your STR + random(-2,2) - enemy DEF (minimum 1).
- **Monster AI varies by type**:
  - Fast monsters (bats, imps) act more often due to the speed/energy system
  - Slow monsters (trolls, giants) sometimes skip turns
  - Some flee when wounded (rats, wolves, warlocks)
  - Some move erratically (bats, wraiths, imps)
  - Some attack at range (imps, warlocks, dragons, monks)
  - Some can open doors while chasing you (bandits, knights, trolls)
- **Gold drops**: bandits drop 3-15g, knights and dragons drop 15-50g
- **Level feelings**: on entering a dungeon level, you get a warning: "This seems quiet" to "This level is DEADLY!"
- **Respawning**: a new monster spawns outside your view every ~80 turns. Cleared levels don't stay safe.
- **Overworld combat**: red-coloured creatures (bandits, wolves, boars, skeletons) are hostile. Bump to fight, same combat rules.
- **85 monster types** loaded from `data/monsters.csv` -- edit the file to add or rebalance monsters.

### Beer & Drunkenness
Buy beer at any inn (2-3g per pint). Each pint increases drunkenness:
1. Merry -- no effect
2. Tipsy -- 10% chance of stumbling (random movement)
3. Drunk -- 25% stumble, -1 INT, +1 STR
4. Very drunk -- 40% stumble, -2 INT, -1 SPD, +2 STR
5. Pass out -- wake up 8 hours later with 50% HP

Drunkenness wears off over time.

### Chivalry
Your chivalry score (0-100) determines how NPCs treat you:
- **Below 15**: Camelot gates are closed to you
- **Below 30**: King Arthur won't speak with you
- **40+**: Lady of the Lake may offer Excalibur
- **50+**: eligible for the Princess quest
- **60+**: castle gates open at night, best prices
- **70+**: The Siege Perilous legendary artifact won't harm you

Raise chivalry by: donating at churches, confession, visiting Holy Island.
Lower chivalry by: looting churches (-12), drinking too much, bad choices.

### The Holy Grail
The Grail is hidden in a random dungeon each game. Gather hints from inn NPCs (beware -- some are red herrings!). Merlin and King Pellam always give truthful hints. Defeat Mordred to claim it, then return it to Arthur (chivalry 30+ required).

### The True Ending
Rescue the princess from the Evil Sorcerer's tower, then visit her at her father's castle. Accept her marriage proposal for the game's true ending -- you become King/Queen with a +10000 score bonus.

## Building

```bash
make          # Build the game
./camelot     # Run the game
make clean    # Clean build files
./camelot -s 12345   # Start with a specific seed
```

Requires: C compiler (clang/gcc), ncurses library, 256-colour terminal recommended.

## Data Files

All game content (monsters, items, spells, quests, etc.) will be defined in CSV files under `data/`. Edit these to rebalance the game, add new content, or create mods -- no recompilation needed.

## License

See [LICENSE](LICENSE) for details.
