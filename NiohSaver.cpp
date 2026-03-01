#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace fs = std::filesystem;

std::string get_base_path() {
    const char* user_profile = std::getenv("USERPROFILE");
    fs::path base = fs::path(user_profile) / "Documents" / "KoeiTecmo" / "NIOH" / "Savedata";
    return base.string();
}

const std::string DECRYPTOR = ".\\Nioh_Savefile_Decrypt.exe";

void copyR(const fs::path& source_path, const fs::path& destination_path) {
    if (!fs::exists(source_path)) {
        std::cout << "Source file not found: " << source_path.string() << std::endl;
        return;
    }
    fs::path parent = destination_path.parent_path();
    std::string stem = destination_path.stem().string();
    std::string extension = destination_path.extension().string();

    int counter = 1;
    fs::path new_destination_path = destination_path;

    while (fs::exists(new_destination_path)) {
        new_destination_path = parent / (stem + "_" + std::to_string(counter) + extension);
        counter++;
    }

    try {
        fs::copy(source_path, new_destination_path, fs::copy_options::copy_symlinks);
        std::cout << "File copied and renamed to: " << new_destination_path.string() << std::endl;
    }
    catch (const fs::filesystem_error& e) {
        std::cout << "Error copying file: " << e.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "An unexpected error occurred: " << e.what() << std::endl;
    }
}

std::pair<std::string, std::string> find_ids() {
    std::string base_path_str = get_base_path();
    if (!fs::exists(base_path_str)) {
        throw std::runtime_error("Error: Nioh save directory not found at " + base_path_str);
    }

    std::vector<std::string> folders;
    for (const auto& entry : fs::directory_iterator(base_path_str)) {
        if (entry.is_directory()) {
            folders.push_back(entry.path().filename().string());
        }
    }

    auto is_digit_string = [](const std::string& s) {
        return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
    };

    std::string steam_id = "";
    for (const auto& f : folders) {
        if (is_digit_string(f)) {
            steam_id = f;
            break;
        }
    }

    std::string epic_id = "";
    for (const auto& f : folders) {
        if (f.length() > 20 && !is_digit_string(f)) {
            epic_id = f;
            break;
        }
    }

    if (steam_id.empty() || epic_id.empty()) {
        return { steam_id, epic_id };
    }

    fs::path steamsavepath = fs::path(base_path_str) / steam_id / "SAVEDATA00" / "SAVEDATA.BIN";
    fs::path epicsavepath = fs::path(base_path_str) / epic_id / "SAVEDATA00" / "SAVEDATA.BIN";

    if (!fs::exists(steamsavepath)) {
        throw std::runtime_error("Error: Missing save file " + steamsavepath.string());
    }
    if (!fs::exists(epicsavepath)) {
        throw std::runtime_error("Error: Missing save file " + epicsavepath.string());
    }

    return { steam_id, epic_id };
}

void run_decryptor(const std::vector<std::string>& args) {
    try {
        std::string command = DECRYPTOR;
        for (const auto& arg : args) {
            command += " " + arg;
        }
        std::cout << "Command: " << command.c_str() << std::endl;
        int result = std::system(command.c_str());
        // int result = 0;
        if (result != 0) {
            throw std::runtime_error("Process returned non-zero exit code: " + std::to_string(result));
        }
    }
    catch (const std::exception& e) {
        std::cout << "Decryptor Error: " << e.what() << std::endl;
    }
}

void process_save(const std::string& mode) {
    const std::string filename = "SAVEDATA.BIN";
    const std::string filenamebackup = "SAVEDATA.BIN.bak";
    const std::string savedirname = "SAVEDATA00";
    
    std::string sid, eid;
    try {
        auto ids = find_ids();
        sid = ids.first;
        eid = ids.second;
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return;
    }

    if (sid.empty() || eid.empty()) {
        std::cout << "Detection failed. Found Steam: " << sid << ", Epic: " << eid << std::endl;
        return;
    }

    std::cout << "Detected IDs -> Steam: " << sid << " | Epic: " << eid << std::endl;

    std::string base_path_str = get_base_path();

    if (mode == "1") { // Epic to Steam
        fs::path input_file = fs::path(base_path_str) / eid / savedirname / filename;
        fs::path output_dir = fs::path(base_path_str) / sid / savedirname;

        // 1. Backup
        std::cout << "Backing up Steam save..." << std::endl;
        fs::path copyfrom = output_dir / filename;
        fs::path copyto = output_dir / filenamebackup;
        std::cout << "Backing up file: " << copyfrom.string() << " to " << copyto.string() << std::endl;
        copyR(copyfrom, copyto);
        run_decryptor({ "-cs", "-i", "\"" + input_file.string() + "\"", "-sid", sid, "-o", filename });
        if (fs::exists(filename)) {
            fs::rename(filename, copyfrom);
            std::cout << "move " << filename << " to " << copyfrom.string() << std::endl;
            std::cout << "Epic to Steam: Transfer complete!" << std::endl;
        } else {
            std::cout << "Error: Decrypted file " << filename << " not found." << std::endl;
        }

    }
    else if (mode == "2") { // Steam to Epic
        fs::path input_file = fs::path(base_path_str) / sid / savedirname / filename;
        fs::path output_dir = fs::path(base_path_str) / eid / savedirname;

        std::cout << "Backing up Epic save..." << std::endl;
        fs::path copyfrom = output_dir / filename;
        fs::path copyto = output_dir / filenamebackup;
        std::cout << "Backing up file: " << copyfrom.string() << " to " << copyto.string() << std::endl;
        copyR(copyfrom, copyto);
        run_decryptor({ "-cs", "-i", "\"" + input_file.string() + "\"", "-o", filename });
        std::fstream f(filename, std::ios::in | std::ios::out | std::ios::binary);
        if (f.is_open()) {
            f.seekp(0x80);
            char buffer[32] = {0};
            std::size_t len = std::min(eid.length(), (std::size_t)32);
            std::copy(eid.begin(), eid.begin() + len, buffer);
            f.write(buffer, 32);
            f.close();
        }

        run_decryptor({ "-i", filename, "-o", filename });
        f.open(filename, std::ios::in | std::ios::out | std::ios::binary);
        if (f.is_open()) {
            f.seekp(0x10);
            unsigned char header_fix[] = { 0x0B, 0x3B, 0xCD, 0x78 };
            f.write(reinterpret_cast<char*>(header_fix), 4);
            f.close();
        }
        if (fs::exists(filename)) {
            fs::rename(filename, copyfrom);
            std::cout << "move " << filename << " to " << copyfrom.string() << std::endl;
            std::cout << "Steam to Epic: Transfer complete!" << std::endl;
        } else {
            std::cout << "Error: Processed file " << filename << " not found." << std::endl;
        }
    }
}

int main() {
    std::cout << "Nioh Save Transfer Tool" << std::endl;
    std::cout << "1: Epic -> Steam\n2: Steam -> Epic" << std::endl;
    std::cout << "Select mode: ";
    std::string choice;
    std::getline(std::cin, choice);
    try {
        process_save(choice);
    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
    }
    return 0;
}
