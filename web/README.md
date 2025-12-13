# OSRS DPS Calculator - Web UI

A web-based DPS calculator for Old School RuneScape, powered by WebAssembly.

## Features

- **Player Stats**: Fetch stats from OSRS hiscores or enter manually
- **WikiSync Support**: Sync gear from RuneLite's WikiSync plugin
- **Equipment Editor**: Visual equipment interface with item search
- **Monster Database**: Search and select from all OSRS monsters and bosses
- **DPS Calculator**: Accurate DPS calculations using OSRS formulas
- **Battle Simulator**: Monte Carlo simulation for kill times
- **Upgrade Advisor**: Find cost-effective gear upgrades with **Duo Optimization**

## Special Weapon Support

The calculator includes special effects for:
- **Osmumten's Fang**: Double accuracy roll on stab, damage clamping (15-85%)
- **Dragon Hunter Lance**: +20% accuracy and damage vs dragons
- **Salve Amulet Variants**: (e), (i), (ei) bonuses vs undead monsters

## Duo Optimization

The upgrade advisor now tests **pairs of items** that synergize together, finding combinations that provide more DPS than either item alone. Duo suggestions are marked with a blue "DUO" badge.

## Prerequisites

1. **Emscripten SDK**: Install from https://emscripten.org/docs/getting_started/downloads.html

```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

## Building

1. Navigate to the project root directory (where `Makefile.emscripten` is located)

2. Build the WebAssembly module:

```bash
emmake make -f Makefile.emscripten
```

3. Copy the JSON data files to the web directory:

```bash
make -f Makefile.emscripten copy-data
```

Or manually:

```bash
cp items-complete.json web/data/
cp monsters-nodrops.json web/data/
cp bosses_complete.json web/data/
cp latest_prices.json web/data/
```

## Running

Serve the `web/` directory with any HTTP server. For example:

```bash
# Python 3
cd web
python -m http.server 8080

# Node.js (npx)
npx serve web

# Or use VS Code Live Server extension
```

Then open http://localhost:8080 in your browser.

## Usage

1. **Set Player Stats**: 
   - Enter your RSN and click "Fetch Stats" to load from hiscores
   - Or manually adjust the combat stat inputs
   - Or click "Sync from WikiSync" if RuneLite is running with WikiSync plugin

2. **Equip Gear**:
   - Click on any equipment slot to open the item selector
   - Search for items by name
   - Selected items will be equipped and bonuses updated

3. **Select Monster**:
   - Use the monster search to find your target
   - Monster stats will be displayed
   - Dragon/Undead attributes are automatically detected for special weapon bonuses

4. **Calculate DPS**:
   - Click "Calculate DPS" to see your damage output
   - Results include max hit, accuracy, attack style, and more
   - Special effects (Fang, DHL, Salve) are automatically applied

5. **Simulate Battles**:
   - Set simulation count (default 1000)
   - Click "Run Simulation" for average kill time estimates

6. **Find Upgrades**:
   - Set your budget
   - Click "Find Upgrades" to see cost-effective improvements
   - Toggle between "Most Efficient" and "Highest DPS" views
   - Look for "DUO" badges for item pair suggestions

## Troubleshooting

### WASM module not loading
- Make sure you've compiled with Emscripten: `emmake make -f Makefile.emscripten`
- Check browser console for errors
- Ensure you're serving via HTTP (not file://)

### Data files not loading
- Run `make -f Makefile.emscripten copy-data` to copy JSON files
- Check that files exist in `web/data/`

### Hiscores fetch failing
- The app uses a CORS proxy which may have rate limits
- Try again after a few seconds
- Player name must be exact (case-insensitive)

### WikiSync not connecting
- Ensure RuneLite is running with WikiSync plugin enabled
- The plugin uses ports 37767-37776

## Development

For a debug build with source maps:

```bash
emmake make -f Makefile.emscripten debug
```

## License

MIT License
