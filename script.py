import os
import shutil
import subprocess

# Paths
BASE_PATH = os.path.join(os.environ['USERPROFILE'], 'Documents', 'KoeiTecmo', 'NIOH', 'Savedata')
DECRYPTOR = r".\Nioh_Savefile_Decrypt.exe"


def copyR(source_path, destination_path):
    """
    Copies a file to a new location. If the destination file already exists,
    it renames the copied file by appending a number.
    """
    if not os.path.exists(source_path):
        print(f"Source file not found: {source_path}")
        return

    # Split the destination path into base name and extension
    base, extension = os.path.splitext(destination_path)
    counter = 1
    new_destination_path = destination_path

    # Check if the destination file already exists and generate a new name if it does
    while os.path.exists(new_destination_path):
        new_destination_path = f"{base}_{counter}{extension}"
        counter += 1

    try:
        # Copy the file with the unique name
        shutil.copy2(source_path, new_destination_path) # Use copy2 to preserve metadata
        print(f"File copied and renamed to: {new_destination_path}")
    except IOError as e:
        print(f"Error copying file: {e}")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")

def find_ids():
    """Detects Steam and Epic IDs based on folder name patterns."""
    if not os.path.exists(BASE_PATH):
        raise RuntimeError (f"Error: Nioh save directory not found at {BASE_PATH}")

    folders = [f for f in os.listdir(BASE_PATH) if os.path.isdir(os.path.join(BASE_PATH, f))]
    
    steam_id = next((f for f in folders if f.isdigit()), None)
    epic_id = next((f for f in folders if len(f) > 20 and not f.isdigit()), None)


    steamsavepath = os.path.join(BASE_PATH, steam_id, "SAVEDATA00", "SAVEDATA.BIN")
    epicsavepath = os.path.join(BASE_PATH, epic_id, "SAVEDATA00", "SAVEDATA.BIN")

    if not os.path.exists(steamsavepath):
        raise RuntimeError(f"Error: Missing save file {steamsavepath}")
    if not os.path.exists(epicsavepath):
        raise RuntimeError(f"Error: Missing save file {epicsavepath}")
    
    return steam_id, epic_id

def run_decryptor(args):
    try:
        # print(f"{[DECRYPTOR] + args}")
        subprocess.run([DECRYPTOR] + args, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Decryptor Error: {e}")

def process_save(mode):
    filename = "SAVEDATA.BIN"
    filenamebackup = "SAVEDATA.BIN.bak"
    savedirname = "SAVEDATA00"
    sid, eid = find_ids()
    if not sid or not eid:
        print(f"Detection failed. Found Steam: {sid}, Epic: {eid}")
        return

    print(f"Detected IDs -> Steam: {sid} | Epic: {eid}")
    
    if mode == "1": # Epic to Steam
        input_file = os.path.join(BASE_PATH, eid, savedirname, filename)
        output_dir = os.path.join(BASE_PATH, sid, savedirname)
        
        # 1. Backup
        print("Backing up Steam save...")
        copyfrom = os.path.join(output_dir, filename)
        copyto = os.path.join(output_dir, filenamebackup)
        print(f"Backing up file: {copyfrom} to {copyto}")
        copyR(copyfrom, copyto)
        
        # 2. Encrypt with {SID}
        # Args: -cs (no checksum), -i (input), -sid (target steam ID), -o (output name)
        run_decryptor(["-cs", "-i", input_file, "-sid", sid, "-o", filename])

        # 3. Move to steam
        shutil.move(filename, copyfrom)
        print(f"move {filename} to {copyfrom}")
        print("Transfer complete!")

    elif mode == "2": # Steam to Epic
        input_file = os.path.join(BASE_PATH, sid, savedirname, filename)
        output_dir = os.path.join(BASE_PATH, eid, savedirname)

        print("Backing up Epic save...")
        copyfrom = os.path.join(output_dir, filename)
        copyto = os.path.join(output_dir, filenamebackup)
        print(f"Backing up file: {copyfrom} to {copyto}")
        copyR(copyfrom, copyto)

        # 1. Decrypt
        run_decryptor(["-cs", "-i", input_file, "-o", filename])

        # 2. Hex Edit Decrypted File (Offset 0x80)
        with open(filename, "r+b") as f:
            f.seek(0x80)
            f.write(eid.encode('utf-8').ljust(32, b'\x00'))
            
        # 3. Re-encrypt
        run_decryptor(["-i", filename, "-o", filename])
        
        # 4. Final Header Fix (Offset 0x10)
        with open(filename, "r+b") as f:
            f.seek(0x10)
            f.write(bytes.fromhex("0B3BCD78"))
        
        # 4. Move to Epic save dir
        shutil.move(filename, copyfrom)
        print(f"move {filename} to {copyfrom}")
        print("mode == 2 Transfer complete!")

if __name__ == "__main__":
    print("Nioh Save Transfer Tool")
    print("1: Epic -> Steam\n2: Steam -> Epic")
    choice = input("Select mode: ")
    process_save(choice)