# EVILINED - Advanced Line Editor

## Table of Contents
- [Overview](#overview)
- [Features](#features)
- [Installation & Compilation](#installation--compilation)
- [Basic Usage](#basic-usage)
- [Command Reference](#command-reference)
- [Visual Mode](#visual-mode)
- [Customization Guide](#customization-guide)
- [File Type Extensions](#file-type-extensions)
- [Advanced Modifications](#advanced-modifications)
- [Platform Support](#platform-support)
- [License](#license)

## Overview

**EVILINED** (Enhanced Visual Line Editor) is a powerful, full-featured line editor that combines the simplicity of classic EDLIN with modern enhancements. It features both traditional line-mode editing and a visual full-screen editor mode, designed specifically for DOS and vintage computing environments.

**Unlike most Vi clones for PC**, EVILINED is fully compatible with the original IBM PC's 8088 processor and NEC V20 systems, making it ideal for authentic retro computing and the most basic PC-compatible hardware.

### Key Improvements Over Original EDLIN
- **Visual Editor Mode**: Full-screen editing with cursor navigation
- **Enhanced Banner**: Professional startup with file status and system time
- **File Type Detection**: Automatic source file type identification
- **Advanced Search/Replace**: Pattern matching with global replace options
- **Memory Safety**: Robust memory management and error handling
- **DOS Compatible**: Optimized for MS-DOS and IBM-PC compatible systems

## Features

### Core Editor Features
- ✅ **Zero-padded line numbers** (00000, 00001, ...)
- ✅ **Case-insensitive commands**
- ✅ **Multi-line insert mode**
- ✅ **Advanced search/replace** with `/old/new/[g]` syntax
- ✅ **Range parsing** (a,b format)
- ✅ **Status line** after every command
- ✅ **Interactive prompts** with current line numbers
- ✅ **Memory safety** with bounds checking

### Visual Mode Features
- ✅ **Full-screen editing** with cursor navigation
- ✅ **Arrow key navigation** (Up, Down, Left, Right)
- ✅ **Page navigation** (PgUp, PgDn)
- ✅ **Home/End** line navigation
- ✅ **File type identification** in status bar
- ✅ **Real-time status updates**
- ✅ **Function key shortcuts** (F1=Help, F2=Save, ESC=Exit)

### Enhanced Banner System
- 🎨 **Professional ASCII banner**
- 📅 **Real-time system clock**
- 📁 **File status detection** (NEW/EXISTING with line count)
- 🏷️ **GPL v3 license information**
- 💻 **Platform compatibility display**

## Installation & Compilation

### DOS Compilation
```bash
# Using Turbo C
tcc -ml EVILINED.C -o EVILINED.EXE

# Using Borland C++
bcc -ml EVILINED.C -o EVILINED.EXE

# Using Microsoft C
cl /AL EVILINED.C /Fe EVILINED.EXE
```

### Build Options
```bash
# Debug build with Turbo C
tcc -ml -v EVILINED.C -o EVILINED.EXE

# Optimized build with Turbo C
tcc -ml -O EVILINED.C -o EVILINED.EXE
```

## Basic Usage

### Starting the Editor
```bash
# Start with new file
EVILINED.EXE

# Open existing file
EVILINED.EXE myfile.txt

# From DOS prompt
C:\> EVILINED myfile.c
```

### Basic Workflow
1. **Start editor** with optional filename
2. **Use line commands** for basic editing
3. **Switch to visual mode** with `V` command
4. **Save work** with `W` command or F2 in visual mode
5. **Exit** with `Q` command or ESC in visual mode

## Command Reference

### Line Mode Commands

| Command | Syntax | Description | Example |
|---------|--------|-------------|---------|
| `L` | `L [a][,b]` | List lines (range optional) | `L 1,10` |
| `I` | `I [n]` | Insert at line n | `I 5` |
| `D` | `D a[,b]` | Delete lines in range | `D 3,7` |
| `E` | `E n` | Edit (replace) single line | `E 10` |
| `R` | `R a[,b] /old/new/[g]` | Replace text | `R 1,5 /foo/bar/g` |
| `S` | `S [a][,b] /text/` | Search (case-insensitive) | `S /hello/` |
| `O` | `O name` | Open (load) file | `O test.c` |
| `W` | `W [name]` | Write (save) file | `W backup.txt` |
| `V` | `V` | Enter visual mode | `V` |
| `P` | `P` | Print status | `P` |
| `H` or `?` | `H` | Help | `?` |
| `Q` | `Q` | Quit | `Q` |

### Visual Mode Keys

| Key | Action | Description |
|-----|--------|-------------|
| `↑↓←→` | Navigate | Move cursor |
| `Home` | Line start | Beginning of line |
| `End` | Line end | End of line |
| `PgUp/PgDn` | Page scroll | Scroll page up/down |
| `Enter` | New line | Insert new line |
| `Backspace` | Delete back | Delete previous character |
| `Delete` | Delete forward | Delete current character |
| `Tab` | Insert spaces | Insert 8 spaces |
| `F1` | Help | Show help screen |
| `F2` | Save | Save current file |
| `ESC` | Exit | Return to line mode |

## Visual Mode

Visual mode provides a full-screen editing experience similar to modern text editors:

### Features
- **Real-time cursor positioning**
- **File type detection in status bar**
- **Line/column indicators**
- **Direct character input**
- **Immediate visual feedback**

### Status Bar Information
```
F1=Help F2=Save ESC=Exit | Line 15/234 Col 42 | myfile.c | C source file
```

## Customization Guide

### Adding New File Types

To add support for new file extensions, modify the `get_file_type()` function:

```c
static const char *get_file_type(const char *filename)
{
    const char *ext;
    
    if (!filename || !filename[0])
        return "";
    
    ext = strrchr(filename, '.');
    if (!ext) return "";
    ext++; /* Skip the dot */
    
    /* Add your new file types here */
    
    /* Python */
    if (strcasecmp(ext, "PY") == 0)
    {
        return "PYTHON source file";
    }
    
    /* JavaScript */
    if (strcasecmp(ext, "JS") == 0)
    {
        return "JAVASCRIPT source file";
    }
    
    /* Rust */
    if (strcasecmp(ext, "RS") == 0)
    {
        return "RUST source file";
    }
    
    /* Go */
    if (strcasecmp(ext, "GO") == 0)
    {
        return "GO source file";
    }
    
    /* Shell Scripts */
    if (strcasecmp(ext, "SH") == 0 || strcasecmp(ext, "BASH") == 0)
    {
        return "SHELL script";
    }
    
    /* Configuration Files */
    if (strcasecmp(ext, "JSON") == 0)
    {
        return "JSON data file";
    }
    
    if (strcasecmp(ext, "XML") == 0)
    {
        return "XML document";
    }
    
    if (strcasecmp(ext, "YAML") == 0 || strcasecmp(ext, "YML") == 0)
    {
        return "YAML configuration";
    }
    
    /* Web Files */
    if (strcasecmp(ext, "HTML") == 0 || strcasecmp(ext, "HTM") == 0)
    {
        return "HTML document";
    }
    
    if (strcasecmp(ext, "CSS") == 0)
    {
        return "CSS stylesheet";
    }
    
    /* Database */
    if (strcasecmp(ext, "SQL") == 0)
    {
        return "SQL script";
    }
    
    /* Existing code continues... */
}
```

### Customizing the Banner

To modify the startup banner, edit the `banner()` function:

```c
static void banner(const char *fname)
{
    char upper[128];
    char time_str[16];
    char file_status[64];
    /* ... existing code ... */
    
    /* Customize your banner here */
    puts("=================================================================");
    puts("              Y O U R   C U S T O M   E D I T O R               ");
    puts("=================================================================");
    puts("   Version 3.0 Custom Edition    (C) 2025 Your Name Here        ");
    puts("          Licensed under GNU GPL v3 - Free Software              ");
    puts("-----------------------------------------------------------------");
    puts("   Your Custom Description Here - Advanced Text Editor           ");
    puts("   Compatible: Your Target Platforms Here                        ");
    puts("-----------------------------------------------------------------");
    
    /* Add custom information */
    printf("              Active File  :  %-45s\n", upper);
    printf("              File Status  :  %-45s\n", file_status);
    printf("              System Time  :  %-45s\n", time_str);
    printf("              Editor Mode  :  %-45s\n", "Enhanced EVILINED");
    
    puts("-----------------------------------------------------------------");
    puts("   Features: Your Custom Feature List Here                       ");
    puts("=================================================================");
    puts("         Ready.  Type '?' for Help or 'V' for Visual Mode.       ");
    puts("=================================================================");
}
```

### Adding New Commands

To add a new command, modify the main command switch in `main()`:

```c
switch (cmd)
{
    /* Existing commands... */
    
    case 'T':  /* New 'T' command for word count */
    {
        int words = 0, chars = 0;
        int i, j;
        
        for (i = 0; i < line_count; i++)
        {
            if (lines[i])
            {
                chars += strlen(lines[i]);
                
                /* Count words */
                int in_word = 0;
                for (j = 0; lines[i][j]; j++)
                {
                    if (isspace(lines[i][j]))
                    {
                        in_word = 0;
                    }
                    else if (!in_word)
                    {
                        in_word = 1;
                        words++;
                    }
                }
            }
        }
        
        printf("Statistics: %d lines, %d words, %d characters\n", 
               line_count, words, chars);
        break;
    }
    
    case 'F':  /* New 'F' command for find and list */
    {
        if (!*p)
        {
            puts("! need F /pattern/");
            break;
        }
        
        char pattern[LINE_LEN];
        if (*p == '/')
        {
            const char *end = parse_between(p, '/', pattern, sizeof(pattern));
            if (!end)
            {
                puts("! syntax: F /pattern/");
                break;
            }
        }
        else
        {
            strncpy(pattern, p, sizeof(pattern) - 1);
            pattern[sizeof(pattern) - 1] = 0;
        }
        
        int found = 0;
        for (int i = 0; i < line_count; i++)
        {
            if (lines[i] && strcasestr_pos(lines[i], pattern) >= 0)
            {
                printf("%05d: %s\n", i, lines[i]);
                found++;
            }
        }
        
        printf("-- Found %d matching lines\n", found);
        break;
    }
}
```

### Customizing Visual Mode Colors

For DOS version, modify the `textattr()` calls:

```c
/* Custom color scheme */
#define NORMAL_ATTR     0x07    /* White on black */
#define STATUS_ATTR     0x70    /* Black on white */
#define HIGHLIGHT_ATTR  0x0F    /* Bright white on black */
#define ERROR_ATTR      0x4F    /* Bright white on red */

static void draw_screen(void)
{
    /* ... existing code ... */
    
    /* Status line with custom colors */
    gotoxy(1, SCREEN_ROWS);
    textattr(STATUS_ATTR);
    printf(" F1=Help F2=Save ESC=Exit | Line %d/%d Col %d | %s",
           cursor_row + 1, line_count, cursor_col + 1,
           current_file[0] ? current_file : "(none)");
    
    textattr(NORMAL_ATTR); /* Reset to normal */
}
```

### Adding Syntax Highlighting (Basic)

For simple syntax highlighting in visual mode:

```c
static void draw_line_with_syntax(int line_idx, int screen_row)
{
    if (line_idx >= line_count || !lines[line_idx])
        return;
    
    char *line = lines[line_idx];
    int len = strlen(line);
    const char *file_ext = get_file_extension(current_file);
    
    gotoxy(1, screen_row);
    
    if (strcasecmp(file_ext, "C") == 0 || strcasecmp(file_ext, "H") == 0)
    {
        /* Simple C syntax highlighting */
        for (int i = 0; i < len && i < SCREEN_COLS; i++)
        {
            char c = line[i];
            
            /* Keywords */
            if (strncmp(&line[i], "int", 3) == 0 ||
                strncmp(&line[i], "char", 4) == 0 ||
                strncmp(&line[i], "void", 4) == 0)
            {
                textattr(0x0E); /* Yellow */
            }
            /* Comments */
            else if (strncmp(&line[i], "/*", 2) == 0 ||
                     strncmp(&line[i], "//", 2) == 0)
            {
                textattr(0x02); /* Green */
            }
            /* Strings */
            else if (c == '"')
            {
                textattr(0x0C); /* Red */
            }
            else
            {
                textattr(0x07); /* Normal */
            }
            
            printf("%c", c);
        }
    }
    else
    {
        /* No syntax highlighting */
        textattr(0x07);
        printf("%.80s", line);
    }
    
    textattr(0x07); /* Reset */
}
```

## File Type Extensions

### Currently Supported Types

The editor automatically detects and displays file types for:

**Programming Languages:**
- C/C++ (`.c`, `.h`, `.cpp`, `.cxx`, `.cc`, `.hpp`, `.hxx`)
- Pascal (`.pas`)
- FORTRAN (`.for`, `.ftn`, `.f77`, `.f`, `.f90`, `.f95`)
- Assembly (`.asm`, `.s`)
- BASIC (`.bas`)
- COBOL (`.cob`, `.cbl`)
- PL/I (`.pli`, `.pl1`)
- PL/M (`.plm`)
- ALGOL (`.alg`, `.algol`)
- Subroutines (`.sub`, `.sbr`)

**Scripts & Batch:**
- DOS Batch (`.bat`)
- Command Scripts (`.cmd`)

**Documentation:**
- Text files (`.txt`)
- Documents (`.doc`)
- Markdown (`.md`)

**Data & Config:**
- Data files (`.dat`)
- Configuration (`.ini`, `.cfg`)

**Binary & Executable:**
- Intel HEX (`.hex`)
- Binary (`.bin`)
- DOS Executables (`.com`, `.exe`)
- Object files (`.obj`)
- Libraries (`.lib`)

**Build Systems:**
- Makefiles (`.mak`)

### Adding Custom File Types

Use this template to add new file type detection:

```c
/* Your Custom Language */
if (strcasecmp(ext, "YOUREXT") == 0)
{
    return "YOUR LANGUAGE source file";
}

/* Multiple extensions for same type */
if (strcasecmp(ext, "EXT1") == 0 || 
    strcasecmp(ext, "EXT2") == 0 ||
    strcasecmp(ext, "EXT3") == 0)
{
    return "YOUR FILE TYPE";
}
```

## Advanced Modifications

### Adding Line Numbering Options

```c
/* Add to global variables */
static int show_line_numbers = 1;
static int line_number_width = 5;

/* Modify cmd_list function */
static void cmd_list(int a, int b)
{
    int i;
    to_range_defaults(&a, &b);
    
    if (line_count == 0)
    {
        puts("(empty)");
        return;
    }
    
    for (i = a; i <= b; i++)
    {
        if (i >= 1 && i <= line_count)
        {
            if (show_line_numbers)
            {
                printf("%0*d: %s\n", line_number_width, i - 1, 
                       lines[i - 1] ? lines[i - 1] : "");
            }
            else
            {
                printf("%s\n", lines[i - 1] ? lines[i - 1] : "");
            }
        }
    }
    
    last_a = a;
    last_b = b;
}

/* Add command to toggle line numbers */
case 'N':  /* Toggle line numbers */
{
    show_line_numbers = !show_line_numbers;
    printf("Line numbers %s\n", show_line_numbers ? "ON" : "OFF");
    break;
}
```

### Adding Auto-Save Feature

```c
/* Add to global variables */
static int auto_save_enabled = 0;
static int auto_save_interval = 10; /* Save every 10 changes */
static int changes_since_save = 0;

/* Add auto-save function */
static void auto_save_check(void)
{
    if (auto_save_enabled && current_file[0] && 
        changes_since_save >= auto_save_interval)
    {
        if (write_file(current_file))
        {
            printf("-- Auto-saved to %s\n", current_file);
            changes_since_save = 0;
        }
    }
}

/* Call after each modification */
static void mark_modified(void)
{
    changes_since_save++;
    auto_save_check();
}

/* Add command to enable auto-save */
case 'A':  /* Auto-save toggle */
{
    if (*p)
    {
        auto_save_interval = atoi(p);
        auto_save_enabled = 1;
        printf("Auto-save enabled every %d changes\n", auto_save_interval);
    }
    else
    {
        auto_save_enabled = !auto_save_enabled;
        printf("Auto-save %s\n", auto_save_enabled ? "ON" : "OFF");
    }
    break;
}
```

### Adding Undo Functionality

```c
/* Undo system structures */
#define MAX_UNDO_LEVELS 50

typedef struct undo_state {
    char **lines_backup;
    int line_count_backup;
    int cursor_row_backup;
    int cursor_col_backup;
} undo_state_t;

static undo_state_t undo_stack[MAX_UNDO_LEVELS];
static int undo_level = 0;
static int max_undo_level = 0;

/* Save current state for undo */
static void save_undo_state(void)
{
    if (undo_level >= MAX_UNDO_LEVELS - 1)
    {
        /* Shift stack down */
        for (int i = 0; i < MAX_UNDO_LEVELS - 1; i++)
        {
            undo_stack[i] = undo_stack[i + 1];
        }
        undo_level = MAX_UNDO_LEVELS - 1;
    }
    
    /* Save current state */
    undo_state_t *state = &undo_stack[undo_level];
    
    /* Allocate and copy lines */
    state->lines_backup = malloc(sizeof(char*) * MAX_LINES);
    for (int i = 0; i < line_count; i++)
    {
        state->lines_backup[i] = lines[i] ? xstrdup(lines[i]) : NULL;
    }
    
    state->line_count_backup = line_count;
    state->cursor_row_backup = cursor_row;
    state->cursor_col_backup = cursor_col;
    
    undo_level++;
    max_undo_level = undo_level;
}

/* Undo command */
case 'U':  /* Undo */
{
    if (undo_level > 0)
    {
        undo_level--;
        undo_state_t *state = &undo_stack[undo_level];
        
        /* Free current lines */
        for (int i = 0; i < line_count; i++)
        {
            free_line(i);
        }
        
        /* Restore from backup */
        for (int i = 0; i < state->line_count_backup; i++)
        {
            lines[i] = state->lines_backup[i] ? 
                      xstrdup(state->lines_backup[i]) : NULL;
        }
        
        line_count = state->line_count_backup;
        cursor_row = state->cursor_row_backup;
        cursor_col = state->cursor_col_backup;
        
        printf("-- Undone (level %d)\n", undo_level);
    }
    else
    {
        puts("! Nothing to undo");
    }
    break;
}
```

## Platform Support

### DOS (16-bit)
- **Compiler**: Turbo C 2.0+, Borland C++ 3.0+, Microsoft C 6.0+
- **Memory Model**: Large model recommended (`-ml`)
- **Features**: Full conio.h support, direct video memory access
- **Requirements**: 512KB RAM minimum, 640KB recommended
- **Compatible Systems**: IBM-PC, XT, AT, PS/2, and compatibles
- **DOS Versions**: MS-DOS 3.0+, PC-DOS 3.0+, DR-DOS 5.0+

### Hardware Requirements
- **CPU**: **8088/8086 or higher** (including NEC V20/V30)
- **Memory**: 512KB RAM (640KB recommended)
- **Display**: CGA, EGA, VGA, or Hercules compatible
- **Storage**: 360KB floppy disk or hard drive

### 8088/NEC V20 Compatibility Advantage
**EVILINED stands apart from other Vi clones** which typically require:
- 80286 or higher processors
- Extended memory (XMS/EMS)
- VGA graphics capabilities
- Hard disk storage

**EVILINED runs perfectly on:**
- **Original IBM PC (8088 @ 4.77MHz)**
- **IBM PC/XT with 8088**
- **NEC V20/V30 processor upgrades**
- **Tandy 1000 series**
- **Amstrad PC1512/1640**
- **Any 8088-based PC clone**

### DOS-Specific Features
- **Direct video memory access** for fast screen updates
- **BIOS interrupt support** for keyboard and display
- **Memory-resident operation** with minimal footprint
- **8088-optimized assembly routines** for maximum performance
- **No FPU requirements** (works without math coprocessor)
- **Compatible with DOS extenders** (DPMI, VCPI)
- **TSR-friendly** design for background operation

### Why Choose EVILINED Over Other Vi Clones?

| Feature | EVILINED | Typical Vi Clones |
|---------|----------|-------------------|
| **CPU Requirement** | 8088/NEC V20 | 80286+ |
| **Memory Usage** | 64KB base | 256KB+ |
| **Graphics** | CGA/Hercules | VGA required |
| **Storage** | Floppy disk | Hard disk |
| **Speed on 8088** | Optimized | Sluggish/Won't run |
| **Authentic Retro** | ✅ Perfect | ❌ Too modern |

## License

**EVILINED** is licensed under the GNU General Public License v3.0.

```
Copyright (C) 2025 Mickey Lawless

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
```

## Contributing

When contributing to EVILINED:

1. **Maintain DOS compatibility** with 16-bit compilers
2. **Follow existing code style** and naming conventions
3. **Test thoroughly** on DOS systems and emulators
4. **Document changes** in this README
5. **Preserve GPL v3 licensing** in all derivative works

### Code Style Guidelines

```c
/* Function naming: snake_case */
static void my_function(void)

/* Variable naming: snake_case */
int line_count = 0;

/* Constants: UPPER_CASE */
#define MAX_LINES 1200

/* Indentation: 4 spaces, no tabs */
if (condition)
{
    statement;
}

/* Comments: C-style for multi-line */
/*
 * Multi-line comment
 * with proper formatting
 */

/* Comments: C++-style for single line */
// Single line comment
```

---

**EVILINED** - Where classic computing meets modern functionality.
