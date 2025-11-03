# 🎵 MP3 Tag Reader and Editor

## Project Overview

This project is a **Command Line Interface (CLI)** application developed in **C** to interact with MP3 audio files. Its primary function is to **read, display, and edit** the metadata, or **ID3 tags**, embedded within an MP3 file.

This project was developed as a hands-on assignment for the **Embedded Systems Course** at **Emertxe Information Technologies**, focusing on low-level file manipulation and data parsing techniques.

---

## 🚀 Key Features

* **ID3v2 Support:** Specifically designed to parse and handle the complex structure of the **ID3v2.3** tag format, ensuring correct frame identification and data extraction.
* **View Mode (`-v`):** Reads and displays key ID3v2 tags such as **Title**, **Artist**, **Album**, **Year**, **Genre**, and **Comment**.
* **Edit Mode (`-e`):** Allows the user to modify the values of existing supported ID3v2 tags by calculating and adjusting frame sizes.
* **Robust Error Handling:** Includes checks for file existence, validity of the MP3 signature, and the presence of ID3 tags.

---

## 🛠️ Technology Stack & Concepts

| Component | Description |
| :--- | :--- |
| **Language** | **C** |
| **Platform** | Linux (Terminal Environment) |
| **Core Concepts** | **File Handling** (`fopen`, `fseek`, `fread`, `fwrite`), **Bitwise Operations** (for syncsafe integers and flag parsing), **Structs**, **Pointers**, and **Command Line Argument Parsing**. |

---

## ⚙️ Usage

The application supports two primary modes: **View** (`-v`) and **Edit** (`-e`).

### 1. **View Mode: Displaying Tags**

To view the tags of an MP3 file, use the `-v` option:

./mp3tag -v <mp3_filename>

**Example:**

./mp3tag -v MyFavoriteSong.mp3

### 2. **Edit Mode: Modifying Tags**

To edit a specific tag, use the `-e` option along with the appropriate modifier and the new value:

./mp3tag -e <mp3_filename> <tag_modifier> <new_tag_value>

| Modifier | Tag to Edit (ID3 Frame) |
| :--- | :--- |
| `-t` | **Title** (`TIT2` frame) |
| `-a` | **Artist** (`TPE1` frame) |
| `-A` | **Album** (`TALB` frame) |
| `-y` | **Year** (`TYER` frame) |

**Example:** To change the Album of the song:

./mp3tag -e MyFavoriteSong.mp3 -A "Greatest Hits Collection"

---

## 📂 Project Files

| File | Description |
| :--- | :--- |
| **main.c** | The entry point of the program. It handles command-line argument parsing and directs flow to the core functions. |
| **mp3tag.h** | Contains **structure definitions** (for ID3 header, frame header) and function prototypes. |
| **mp3tag.c** | Implementation of core logic: `View()`, `Edit()`, and helper functions for bitwise conversions. |

---

## 🧑‍💻 Author and Acknowledgements

**Author:** Adhish v

**Course:** Emertxe Embedded Systems Course

**Date:** 2025

*Special thanks to Emertxe Information Technologies for the detailed curriculum and guidance that made this project possible.*
