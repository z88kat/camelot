# Knights of Camelot

**The Quest for the Holy Grail** -- a terminal-based ASCII roguelike set in Arthurian England.

## About

Knights of Camelot is a full-featured roguelike game written in C using ncurses. You play as a knight, wizard, or ranger on a quest to find the Holy Grail. Explore an overworld map of medieval England, visit towns and castles, delve into dangerous dungeons, and encounter legendary characters from Arthurian myth.

The game features permadeath -- when you die, your character is gone forever. But your legacy lives on: gold stored at the bank, items left at home, and your gravestone in Canterbury cathedral carry forward to future characters.

## Features

### World
- Hand-crafted overworld map of England with 10+ towns, 17 castles (14 active + 3 abandoned), landmarks, and 7+ dungeons
- Towns with equipment shops, potion shops, inns (with beer!), churches, mystics, pawn shops, banks, and wells
- Active castles ruled by Arthurian kings who travel the realm, and abandoned castles to explore
- Landmarks: Stonehenge, Hadrian's Wall, White Cliffs of Dover, Avalon, Faerie Rings
- Lakes with islands, boats, and the occasional ghost ship or Nessie encounter
- Random cottages, caves with hermits, and overworld encounters
- Day/night cycle with a 30-day lunar calendar and recurring events (feasts, tournaments, holy days)
- Weather system: rain, storms, fog, snow, wind -- and rainbows after the rain

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
| Symbol | Colour | Terrain | Passable | Notes |
|--------|--------|---------|----------|-------|
| `"` | Green | Grassland | Yes | Normal speed |
| `.` | Yellow | Road | Yes | Fast travel, safest route |
| `T` | Bright green | Forest | Yes | Slow, nature encounters, reduced visibility |
| `^` | White | Hills | Yes | Slow, increased encounter chance |
| `A` | Bright white | Mountains | **No** | Impassable -- go around or find a pass |
| `,` | Green | Marsh | Yes | Very slow, chance of getting stuck |
| `%` | Green | Swamp | Yes | Very slow, poison damage risk |
| `~` | Blue | Sea | **No** | Impassable |
| `=` | Blue | River | **No** | Impassable -- find a bridge (`H`) to cross |
| `o` | Blue | Lake | **No** | Impassable -- use boat (`B`) or Walk on Water |
| `H` | Brown | Bridge | Yes | Road speed, the only way to cross rivers |
| `B` | Brown | Boat | Yes | Step on to sail to an island |

### Overworld Locations
| Symbol | Colour | Location Type |
|--------|--------|---------------|
| `*` | Various | Town (enter to visit shops, inns, etc.) |
| `#` | White/yellow | Castle (active or abandoned) |
| `+` | Various | Landmark (Stonehenge, Hadrian's Wall, etc.) |
| `>` | White | Dungeon entrance (press `>` to descend) |
| `V` | Red | Volcano (Mount Draig -- dungeon entrance) |
| `@` | Bright white | You (the player) |

### Dungeon Terrain
| Symbol | Colour | Terrain | Notes |
|--------|--------|---------|-------|
| `#` | Gray | Wall | Impassable, blocks line of sight |
| `.` | White | Floor | Standard walkable tile |
| `+` | Brown | Closed door | Passable, blocks sight. `o` to open |
| `/` | Brown | Open door | Passable, allows sight |
| `>` | White | Stairs down | Press `>` to descend |
| `<` | White | Stairs up | Press `<` to ascend |
| `=` | Brown | Chest | May be locked or trapped |

## Keyboard Commands

### Movement
| Key | Action |
|-----|--------|
| `h` `j` `k` `l` `y` `u` `b` `n` | Move (vi-keys, 8-directional) |
| Arrow keys | Move (4-directional) |
| Numpad 1-9 | Move (8-directional) |
| Shift + direction | Auto-run until interrupted |
| `.` or `5` | Wait one turn |
| `>` | Descend stairs |
| `<` | Ascend stairs |

### Actions
| Key | Action |
|-----|--------|
| `g` | Pick up item |
| `i` | Open inventory |
| `z` | Cast a spell |
| `f` | Fire ranged weapon |
| `t` | Throw an item |
| `a` | Apply/use item on another (e.g., pour holy water) |
| `o` | Open door |
| `c` | Close door |
| `s` | Search adjacent walls for secret doors |
| `D` | Disarm trap |
| `x` | Dig (treasure maps) |
| `d` | Dismount horse |
| `p` | Play instrument (Tristan's Harp) |
| `J` | Quest journal |
| `S` | Save game |

### UI
| Key | Action |
|-----|--------|
| `?` | Help system |
| `;` | Look/examine mode |
| `@` | Full character sheet |
| `Ctrl+P` | Message history |
| `=` | Settings |
| `q` | Quit game |

## Player Guide

### Getting Started
1. **Create your character**: choose class, gender, name, appearance, and roll your stats. Press `r` to re-roll stats until you're happy, then Enter to accept. Press `C` for cheat options (score will be 0).
2. **Visit King Arthur**: your first task is to find Arthur in Camelot Castle's throne room. He'll grant you the quest for the Holy Grail and make you a knight.
3. **Explore Camelot**: visit the shops to buy starting gear, the inn for food and quests, and the church for blessings. Don't forget to check your home for items left by previous characters!

### Survival Tips
- **Eat regularly**: hunger drains your stats and eventually kills you. Buy food, pick apples from trees, and cook raw meat at campfires (you'll need a tinderbox).
- **Watch your weight**: heavy armour and hoarded loot slow you down. Sell gems at pawn shops, stash surplus at home.
- **Identify potions carefully**: drinking unknown potions is risky. Pay potion shops to identify them, or use the Identify spell.
- **Save often**: press `S` to save. There's only one save slot and death is permanent.
- **Bank your gold**: deposit gold at banks in major cities. It survives death for your next character.

### Combat Advice
- **Know when to run**: you can always try to flee (30%+ chance). Don't fight enemies you can't beat.
- **Use terrain**: doorways create chokepoints. Lure enemies into corridors for one-on-one fights.
- **Rangers detect traps**: playing Ranger gives passive trap detection and lockpicking.
- **Wizards are fragile**: rely on Shield spell and ranged magic. The Mana Shield ability at level 10 is a lifesaver.
- **Silver weapons hurt werewolves**: keep a Silver Dagger handy for nighttime travel.

### Chivalry
Your chivalry score (0-100) determines how NPCs treat you:
- **Below 15**: Camelot gates are closed to you
- **Below 30**: King Arthur won't speak with you
- **40+**: Lady of the Lake may offer Excalibur
- **50+**: eligible for the Princess quest
- **60+**: best prices and quests at castles
- **70+**: The Siege Perilous legendary artifact won't harm you

Raise chivalry by: rescuing damsels, completing quests, donating at churches, praying at shrines.
Lower chivalry by: attacking innocents, stealing, looting shrines, witch tasks, killing priests.

### The Holy Grail
The Grail is hidden in a random dungeon each game. Gather hints from inn NPCs (beware -- some are red herrings!). Merlin and King Pellam always give truthful hints. The Carbonek shrine reveals the exact location. Defeat Mordred to claim it, then return it to Arthur (chivalry 30+ required).

### The True Ending
Rescue the princess from the Evil Sorcerer's tower, then visit her at her father's castle. Accept her marriage proposal for the game's true ending -- you become King/Queen with a +10000 score bonus.

## Building

```bash
make          # Build the game
./camelot     # Run the game
make clean    # Clean build files
```

Requires: C compiler (clang/gcc), ncurses library, 256-colour terminal recommended.

## Data Files

All game content (monsters, items, spells, quests, etc.) is defined in CSV files under `data/`. Edit these to rebalance the game, add new content, or create mods -- no recompilation needed.

## License

See [LICENSE](LICENSE) for details.
