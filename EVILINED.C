/* ------------------------------------------------------
 * EviLinEd - Full EDLIN-like Line Editor 
 * Compatible with Turbo C (DOS)
 *
 * Copyright (C) 2025 M. Lawless
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Features:
 *  - Original full implementation logic retained
 *  - Zero-padded line numbers: 00000, 00001, ...
 *  - Banner with uppercase filename and system time
 *  - Case-insensitive commands
 *  - Status line after every command
 *  - Multi-line insert mode
 *  - Replace/Search with /old/new/[g] syntax
 *  - Range parsing, memory safety, last_a/last_b tracking
 *  - **Each input line is preceded by its current line number**
 *  - Visual input mode, emulating vi interface.
 *  - Source file type identification in visual mode 
 *  - 
 * ------------------------------------------------------ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <conio.h>
#include <dos.h>

#define MAX_LINES 1200
#define LINE_LEN  256
#define INPUT_LEN 512
#define SCREEN_ROWS 24
#define SCREEN_COLS 80

static char *lines[MAX_LINES];
static int   line_count = 0;
static char  current_file[128] = "";
static int   last_a = 1;
static int   last_b = 0; /* last used range; b==0 means unset */
static int   cursor_row = 0;
static int   cursor_col = 0;
static int   top_line = 0;
static unsigned int video_segment = 0xB800;

/* -------- utility -------- */

static void detect_video_adapter(void)
{
    unsigned char far *bios_data;
    
    /* Check BIOS data area at 0040:0049 for video mode */
    bios_data = MK_FP(0x0040, 0x0049);
    
    /* If mode is 7, it's monochrome (MDA/Hercules) */
    if (*bios_data == 7)
    {
        video_segment = 0xB000; /* Monochrome */
    }
    else
    {
        video_segment = 0xB800; /* Color (CGA/EGA/VGA) */
    }
}

static int strcasecmp(const char *s1, const char *s2)
{
    while (*s1 && *s2)
    {
        int c1 = tolower((unsigned char) *s1);
        int c2 = tolower((unsigned char) *s2);
        if (c1 != c2)
        {
            return c1 - c2;
        }
        s1++;
        s2++;
    }
    return tolower((unsigned char) *s1) - tolower((unsigned char) *s2);
}

static void get_time_string(char *buffer, size_t bufsize)
{
    time_t rawtime;
    struct tm *timeinfo;
    
    (void) bufsize; /* suppress warning */
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    /* Format: HH:MM:SS (24-hour format, MS-DOS compatible) */
    sprintf(buffer, "%02d:%02d:%02d", 
            timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}

static void chomp(char *s)
{
    size_t n = strlen(s);

    if (n && (s[n - 1] == '\n' || s[n - 1] == '\r'))
    {
        s[--n] = '\0';
    }

    if (n && s[n - 1] == '\r')
    {
        s[--n] = '\0';
    }
}

/* Lowercase search */

static int strcasestr_pos(const char *hay, const char *needle)
{
    size_t i, j;
    size_t H = strlen(hay);
    size_t N = strlen(needle);

    if (N == 0)
    {
        return 0;
    }

    for (i = 0; i + N <= H; ++i)
    {
        for (j = 0; j < N; ++j)
        {
            char a = (char) tolower((unsigned char) hay[i + j]);
            char b = (char) tolower((unsigned char) needle[j]);

            if (a != b)
            {
                break;
            }
        }

        if (j == N)
        {
            return (int) i;
        }
    }

    return -1;
}

static void to_range_defaults(int *a, int *b)
{
    if (*a < 1)
    {
        *a = 1;
    }

    if (*b < 1 || *b > line_count)
    {
        *b = line_count;
    }

    if (*a > *b && line_count > 0)
    {
        int t = *a;
        *a = *b;
        *b = t;
    }
}

static char *xstrdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = (char *) malloc(n);

    if (!p)
    {
        return NULL;
    }

    memcpy(p, s, n);

    return p;
}

static void free_line(int idx)
{
    if (idx >= 0 && idx < line_count && lines[idx])
    {
        free(lines[idx]);
        lines[idx] = NULL;
    }
}

/* shift lines up/down to make/remove space */

static int make_room(int pos, int count)
{
    int i;

    if (count <= 0)
    {
        return 1;
    }

    if (line_count + count > MAX_LINES)
    {
        return 0;
    }

    for (i = line_count - 1; i >= pos; --i)
    {
        lines[i + count] = lines[i];
    }

    line_count += count;

    return 1;
}

static void close_gap(int start, int count)
{
    int i;

    for (i = start; i + count < line_count; ++i)
    {
        lines[i] = lines[i + count];
    }

    line_count -= count;
}

/* replace old->new in line */

static int replace_in_line(char **s_ptr, const char *oldp, const char *newp, int global)
{
    char *s = *s_ptr;
    size_t so = strlen(oldp);
    size_t sn = strlen(newp);
    int made = 0;
    int limit = 1024;

    if (so == 0)
    {
        return 0;
    }

    for (;;)
    {
        char *found = strstr(s, oldp);

        if (!found)
        {
            break;
        }

        {
            size_t prefix = (size_t) (found - s);
            size_t suffix = strlen(found + so);

            if (prefix + sn + suffix + 1 >= LINE_LEN)
            {
                break;
            }

            {
                char *buf = (char *) malloc(LINE_LEN);

                if (!buf)
                {
                    break;
                }

                memcpy(buf, s, prefix);
                memcpy(buf + prefix, newp, sn);
                memcpy(buf + prefix + sn, found + so, suffix + 1);
                free(s);
                s = buf;
                *s_ptr = s;
                ++made;
            }
        }

        if (!global)
        {
            break;
        }

        if (--limit <= 0)
        {
            break;
        }
    }

    return made;
}

/* parse range like a,b */

static int parse_range(const char *p, int *a, int *b)
{
    int x = -1;
    int y = -1;
    const char *c = p;

    while (isspace((unsigned char) *c))
    {
        ++c;
    }

    if (*c == '\0')
    {
        *a = 1;
        *b = line_count;
        return 1;
    }

    if (*c == ',')
    {
        ++c;
        y = atoi(c);
        *a = 1;
        *b = (y > 0 ? y : line_count);
        return 1;
    }

    if (isdigit((unsigned char) *c))
    {
        x = atoi(c);

        while (isdigit((unsigned char) *c))
        {
            ++c;
        }

        while (isspace((unsigned char) *c))
        {
            ++c;
        }

        if (*c == ',')
        {
            ++c;

            while (isspace((unsigned char) *c))
            {
                ++c;
            }

            y = (*c ? atoi(c) : line_count);
        }
        else
        {
            y = x;
        }

        *a = (x > 0 ? x : 1);
        *b = (y > 0 ? y : line_count);

        return 1;
    }

    return 0;
}

static const char *parse_between(const char *p, char delim, char *out, size_t outsz)
{
    size_t n = 0;

    if (*p != delim)
    {
        return NULL;
    }

    ++p;

    while (*p && *p != delim)
    {
        if (n + 1 < outsz)
        {
            out[n++] = *p;
        }

        ++p;
    }

    if (*p != delim)
    {
        return NULL;
    }

    out[n] = '\0';
    return p + 1;
}

/* -------- file ops -------- */

static int load_file(const char *name)
{
    FILE *f = fopen(name, "rt");
    char buf[LINE_LEN];
    int i;

    if (!f)
    {
        return 0;
    }

    for (i = 0; i < line_count; ++i)
    {
        free_line(i);
    }

    line_count = 0;

    while (fgets(buf, sizeof(buf), f))
    {
        chomp(buf);

        if (!(lines[line_count] = xstrdup(buf)))
        {
            fclose(f);
            return 0;
        }

        if (++line_count >= MAX_LINES)
        {
            fclose(f);
            return 0;
        }
    }

    fclose(f);
    strncpy(current_file, name, sizeof(current_file) - 1);
    current_file[sizeof(current_file) - 1] = 0;
    last_a = 1;
    last_b = line_count;

    return 1;
}

static int write_file(const char *name)
{
    FILE *f = fopen(name, "wt");
    int i;

    if (!f)
    {
        return 0;
    }

    for (i = 0; i < line_count; i++)
    {
        fputs(lines[i] ? lines[i] : "", f);
        fputc('\n', f);
    }

    fclose(f);
    strncpy(current_file, name, sizeof(current_file) - 1);
    current_file[sizeof(current_file) - 1] = 0;

    return 1;
}

/* -------- commands -------- */

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
            printf("%05d: %s\n", i - 1, lines[i - 1] ? lines[i - 1] : "");
        }
    }

    last_a = a;
    last_b = b;
}

static void cmd_delete(int a, int b)
{
    int count;
    int i;

    to_range_defaults(&a, &b);

    if (a < 1)
    {
        a = 1;
    }

    if (b > line_count)
    {
        b = line_count;
    }

    if (line_count == 0 || a > b)
    {
        return;
    }

    for (i = a - 1; i <= b - 1; i++)
    {
        free_line(i);
    }

    count = b - a + 1;
    close_gap(a - 1, count);
    last_a = a;
    last_b = (a <= line_count) ? a : line_count;
}

static void cmd_insert(int n)
{
    char buf[LINE_LEN];
    int pos;

    if (n < 1 || n > line_count + 1)
    {
        n = line_count + 1;
    }

    pos = n - 1;
    printf("-- Insert at  Line %05d  --\n", n - 1);

    for (;;)
    {
        printf("%05d: ", pos + 1);

        if (!fgets(buf, sizeof(buf), stdin))
        {
            break;
        }

        chomp(buf);

        if (strcmp(buf, ".") == 0)
        {
            break;
        }

        if (!make_room(pos, 1))
        {
            puts("! out of space");
            break;
        }

        lines[pos] = xstrdup(buf);

        if (!lines[pos])
        {
            puts("! alloc failed");
            break;
        }

        pos++;
    }

    last_a = n;
    last_b = pos;
}

static void cmd_edit(int n)
{
    char buf[LINE_LEN];

    if (n < 1 || n > line_count)
    {
        puts("! bad line");
        return;
    }

    printf("%05d: %s\n", n - 1, lines[n - 1] ? lines[n - 1] : "");
    printf("%05d: ", n);

    if (!fgets(buf, sizeof(buf), stdin))
    {
        return;
    }

    chomp(buf);
    free_line(n - 1);
    lines[n - 1] = xstrdup(buf);

    if (!lines[n - 1])
    {
        puts("! alloc failed");
    }

    last_a = n;
    last_b = n;
}

static void cmd_replace(int a, int b, const char *spec)
{
    char oldp[LINE_LEN];
    char newp[LINE_LEN];
    int global = 0;
    const char *p = spec;
    int i;
    int total = 0;

    while (isspace((unsigned char) *p))
    {
        ++p;
    }

    p = parse_between(p, '/', oldp, sizeof(oldp));

    if (!p)
    {
        puts("! syntax: R a,b /old/new/[g]");
        return;
    }

    while (isspace((unsigned char) *p))
    {
        ++p;
    }

    p = parse_between(p, '/', newp, sizeof(newp));

    if (!p)
    {
        puts("! syntax: R a,b /old/new/[g]");
        return;
    }

    while (isspace((unsigned char) *p))
    {
        ++p;
    }

    if (*p == 'g' || *p == 'G')
    {
        global = 1;
    }

    to_range_defaults(&a, &b);

    for (i = a; i <= b; i++)
    {
        if (i >= 1 && i <= line_count && lines[i - 1])
        {
            total += replace_in_line(&lines[i - 1], oldp, newp, global);
        }
    }

    printf("Replaced %d occurrence(s).\n", total);
    last_a = a;
    last_b = b;
}

static void cmd_search(int a, int b, const char *spec)
{
    char pat[LINE_LEN];
    const char *p = spec;
    int i;
    int hits = 0;

    while (isspace((unsigned char) *p))
    {
        ++p;
    }

    if (*p == '/')
    {
        p = parse_between(p, '/', pat, sizeof(pat));

        if (!p)
        {
            puts("! syntax: S a,b /text/");
            return;
        }
    }
    else
    {
        size_t n = 0;

        while (*p && isspace((unsigned char) *p))
        {
            ++p;
        }

        while (*p && n + 1 < sizeof(pat))
        {
            pat[n++] = *p++;
        }

        pat[n] = 0;
    }

    to_range_defaults(&a, &b);

    for (i = a; i <= b; i++)
    {
        if (i >= 1 && i <= line_count && lines[i - 1])
        {
            if (strcasestr_pos(lines[i - 1], pat) >= 0)
            {
                printf("%05d: %s\n", i - 1, lines[i - 1]);
                hits++;
            }
        }
    }

    printf("-- %d match(es)\n", hits);
    last_a = a;
    last_b = b;
}

/* -------- fullscreen editor -------- */

static const char *get_file_type(const char *filename)
{
    const char *ext;
    int i;
    
    if (!filename || !filename[0])
    {
        return "";
    }
    
    /* Find last dot */
    ext = strrchr(filename, '.');
    if (!ext)
    {
        return "";
    }
    
    ext++; /* Skip the dot */
    
    /* FORTRAN variants */
    if (strcasecmp(ext, "FOR") == 0 || strcasecmp(ext, "FTN") == 0 ||
        strcasecmp(ext, "F77") == 0 || strcasecmp(ext, "F") == 0 ||
        strcasecmp(ext, "F90") == 0 || strcasecmp(ext, "F95") == 0)
    {
        return "FORTRAN source file";
    }
    
    /* Assembly */
    if (strcasecmp(ext, "ASM") == 0 || strcasecmp(ext, "S") == 0)
    {
        return "ASSEMBLER source file";
    }

    /* SUBROUTINE */
    if (strcasecmp(ext, "SUB") == 0 || strcasecmp(ext, "SBR") == 0)
    {
	return "SUBROUTINE source file";
    }

    /* C/C++ */
    if (strcasecmp(ext, "C") == 0)
    {
        return "C source file";
    }
    if (strcasecmp(ext, "H") == 0)
    {
        return "C header file";
    }
    if (strcasecmp(ext, "CPP") == 0 || strcasecmp(ext, "CXX") == 0 ||
        strcasecmp(ext, "CC") == 0)
    {
        return "C++ source file";
    }
    if (strcasecmp(ext, "HPP") == 0 || strcasecmp(ext, "HXX") == 0)
    {
        return "C++ header file";
    }
    
    /* Pascal */
    if (strcasecmp(ext, "PAS") == 0)
    {
        return "PASCAL source file";
    }
    
    /* BASIC */
    if (strcasecmp(ext, "BAS") == 0)
    {
        return "BASIC source file";
    }
    
    /* COBOL */
    if (strcasecmp(ext, "COB") == 0 || strcasecmp(ext, "CBL") == 0)
    {
        return "COBOL source file";
    }
    
    /* PL/I */
    if (strcasecmp(ext, "PLI") == 0 || strcasecmp(ext, "PL1") == 0)
    {
        return "PL/I source file";
    }
    
    /* PL/M */
    if (strcasecmp(ext, "PLM") == 0)
    {
        return "PL/M source file";
    }
    
    /* ALGOL */
    if (strcasecmp(ext, "ALG") == 0 || strcasecmp(ext, "ALGOL") == 0)
    {
        return "ALGOL source file";
    }
    
    /* Batch/Script */
    if (strcasecmp(ext, "BAT") == 0)
    {
        return "DOS batch file";
    }
    if (strcasecmp(ext, "CMD") == 0)
    {
        return "Command script";
    }
    
    /* Documentation */
    if (strcasecmp(ext, "TXT") == 0)
    {
        return "Text file";
    }
    if (strcasecmp(ext, "DOC") == 0)
    {
        return "Document file";
    }
    if (strcasecmp(ext, "MD") == 0)
    {
        return "Markdown file";
    }
    
    /* Data files */
    if (strcasecmp(ext, "DAT") == 0)
    {
        return "Data file";
    }
    if (strcasecmp(ext, "INI") == 0 || strcasecmp(ext, "CFG") == 0)
    {
        return "Configuration file";
    }
    
    /* Binary/Hex formats */
    if (strcasecmp(ext, "HEX") == 0)
    {
        return "Intel HEX file";
    }
    if (strcasecmp(ext, "BIN") == 0)
    {
        return "Binary file";
    }
    if (strcasecmp(ext, "COM") == 0)
    {
        return "DOS executable";
    }
    if (strcasecmp(ext, "EXE") == 0)
    {
        return "DOS executable";
    }
    if (strcasecmp(ext, "OBJ") == 0)
    {
        return "Object file";
    }
    if (strcasecmp(ext, "LIB") == 0)
    {
        return "Library file";
    }
    
    /* Makefiles */
    if (strcasecmp(ext, "MAK") == 0)
    {
        return "Makefile";
    }
    
    return "";
}

static void draw_screen(void)
{
    int i, row;
    const char *file_type;
    
    clrscr();
    
    /* Draw lines */
    for (row = 0; row < SCREEN_ROWS - 1; row++)
    {
        int line_idx = top_line + row;
        gotoxy(1, row + 1);
        
        if (line_idx < line_count)
        {
            char display[SCREEN_COLS + 1];
            int len = strlen(lines[line_idx]);
            
            if (len > SCREEN_COLS)
            {
                len = SCREEN_COLS;
            }
            
            memcpy(display, lines[line_idx], len);
            display[len] = '\0';
            printf("%s", display);
        }
        else
        {
            printf("~");
        }
    }
    
    /* Status line */
    gotoxy(1, SCREEN_ROWS);
    textattr(0x70); /* reverse video */
    printf(" F1=Help F2=Save ESC=Exit | Line %d/%d Col %d | %s",
	   cursor_row + 1, line_count, cursor_col + 1,
	   current_file[0] ? current_file : "(none)");

    /* File type in bottom right corner */
    file_type = get_file_type(current_file);
    if (file_type[0])
    {
        int type_len = strlen(file_type);
        int pos = SCREEN_COLS - type_len;
        if (pos > wherex())
        {
            gotoxy(pos, SCREEN_ROWS);
            printf("%s", file_type);
        }
    }
    
    /* Pad rest of status line */
    {
        int x = wherex();
        while (x < SCREEN_COLS)
        {
            printf(" ");
            x++;
        }
    }
    
    textattr(0x07); /* normal */
    
    /* Position cursor */
    gotoxy(cursor_col + 1, cursor_row - top_line + 1);
}

static void write_char_at_cursor(char c)
{
    int screen_y = cursor_row - top_line + 1;
    char far *video;
    int offset;
    
    /* Direct video memory write for single character */
    /* cursor_col has already been incremented by insert_char() */
    /* so write at cursor_col - 1 */
    video = MK_FP(video_segment, 0);
    offset = ((screen_y - 1) * SCREEN_COLS + (cursor_col - 1)) * 2;
    
    video[offset] = c;
    video[offset + 1] = 0x07; /* White on black */
    
    /* Position cursor at new location */
    gotoxy(cursor_col + 1, screen_y);
}

static void draw_current_line(void)
{
    int screen_y = cursor_row - top_line + 1;
    char far *video;
    int len;
    int i;
    int offset;
    
    if (cursor_row >= line_count)
    {
        return;
    }
    
    len = strlen(lines[cursor_row]);
    if (len > SCREEN_COLS)
    {
        len = SCREEN_COLS;
    }
    
    /* Direct video memory write */
    video = MK_FP(video_segment, 0);
    offset = ((screen_y - 1) * SCREEN_COLS) * 2;
    
    /* Write line content */
    for (i = 0; i < len; i++)
    {
        video[offset + i * 2] = lines[cursor_row][i];
        video[offset + i * 2 + 1] = 0x07; /* White on black */
    }
    
    /* Pad with spaces */
    for (i = len; i < SCREEN_COLS; i++)
    {
        video[offset + i * 2] = ' ';
        video[offset + i * 2 + 1] = 0x07;
    }
    
    /* Restore cursor position */
    gotoxy(cursor_col + 1, screen_y);
}

static void update_status_line(void)
{
    char far *video;
    char status[SCREEN_COLS + 1];
    int len;
    int i;
    int offset;
    unsigned char attr;
    
    /* Build status string */
    len = sprintf(status, " F1=Help F2=Save F10=Exit | Ln %d/%d Col %d",
                  cursor_row + 1, line_count, cursor_col + 1);
    
    /* Pad to full width */
    for (i = len; i < SCREEN_COLS; i++)
    {
        status[i] = ' ';
    }
    
    /* Direct video memory write */
    video = MK_FP(video_segment, 0);
    offset = ((SCREEN_ROWS - 1) * SCREEN_COLS) * 2;
    
    /* Reverse video attribute (works on both MDA and CGA) */
    attr = 0x70; /* Black on white / reverse video */
    
    /* Write status line with reverse video attribute */
    for (i = 0; i < SCREEN_COLS; i++)
    {
        video[offset + i * 2] = status[i];
        video[offset + i * 2 + 1] = attr;
    }
    
    /* Don't move cursor - leave it where it is */
}

static void ensure_line_exists(int line_idx)
{
    while (line_count <= line_idx)
    {
        if (line_count >= MAX_LINES)
        {
            return;
        }
        
        lines[line_count] = xstrdup("");
        if (!lines[line_count])
        {
            return;
        }
        line_count++;
    }
}

static void insert_char(char c)
{
    int line_idx = cursor_row;
    char *line;
    int len;
    char *new_line;
    
    ensure_line_exists(line_idx);
    line = lines[line_idx];
    len = strlen(line);
    
    if (cursor_col > len)
    {
        cursor_col = len;
    }
    
    if (len >= LINE_LEN - 1)
    {
        return;
    }
    
    new_line = (char *) malloc(LINE_LEN);
    if (!new_line)
    {
        return;
    }
    
    memcpy(new_line, line, cursor_col);
    new_line[cursor_col] = c;
    memcpy(new_line + cursor_col + 1, line + cursor_col, len - cursor_col + 1);
    
    free(line);
    lines[line_idx] = new_line;
    cursor_col++;
}

static void delete_char(void)
{
    int line_idx = cursor_row;
    char *line;
    int len;
    
    if (line_idx >= line_count)
    {
        return;
    }
    
    line = lines[line_idx];
    len = strlen(line);
    
    if (cursor_col >= len)
    {
        /* Join with next line */
        if (line_idx + 1 < line_count)
        {
            char *next_line = lines[line_idx + 1];
            int next_len = strlen(next_line);
            
            if (len + next_len < LINE_LEN)
            {
                char *new_line = (char *) malloc(LINE_LEN);
                if (new_line)
                {
                    memcpy(new_line, line, len);
                    memcpy(new_line + len, next_line, next_len + 1);
                    free(line);
                    lines[line_idx] = new_line;
                    free_line(line_idx + 1);
                    close_gap(line_idx + 1, 1);
                }
            }
        }
    }
    else
    {
        memmove(line + cursor_col, line + cursor_col + 1, len - cursor_col);
    }
}

static void backspace_char(void)
{
    if (cursor_col > 0)
    {
        cursor_col--;
        delete_char();
    }
    else if (cursor_row > 0)
    {
        /* Move to end of previous line and join */
        cursor_row--;
        cursor_col = strlen(lines[cursor_row]);
        delete_char();
    }
}

static void insert_newline(void)
{
    int line_idx = cursor_row;
    char *line;
    int len;
    char *new_line;
    
    ensure_line_exists(line_idx);
    line = lines[line_idx];
    len = strlen(line);
    
    if (cursor_col > len)
    {
        cursor_col = len;
    }
    
    if (line_count >= MAX_LINES)
    {
        return;
    }
    
    if (!make_room(line_idx + 1, 1))
    {
        return;
    }
    
    new_line = (char *) malloc(LINE_LEN);
    if (!new_line)
    {
        return;
    }
    
    memcpy(new_line, line + cursor_col, len - cursor_col + 1);
    line[cursor_col] = '\0';
    lines[line_idx + 1] = new_line;
    
    cursor_row++;
    cursor_col = 0;
}

static void show_help_screen(void)
{
    clrscr();
    printf("=================================================================\n");
    printf("             LINED - FULLSCREEN EDITOR - HELP                    \n");
    printf("=================================================================\n\n");
    printf("  NAVIGATION:\n");
    printf("    Arrow Keys    - Move cursor\n");
    printf("    Home          - Beginning of line\n");
    printf("    End           - End of line\n");
    printf("    PgUp/PgDn     - Scroll page up/down\n\n");
    printf("  EDITING:\n");
    printf("    Type          - Insert characters\n");
    printf("    Tab           - Insert 8 spaces\n");
    printf("    Enter         - Insert new line\n");
    printf("    Backspace     - Delete previous character\n");
    printf("    Delete        - Delete current character\n\n");
    printf("  FILE OPERATIONS:\n");
    printf("    F2            - Save file\n");
    printf("    F10           - Exit to line mode\n\n");
    printf("=================================================================\n");
    printf("\n  Press any key to continue...");
    getch();
}

static void cmd_fullscreen(void)
{
    int running = 1;
    int ch;
    int need_full_redraw = 1;
    
    /* Detect video adapter type */
    detect_video_adapter();
    
    if (line_count == 0)
    {
        ensure_line_exists(0);
    }
    
    cursor_row = 0;
    cursor_col = 0;
    top_line = 0;
    
    while (running)
    {
        if (need_full_redraw)
        {
            draw_screen();
            need_full_redraw = 0;
        }
        
        ch = getch();
        
        if (ch == 0 || ch == 0xE0) /* Extended key */
        {
            ch = getch();
            
            switch (ch)
            {
                case 72: /* Up arrow */
                    if (cursor_row > 0)
                    {
                        cursor_row--;
                        if (cursor_row < top_line)
                        {
                            top_line = cursor_row;
                            need_full_redraw = 1;
                        }
                        else
                        {
                            gotoxy(cursor_col + 1, cursor_row - top_line + 1);
                        }
                        if (cursor_col > strlen(lines[cursor_row]))
                        {
                            cursor_col = strlen(lines[cursor_row]);
                            gotoxy(cursor_col + 1, cursor_row - top_line + 1);
                        }
                    }
                    break;
                    
                case 80: /* Down arrow */
                    if (cursor_row < line_count - 1)
                    {
                        cursor_row++;
                        if (cursor_row >= top_line + SCREEN_ROWS - 1)
                        {
                            top_line = cursor_row - SCREEN_ROWS + 2;
                            need_full_redraw = 1;
                        }
                        else
                        {
                            gotoxy(cursor_col + 1, cursor_row - top_line + 1);
                        }
                        if (cursor_col > strlen(lines[cursor_row]))
                        {
                            cursor_col = strlen(lines[cursor_row]);
                            gotoxy(cursor_col + 1, cursor_row - top_line + 1);
                        }
                    }
                    break;
                    
                case 75: /* Left arrow */
                    if (cursor_col > 0)
                    {
                        cursor_col--;
                        gotoxy(cursor_col + 1, cursor_row - top_line + 1);
                    }
                    else if (cursor_row > 0)
                    {
                        cursor_row--;
                        cursor_col = strlen(lines[cursor_row]);
                        if (cursor_row < top_line)
                        {
                            top_line = cursor_row;
                            need_full_redraw = 1;
                        }
                        else
                        {
                            gotoxy(cursor_col + 1, cursor_row - top_line + 1);
                        }
                    }
                    break;
                    
                case 77: /* Right arrow */
                    if (cursor_row < line_count)
                    {
                        int len = strlen(lines[cursor_row]);
                        if (cursor_col < len)
                        {
                            cursor_col++;
                            gotoxy(cursor_col + 1, cursor_row - top_line + 1);
                        }
                        else if (cursor_row < line_count - 1)
                        {
                            cursor_row++;
                            cursor_col = 0;
                            if (cursor_row >= top_line + SCREEN_ROWS - 1)
                            {
                                top_line = cursor_row - SCREEN_ROWS + 2;
                                need_full_redraw = 1;
                            }
                            else
                            {
                                gotoxy(cursor_col + 1, cursor_row - top_line + 1);
                            }
                        }
                    }
                    break;
                    
                case 71: /* Home */
                    cursor_col = 0;
                    gotoxy(cursor_col + 1, cursor_row - top_line + 1);
                    break;
                    
                case 79: /* End */
                    if (cursor_row < line_count)
                    {
                        cursor_col = strlen(lines[cursor_row]);
                        gotoxy(cursor_col + 1, cursor_row - top_line + 1);
                    }
                    break;
                    
                case 73: /* PgUp */
                    cursor_row -= (SCREEN_ROWS - 1);
                    if (cursor_row < 0)
                    {
                        cursor_row = 0;
                    }
                    top_line = cursor_row;
                    need_full_redraw = 1;
                    if (cursor_col > strlen(lines[cursor_row]))
                    {
                        cursor_col = strlen(lines[cursor_row]);
                    }
                    break;
                    
                case 81: /* PgDn */
                    cursor_row += (SCREEN_ROWS - 1);
                    if (cursor_row >= line_count)
                    {
                        cursor_row = line_count - 1;
                    }
                    if (cursor_row < 0)
                    {
                        cursor_row = 0;
                    }
                    top_line = cursor_row;
                    need_full_redraw = 1;
                    if (cursor_col > strlen(lines[cursor_row]))
                    {
                        cursor_col = strlen(lines[cursor_row]);
                    }
                    break;
                    
                case 59: /* F1 - Help */
                    show_help_screen();
                    need_full_redraw = 1;
                    break;
                    
                case 60: /* F2 - Save */
                    if (current_file[0])
                    {
                        write_file(current_file);
                    }
                    update_status_line();
                    break;
                    
                case 68: /* F10 - Exit */
                    running = 0;
                    break;
                    
                case 83: /* Delete */
                    delete_char();
                    draw_current_line();
                    update_status_line();
                    break;
            }
        }
        else if (ch == 13) /* Enter */
        {
            insert_newline();
            if (cursor_row >= top_line + SCREEN_ROWS - 1)
            {
                top_line = cursor_row - SCREEN_ROWS + 2;
            }
            need_full_redraw = 1;
        }
        else if (ch == 9) /* Tab */
        {
            insert_char(' ');
            insert_char(' ');
            insert_char(' ');
            insert_char(' ');
            insert_char(' ');
            insert_char(' ');
            insert_char(' ');
            insert_char(' ');
            draw_current_line();
            update_status_line();
        }
        else if (ch == 8) /* Backspace */
        {
            backspace_char();
            if (cursor_row < top_line)
            {
                top_line = cursor_row;
                need_full_redraw = 1;
            }
            else
            {
                draw_current_line();
                update_status_line();
            }
        }
        else if (ch == 27) /* Escape */
        {
            running = 0;
        }
        else if (ch >= 32 && ch < 127) /* Printable characters */
        {
            insert_char((char) ch);
            write_char_at_cursor((char) ch);
            update_status_line();
        }
    }
    
    clrscr();
}

/* -------- REPL & banner -------- */

static void help(void)
{
    puts("Commands:");
    puts("  L [a][,b]           list lines");
    puts("  I [n]               insert at n (end with a single '.')");
    puts("  D a[,b]             delete lines");
    puts("  E n                 edit (replace) line");
    puts("  R a[,b] /old/new/[g]  replace; 'g' = global per line");
    puts("  S [a][,b] /text/    search (case-insensitive)");
    puts("  O name              open (load) file");
    puts("  W [name]            write (save) file");
    puts("  V                   fullscreen visual editor mode");
    puts("  P                   print status");
    puts("  H or ?              help");
    puts("  Q                   quit");
}

static void status_line(void)
{
    printf("Lines: %d  File: %s\n", line_count, current_file[0] ? current_file : "(none)");
}

static void banner(const char *fname)
{
    char upper[128];
    char time_str[16];
    char file_status[64];
    int i;
    FILE *test_file;

    strncpy(upper, fname, sizeof(upper) - 1);
    upper[sizeof(upper) - 1] = 0;

    for (i = 0; upper[i]; i++)
    {
        upper[i] = (char) toupper((unsigned char) upper[i]);
    }

    get_time_string(time_str, sizeof(time_str));

    /* Determine file status */
    if (strcmp(fname, "(none)") == 0)
    {
        strcpy(file_status, "NEW FILE");
    }
    else
    {
        test_file = fopen(fname, "r");
        if (test_file)
        {
            fclose(test_file);
            sprintf(file_status, "EXISTING FILE (%d LINES)", line_count);
        }
        else
        {
            strcpy(file_status, "NEW FILE");
        }
    }

    puts("=================================================================");
    puts("              E V I L I N E D   Advanced Line Editor             ");
    puts("=================================================================");
    puts("   Version 2.0 Enhanced Edition  (C)  2025-2026 Mickey Lawless   ");
    puts("          Licensed under GNU GPL v3 - Free Software              ");
    puts("-----------------------------------------------------------------");
    puts("   Full-Featured Line Editor with Visual Mode & Advanced Search  ");
    puts("   Compatible: IBM-PC / CP-M / MS-DOS / Terminal Environments    ");
    puts("-----------------------------------------------------------------");
    printf("               Active File    :   %-45s\n", upper);
    printf("               File Status    :   %-45s\n", file_status);
    printf("               System Time    :   %-45s\n", time_str);
    puts("-----------------------------------------------------------------");
    puts("   Features: Multi-line Insert, Search/Replace, Visual Editor,   ");
    puts("   Range Operations, Case-Insensitive Search, Memory Management  ");
    puts("=================================================================");
    puts("         Ready.  Type '?' for Help or 'V' for Visual Mode.       ");
    puts("            !  Visual Mode not teletype compatible.  !           ");
    puts("=================================================================");
    puts("");
}

static void prompt(void)
{
    printf("* ");
    fflush(stdout);
}

int main(int argc, char **argv)
{
    char in[INPUT_LEN];

    if (argc > 1)
    {
        if (!load_file(argv[1]))
        {
            printf("! couldn't open '%s' (starting empty)\n", argv[1]);
            strncpy(current_file, argv[1], sizeof(current_file) - 1);
            current_file[sizeof(current_file) - 1] = 0;
        }
    }

    banner(argc > 1 ? argv[1] : "(none)");
    status_line();

    for (;;)
    {
        char cmd = 0;
        char *p;
        int a = -1;
        int b = -1;
        int n;

        prompt();

        if (!fgets(in, sizeof(in), stdin))
        {
            break;
        }

        chomp(in);
        p = in;

        while (isspace((unsigned char) *p))
        {
            ++p;
        }

        if (!*p)
        {
            continue;
        }

        cmd = (char) toupper((unsigned char) *p++);

        while (isspace((unsigned char) *p))
        {
            ++p;
        }

        switch (cmd)
        {
            case 'L':
            {
                if (!*p)
                {
                    a = 1;
                    b = line_count;
                }
                else if (!parse_range(p, &a, &b))
                {
                    puts("! bad range");
                    break;
                }

                cmd_list(a, b);
                break;
            }

            case 'I':
            {
                n = (*p ? atoi(p) : line_count + 1);
                cmd_insert(n);
                break;
            }

            case 'D':
            {
                if (!parse_range(p, &a, &b))
                {
                    puts("! need D a[,b]");
                    break;
                }

                cmd_delete(a, b);
                break;
            }

            case 'E':
            {
                if (!*p)
                {
                    puts("! need E n");
                    break;
                }

                n = atoi(p);
                cmd_edit(n);
                break;
            }

            case 'R':
            {
                char buf[INPUT_LEN];
                const char *spec;
                int have_range = 0;

                strncpy(buf, p, sizeof(buf) - 1);
                buf[sizeof(buf) - 1] = 0;
                spec = strchr(buf, '/');

                if (spec)
                {
                    char tmp[INPUT_LEN];
                    size_t rlen = (size_t) (spec - buf);

                    if (rlen >= sizeof(tmp))
                    {
                        rlen = sizeof(tmp) - 1;
                    }

                    memcpy(tmp, buf, rlen);
                    tmp[rlen] = 0;

                    if (tmp[0])
                    {
                        have_range = parse_range(tmp, &a, &b);
                    }
                    else
                    {
                        have_range = 1;
                    }

                    if (!have_range)
                    {
                        puts("! bad range");
                        break;
                    }

                    cmd_replace(have_range ? a : 1, have_range ? b : line_count, spec);
                }
                else
                {
                    puts("! syntax: R a,b /old/new/[g]");
                }

                break;
            }

            case 'O':
            {
                if (!*p)
                {
                    puts("! need filename");
                    break;
                }

                if (!load_file(p))
                {
                    puts("! open failed");
                }
                else
                {
                    printf("-- loaded %d line(s)\n", line_count);
                }

                break;
            }

            case 'S':
            {
                char buf[INPUT_LEN];
                const char *spec;
                int have_range = 0;

                strncpy(buf, p, sizeof(buf) - 1);
                buf[sizeof(buf) - 1] = 0;
                spec = strchr(buf, '/');

                if (spec)
                {
                    char tmp[INPUT_LEN];
                    size_t rlen = (size_t) (spec - buf);

                    if (rlen >= sizeof(tmp))
                    {
                        rlen = sizeof(tmp) - 1;
                    }

                    memcpy(tmp, buf, rlen);
                    tmp[rlen] = 0;

                    if (tmp[0])
                    {
                        have_range = parse_range(tmp, &a, &b);
                    }
                    else
                    {
                        have_range = 1;
                    }

                    if (!have_range)
                    {
                        puts("! bad range");
                        break;
                    }

                    cmd_search(have_range ? a : 1, have_range ? b : line_count, spec);
                }
                else
                {
                    cmd_search(1, line_count, p);
                }

                break;
            }

            case 'W':
            {
                if (*p)
                {
                    if (!write_file(p))
                    {
                        puts("! write failed");
                    }
                    else
                    {
                        printf("-- wrote %d line(s) to %s\n", line_count, p);
                    }
                }
                else if (current_file[0])
                {
                    if (!write_file(current_file))
                    {
                        puts("! write failed");
                    }
                    else
                    {
                        printf("-- wrote %d line(s) to %s\n", line_count, current_file);
                    }
                }
                else
                {
                    puts("! W needs filename (no current file)");
                }

                break;
            }

            case 'V':
            {
                cmd_fullscreen();
                break;
            }

            case 'P':
            {
                status_line();
                break;
            }

            case 'H':
            case '?':
            {
                help();
                break;
            }

            case 'Q':
            {
                return 0;
            }

            default:
            {
                puts("?");
                break;
            }
        }

        status_line();
    }

    return 0;
}
