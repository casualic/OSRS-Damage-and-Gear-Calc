#!/bin/bash

# OSRS DPS Calculator - Web Build Script

echo "=== OSRS DPS Calculator Web Build ==="

# Check for Emscripten
if ! command -v emcc &> /dev/null; then
    echo "Error: Emscripten not found!"
    echo "Please install Emscripten SDK first:"
    echo "  git clone https://github.com/emscripten-core/emsdk.git"
    echo "  cd emsdk"
    echo "  ./emsdk install latest"
    echo "  ./emsdk activate latest"
    echo "  source ./emsdk_env.sh"
    exit 1
fi

# Create web directories
echo "[1/3] Creating directories..."
mkdir -p web/css web/js web/data

# Build WASM
echo "[2/3] Building WebAssembly module..."
emmake make -f Makefile.emscripten

if [ $? -ne 0 ]; then
    echo "Error: WASM build failed!"
    exit 1
fi

# Copy data files
echo "[3/3] Copying data files..."
cp items-complete.json web/data/ 2>/dev/null || echo "  Warning: items-complete.json not found"
cp monsters-nodrops.json web/data/ 2>/dev/null || echo "  Warning: monsters-nodrops.json not found"
cp bosses_complete.json web/data/ 2>/dev/null || echo "  Warning: bosses_complete.json not found"
cp latest_prices.json web/data/ 2>/dev/null || echo "  Warning: latest_prices.json not found"

echo ""
echo "=== Build Complete! ==="
echo ""
echo "To run the application:"
echo "  cd web"
echo "  python -m http.server 8080"
echo ""
echo "Then open http://localhost:8080 in your browser"
