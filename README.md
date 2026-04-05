# Knights of Camelot

**The Quest for the Holy Grail** -- a terminal-based ASCII roguelike set in Arthurian England.

## About

Knights of Camelot is a full-featured roguelike game written in C using ncurses. You play as a knight, wizard, or ranger on a quest to find the Holy Grail. Explore an overworld map of medieval England, visit towns and castles, delve into dangerous dungeons, and encounter legendary characters from Arthurian myth.

## Current State

**Implemented (Phases 1-11):**
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
- 150+ items: weapons, armor, potions, food, scrolls, tools, 25 rings, 25 amulets, 20 gems, 20 treasures
- Weight-based inventory with encumbrance and 9 equipment slots (including amulet)
- Quest system with 15 side quests
- Leveling system with 20 levels and class-based HP/MP progression
- Chivalry system (0-100) with titles from Knave to Paragon of Virtue
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
| `@` | White | Traveller | Walks roads, shares tips |
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

Each level is 160x80 tiles (scrollable), generated with BSP rooms and corridors. Levels are persistent -- items you drop stay where you left them.

## Keyboard Commands

### Character Creation
| Key | Action |
|-----|--------|
| `1` `2` `3` | Select class (Knight/Wizard/Ranger) or gender (Male/Female) |
| `r` | Random name / randomise appearance / re-roll stats and gold |
| `1`-`4` | Cycle appearance options (hair, eyes, build, feature) |
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
| `>` | Descend into a dungeon entrance |
| `c` | Camp (rest 8 hours, restore 50% HP/MP) |
| `H` | Mount/dismount horse (if owned) |
| `z` | Cast a spell |
| `i` | Inventory |
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

#### Shops (Blacksmith, Apothecary, Baker, Jeweller, Pawnbroker)
| Key | Action |
|-----|--------|
| `a`-`z` | Buy an item from the list |
| `S` | Switch to sell mode (sell from your inventory) |
| `q` | Leave the shop |

Stock is randomised each visit:
- **Blacksmith**: 6-12 weapons and armor
- **Apothecary**: 6-12 potions and scrolls
- **Baker**: 3-6 food items
- **Jeweller**: 4-10 rings, amulets, and gems
- **Pawnbroker**: 8-16 items of any type

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

#### Stable
| Key | Action |
|-----|--------|
| `b` | Buy a horse (150 gold, one-time purchase) |

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
| `i` | Open inventory |
| `z` | Cast a spell |
| `;` | Look/identify mode -- examine tiles, monsters, items |
| `o` + direction | Open a door |
| `c` + direction | Close a door |
| `s` | Search adjacent walls for secret doors |
| `D` | Disarm an adjacent revealed trap (INT+SPD check) |
| `R` | Rest to recover HP/MP (interrupted if monster approaches) |
| `M` | Dungeon minimap (full level overview) |
| `T` | Toggle torch on/off (light radius 10 vs 2) |
| `q` | Quit game |

### UI (all modes)
| Key | Action |
|-----|--------|
| `i` | Inventory |
| `z` | Cast spell |
| `Z` | View spellbook (all known spells with stats) |
| `J` | Quest journal |
| `M` | Minimap |
| `P` | Message history (scroll with Up/Down, q to close) |
| `q` | Quit game / leave town |

## Player Guide

### Getting Started
1. **Create your character**: choose a class, gender, name, and roll your stats. Starting gold is random (30-200). Each class plays differently:
   - **Knight**: High HP (30), strong melee (+2 STR/DEF), limited magic (4 spells, 8 MP). Starts with Longsword, Chainmail, Shield spell.
   - **Wizard**: Low HP (18), powerful magic (15 spells, 30 MP, +3 INT). Starts with Wooden Staff, Tattered Robes, Magic Missile.
   - **Ranger**: Balanced (24 HP, 6 spells, 15 MP, +2 SPD). Starts with Short Sword, Leather Armor, Detect Traps.
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
During character creation you customise your appearance (hair colour, eye colour, build, and distinguishing feature). These are purely cosmetic and displayed on the character creation screen. Press number keys to cycle options or `r` to randomise.

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
Walk onto a `B` tile to board a boat. Sail across lake water freely. Step onto land to disembark.

### Horses
Buy a horse from any **Stablemaster** (`S`) for **150 gold** (one-time purchase). Once owned:
- Press **`H`** on the overworld to mount or dismount
- **Doubles your travel speed** on all terrain (roads: 5→3 min, grassland: 10→5 min, swamp: 35→18 min)
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

### Combat
- **Bump-to-attack**: walk into an enemy to attack.
- **Hit chance**: 70% base + your STR - enemy DEF*2 (min 15%, max 95%).
- **Critical hits**: 10% chance for double damage.
- **Damage**: your STR + weapon power + random(-2,2) - enemy DEF - armor (minimum 1).
- **Shield spell**: absorbs damage before it hits your HP.
- **Equipped rings and amulets** add their power bonus to your stats.

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

Select an item with `a`-`z`, then: **`e`** equip, **`d`** drop, **`u`** use (potions/food).
Press **`g`** in dungeons to pick up items on the ground. Gold goes straight to your wallet.

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

Raise chivalry by: donating at churches, confession, visiting Holy Island, completing quests.
Lower chivalry by: looting churches (-12), bad choices.

### Abbeys
Eight historical abbeys (`A` white) across England:
- Westminster Abbey, Whitby Abbey, Rievaulx Abbey, Bath Abbey, St Mary's Abbey, Cleeve Abbey, Mount Grace Priory, Camelot Abbey
- Each has an **inn with extra strong beer** (4g/pint), a **potion shop**, and a **church**
- Populated by **monks** or **nuns** (never both)
- Always open -- no night lockout

### Landmarks
Press Enter on a landmark to interact:
- **Stonehenge**: +1 to a random stat (once per game)
- **Faerie Ring**: random effect -- stat boost, gold, teleport
- **Avalon**: full HP/MP restore
- **Holy Island**: +2 chivalry, full HP restore
- **Lady of the Lake**: offers Excalibur (see Famous Characters below)
- **Magic Circles**: random effect -- healing, mana, blessing, gold, teleport, or fire/drain. Renew after 10-30 days
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

## Building

```bash
make          # Build the game
./camelot     # Run the game
make clean    # Clean build files
```

Requires: C compiler (clang/gcc), ncurses library.

## Data Files

All game content is defined in CSV files under `data/` -- edit these to rebalance the game, add new content, or create mods without recompiling:

| File | Contents |
|------|----------|
| `data/monsters.csv` | 85+ monster types with stats, AI flags, drops |
| `data/items.csv` | 150+ items: weapons, armor, rings, amulets, gems, treasures, potions, food, scrolls, tools |
| `data/spells.csv` | 50 spells across Light, Dark, Nature, Universal schools |
| `data/quests.csv` | 15 side quests (delivery, fetch, kill) |
| `data/towns.csv` | 43 towns, castles, and abbeys with services |
| `data/locations.csv` | 70+ overworld locations with coordinates |
| `data/creatures.csv` | 12 overworld creature types |
| `data/names.csv` | 48 period-appropriate random names |

## License

See [LICENSE](LICENSE) for details.
