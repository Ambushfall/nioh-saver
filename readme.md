# Nioh Save Transfer Tool

NOTE: THIS DOES NOT HANDLE ANY OTHER SAVE NUMBERINGS


A Python-powered utility to bridge the gap between **Epic Games Store** and **Steam** versions of Nioh. This tool automates the decryption, hex manipulation, and ID re-signing required to make save files cross-compatible.

_There's also a C++ Release created for the sole purpose of having a single runtime binary, knock yourself out_

## Features
- **Auto-Detection:** Automatically locates your Nioh save directories for both Steam and Epic.
- **Epic to Steam:** Re-signs Epic saves with your Steam3ID and recalculates integrity.
- **Steam to Epic:** Automates the tedious manual hex editing (Offset `0x80` ID injection and `0x10` header fix).
- **Backup before you F up:** Creates `.bak` files of your existing saves before performing any operations. (_No they don't get overwritten_)

---

## Prerequisites
1. **The Decryptor:** You must have `Nioh_Savefile_Decrypt.exe` in the same folder as this tool. 
   - *Credit to [Pawrep's Nioh Save Decryptor](https://github.com/pawREP/Nioh-Savedata-Decryption-Tool) for the decryptor and his amazing work.*
2. **Python 3.x:** Required if running the script directly. Download from [Python.org](https://www.python.org).

---

##  How to Use

### Option A: Running from Source
1. Place `script.py` and `Nioh_Savefile_Decrypt.exe` in the same directory.
2. Run the script: `python nioh_auto_tool.py`
3. Choose 1 or 2 (Epic to Steam or Steam to Epic)


---

_This tool is not affiliated with Koei Tecmo or Team Ninja._ 

_Use at your own risk._ 

_Always verify your save files and .bak files before deleting anything manually._