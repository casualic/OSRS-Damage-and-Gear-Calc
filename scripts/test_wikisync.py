#!/usr/bin/env python3
"""
WikiSync Protocol Tester
Run this first to discover the exact message format WikiSync expects.
Then port the working protocol to C++.

pip install websockets
"""

import asyncio
import websockets
import json
import os
from datetime import datetime

PORT_START = 37767
PORT_END = 37776

# Global state to track username across messages
current_state = {
    "username": "unknown_player"
}

async def test_wikisync():
    """Try to connect to WikiSync and discover the protocol."""
    
    # WikiSync likely checks the Origin header to prevent third-party use
    headers = {
        "Origin": "https://tools.runescape.wiki",
        "User-Agent": "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36"
    }

    for port in range(PORT_START, PORT_END + 1):
        # Use localhost to allow IPv6/IPv4 resolution
        uri = f"ws://localhost:{port}"
        print(f"Trying {uri}...")
        
        try:
            # Add extra_headers to mimic the legitimate website
            async with websockets.connect(uri, close_timeout=2, additional_headers=headers) as ws:
                print(f"✓ Connected on port {port}!")
                
                # Start a task to listen for messages continuously
                async def listen():
                    try:
                        while True:
                            msg = await ws.recv()
                            print(f"\n[Received]: {msg[:200]}..." if len(msg) > 200 else f"\n[Received]: {msg}")
                            parse_and_save(msg)
                    except websockets.exceptions.ConnectionClosed:
                        print("Connection closed.")

                listener_task = asyncio.create_task(listen())

                # Try sending requests to trigger equipment data
                requests_to_try = [
                    '{"type":"REQUEST_PLAYER_DATA"}',
                    '{"type":"GET_PLAYER"}', 
                    '{"type":"EQUIPMENT"}',
                    '{"action":"get_equipment"}',
                    '{"_wsType":"GetPlayer"}',
                ]
                
                print("\nSending discovery requests...")
                for req in requests_to_try:
                    print(f"Sending: {req}")
                    await ws.send(req)
                    await asyncio.sleep(1) # Give it time to respond
                
                print("\nListening for 10 seconds... (Try changing equipment in-game!)")
                await asyncio.sleep(10)
                
                listener_task.cancel()
                return

        except ConnectionRefusedError:
            continue
        except Exception as e:
            print(f"  Error connecting to {port}: {e}")
            continue
    
    print("\n✗ Could not connect to WikiSync on any port.")
    print("Make sure RuneLite is running with WikiSync plugin enabled.")
    return None


def parse_and_save(data_str):
    """Parse, display, and save the data."""
    try:
        parsed = json.loads(data_str)
        
        # Update username if found
        if parsed.get('_wsType') == 'UsernameChanged':
            current_state['username'] = parsed.get('username', 'unknown_player')
            print(f"✓ Detected Username: {current_state['username']}")
            
        # Check for equipment data (typically in GetPlayer)
        if parsed.get('_wsType') == 'GetPlayer':
            save_data(parsed)
            
            # Display summary
            if 'payload' in parsed and 'loadouts' in parsed['payload']:
                 print("✓ Found Loadout Data")
                 
        return parsed
    except json.JSONDecodeError:
        print(f"Raw (not JSON): {data_str}")
        return data_str

def save_data(data):
    """Save the JSON data to a file."""
    try:
        username = current_state['username']
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"data/wikisync/{username}_{timestamp}.json"
        
        # Ensure directory exists
        os.makedirs(os.path.dirname(filename), exist_ok=True)
        
        with open(filename, 'w') as f:
            json.dump(data, f, indent=2)
            
        print(f"✓ Saved player data to {filename}")
        
    except Exception as e:
        print(f"Error saving data: {e}")


if __name__ == "__main__":
    print("WikiSync Protocol Tester")
    print("=" * 40)
    print("Make sure RuneLite is running with WikiSync enabled.\n")
    
    asyncio.run(test_wikisync())
