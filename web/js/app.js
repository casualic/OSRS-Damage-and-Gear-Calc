// app.js - Main application entry point for OSRS DPS Calculator

// Global state
const state = {
    wasmModule: null,
    player: null,
    monster: null,
    itemDb: null,
    monsterDb: null,
    bossDb: null,
    priceDb: null,
    equippedItems: {},
    selectedSlot: null,
    rawSuggestions: [], // Store raw API response
    activeTab: 'efficiency', // Track active sort tab
    ttkChart: null // Chart.js instance
};

// Initialize the application
async function init() {
    try {
        // Load WASM module
        await loadWasmModule();
        
        // Load JSON databases
        await loadDatabases();
        
        // Initialize UI
        initializeUI();
        
        // Create default player
        state.player = new state.wasmModule.Player("Player");
        setDefaultStats();
        
        // Hide loading overlay
        document.getElementById('loading-overlay').classList.add('hidden');
        
        console.log('OSRS DPS Calculator initialized successfully!');
    } catch (error) {
        console.error('Failed to initialize:', error);
        document.getElementById('loading-overlay').innerHTML = `
            <p style="color: #f44336;">Failed to load: ${error.message}</p>
            <p style="color: #a0a0b0; margin-top: 10px;">Make sure the WASM module is compiled.</p>
        `;
    }
}

// Load the WebAssembly module
async function loadWasmModule() {
    if (typeof createOSRSCalc === 'undefined') {
        throw new Error('WASM module not found. Compile with: emmake make -f Makefile.emscripten');
    }
    
    state.wasmModule = await createOSRSCalc();
    console.log('WASM module loaded');
}

// Load JSON databases
async function loadDatabases() {
    const loadJSON = async (path) => {
        const response = await fetch(path);
        if (!response.ok) throw new Error(`Failed to load ${path}`);
        return response.json();
    };

    try {
        // Load all databases in parallel
        const [itemDb, monsterDb, bossDb, priceDb] = await Promise.all([
            loadJSON('data/items-complete.json').catch(() => ({})),
            loadJSON('data/monsters-nodrops.json').catch(() => ({})),
            loadJSON('data/bosses_complete.json').catch(() => []),
            loadJSON('data/latest_prices.json').catch(() => ({ data: {} }))
        ]);

        state.itemDb = itemDb;
        state.monsterDb = monsterDb;
        state.bossDb = bossDb;
        state.priceDb = priceDb;

        console.log(`Loaded: ${Object.keys(itemDb).length} items, ${Object.keys(monsterDb).length} monsters, ${bossDb.length || 0} bosses`);
    } catch (error) {
        console.warn('Some databases failed to load:', error);
    }
}

// Set default combat stats
function setDefaultStats() {
    if (!state.player) return;
    
    const defaultStats = {
        'Attack': 99, 'Strength': 99, 'Defence': 99,
        'Ranged': 99, 'Magic': 99, 'Hitpoints': 99, 'Prayer': 99
    };
    
    for (const [stat, level] of Object.entries(defaultStats)) {
        state.player.setStat(stat, level);
    }
}

// Initialize UI event listeners
function initializeUI() {
    // Player stats inputs
    document.getElementById('stat-attack').addEventListener('change', (e) => updateStat('Attack', e.target.value));
    document.getElementById('stat-strength').addEventListener('change', (e) => updateStat('Strength', e.target.value));
    document.getElementById('stat-defence').addEventListener('change', (e) => updateStat('Defence', e.target.value));
    document.getElementById('stat-ranged').addEventListener('change', (e) => updateStat('Ranged', e.target.value));
    document.getElementById('stat-magic').addEventListener('change', (e) => updateStat('Magic', e.target.value));
    document.getElementById('stat-hitpoints').addEventListener('change', (e) => updateStat('Hitpoints', e.target.value));
    document.getElementById('stat-prayer').addEventListener('change', (e) => updateStat('Prayer', e.target.value));
    
    // Fetch stats button
    document.getElementById('fetch-stats-btn').addEventListener('click', fetchHiscores);
    
    // WikiSync button
    document.getElementById('wikisync-btn').addEventListener('click', connectWikiSync);
    
    // Equipment slots
    document.querySelectorAll('.equip-slot').forEach(slot => {
        slot.addEventListener('click', () => openItemModal(slot.dataset.slot));
    });
    
    // Item search
    document.getElementById('item-search-input').addEventListener('input', (e) => searchItems(e.target.value));
    
    // Monster search
    document.getElementById('monster-search').addEventListener('input', (e) => searchMonsters(e.target.value));
    
    // Calculate DPS button
    document.getElementById('calculate-btn').addEventListener('click', calculateDPS);
    
    // Simulate button
    document.getElementById('simulate-btn').addEventListener('click', runSimulation);
    
    // Upgrade advisor button
    document.getElementById('suggest-upgrades-btn').addEventListener('click', findUpgrades);
    
    // Upgrade tabs
    document.querySelectorAll('.tab-btn').forEach(btn => {
        btn.addEventListener('click', () => switchUpgradeTab(btn.dataset.tab));
    });

    // Upgrade filters
    document.getElementById('exclude-duo').addEventListener('change', applyFiltersAndSort);
    
    // Modal close
    document.querySelector('.modal-close').addEventListener('click', closeItemModal);
    document.getElementById('item-modal').addEventListener('click', (e) => {
        if (e.target.id === 'item-modal') closeItemModal();
    });
    
    // Modal item search
    document.getElementById('modal-item-search').addEventListener('input', (e) => searchItemsForSlot(e.target.value));

    // Buffs - Initialize and Add Listeners
    const pietyCb = document.getElementById('prayer-piety');
    const rigourCb = document.getElementById('prayer-rigour');
    const scCb = document.getElementById('potion-super-combat');
    
    // Reset to unchecked on load to match default player state
    if (pietyCb) pietyCb.checked = false;
    if (rigourCb) rigourCb.checked = false;
    if (scCb) scCb.checked = false;

    if (pietyCb) pietyCb.addEventListener('change', (e) => updateBuff('piety', e.target.checked));
    if (rigourCb) rigourCb.addEventListener('change', (e) => updateBuff('rigour', e.target.checked));
    if (scCb) scCb.addEventListener('change', (e) => updateBuff('superCombat', e.target.checked));
}

// Update buff status
function updateBuff(buff, active) {
    if (!state.player) return;
    
    try {
        if (buff === 'piety') state.player.setPiety(active);
        else if (buff === 'rigour') state.player.setRigour(active);
        else if (buff === 'superCombat') state.player.setSuperCombat(active);
        
        // If super combat is toggled, it affects internal calculations but not the displayed base stat
        // We could trigger a recalc if we wanted live updates
    } catch (e) {
        console.error('Error updating buff:', e);
    }
    calculateDPS();
}

// Update player stat
function updateStat(stat, value) {
    if (!state.player) return;
    state.player.setStat(stat, parseInt(value) || 1);
    calculateDPS();
}

// Fetch hiscores from OSRS API
async function fetchHiscores() {
    const username = document.getElementById('username').value.trim();
    if (!username) {
        alert('Please enter a username');
        return;
    }

    const btn = document.getElementById('fetch-stats-btn');
    btn.disabled = true;
    btn.textContent = 'Loading...';

    try {
        // Use a CORS proxy or direct API
        const proxyUrl = 'https://corsproxy.io/?';
        const apiUrl = `https://secure.runescape.com/m=hiscore_oldschool/index_lite.ws?player=${encodeURIComponent(username)}`;
        
        const response = await fetch(proxyUrl + encodeURIComponent(apiUrl));
        if (!response.ok) throw new Error('Player not found');
        
        const data = await response.text();
        parseHiscores(data);
        
        state.player.setUsername(username);
        document.getElementById('username').value = username;
        
    } catch (error) {
        console.error('Failed to fetch hiscores:', error);
        alert('Failed to fetch stats. Player may not exist or hiscores are unavailable.');
    } finally {
        btn.disabled = false;
        btn.textContent = 'Fetch Stats';
    }
}

// Parse hiscores CSV response
function parseHiscores(csv) {
    const lines = csv.trim().split('\n');
    const skills = [
        'Overall', 'Attack', 'Defence', 'Strength', 'Hitpoints', 'Ranged',
        'Prayer', 'Magic', 'Cooking', 'Woodcutting', 'Fletching', 'Fishing',
        'Firemaking', 'Crafting', 'Smithing', 'Mining', 'Herblore', 'Agility',
        'Thieving', 'Slayer', 'Farming', 'Runecraft', 'Hunter', 'Construction'
    ];

    const statInputs = {
        'Attack': 'stat-attack',
        'Strength': 'stat-strength',
        'Defence': 'stat-defence',
        'Ranged': 'stat-ranged',
        'Magic': 'stat-magic',
        'Hitpoints': 'stat-hitpoints',
        'Prayer': 'stat-prayer'
    };

    for (let i = 0; i < Math.min(lines.length, skills.length); i++) {
        const parts = lines[i].split(',');
        if (parts.length >= 2) {
            const level = parseInt(parts[1]) || 1;
            const skill = skills[i];
            
            if (state.player) {
                state.player.setStat(skill, level);
            }
            
            if (statInputs[skill]) {
                document.getElementById(statInputs[skill]).value = level;
            }
        }
    }
}

// Connect to WikiSync
async function connectWikiSync() {
    const btn = document.getElementById('wikisync-btn');
    btn.disabled = true;
    btn.textContent = 'Connecting...';

    // Try ports 37767-37776
    for (let port = 37767; port <= 37776; port++) {
        try {
            const ws = new WebSocket(`ws://localhost:${port}`);
            
            await new Promise((resolve, reject) => {
                ws.onopen = () => {
                    console.log(`Connected to WikiSync on port ${port}`);
                    ws.send(JSON.stringify({ _wsType: 'GetPlayer' }));
                    resolve();
                };
                ws.onerror = reject;
                setTimeout(reject, 1000);
            });

            ws.onmessage = (event) => {
                try {
                    const data = JSON.parse(event.data);
                    if (data._wsType === 'GetPlayer' && data.payload) {
                        loadWikiSyncData(data.payload);
                    }
                } catch (e) {
                    console.error('Error parsing WikiSync data:', e);
                }
            };

            btn.textContent = '✓ Connected';
            setTimeout(() => {
                btn.disabled = false;
                btn.textContent = '🔄 Sync from WikiSync';
            }, 2000);
            return;

        } catch (e) {
            continue;
        }
    }

    btn.disabled = false;
    btn.textContent = '🔄 Sync from WikiSync';
    alert('Could not connect to WikiSync. Make sure RuneLite is running with the WikiSync plugin enabled.');
}

// Load WikiSync data
function loadWikiSyncData(payload) {
    if (payload.loadouts && payload.loadouts.length > 0) {
        const equipment = payload.loadouts[0].equipment;
        
        for (const [slot, itemData] of Object.entries(equipment)) {
            if (itemData && itemData.id) {
                equipItem(slot, itemData.id);
            }
        }
    }
    
    updateEquipmentDisplay();
    updateBonuses();
}

// Search items
function searchItems(query) {
    const resultsDiv = document.getElementById('item-search-results');
    
    if (query.length < 2) {
        resultsDiv.classList.remove('active');
        return;
    }

    const results = searchItemDatabase(query, 20);
    displaySearchResults(resultsDiv, results, (item) => {
        const slot = item.equipment?.slot || 'weapon';
        equipItem(slot, item.id);
        resultsDiv.classList.remove('active');
        document.getElementById('item-search-input').value = '';
    });
}

// Search monsters
function searchMonsters(query) {
    const resultsDiv = document.getElementById('monster-search-results');
    
    if (query.length < 2) {
        resultsDiv.classList.remove('active');
        return;
    }

    const results = searchMonsterDatabase(query, 20);
    displayMonsterResults(resultsDiv, results, (monster) => {
        selectMonster(monster);
        resultsDiv.classList.remove('active');
        document.getElementById('monster-search').value = monster.name;
    });
}

// Search item database
function searchItemDatabase(query, limit = 20) {
    const results = [];
    const queryLower = query.toLowerCase();

    for (const [id, item] of Object.entries(state.itemDb)) {
        if (item.name && item.name.toLowerCase().includes(queryLower)) {
            if (item.equipment) {
                results.push({ ...item, id: parseInt(id) });
                if (results.length >= limit) break;
            }
        }
    }

    return results;
}

// Search monster database
function searchMonsterDatabase(query, limit = 20) {
    const results = [];
    const queryLower = query.toLowerCase();

    // Search regular monsters
    for (const [id, monster] of Object.entries(state.monsterDb)) {
        if (monster.name && monster.name.toLowerCase().includes(queryLower)) {
            if (monster.hitpoints > 0) {
                results.push({ ...monster, id: parseInt(id), source: 'monster' });
                if (results.length >= limit) break;
            }
        }
    }

    // Search bosses
    if (Array.isArray(state.bossDb)) {
        for (const boss of state.bossDb) {
            if (boss.name && boss.name.toLowerCase().includes(queryLower)) {
                if (boss.hitpoints > 0 && results.length < limit) {
                    results.push({ ...boss, source: 'boss' });
                }
            }
        }
    }

    return results.slice(0, limit);
}

// Display search results
function displaySearchResults(container, results, onSelect) {
    if (results.length === 0) {
        container.innerHTML = '<div class="search-result-item">No results found</div>';
        container.classList.add('active');
        return;
    }

    container.innerHTML = results.map(item => `
        <div class="search-result-item" data-id="${item.id}">
            <span class="item-name">${item.name}</span>
            <span class="item-slot">${item.equipment?.slot || ''}</span>
        </div>
    `).join('');

    container.querySelectorAll('.search-result-item').forEach((el, i) => {
        el.addEventListener('click', () => onSelect(results[i]));
    });

    container.classList.add('active');
}

// Display monster results
function displayMonsterResults(container, results, onSelect) {
    if (results.length === 0) {
        container.innerHTML = '<div class="search-result-item">No results found</div>';
        container.classList.add('active');
        return;
    }

    container.innerHTML = results.map(monster => `
        <div class="search-result-item">
            <span class="item-name">${monster.name}</span>
            <span class="item-slot">HP: ${monster.hitpoints}</span>
        </div>
    `).join('');

    container.querySelectorAll('.search-result-item').forEach((el, i) => {
        el.addEventListener('click', () => onSelect(results[i]));
    });

    container.classList.add('active');
}

// Equip an item
function equipItem(slot, itemId) {
    if (!state.wasmModule || !state.player) return;

    const itemData = state.itemDb[itemId.toString()];
    if (!itemData) return;

    // Create WASM Item and load stats
    const item = new state.wasmModule.Item(itemId);
    state.wasmModule.loadItemFromJson(item, itemId, JSON.stringify(state.itemDb));
    
    // Normalize slot name
    let normalizedSlot = slot;
    if (itemData.equipment && itemData.equipment.slot) {
        normalizedSlot = itemData.equipment.slot;
    }
    if (normalizedSlot === '2h') normalizedSlot = 'weapon';

    // Equip in player
    state.player.equip(normalizedSlot, item);
    state.equippedItems[normalizedSlot] = { id: itemId, name: itemData.name, data: itemData };

    // If 2h weapon, unequip shield
    if (itemData.equipment && itemData.equipment.slot === '2h') {
        state.player.unequip('shield');
        delete state.equippedItems['shield'];
    }

    updateEquipmentDisplay();
    updateBonuses();
    calculateDPS();
}

// Update equipment display
function updateEquipmentDisplay() {
    document.querySelectorAll('.equip-slot').forEach(slotEl => {
        const slot = slotEl.dataset.slot;
        const equipped = state.equippedItems[slot];

        if (equipped) {
            slotEl.classList.add('equipped');
            slotEl.querySelector('.slot-icon').innerHTML = `
                <span class="item-name">${equipped.name.substring(0, 12)}</span>
            `;
        } else {
            slotEl.classList.remove('equipped');
            slotEl.querySelector('.slot-icon').innerHTML = '';
        }
    });
}

// Update equipment bonuses display
function updateBonuses() {
    if (!state.player) return;

    const bonuses = {
        'bonus-attack-stab': state.player.getEquipmentBonus('attack_stab'),
        'bonus-attack-slash': state.player.getEquipmentBonus('attack_slash'),
        'bonus-attack-crush': state.player.getEquipmentBonus('attack_crush'),
        'bonus-attack-magic': state.player.getEquipmentBonus('attack_magic'),
        'bonus-attack-ranged': state.player.getEquipmentBonus('attack_ranged'),
        'bonus-defence-stab': state.player.getEquipmentBonus('defence_stab'),
        'bonus-defence-slash': state.player.getEquipmentBonus('defence_slash'),
        'bonus-defence-crush': state.player.getEquipmentBonus('defence_crush'),
        'bonus-defence-magic': state.player.getEquipmentBonus('defence_magic'),
        'bonus-defence-ranged': state.player.getEquipmentBonus('defence_ranged'),
        'bonus-melee-strength': state.player.getEquipmentBonus('melee_strength') || state.player.getEquipmentBonus('strength_bonus'),
        'bonus-ranged-strength': state.player.getEquipmentBonus('ranged_strength'),
        'bonus-prayer': state.player.getEquipmentBonus('prayer')
    };

    for (const [id, value] of Object.entries(bonuses)) {
        const el = document.getElementById(id);
        if (el) {
            el.textContent = value >= 0 ? `+${value}` : value;
        }
    }

    const magicDmg = state.player.getEquipmentBonus('magic_damage');
    const magicDmgEl = document.getElementById('bonus-magic-damage');
    if (magicDmgEl) {
        magicDmgEl.textContent = `${magicDmg}%`;
    }
}

// Select a monster
function selectMonster(monsterData) {
    if (!state.wasmModule) return;

    state.monster = new state.wasmModule.Monster(monsterData.name);
    
    // Load monster stats
    if (monsterData.source === 'boss') {
        state.wasmModule.loadMonsterFromJson(state.monster, monsterData.name, JSON.stringify(state.bossDb));
    } else {
        state.wasmModule.loadMonsterFromJson(state.monster, monsterData.name, JSON.stringify(state.monsterDb));
    }

    // Update UI
    document.getElementById('selected-monster-name').textContent = monsterData.name;
    document.getElementById('monster-hp').textContent = monsterData.hitpoints || state.monster.getInt('hitpoints');
    document.getElementById('monster-combat').textContent = monsterData.combat_level || state.monster.getInt('combat_level') || '-';
    document.getElementById('monster-def-level').textContent = monsterData.defence_level || state.monster.getInt('defence_level') || '-';
    calculateDPS();
}

// Open item modal for a slot
function openItemModal(slot) {
    state.selectedSlot = slot;
    document.getElementById('modal-slot-name').textContent = slot.charAt(0).toUpperCase() + slot.slice(1);
    document.getElementById('modal-item-search').value = '';
    document.getElementById('modal-item-results').innerHTML = '';
    document.getElementById('item-modal').classList.add('active');
    document.getElementById('modal-item-search').focus();
}

// Close item modal
function closeItemModal() {
    document.getElementById('item-modal').classList.remove('active');
    state.selectedSlot = null;
}

// Search items for slot in modal
function searchItemsForSlot(query) {
    const resultsDiv = document.getElementById('modal-item-results');
    
    if (query.length < 2) {
        resultsDiv.innerHTML = '';
        return;
    }

    const slot = state.selectedSlot;
    const results = [];
    const queryLower = query.toLowerCase();

    for (const [id, item] of Object.entries(state.itemDb)) {
        if (item.name && item.name.toLowerCase().includes(queryLower)) {
            if (item.equipment) {
                const itemSlot = item.equipment.slot;
                // Match slot or 2h for weapon slot
                if (itemSlot === slot || (slot === 'weapon' && itemSlot === '2h')) {
                    results.push({ ...item, id: parseInt(id) });
                    if (results.length >= 50) break;
                }
            }
        }
    }

    resultsDiv.innerHTML = results.map(item => `
        <div class="modal-result-item" data-id="${item.id}">
            ${item.name}
        </div>
    `).join('');

    resultsDiv.querySelectorAll('.modal-result-item').forEach((el, i) => {
        el.addEventListener('click', () => {
            equipItem(slot, results[i].id);
            closeItemModal();
        });
    });
}

// Calculate DPS
function calculateDPS() {
    if (!state.wasmModule || !state.player || !state.monster) {
        // Silent return if not ready
        if (!state.monster || !state.monster.getName()) return;
        return;
    }

    try {
        const battle = new state.wasmModule.Battle(state.player, state.monster);
        const resultsJson = state.wasmModule.getBattleResultsJson(battle);
        const results = JSON.parse(resultsJson);

        // Update UI
        document.getElementById('dps-value').textContent = results.dps.toFixed(2);
        document.getElementById('max-hit').textContent = results.maxHit;
        document.getElementById('hit-chance').textContent = (results.hitChance * 100).toFixed(1) + '%';
        document.getElementById('attack-style').textContent = `${results.style} (${results.stance})`;
        document.getElementById('attack-speed').textContent = `${results.attackSpeed} ticks`;
        document.getElementById('avg-ttk').textContent = results.avgTTK.toFixed(1) + 's';
        document.getElementById('kills-per-hour').textContent = results.killsPerHour.toFixed(1);

        // Calculate and Render CDF
        const ttkJson = state.wasmModule.getTTKDistribution(battle, 1000);
        const ttks = JSON.parse(ttkJson);
        renderTTKChart(ttks);

    } catch (error) {
        console.error('Error calculating DPS:', error);
    }
}

function renderTTKChart(ttks) {
    const ctx = document.getElementById('ttk-chart').getContext('2d');
    
    // Process data for CDF
    const uniqueTimes = [];
    const probabilities = [];
    const total = ttks.length;
    
    // ttks is sorted
    let currentCount = 0;
    
    // Add 0 point
    uniqueTimes.push(0);
    probabilities.push(0);

    for (let i = 0; i < total; i++) {
        currentCount++;
        // If next value is different or end of array
        if (i === total - 1 || ttks[i] !== ttks[i+1]) {
            uniqueTimes.push(ttks[i]);
            probabilities.push((currentCount / total) * 100);
        }
    }
    
    if (state.ttkChart) {
        state.ttkChart.destroy();
    }
    
    state.ttkChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: uniqueTimes,
            datasets: [{
                label: 'Cumulative Probability of Kill (%)',
                data: probabilities,
                borderColor: 'rgb(75, 192, 192)',
                backgroundColor: 'rgba(75, 192, 192, 0.1)',
                tension: 0.1,
                fill: true,
                pointRadius: 0
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                x: {
                    title: { display: true, text: 'Time (seconds)' },
                    type: 'linear'
                },
                y: {
                    title: { display: true, text: 'Probability (%)' },
                    min: 0,
                    max: 100
                }
            },
            plugins: {
                tooltip: {
                    callbacks: {
                        label: function(context) {
                            return context.parsed.y.toFixed(1) + '% chance to kill by ' + context.parsed.x.toFixed(1) + 's';
                        }
                    },
                    intersect: false,
                    mode: 'index'
                }
            }
        }
    });
}


// Run simulation
function runSimulation() {
    if (!state.wasmModule || !state.player || !state.monster) {
        alert('Please select a monster first');
        return;
    }

    const simCount = parseInt(document.getElementById('sim-count').value) || 1000;
    const btn = document.getElementById('simulate-btn');
    btn.disabled = true;
    btn.textContent = 'Simulating...';

    setTimeout(() => {
        try {
            const battle = new state.wasmModule.Battle(state.player, state.monster);
            battle.optimizeAttackStyle();
            const avgTicks = battle.runSimulations(simCount);
            const avgSeconds = avgTicks * 0.6;
            const monsterHP = state.monster.getInt('hitpoints');
            const killsPerHour = monsterHP > 0 ? 3600 / avgSeconds : 0;

            document.getElementById('avg-ttk').textContent = avgSeconds.toFixed(1) + 's';
            document.getElementById('kills-per-hour').textContent = killsPerHour.toFixed(1);

        } catch (error) {
            console.error('Error running simulation:', error);
        } finally {
            btn.disabled = false;
            btn.textContent = 'Run Simulation';
        }
    }, 10);
}

// Find upgrades
function findUpgrades() {
    if (!state.wasmModule || !state.player || !state.monster) {
        alert('Please select a monster first');
        return;
    }

    const maxBudget = parseInt(document.getElementById('max-budget').value) || 0;
    const btn = document.getElementById('suggest-upgrades-btn');
    btn.disabled = true;
    btn.textContent = 'Analyzing...';

    setTimeout(() => {
        try {
            const advisor = new state.wasmModule.UpgradeAdvisor();
            advisor.initialize(
                state.player, 
                state.monster, 
                JSON.stringify(state.itemDb),
                JSON.stringify(state.priceDb)
            );

            const excludeThrowables = document.getElementById('exclude-throwables').checked;
            const excludeAmmo = document.getElementById('exclude-ammo').checked;
            
            const suggestionsJson = advisor.suggestUpgrades(maxBudget, excludeThrowables, excludeAmmo);
            state.rawSuggestions = JSON.parse(suggestionsJson);

            applyFiltersAndSort();

        } catch (error) {
            console.error('Error finding upgrades:', error);
            alert('Error finding upgrades. Check console for details.');
        } finally {
            btn.disabled = false;
            btn.textContent = 'Find Upgrades';
        }
    }, 10);
}

// Apply filters and sort to suggestions
function applyFiltersAndSort() {
    if (!state.rawSuggestions || state.rawSuggestions.length === 0) {
        displayUpgrades([]);
        return;
    }

    // Filter
    const excludeDuo = document.getElementById('exclude-duo').checked;
    let filtered = [...state.rawSuggestions];

    if (excludeDuo) {
        filtered = filtered.filter(sug => {
            const itemNames = sug.itemNames || [sug.itemName];
            const isDuo = sug.isDuo || itemNames.length > 1;
            return !isDuo;
        });
    }

    // Sort
    if (state.activeTab === 'efficiency') {
        filtered.sort((a, b) => b.dpsPerMillionGP - a.dpsPerMillionGP);
    } else {
        filtered.sort((a, b) => b.dpsIncrease - a.dpsIncrease);
    }

    displayUpgrades(filtered);
}

// Display upgrades (supports both single and duo suggestions)
function displayUpgrades(suggestions) {
    const tbody = document.getElementById('upgrade-table-body');

    if (suggestions.length === 0) {
        tbody.innerHTML = '<tr><td colspan="5" class="empty-message">No upgrades found within budget</td></tr>';
        return;
    }

    tbody.innerHTML = suggestions.map(sug => {
        // Handle both old format (itemName) and new format (itemNames array)
        const itemNames = sug.itemNames || [sug.itemName];
        const slots = sug.slots || [sug.slot];
        const isDuo = sug.isDuo || itemNames.length > 1;
        
        // Format item names - join with "+" for duos
        const itemDisplay = isDuo 
            ? `<span class="duo-badge">DUO</span> ${itemNames.join(' + ')}`
            : itemNames[0];
        
        // Format slots
        const slotDisplay = slots.join(', ');
        
        return `
            <tr class="${isDuo ? 'duo-row' : ''}">
                <td class="item-cell">${itemDisplay}</td>
                <td>${slotDisplay}</td>
                <td>${formatPrice(sug.price)}</td>
                <td class="text-green">+${sug.dpsIncrease.toFixed(3)}</td>
                <td>${sug.dpsPerMillionGP.toFixed(3)}</td>
            </tr>
        `;
    }).join('');
}

// Switch upgrade tab
function switchUpgradeTab(tab) {
    document.querySelectorAll('.tab-btn').forEach(btn => {
        btn.classList.toggle('active', btn.dataset.tab === tab);
    });

    state.activeTab = tab;
    applyFiltersAndSort();
}

// Format price
function formatPrice(price) {
    if (price >= 1000000) {
        return (price / 1000000).toFixed(1) + 'M';
    } else if (price >= 1000) {
        return (price / 1000).toFixed(0) + 'K';
    }
    return price.toString();
}

// Close search results when clicking outside
document.addEventListener('click', (e) => {
    if (!e.target.closest('.item-search') && !e.target.closest('.monster-section')) {
        document.getElementById('item-search-results').classList.remove('active');
        document.getElementById('monster-search-results').classList.remove('active');
    }
});

// Initialize on page load
document.addEventListener('DOMContentLoaded', init);
