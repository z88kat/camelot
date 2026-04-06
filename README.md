# Knights of Camelot

**The Quest for the Holy Grail** -- a terminal-based ASCII roguelike set in Arthurian England.

## About

Knights of Camelot is a full-featured roguelike game written in C using ncurses. You play as a knight, wizard, or ranger on a quest to find the Holy Grail. Explore an overworld map of medieval England, visit towns and castles, delve into dangerous dungeons, and encounter legendary characters from Arthurian myth.

## Current State

**Implemented (Phases 1-14):**
- Character creation: class (Knight/Wizard/Ranger), gender, name, stat rolling, random gold (30-200)
- 50-spell system with Light, Dark, Nature, and Universal schools
- A* pathfinding and FSM AI for monsters (IDLE/CHASE/FLEE states)
- Monster special abilities: breath weapons, summoning, healing, debuffs, explode-on-death, fear auras
- Overworld map of England (500x250 tiles) with procedural terrain, coastlines, rivers, lakes, mountains, islands
- 14 towns, 16 active castles, 3 abandoned castles, 8 abbeys with walkable interior maps
- Day/night cycle, regional weather, 30-day lunar cycle
- Town services: Inn, Church, Blacksmith, Apothecary, Baker, Jeweller, Pawnbroker, Mystic, Bank, Well, Stable
- BSP-generated dungeons (160x80 tiles, scrollable) with multiple levels
- 85+ monster types, bump-to-attack combat with hit/miss rolls and critical hits
- 280+ items: weapons, armor, potions, food, scrolls, spell scrolls, tools, 25 rings, 25 amulets, 20 gems, 25 treasures
- Weight-based inventory with encumbrance and 9 equipment slots (including amulet)
- Quest system with 15 side quests and the Holy Grail main quest
- Dungeon bosses on deepest levels (12 unique bosses + Mordred as Grail guardian)
- Leveling system with 20 levels and class-based HP/MP progression
- Chivalry system (0-100) with titles from Knave to Paragon of Virtue
- Title screen with ASCII art, continue/new game/high scores/fallen heroes
- Save/load with permadeath (save deleted on death)
- Death screen with score calculation, high score table, fallen heroes record
- All game data loaded from CSV files (monsters, items, spells, quests, towns, locations, creatures, names)

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
| `o` | Blue | Lake | **No** | -- | Use boat (`B`) or Walk on Water spell |
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
| `@` | White | Traveller | Walks roads. 75% tips, 10% gift, 10% steal gold, 5% steal item |
| `@` | Yellow | Pilgrim | Walks roads, heading to Canterbury |
| `@` | Green | Merchant | Walks roads, advertises wares |
| `@` | Brown | Peasant | Wanders grassland |
| `d` | Brown | Deer | In forests, flees when approached |
| `s` | White | Sheep | On grassland, bleats |
| `r` | Brown | Rabbit | On grassland, darts away |
| `v` | Gray | Crow | Anywhere, flies away |
| `D` | Bright green | Druid | Near magic circles/Stonehenge |
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
| `$` | Yellow | Blacksmith (weapons & armor) |
| `!` | Magenta | Apothecary (potions & scrolls) |
| `%` | Brown | Baker (food) |
| `*` | Bright cyan | Jeweller (rings, amulets & gems) |
| `P` | Green | Pawnbroker (buys/sells anything) |
| `?` | Magenta | Mystic |
| `B` | Yellow | Banker |
| `S` | Brown | Stablemaster |
| `~` | Cyan | Hot Springs (Bath only) |
| `+` | Gray | Graveyard (Canterbury only) |
| `O` | Blue | Well |
| `K` | Yellow | King (castles) |
| `Q` | Magenta | Queen (castles) |
| `G` | White | Guard (castles) |
| `T` | White | Townfolk (wanders) |
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
| `>` | White | Stairs down | Press `>` to descend |
| `<` | White | Stairs up | Press `<` to ascend (return to overworld from level 1) |
| `^` | Red | Revealed trap | Safe to walk over. Press `D` adjacent to disarm |
| `0` | Cyan | Exit portal | Deepest level only -- teleports to overworld |
| `(` | Various | Magic circle | Step on for random effect (single use) |
| `~` | Blue | Shallow water | Passable, -1 HP per step |
| `^` | Bright red | Lava | 5-10 HP per step! Deeper levels only |
| `_` | Cyan | Ice | 30% chance to slip and slide |
| `"` | Green | Fungal growth | 20% chance of poison spores (1-3 HP) |
| `%` | Brown | Rubble | **Impassable** -- needs pickaxe to clear |
| `*` | Bright cyan | Crystal formation | Glows softly, atmospheric |

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

### Dungeon Items
| Symbol | Colour | Item Type |
|--------|--------|-----------|
| `\|` | Various | Weapon (swords, axes, maces, staves) |
| `[` | Various | Body armor |
| `)` | Various | Shield |
| `^` | Various | Helmet |
| `]` | Various | Boots or gloves |
| `!` | Various | Potion |
| `?` | Various | Scroll |
| `%` | Various | Food |
| `(` | Various | Tool (torch, lockpick, pickaxe) |
| `o` | Various | Ring (magical, equippable) |
| `"` | Various | Amulet (magical, equippable) |
| `*` | Various | Gem (valuable treasure) |
| `&` | Various | Treasure (Golden Teddy Bear, Crystal Skull, etc.) |
| `$` | Yellow | Gold pile |

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

**Abandoned Castles** can also be explored as mini-dungeons (press `>` while standing on one):

| Castle | Location | Floors | Theme |
|--------|----------|--------|-------|
| Castle Dolorous Garde | Central England | 2-3 | Haunted -- ghosts and skeletons |
| Castle Perilous | Near Camelot | 2-3 | Dark sorcery -- magical traps |
| Bamburgh Castle | Northern coast | 2-3 | Bandit stronghold |

Each level is 160x80 tiles (scrollable), generated with BSP rooms and corridors. Levels are persistent -- items you drop stay where you left them.

## Keyboard Commands

### Character Creation
| Key | Action |
|-----|--------|
| `1` `2` `3` | Select class (Knight/Wizard/Ranger) or gender (Male/Female) |
| `r` | Random name / randomise appearance / re-roll stats and gold |
| `1`-`4` | Cycle appearance options (hair, eyes, build, feature) |
| `C` | Cheat mode (on stats screen -- God Mode, Rich Start, Max Stats) |
| `Enter` | Accept and continue |
| Backspace | Delete character in name entry |

Creation flow: Class -> Gender -> Name -> Appearance -> Stats -> Story

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
| `>` | Descend into a dungeon entrance or abandoned castle |
| `c` | Camp (rest 8 hours, restore 50% HP/MP, risk of ambush) |
| `f` | Fish from a shore tile (must be adjacent to water) |
| `F` | Forage for food (forests, grassland, hills) |
| `x` | Dig for buried treasure (must be near marked spot) |
| `K` | Cook raw food (needs Torch or Tinderbox, campable terrain) |
| `H` | Mount/dismount horse (if owned) |
| `S` | Save game |
| `z` | Cast a spell |
| `i` | Inventory |
| `M` | Minimap (full overworld overview, press `L` to toggle location labels) |
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

#### Shops (Blacksmith, Apothecary, Baker, Jeweller, Pawnbroker)
| Key | Action |
|-----|--------|
| `a`-`z` | Buy an item from the list |
| `S` | Switch to sell mode (sell from your inventory) |
| `h` | Haggle for a discount (INT + chivalry check) |
| `t` | Attempt to steal an item (SPD check, risky!) |
| `q` | Leave the shop |

Stock is randomised each visit:
- **Blacksmith**: 6-12 weapons and armor
- **Apothecary**: 6-12 potions and scrolls
- **Baker**: 3-6 food items
- **Jeweller**: 4-10 rings, amulets, and gems
- **Pawnbroker**: 8-16 items of any type

**Haggling**: Success gives you a 10-20% gold credit. Chance scales with INT and chivalry (max 80%). Low chivalry makes shopkeepers less receptive.

**Stealing**: SPD check (max 70%). Success: free item but -8 chivalry. Failure: guards attack (3-10 HP damage), -5 chivalry, kicked out of the shop. A desperate option for the gold-poor or morally flexible.

#### Church
| Key | Action |
|-----|--------|
| `p` | Pray (restore 25% HP/MP, free) |
| `d` | Donate gold (+1 chivalry per 20g) |
| `c` | Confession (+3 chivalry, once per visit, requires chivalry below 50) |
| `u` | Cure poison and curses (30 gold -- full HP/MP, restores debuffed stats) |
| `l` | Loot the church (-12 chivalry! Permanently banned) |
| `q` | Leave the church |

The priest can cure poison from rotten fish, stat debuffs from monster curses, and other ailments for a 30 gold donation. Also fully restores HP and MP.

#### Bank
| Key | Action |
|-----|--------|
| `d` | Deposit 50 gold (5% fee) |
| `D` | Deposit all gold (5% fee) |
| `w` | Withdraw 50 gold |
| `W` | Withdraw all gold |
| `q` | Leave the bank |

#### Stable
| Key | Action |
|-----|--------|
| `1` | Buy a Pony (40-64g -- 1.5x speed, no hill penalty) |
| `2` | Buy a Palfrey (85-119g -- 2x speed) |
| `3` | Buy a Destrier (170-234g -- 2x speed, +2 DEF in combat) |
| `s` | Sell your current horse (if you own one) |

Prices vary by town. Can only own one horse at a time.

#### Mystic
Pay 5 gold for a fortune. 60% chance of +1 to a random stat, 40% chance of -1.

#### Well
Climb down for a random outcome: treasure (40%), rat attack + loot (25%), or empty (35%).

### Dungeon
| Key | Action |
|-----|--------|
| `<` | Ascend stairs (return to overworld from level 1) |
| `>` | Descend stairs / use exit portal |
| `f` | Fire ranged weapon at nearest visible enemy (requires bow/crossbow + ammo) |
| `g` | Pick up item on the ground |
| `i` | Open inventory |
| `z` | Cast a spell |
| `;` | Look/identify mode -- examine tiles, monsters, items |
| `o` + direction | Open a door |
| `c` + direction | Close a door |
| `s` | Search adjacent walls for secret doors |
| `D` | Disarm an adjacent revealed trap (INT+SPD check) |
| `R` | Rest to recover HP/MP (interrupted if monster approaches) |
| `M` | Dungeon minimap (full level overview) |
| `T` | Light/extinguish torch or lantern (uses fuel) |
| `q` | Quit game |

### UI (all modes)
| Key | Action |
|-----|--------|
| `?` | Help system (4 pages: commands, UI/items, symbols, tips) |
| `=` | Settings (version, seed, file locations, terminal info) |
| `@` | Character sheet (appearance, stats, affinities, combat info) |
| `i` | Inventory |
| `z` | Cast spell (overworld/dungeon) |
| `Z` | View spellbook (all known spells with stats) |
| `J` | Quest journal (main quest + side quests) |
| `;` | Look/identify mode (overworld and dungeon -- examine tiles, locations, creatures) |
| `M` | Minimap |
| `P` | Message history (scroll with Up/Down, q to close) |
| `S` | Save game (overworld only) |
| `q` | Quit (prompts: `s` save & quit, `q` quit without saving, `Esc` cancel) |

## Player Guide

### Getting Started
1. **Create your character**: choose a class, gender, name, and roll your stats. Starting gold is random (30-200). Each class plays differently:
   - **Knight**: High HP (30), strong melee (+2 STR/DEF), limited magic (4 spells, 8 MP). Starts with Longsword, Chainmail, Shield spell, Torch, Bread.
   - **Wizard**: Low HP (18), powerful magic (15 spells, 30 MP, +3 INT). Starts with Wooden Staff, Tattered Robes, Magic Missile, Torch, Bread.
   - **Ranger**: Balanced (24 HP, 6 spells, 15 MP, +2 SPD). Starts with Short Sword, Leather Armor, Detect Traps, Torch, Bread.
2. You start at **Camelot** on the overworld. Walk to the yellow `*` and press **Enter** to visit the town.
3. Inside the town, **bump into NPCs** to interact -- Innkeeper, Blacksmith, Baker, Jeweller, etc.
4. Walk through the **gate** (`/` at the bottom) to leave and explore the world.
5. Visit **Camelot Castle** `#` to meet the King in the Throne Room.

### Classes & Spells

#### Spell Casting
Press **`z`** to open the spell menu. Select a spell with `a`-`z` to cast it. Each spell costs MP. Spells you can't afford are greyed out. Spells are colour-coded by school:
- **Universal** (white): no affiliation required
- **Light** (yellow): healing, protection, holy damage. Requires Light affinity (gained from churches, Holy Island, Merlin)
- **Dark** (magenta): draining, cursing, summoning undead. Requires Dark affinity (gained from Morgan le Fay, dark bargains)
- **Nature** (green): elemental damage, beast forms, terrain control. Requires Nature affinity (gained from forests, Stonehenge, druids)

Spells with an affiliation threshold will fail if your affinity is too low: "Your Light affinity is too low! (need 15, have 3)"

**Spell types:**
- **Damage** (Magic Missile, Fireball, Holy Strike, Dark Bolt, Lightning): damages the nearest visible monster
- **Heal** (Heal, Greater Heal, Regrowth): restores your HP
- **Shield**: absorbs incoming damage until depleted
- **Buff** (Divine Shield, Beast Form, Haste, Blessing): temporarily boosts your stats (+3 for N turns)
- **Teleport** (Teleport, Blink): moves you to a random safe tile
- **Reveal** (Map, Holy Light): reveals the entire dungeon floor
- **Detect** (Detect Traps, Identify): reveals hidden traps
- **Fear** (Fear, Turn Undead, Nightmare): forces nearby enemies to flee
- **Drain** (Drain Life, Sacred Flame, Soul Steal): damages an enemy and heals you for half the damage dealt

#### Spellbook
Press **`Z`** (Shift+Z) to view your spellbook. Shows all known spells with their effect type, MP cost, damage, range, and school. This is a reference screen -- to actually cast, press `z` instead.

#### Learning New Spells
You start with **1 spell** based on your class. To learn more:
1. Find **Spell Scrolls** in dungeons (they appear as `?` like regular scrolls but are named "Scroll: [Spell Name]")
2. Buy them from the **Apothecary** in towns (they stock scrolls alongside potions)
3. Open your **inventory** (`i`), select the spell scroll, and press **`u`** to use it

**Requirements to learn:**
- You must meet the spell's **level requirement** (shown in spells.csv). If you're too low level: "You cannot comprehend the magic within..."
- You can't learn a spell you already know

**Spell capacity by class:**
| Class | Max Spells | What happens when full |
|-------|-----------|----------------------|
| Knight | 4 | A random existing spell is forgotten and replaced |
| Ranger | 6 | A random existing spell is forgotten and replaced |
| Wizard | 15 | Can freely learn up to 15 spells |

When a spell is replaced: "The new magic pushes an old spell from your mind... you forget [old spell] and learn [new spell]!" You cannot choose which spell is lost.

**24 Spell Scrolls** can be found, covering all four schools:
- **Universal**: Magic Missile, Light, Shield, Teleport, Haste, Blink, Fireball
- **Light**: Heal, Greater Heal, Holy Strike, Divine Shield, Smite, Turn Undead, Blessing
- **Dark**: Drain Life, Shadow Step, Curse, Fear, Dark Bolt
- **Nature**: Entangle, Lightning, Bark Skin, Regrowth, Walk on Water

#### MP Regeneration
- **1 MP every 20 turns** (passive regeneration)
- Rest at an inn (full MP restore)
- Mana Potions (10 MP), Greater Mana Potions (30 MP)
- Camping on overworld (restores 50% MP)
- Praying at a church (restores 25% MP)
- Level up (full MP restore)

#### Leveling Up
Gain XP by killing monsters and completing quests. When you reach the next XP threshold, a **level-up screen** appears where you **choose +1 to any stat** (STR, DEF, INT, or SPD):
- HP and MP increase based on your class (Knight: +3 HP/+1 MP, Wizard: +1 HP/+4 MP, Ranger: +2 HP/+2 MP)
- Full HP/MP restore on level-up
- Carry capacity increases (+2 per level)
- Max level: 20

**Class perks at milestone levels:**

| Level | Knight | Wizard | Ranger |
|-------|--------|--------|--------|
| 3 | +1 spell capacity | +1 spell capacity | +1 spell capacity |
| 5 | +2 max HP per level | -1 MP cost on spells | Trap detection doubled |
| 10 | Shield Bash (20% stun) | Mana Shield (absorb with MP) | Double Shot (30% double hit) |
| 15 | Immunity to fear | Free spell/turn if INT>10 | Always act first |

Knights also gain an extra +2 max HP per level from level 6 onwards.

**XP thresholds:** 50, 120, 250, 450, 750, 1200, 1800, 2600, 3600, 5000, 6800, 9000, 12000, 15500, 20000, 25000, 31000, 38000, 46000

#### Appearance
During character creation you customise your appearance (hair colour, eye colour, build, and distinguishing feature). These are purely cosmetic. Press number keys to cycle options or `r` to randomise.

#### Character Sheet
Press **`@`** at any time to view your full character sheet, including:
- Name, class, gender, level, and chivalry title
- Appearance (hair, eyes, build, distinguishing feature)
- All stats (HP/MP, STR/DEF/INT/SPD, gold, weight)
- Spell affinities (Light, Dark, Nature)
- Combat summary (XP, kills, quests completed, shield absorb, mount status, spells known)

### Time & Travel
- Time passes with every action. Each step advances the game clock.
- **Dungeon movement**: 1 minute per step.
- **Overworld movement**: varies by terrain. Roads (5 min), grassland (10 min), forest (20 min), hills (25 min), swamp (35 min). **Riding a horse halves all travel times.**
- **Stick to roads** when possible -- they are 7x faster than swamps. Buy a horse early for even faster travel.
- **Day/night cycle**: shops close at night, castles lock their gates, visibility drops. Time shown on status bar: [Dawn], [Morning], [Midday], [Afternoon], [Dusk], [Evening], [Night].
- **Weather** changes as you travel. Affects visibility and travel speed. Scotland is colder, Wales is foggy, Whitby is always raining.

### Camping
Press `c` on grassland, road, or forest to camp for 8 hours. Restores 50% HP/MP.

### Boats & Lakes
Walk onto a `B` tile to board a boat. Sail across lake water freely. Step onto land to disembark -- the boat is left at the water's edge behind you for later use.

**While sailing**, each move has a chance of an encounter:
- **Water monster attack** (3%): A Water Serpent, Kelpie, Giant Pike, or Nixie rises from the depths. Quick combat -- you strike first, then it retaliates if alive
- **Auto-fishing** (1%): You trail a line and catch a fish
- **Ghost Ship** (1%): A spectral ship emerges from the mist! Drowned knights attack (5-12 damage), but you plunder 50-200 gold and 50 XP from the hold
- **Atmospheric flavour** (3%): Descriptions of the water, mist, and splashing

### Fishing
Press **`f`** on the overworld while standing next to water (lake, river, or sea) to cast a line. Fishing takes 30 minutes of game time.
- **40% chance**: Catch a fish (Fresh Fish, Salted Fish, or Smoked Salmon) -- good food for dungeon delving
- **15% chance**: Reel in an old pouch with 5-25 gold
- **45% chance**: Nothing bites -- try again later

Fishing is a free, repeatable way to stock up on food without spending gold. Look for shore tiles near lakes and rivers.

### Foraging
Press **`F`** (Shift+F) on the overworld to search for wild food. Works in **forests** (apples, berries, mushrooms), **grassland** (apples, berries, turnips), and **hills** (berries, mushrooms). Takes 30 minutes of game time.
- **35% chance**: Find food (added to inventory)
- **10% chance**: Find healing herbs (+3-8 HP on the spot)
- **5% chance**: Find scattered coins (2-10 gold)
- **50% chance**: Nothing found

A free way to keep your food supply topped up while travelling. Combine with cooking (`K`) for maximum benefit -- forage Raw Meat from beast encounters, then cook it for 3x healing power.

### Treasure Maps
**Tattered Maps** (`&` brown) are rare items found in dungeons. Use one from your inventory (`i` then `u`) to view it -- it shows a crude ASCII map with a red `X` marking the treasure location and a hint about the region (Northern/Central/Southern England).

Navigate to the marked spot on the overworld and press **`x`** to dig. You must be within 1 tile of the `X`. Digging takes 15 minutes.

**Cache types** (random):
- **Small** (30%): 50-100 gold
- **Medium** (30%): 150-300 gold + a weapon, armor, or potion
- **Large** (25%): 500-800 gold + a rare ring, amulet, gem, or spell scroll
- **Bandit trap** (10%): ambush! Fight bandits (5-12 HP damage), loot their pockets
- **Undead guardian** (5%): a Barrow-Wight rises! Tough fight (8-15 HP damage), but 100-250 gold reward

5-8 treasure maps are scattered across the game's dungeons. Each map reveals the nearest unfound treasure to your current position.

### Cottages & Caves
**Cottages** (`n` brown) and **caves** (`O` gray) are scattered across the map. Press Enter to explore. Each has a random encounter inside:

**Cottages** (4 outcomes):
- **Empty** (30%): abandoned shelter, rest for +25% HP/MP
- **Friendly NPC** (35%): hermit (dungeon rumours), healer (full HP), retired knight (+1 STR), or peasant (+50% HP). Always +2 chivalry
- **Bandits** (20%): fight two bandits (3-8 HP damage), loot 8-25 gold
- **Alchemist lab** (15%): find gold and a mana potion (+10 MP)

**Caves** (4 outcomes):
- **Empty** (25%): dry shelter, rest for +25% HP
- **Bear lair** (30%): fight a bear (4-10 HP damage), loot 5-15 gold
- **Hermit** (35%): blessing (+1 random stat, +2 chivalry)
- **Bandit hideout** (10%): fight bandits (2-6 HP damage), loot 15-40 gold

Both reset after **5-20 days** (cottages) or **7-20 days** (caves) for revisiting.

### Horses
Buy a horse from any **Stablemaster** (`S`). Three types available at varying prices per town (see Stable keys above). Once owned:
- Press **`H`** on the overworld to mount or dismount
- **Halves travel time** on all terrain (Palfrey/Destrier: roads 5→3 min, grassland 10→5 min; Pony: 1.5x speed, hills treated as grassland)
- Status bar shows **"Riding"** while mounted
- You **automatically dismount** when entering a town, castle, dungeon, or boat
- You **automatically remount** when leaving a town or dungeon back to the overworld
- Stables are found in major towns: Camelot, London, York, Winchester

A horse is one of the best early investments -- it makes exploring the large overworld much faster.

### Dungeons
- Walk onto a dungeon entrance `>` on the overworld and press `>` to enter.
- Each dungeon is randomly generated with BSP rooms and corridors (160x80 tiles).
- The depth is randomised each playthrough.
- Walk into closed doors `+` to auto-open them. Locked doors `+` (red) require bashing (STR check).
- Press `s` to search adjacent walls for **secret doors**.
- **Traps** are hidden. Stepping on one triggers it. Revealed traps show as `^` (red). Press `D` to disarm.
- The **deepest level** has an **exit portal** `0` (cyan) -- teleports back to the overworld.
- Press `<` on stairs up to ascend. On level 1, ascending returns to the overworld.
- Press `M` for a **dungeon minimap**.
- Levels are **persistent** -- items stay where you left them.
- **Level feelings** on entry warn you of danger: "This seems quiet" to "This level is DEADLY!"

#### Torches & Lanterns
Dungeons are dark -- without a light source your vision radius is only **2 tiles**. Press **`T`** to light a torch or lantern from your inventory.

| Light Source | Fuel (turns) | Cost | Notes |
|-------------|-------------|------|-------|
| **Torch** | 200 | 5g | Cheap, short burn. Consumed when spent |
| **Lantern** | 500 | 25g | Long burn. Stays when empty -- refuel with Oil Flask |
| **Oil Flask** | +300 | 10g | Refuels a spent lantern. Press `T` to use |

- Lit light gives **FOV radius 10** (full dungeon visibility)
- Fuel decreases by 1 each turn. Warning at 20 fuel remaining
- At 0 fuel: auto-extinguishes -- "Plunged into darkness!"
- **Extinguish** with `T` to save fuel -- relight later
- Dungeon status bar shows remaining fuel: `Torch:143` or `Lamp:387`
- **Lanterns** are reusable -- when empty, refuel with an Oil Flask (press `T`)
- **Torches** are single-use -- when spent, they're gone
- Buy from Blacksmith or Pawnbroker. You start with one Torch in your inventory

**Tip:** Carry a Lantern and a couple of Oil Flasks for long dungeon runs. Extinguish your light when resting to conserve fuel.

### Combat
- **Bump-to-attack**: walk into an enemy to attack.
- **Hit chance**: 70% base + your STR - enemy DEF*2 (min 15%, max 95%).
- **Critical hits**: 10% chance for double damage.
- **Damage**: your STR + weapon power + random(-2,2) - enemy DEF - armor (minimum 1).
- **Shield spell**: absorbs damage before it hits your HP.
- **Equipped rings and amulets** add their power bonus to your stats.
- **BUC bonus**: blessed weapons deal +2 damage, cursed weapons deal -2.

#### Ranged Combat
Press **`f`** in a dungeon to fire your ranged weapon at the nearest visible enemy:
- **Requires**: a ranged weapon equipped (Short Bow, Longbow, Crossbow, or Sling) AND matching ammo in inventory
- **Ammo types**: Arrows (for bows), Bolts (crossbow), Stones (sling), Throwing Knives (standalone). Also Silver Arrows and Fire Arrows
- **Range**: Short Bow 6 tiles, Longbow 10, Crossbow 8, Sling 5
- **Damage**: weapon power + ammo modifier + STR/2 + random
- **Ammo recovery**: 50% chance to recover arrows/bolts after firing (stones consumed)
- **Critical hits**: 10% chance for double damage, same as melee
- Buy ammo from Blacksmiths, find in dungeons

#### Blessed / Cursed / Uncursed (BUC)
Every item has a hidden BUC state: **Blessed** (10%), **Uncursed** (75%), or **Cursed** (15%). BUC is unknown until identified.

**Identify BUC**: pray at a church (`p`) -- the holy light reveals the BUC state of all carried items.

**Effects**:
- **Blessed** (shown in yellow): weapons +2 damage, potions heal more, armor +1 DEF
- **Cursed** (shown in red): weapons -2 damage, armor -1 DEF
- **Uncursed**: normal effects

#### Wandering Spirits
At **night** on the overworld, spectral presences may drain your MP (1-3 MP, ~7% chance per 15 turns). Stay indoors or carry holy items to avoid them.

#### Monster AI
Monsters use A* pathfinding to chase you through corridors and around obstacles.
- **IDLE**: monsters don't notice you until you enter their line of sight
- **CHASE**: monsters pursue you, remembering your last known position
- **FLEE**: wounded monsters (below 25% HP) run away
- **Special abilities**:
  - **Ghosts/Wraiths/Spectres**: walk through walls, chill debuff (-1 stat)
  - **Dragons**: breath weapon (high fire damage in a line, 4-turn cooldown)
  - **Necromancers/Witches/Sorcerers**: summon allies (skeletons, spiders, imps)
  - **Troll Shamans/Dark Monks**: heal nearby wounded allies
  - **Witches/Enchantresses**: curse you (random -1 stat)
  - **Death Knights/Demons/Mordred**: fear aura (dread message when nearby)
  - **Imps**: explode on death (2-5 fire damage to adjacent tiles)
  - **Black Knights/Hellhounds**: always chase, never idle
  - **Werewolves**: bite has 10% chance (Alpha: 20%) to inflict **Lycanthropy** (see below)

#### Lycanthropy (Werewolf Curse)
If bitten by a werewolf, you gain **Lycanthropy**: a permanent +3 STR and +2 SPD, but every **Full Moon (Day 15)** you involuntarily transform:
- You black out and wake at a random overworld location
- You lose 1-2 random items from your inventory
- 20% chance you killed an NPC during the night (-5 chivalry)
- Full HP restored ("you... ate something")

**Cure lycanthropy**: church cure (`u` for 30 gold), Purify spell, or Holy Water potion. Curing removes the +3 STR/+2 SPD bonus.

#### Vampirism (Vampire Curse)
If bitten by a Vampire or Vampire Lord (10% chance per hit), you contract **Vampirism**:

**Benefits:**
- **+3 STR, +2 SPD** permanently while cursed
- **HP drain**: steal 2 HP per melee hit you land on enemies
- **Dark vision**: full FOV in dungeons without a torch
- **No hunger**: you don't need to eat (you... sustain differently)

**Drawbacks:**
- **Sunlight burns**: 3 HP damage per 10 turns while outdoors during daytime. Can kill you!
- **Holy ground burns**: entering a church costs 5 HP and blocks normal services
- **-10 chivalry** immediately on infection
- **NPCs react with fear**

**Cure vampirism:**
- **Church**: the priest offers a painful exorcism (50 gold, costs 20 HP)
- **Merlin** (Glastonbury): cures you for free when you visit
- **Purify spell**: removes the curse

Vampirism and lycanthropy are **mutually exclusive** -- you can only have one curse at a time. Curing removes the stat bonuses.

**Strategy**: Vampirism is powerful for dungeon crawling (dark vision + HP drain + high stats) but dangerous on the overworld (sunlight). Plan your travel for night-time, and carry a cure option in case you need to visit a church.

### Items & Inventory
Press **`i`** to open your inventory.

**Equipment slots** (9 total):
| Slot | Item Types |
|------|-----------|
| Weapon | Swords, axes, maces, staves |
| Armor | Body armor (robes to plate) |
| Shield | Bucklers to tower shields |
| Helmet | Caps and helms |
| Boots | Leather or iron boots |
| Gloves | Gloves and gauntlets |
| Ring 1 | Magical rings |
| Ring 2 | Magical rings |
| Amulet | Magical amulets |

Select an item with `a`-`z`, then: **`e`** equip, **`d`** drop, **`u`** use (potions/food/spell scrolls).
Press **`g`** in dungeons to pick up items on the ground. Gold goes straight to your wallet.

#### Potions (Unidentified)
Potions are **unidentified** when first found. They appear as coloured liquids ("Bubbling Red Potion", "Murky Green Potion", etc.) instead of their true names. The colour-to-effect mapping is **randomised each game**, so a red potion might be healing in one game and poison in another.

**How to identify potions:**
- **Drink it** (`u` in inventory) -- risky but free. The potion is identified after drinking: "You identify it as: Healing Potion"
- **Church cure** (`u` at church for 30 gold) -- if you drank something bad, the priest can help

Once identified, **all potions of that type** show their true name for the rest of the game.

**Potion types** (22 total): Healing, Greater Healing, Superior Healing, Mana, Greater Mana, Superior Mana, Strength (+STR), Speed (+SPD), Defence (+DEF), Intelligence (+INT), Poison Antidote, Fire Resist, Frost Resist, Invisibility, Berserk, Clarity, Fortitude, Luck, Night Vision, Regeneration, Elixir of Life, Holy Water.

#### Cooking
Press **`K`** (Shift+K) on the overworld to cook raw food at a campfire. Requirements:
- **Campable terrain** (grassland, road, or forest)
- **Torch** or **Tinderbox** in your inventory
- **Raw food** (Raw Meat, Raw Boar Meat, Raw Venison -- dropped by beasts)

Cooking takes 20 minutes and transforms raw food into **Cooked Meat** with **3x the healing power** (e.g. Raw Meat power 5 becomes Cooked Meat power 15). Always cook your meat before eating!

#### Fish Spoilage
Fish (Fresh Fish, Salted Fish, Smoked Salmon) goes **rotten after 2 days**. Rotten fish shows in red as "(ROTTEN!)" in your inventory. Eating rotten fish causes **5-12 HP damage** and **-1 STR** (poison). Eat or sell your fish quickly, or visit a church to cure the poison (30 gold).

#### Gems & Treasures
Gems and treasures are found in dungeons and cannot be equipped -- they exist purely as valuable loot. Sell them at the **Pawnbroker** for gold.

**Gems** (`*`): Rough Quartz (15g) to Star Sapphire (300g). 20 types, rarer gems found deeper.

**Treasures** (`&`): Unique collectibles like Golden Teddy Bear (150g), Crystal Skull (250g), Unicorn Horn (350g), Mermaid's Tear (200g), Jewelled Crown (200g), and more. 20 types.

#### Rings & Amulets
Magical rings and amulets enhance your stats when equipped. Each has a **power bonus** (+1 to +4) and a **minimum level requirement** (the `min_depth` value in the data). Higher-level items are rarer and more powerful.

**Rings** (`o`, equip in Ring 1 or Ring 2): 25 types
- Basic: Ring of Protection, Ring of Strength, Ring of Defence, Ring of Speed, Ring of Intelligence (+1 each, level 1-2)
- Advanced: Ring of Fortitude, Ring of Might, Ring of Agility, Ring of Wisdom (+2, level 4-5)
- Legendary: Ring of the Bear, Ring of the Eagle, Ring of the Serpent, Ring of the Wolf (+3, level 7)
- Unique: Ring of the Grail (+4, level 10)
- Resist rings: Fire Resist, Frost Resist, Poison Resist (+2, level 3)
- Special: Ring of Regeneration, Ring of Mana, Ring of Shadows

**Amulets** (`"`, equip in Amulet slot): 25 types
- Basic: Amulet of Warding, Vigour, Grace, Insight (+1, level 1-2)
- Advanced: Amulet of Valour, Endurance, the Arcane, the Hunter (+2, level 4-5)
- Legendary: Amulet of the Crusader, the Mage, the Dragon (+3, level 7-8)
- Unique: Amulet of the Lake (+4, level 10), Amulet of Eternity (+4, level 12)

### Shops
Bump into shop NPCs to buy/sell:
- **Blacksmith** `$` (yellow): weapons and armor (6-12 items)
- **Apothecary** `!` (magenta): potions and scrolls (6-12 items)
- **Baker** `%` (brown): food items (3-6 items)
- **Jeweller** `*` (bright cyan): rings, amulets, and gems (4-10 items)
- **Pawnbroker** `P` (green): buys/sells anything including gems and treasures (8-16 items)

Stock is randomised each visit. Not every town has every shop -- major towns (Camelot, London, York, Winchester) have the most services. Jewellers are found in larger towns and select castles.

### Chivalry
Your chivalry score (0-100) determines how NPCs treat you and grants a title:

| Score | Title |
|-------|-------|
| 0-15 | Knave |
| 16-30 | Squire |
| 31-50 | Knight |
| 51-70 | Noble Knight |
| 71-85 | Champion |
| 86-100 | Paragon of Virtue |

Your title is displayed in town alongside your chivalry score.

Raise chivalry by: donating at churches, confession, visiting Holy Island, completing quests, curing at church (+1).
Lower chivalry by: looting churches (-12), desecrating magic circles (-15), stealing from shops (-8), getting caught stealing (-5), accepting dark bargains (-3).

### Bath Hot Springs
Inside the town of **Bath**, bump into the **Hot Springs** (`~` cyan) building to visit the Roman baths. Press **`b`** to bathe:
- **Full HP and MP** restored
- **Cures lycanthropy** and **vampirism** (removes stat bonuses)
- **Restores debuffed stats** (any stat below 5 is raised to 5)
- Free -- no gold cost

Bath is the only free cure for both curses in the game. Worth the trip if you've been bitten.

### Abbeys
Eight historical abbeys (`A` white) across England:
- Westminster Abbey, Whitby Abbey, Rievaulx Abbey, Bath Abbey, St Mary's Abbey, Cleeve Abbey, Mount Grace Priory, Camelot Abbey
- Each has an **inn with extra strong beer** (4g/pint), a **potion shop**, and a **church**
- Populated by **monks** or **nuns** (never both)
- Always open -- no night lockout

### Landmarks
Press Enter on a landmark to interact:
- **Stonehenge**: +1 to a random stat (once per game)
- **Faerie Ring**: random effect -- stat boost, gold, MP restore, stat swap, teleport. Rare chance of the **Faerie Queen** (once per game): offers a pact granting +10 Nature affinity, a Faerie Blade (power 10), and a Nature spell
- **Loch Ness**: encounter with **Nessie**, the legendary sea monster. Fight her (6 rounds, very tough) for Nessie Scale Mail (DEF +7) and 200 XP, or offer her a fish for 100-250 gold and safe passage. One-time encounter
- **Avalon**: full HP/MP restore
- **Holy Island**: +2 chivalry, full HP restore
- **Lady of the Lake**: offers Excalibur (see Famous Characters below)
- **Magic Circles**: random effect -- healing, mana, blessing, gold, teleport, or fire/drain. Renew after 10-30 days. You can also choose to **loot** a circle (`l`) for 80-200 gold, but at -15 chivalry and a 40% chance of a stat curse. Desecrating a sacred site is never without consequence
- **Rainbow** (`=` cycling colours): appears after rain. Race to its end for 100-500 gold!

### Famous Characters

Legendary figures from Arthurian myth appear at fixed locations. Bump into them to interact.

#### King Arthur (Camelot Castle)
Your first visit to Arthur is the most important event in the game. He grants you:
- **The Grail Quest** -- the main quest to find the Holy Grail
- **Knighthood** -- +2 DEF, +1 STR, +5 chivalry
- A narrative cutscene with dialogue

After you've completed **3 or more side quests**, return to Arthur to receive a **Bastard Sword** as a reward for your service.

#### Queen Guinevere (Camelot Castle)
Arthur's queen responds to your chivalry:
- **Chivalry 50+**: warm greeting. If your Light affinity is 15+, she gives you an **Amulet of Insight**
- **Below 50**: cool dismissal -- prove yourself first

#### Merlin (Glastonbury)
The wizard Merlin replaces the regular Mystic in Glastonbury. When you visit him:
- He **teaches you a spell** you don't know (if your spellbook has room)
- If your spellbook is full, he instead grants **+1 INT** and **+3 max MP**
- He gives a **hint about the Grail's location** in Glastonbury Tor
- Visiting Merlin grants **+3 Light affinity**

#### Morgan le Fay (Cornwall)
The sorceress Morgan replaces the regular Mystic in Cornwall. She offers a **dark bargain**:
- A powerful permanent stat boost (+3 STR, +3 INT, or +3 SPD)
- At a cost: permanent max HP reduction (-8 to -10)
- Accepting grants +5 Dark affinity but costs -3 chivalry
- You can **decline** with no penalty

This is a genuine dilemma -- the stat boost is huge but losing max HP is permanent. Choose wisely.

#### Lady of the Lake (overworld landmark)
A mystical figure at a lake who offers **Excalibur**, the most powerful weapon in the game (power 14, +2 STR). To receive it you must prove worthy:
- **Chivalry 40+** AND
- **5 completed quests** OR **Light affinity 15+**

If you're not yet worthy, she tells you to return later. Excalibur is a one-time gift.

#### Other Kings (castles)
Kings in other castles react based on your chivalry:
- **60+**: "Your fame precedes you!" -- warm welcome
- **30-59**: neutral greeting
- **Below 30**: "I have heard troubling things about you..." -- suspicion

### Special Overworld Encounters

As you explore, rare events trigger based on your location, time, and luck:

#### Travellers
Bump into travellers (`@` white) on roads. Most share rumours and tips, but beware:
- **75%**: friendly dialogue -- tips about towns, dungeons, and dangers
- **10%**: **gives you an item** -- "I found this on the road. No use to me." (food, potion, or tool)
- **10%**: **steals gold** -- pickpockets 3-15 gold from your purse and vanishes
- **5%**: **steals an item** -- lifts a random item from your pack and disappears

#### Travelling Kings
Kings from various castles ride the overworld roads as `K` (yellow bold). Bump into them for dialogue. High chivalry (50+) earns respect and a 30% chance of a gold gift. They appear approximately every 50 turns.

#### Breunis sans Pitie (Recurring Villain)
A merciless knight who ambushes you on roads and grassland (~1 per 300 turns, 20% chance). A tough multi-round fight that gets **harder each time** -- he gains +2 STR, +1 DEF, +5 HP per defeat. First victory drops the **Dark Blade of Breunis** (power 11, unique weapon). He always comes back for more.

#### Mad Knights
Rare encounters on roads (~1 per 400 turns). Unpredictable -- 50/50 aggressive or passive:
- **Aggressive**: attacks immediately (5-12 HP damage), drops gold on death
- **Passive**: wanders off muttering, sometimes drops gold
- **Cursed** (10% chance): if you have **Holy Water** or the **Purify spell**, you can cure them for **+5 chivalry**, a Longsword, and +30 XP. A major reward for mercy

#### Damsel in Distress
Random rescue events (~1 per 350 turns). Fight off attackers to save her. Rewards vary:
- Gold (20-60g)
- A kiss (full HP/MP restore)
- A lucky charm (+1 STR or +1 INT)
- All rescues grant **+5 chivalry**

#### Witch Encounters & Geas
In swamps, forests, and marshes (~1 per 500 turns), a witch curses you with a **geas** -- a timed task you must complete or be cursed. Tasks include:
- Bring a gem from a dungeon
- Slay a wolf
- Stand in a stone circle at midnight
- Bring a potion
- Fetch a mushroom from the forest
- Deliver an offering to Canterbury

**Complete in time**: geas lifts, +2 Dark affinity, -2 chivalry (doing a witch's bidding is dishonourable). The required item is consumed.
**Fail**: cursed with -2 to a random stat and -3 chivalry. Cure at a church or with Purify spell.

The witch's geas is always a dilemma -- complete it (small chivalry loss) or refuse (bigger loss + curse).

#### Wood Nymph
A mischievous forest spirit (~1 per 250 turns in forests). She cannot be attacked -- she vanishes if you try.
- **90% chance**: she **steals a random item** from your inventory. "A wood nymph snatches your Longsword and vanishes!" The item is gone permanently
- **10% chance**: she gives you a **Nymph's Kiss** instead -- +2 SPD for 100 turns
- **SPD check**: if your SPD is above 6, you have a 30% chance to dodge the theft

Travel through forests quickly or keep your SPD high to avoid losing precious items.

### Jousting Tournaments
Tournaments are held **4 days per 30-day cycle** at each castle, on different days per castle. When a tournament is on, you'll see "A jousting tournament today!" on entry. Bump into a **guard** (`G`) to enter. When no tournament is on, the guard tells you how many days until the next one.
- **Entry fee**: 20-50 gold (varies each time)
- **Requires a horse** -- Destrier gives a +8 jousting hit bonus (on top of its +2 DEF in regular combat)
- **One entry per tournament** -- you can't joust again until the next tournament day
- **3 rounds** against progressively tougher opponents:

| Round | Opponent | Prize |
|-------|----------|-------|
| 1 | Local Squire | 30 gold, 10 XP |
| 2 | Rival Knight | 80 gold, 20 XP |
| 3 | Castle Champion | 200 gold, 30 XP, +5 chivalry |

**How jousting works:**
1. Choose your aim: **`h`** head (risky, instant KO but easy to miss), **`c`** chest (balanced), **`s`** shield (safe, low damage)
2. Watch the charge animation -- two knights ride toward each other
3. Press **`Space`** at the right moment as the knights meet:
   - **Perfect timing** (hit within 1 tick of centre): automatic win, "PERFECT TIMING!"
   - **Good timing** (within 3 ticks): solid hit, likely win
   - **Poor/missed**: likely unhorsed

**Losing**: take 3-10 HP damage, -1 chivalry, eliminated from the tournament. Can retry next visit.

**Winning all 3 rounds**: "TOURNAMENT CHAMPION!" -- a prestigious achievement.

### Castle Cat
Bump into a cat (`c` yellow) in any castle to pick it up as a pet:
- **+1 SPD** while the cat follows you
- **No rat encounters** spawn while you have the cat
- The cat wanders off after **100-300 turns** (because cats)
- **30% chance** the cat leaves a gift before departing: a shiny coin (5g), a mouse (Dried Meat), or a hairball (nothing useful)
- Only one cat at a time

### Cheat Mode
On the stats screen during character creation, press **`C`** to open the cheat menu:
- **God Mode**: HP/MP set to 999
- **Rich Start**: Gold set to 9999
- **Max Stats**: all stats set to 15
- **All of the above**

Cheats permanently mark your character -- score is always 0 and the run is flagged as CHEAT.

### Beer & Drunkenness
Buy beer at any inn (2-4g per pint). Each pint increases drunkenness:
1. Merry -- no effect
2. Tipsy -- 10% chance of stumbling (random movement)
3. Drunk -- 25% stumble, -1 INT, +1 STR
4. Very drunk -- 40% stumble, -2 INT, -1 SPD, +2 STR
5. Pass out -- wake up 8 hours later with 50% HP

### Resting
Press **`R`** in a dungeon to rest and recover HP/MP:
- Recovers 1 HP every 5 turns, 1 MP every 8 turns
- Rests until full HP or 100 turns max
- **Interrupted** if a monster reaches an adjacent tile

### The Holy Grail (Main Quest)

The main quest of the game -- find the Holy Grail and return it to King Arthur.

**Quest flow:**
1. **Visit King Arthur** at Camelot Castle. He grants the quest, knights you (+2 DEF, +1 STR), and the Grail is placed in a **random dungeon** each playthrough
2. **Gather hints**: talk to townfolk (25% chance of a Grail hint when Grail quest active). **70% of hints are true**, 30% are red herrings. Cross-reference multiple hints!
3. **Reliable sources**: **Merlin** (Glastonbury) always gives the correct dungeon name. Trust him
4. **Find the Grail**: reach the **deepest level** of the correct dungeon. **Mordred** guards the Grail (HP 80, STR 20, DEF 14 -- the toughest fight in the game). Defeat him and pick up the Grail (`*` yellow)
5. **Return to Arthur**: bring the Grail to Arthur at Camelot Castle. He requires **chivalry 30+** to accept it. If too low, raise chivalry first (pray, donate, do good deeds)
6. **Rewards**: 5000 gold, +10 chivalry, +500 XP, title "Grail Knight"
7. **Continue playing**: the game does NOT end -- keep exploring, completing quests, and building your character

### Dungeon Bosses

Every dungeon has a **boss** on its deepest level:

| Dungeon | Boss | Special |
|---------|------|---------|
| Camelot Catacombs | Dark Monk | Heals allies |
| Tintagel Caves | Black Knight | Tough melee fighter |
| Sherwood Depths | Evil Sorcerer | Ranged attacks, summons |
| Mount Draig | Red Dragon | Breath weapon (fire) |
| Glastonbury Tor | Dark Templar | Fear aura |
| Whitby Abbey | Vampire Lord | Debuffs |
| White Cliffs Cave | Sea Serpent King | Heavy hitter |
| Avalon Shrine | Guardian Spirit | Walks through walls |
| Orkney Barrows | Barrow King | Summons undead |
| Castle Dolorous Garde | Ghost Knight | Ghost |
| Castle Perilous | Dark Enchantress | Ranged + debuffs |
| Bamburgh Castle | Bandit King | Tough melee |

If the Grail is in a dungeon, **Mordred** replaces the normal boss as the final guardian.

### Dungeon Chests
Chests (`=` brown or red) appear in dungeon rooms (1-3 per level). Walk onto one to interact:
- **Unlocked** (`=` brown): opens immediately, gives gold + a random item
- **Locked** (`=` red): requires a Lockpick, Ranger skill (automatic), or bashing (STR check, 30% chance to destroy contents)
- **Trapped** (20%): triggers a poison dart, explosion, or gas cloud on opening
- **Mimic** (5%): the chest is actually a monster! Tough fight, drops gold on defeat

Chest loot scales with dungeon depth -- deeper chests contain better items.

### Travelling Kings
Kings from various castles travel the overworld roads as `K` (yellow bold). Bump into them for dialogue -- high chivalry (50+) may earn you a gold purse. They appear approximately every 50 turns.

### Rescue the Princess
At any castle, a king with high regard for you (chivalry 50+) may ask you to rescue his kidnapped daughter from an Evil Sorcerer's tower:
1. **Accept the quest**: a **Sorcerer's Tower** (`|` magenta) appears in the hills
2. **Clear the tower**: 3-5 levels of magical enemies, with the Evil Sorcerer as the boss
3. **Free the princess**: defeating the Evil Sorcerer rescues her
4. **Claim your reward**: visit any castle king afterwards
   - **Accept marriage**: the **TRUE ENDING** -- become King/Queen, +10000 score
   - **Decline**: +5 chivalry, +500 gold, continue playing

The princess quest and the Grail quest are independent -- you can complete both, one, or neither.

### Title Screen
When you launch the game, you see the title screen with an ASCII art banner and menu:
- **[C] Continue** -- resume from your saved game (only shown if a save exists)
- **[N] New Game** -- start a new character
- **[H] High Scores** -- view the top 10 scores across all playthroughs
- **[F] Fallen Heroes** -- view all your previous dead characters
- **[Q] Quit**

### Saving & Permadeath
Press **`S`** on the overworld to save your game to `~/.camelot/save.dat`. Only one save slot exists -- no save scumming!

**Permadeath**: when you die, your save is **deleted**. Your character is gone forever. But your legacy lives on:
- Your **score** is added to the high score table
- Your character is recorded in the **Fallen Heroes** list
- Both persist across all future playthroughs

### Death Screen
When you die, a full-screen summary shows:
- Character name, class, chivalry title
- Cause of death (e.g. "Slain by Red Dragon")
- Final stats (level, STR, DEF, INT, SPD)
- Journey summary (turns, kills, gold earned, quests completed, spells learned)
- Whether you found the Holy Grail (+5000 score bonus)
- **Final score**

### Scoring
Your score is calculated as:

```
Score = (kills * 10) + gold_earned + (quests_completed * 150)
Score = Score * chivalry_multiplier
If Grail found: Score + 5000
If cheat mode: Score = 0
```

**Chivalry multiplier**: 0.5x at chivalry 0, 1.0x at 50, up to 2.0x at 100. High chivalry rewards honourable play.

### High Scores
Top 10 scores saved to `~/.camelot/scores.dat`. Each entry shows name, class, score, cause of death, and whether the Grail was found. Viewable from the title screen.

### Fallen Heroes
Up to 50 fallen characters recorded in `~/.camelot/fallen_heroes.dat`. Shows name, level, class, score, and cause of death for each. Most recent death listed first. "X brave souls have perished in the quest for the Grail."

### Canterbury Graveyard
In **Canterbury**, bump into the **Graveyard** (`+` gray) building to visit. Gravestones show all your previous fallen characters from past games:
```
+---------------------------+
| Here lies Sir Galahad      |
| Knight     Level 7         |
| Slain by a Red Dragon      |
+---------------------------+
```
Visiting grants **+1 chivalry** (paying respects to the fallen). Data persists in `~/.camelot/fallen_heroes.dat` across all playthroughs.

### Player Home (Camelot)
Visit **Camelot** town and press **`V`** to enter your home. Your home has:
- **Storage chest**: unlimited capacity, items **persist across deaths** in `~/.camelot/home_chest.dat`
  - `s` to stash an item from your inventory into the chest
  - `t` to take an item from the chest into your inventory
  - Items are tagged with who stored them and when: "Longsword (left by Sir Galahad, day 12)"
- **Bed**: free rest until morning (full HP/MP, advance to 7:00)
  - `r` to rest

This creates a **meta-progression loop**: stash good weapons, rings, and potions for future characters. Combined with the bank (cross-game gold persistence -- not yet implemented), your legacy grows with each playthrough.

### In-Game Help
Press **`?`** at any time to open the help system. 4 pages, navigate with `<` `>` or arrow keys:

1. **Commands** -- all keybindings for movement, overworld, and dungeon
2. **UI & Items** -- character sheet, inventory, shops, spellbook, quest journal
3. **Symbols** -- legend of all ASCII characters (NPCs, terrain, items, overworld)
4. **Tips** -- gameplay advice for new players

Press `q` to close.

### Morgue Files
When you die, a detailed text file is saved to `~/.camelot/morgue/[name]_day[N].txt` containing:
- Character name, class, level, cause of death
- Final stats, chivalry, equipment, inventory
- Quests completed, spells known, kills, score
- Game seed (for challenge runs or sharing)

Morgue files persist indefinitely and can be shared for comparing runs.

### Settings
Press **`=`** to view game settings and info:
- Game version
- Game seed (for sharing challenge runs)
- File locations (save, morgue, scores, home chest)
- Terminal size

### Debug Mode
Launch with `./camelot -d` to enable debug mode:
- **`]`** in dungeons: skip to next level
- **`[`** in dungeons: reveal entire map
- Score is always 0 (debug implies cheat mode)

### Funny Messages
10% of combat messages are replaced with humorous alternatives. Death screens include a random epitaph. Keep your eyes open for easter eggs!

## Building

```bash
make              # Build the game
./camelot         # Run the game
./camelot -s 123  # Start with a specific seed
./camelot -d      # Debug mode (full map, level skip)
make clean        # Clean build files
```

Requires: C compiler (clang/gcc), ncurses library.

## Data Files

All game content is defined in CSV files under `data/` -- edit these to rebalance the game, add new content, or create mods without recompiling:

| File | Contents |
|------|----------|
| `data/monsters.csv` | 85+ monster types with stats, AI flags, drops |
| `data/items.csv` | 280+ items: weapons, armor, rings, amulets, gems, treasures, potions, food, scrolls, spell scrolls, tools |
| `data/spells.csv` | 50 spells across Light, Dark, Nature, Universal schools |
| `data/quests.csv` | 15 side quests (delivery, fetch, kill) |
| `data/towns.csv` | 43 towns, castles, and abbeys with services |
| `data/locations.csv` | 70+ overworld locations with coordinates |
| `data/creatures.csv` | 12 overworld creature types |
| `data/names.csv` | 48 period-appropriate random names |

## License

See [LICENSE](LICENSE) for details.
