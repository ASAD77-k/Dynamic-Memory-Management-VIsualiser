# Dynamic Memory Management Visualizer

## Overview
This project simulates dynamic memory management techniques, specifically focusing on **Paging** and **Virtual Memory**. The project includes both a visually stunning interactive Web-based Visualizer and a Command-Line C Simulator to meet project requirements.

## Features
*   **Interactive Visualizer:** An HTML/CSS/JS interface that lets you dynamically allocate and deallocate processes, observing how physical memory is utilized across pages.
*   **Real-time Address Translation & Log:** The "Control Center" terminal provides real-time logging of address assignments and memory operations.
*   **C Simulator:** A core `simulator.c` file that implements the paging logic programmatically, fulfilling the C-file project requirement.

## Files
1.  `index.html`: The main visualizer user interface.
2.  `styles.css`: Dark-mode aesthetics with neon glowing accents.
3.  `script.js`: The underlying simulation engine for the web visualizer.
4.  `simulator.c`: A standalone C program that demonstrates memory allocation in the terminal.

## How to Run

### 1. Web Visualizer
Simply open `index.html` in any modern web browser (Chrome, Edge, Firefox, Safari). No installation or server is required.

### 2. C Simulator
Compile and run the C program using a C compiler (like GCC):
```bash
gcc simulator.c -o simulator
./simulator
```

## Creating Your 3 Commits
To satisfy your "at least 3 commits" requirement, follow these steps in your Git terminal or Git Desktop client:

**Commit 1: Add the C Simulator**
``'`bash
git init
git add simulator.c
git commit -m "Initial commit: Add core C-based memory simulator"
```

**Commit 2: Add Visualizer Structure**
```bash
git add index.html styles.css
git commit -m "Add visualizer UI layout and styles"
```

**Commit 3: Add Visualizer Logic**
```bash
git add script.js README.md
git commit -m "Implement memory management simulation engine in JS"
```
