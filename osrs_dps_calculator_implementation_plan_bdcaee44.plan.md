# OSRS DPS Calculator Implementation Plan

This plan addresses the requirements to move WikiSync logic to the Player class, implement gear stats, and create a Battle simulator.

## 1. Refactor Item Class (`item.h`, `item.cpp`)

-   **Goal**: Add ID-based lookup functionality while preserving existing name-based lookup.
-   **Changes**:
    -   Add `id` attribute to `Item` class (initialize to -1).
    -   Add constructor `Item(int id)`.
    -   Modify `fetchStats(const std::string& filepath)`:
        -   If `id` is valid (> -1): Look up item directly in JSON using the ID key (O(1)).
        -   If `id` is invalid but `name` is set: Iterate JSON to find matching name (O(N)) (preserve existing behavior).
    -   Ensure methods exist to retrieve specific combat stats (Slash Bonus, Melee Strength, etc.).

## 2. Refactor Player Class (`player.h`, `player.cpp`)

-   **Goal**: Integrate WikiSync networking and gear management to modify player stats.
-   **Changes**:
    -   **Networking**: Move `run_wikisync_client` logic from `main.cpp` to a new method `Player::fetchGearFromClient()`.
    -   **Gear Management**: Add a `std::map<std::string, Item> gear` member to `Player`.
    -   **Data Parsing**: Implement `loadGearStats(const std::string& itemDbPath)`:
        1.  Read `wikisync_data.json` (saved by the networking step).
        2.  Parse equipped item IDs from the JSON.
        3.  Load `items-complete.json` once.
        4.  For each slot (head, body, legs, etc.):
            -   Create an `Item(id)`.
            -   Call `item.fetchStats()` (using the ID lookup).
            -   Store in `gear` map.
    -   **Bonuses**: Add `getEffectiveStat(std::string stat)`:
        -   Start with base skill level (e.g., Strength 99).
        -   Iterate through `gear` and add relevant bonuses (e.g., `item.getInt("strength_bonus")`).
        -   Return total.

## 3. Create Battle Class (`battle.h`, `battle.cpp`)

-   **Goal**: Encapsulate combat formulas and simulation loop.
-   **Structure**:
    -   Constructor: `Battle(Player& p, Monster& m)`.
    -   **Methods** (migrated from `main.cpp`):
        -   `calculateMaxHit()`: Uses `player.getEffectiveStat("Strength")`.
        -   `calculateHitChance()`: Uses `player.getEffectiveStat("Attack")` vs Monster Defence.
        -   `simulate()`: Runs the fight loop (ticks) until monster dies. Returns ticks/time taken.
        -   `runSimulations(int n)`: Runs `n` fights and returns average TTK.

## 4. Main Integration (`main.cpp`)

-   **Goal**: Execute the full flow.
-   **Flow**:
    1.  Initialize `Player`.
    2.  Call `player.fetchGearFromClient()` (fetches IDs).
    3.  Call `player.loadGearStats("items-complete.json")` (converts IDs to Stats).
    4.  Initialize `Monster` (e.g., "Khazard warlord").
    5.  Initialize `Battle` with player and monster.
    6.  Run simulation and print TTK.

## 5. Cleanup

-   Remove the standalone combat functions and WikiSync logic from `main.cpp`.

