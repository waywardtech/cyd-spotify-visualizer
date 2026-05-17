#!/usr/bin/env python3
"""
╔══════════════════════════════════════════════════════════╗
║  get_spotify_token.py                                    ║
║  Run ONCE on your PC to get a Spotify refresh token      ║
║  Paste the result into spotify_viz_wifi.ino              ║
╚══════════════════════════════════════════════════════════╝

Requirements:
    pip install requests

Steps:
    1. Go to https://developer.spotify.com/dashboard
    2. Create an app (any name)
    3. Set Redirect URI to:  http://localhost:8888/callback
    4. Copy Client ID and Client Secret below
    5. Run this script — it opens your browser
    6. Authorize — you'll be redirected to localhost (page won't load, that's fine)
    7. Paste the FULL redirect URL when prompted
    8. Copy the refresh_token into your .ino file
"""

import os
import sys
import json
import base64
import urllib.parse
import urllib.request
import webbrowser

# ── Fill these in ──────────────────────────────────────────────
CLIENT_ID     = "YOUR_CLIENT_ID"
CLIENT_SECRET = "YOUR_CLIENT_SECRET"
REDIRECT_URI  = "http://localhost:8888/callback"
# ───────────────────────────────────────────────────────────────

SCOPES = "user-read-currently-playing user-read-playback-state"

def main():
    if CLIENT_ID == "YOUR_CLIENT_ID":
        print("❌  Edit this script and fill in CLIENT_ID and CLIENT_SECRET first.")
        sys.exit(1)

    # Step 1: Build auth URL
    params = {
        "client_id":     CLIENT_ID,
        "response_type": "code",
        "redirect_uri":  REDIRECT_URI,
        "scope":         SCOPES,
    }
    auth_url = "https://accounts.spotify.com/authorize?" + urllib.parse.urlencode(params)

    print("\n🎵  Spotify Token Helper for CYD Visualizer")
    print("─" * 50)
    print("Opening browser for Spotify authorization...")
    print(f"\nIf browser doesn't open, visit:\n  {auth_url}\n")
    webbrowser.open(auth_url)

    # Step 2: Get redirected URL from user
    print("After authorizing, you'll be redirected to a localhost URL.")
    print("The page won't load — that's expected.")
    print("\nPaste the FULL URL from your browser's address bar:")
    redirected = input("  > ").strip()

    # Extract code
    parsed = urllib.parse.urlparse(redirected)
    code = urllib.parse.parse_qs(parsed.query).get("code", [None])[0]
    if not code:
        print("❌  Couldn't find 'code' in that URL. Try again.")
        sys.exit(1)

    # Step 3: Exchange code for tokens
    credentials = base64.b64encode(f"{CLIENT_ID}:{CLIENT_SECRET}".encode()).decode()
    data = urllib.parse.urlencode({
        "grant_type":   "authorization_code",
        "code":         code,
        "redirect_uri": REDIRECT_URI,
    }).encode()

    req = urllib.request.Request(
        "https://accounts.spotify.com/api/token",
        data=data,
        headers={
            "Authorization": f"Basic {credentials}",
            "Content-Type":  "application/x-www-form-urlencoded",
        }
    )

    with urllib.request.urlopen(req) as resp:
        tokens = json.loads(resp.read())

    refresh_token = tokens.get("refresh_token", "")
    access_token  = tokens.get("access_token",  "")

    if not refresh_token:
        print("❌  No refresh token in response:", tokens)
        sys.exit(1)

    print("\n" + "═" * 55)
    print("✅  SUCCESS! Copy this into your .ino file:")
    print("═" * 55)
    print(f'\n#define REFRESH_TOKEN  "{refresh_token}"\n')
    print("═" * 55)
    print("\nAlso make sure these are set:")
    print(f'  #define CLIENT_ID      "{CLIENT_ID}"')
    print(f'  #define CLIENT_SECRET  "{CLIENT_SECRET}"')
    print("\n" + "⚠️  SECURITY WARNING".center(55, "─"))
    print("The refresh token doesn't expire (unless you revoke app access).")
    print("Keep it secret — it gives access to your Spotify account.")
    print("Do NOT commit this to version control or share it.\n")

if __name__ == "__main__":
    main()
