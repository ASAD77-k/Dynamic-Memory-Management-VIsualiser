/*
 * Dynamic Memory Management Visualizer
 * CSE-316 Operating Systems - CA2 Project
 *
 * This C program simulates dynamic memory management techniques:
 * - Paging
 * - Segmentation
 * - Virtual Memory Address Translation
 *
 * Compile: gcc -o memory_manager memory_manager.c
 * Run:     ./memory_manager
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ─── Constants ─────────────────────────────────────────────── */
#define PHYSICAL_MEMORY_SIZE 64 /* in frames / blocks        */
#define PAGE_SIZE 4             /* KB per page               */
#define MAX_PAGES 16            /* max pages per process     */
#define MAX_SEGMENTS 8          /* max segments per process  */
#define MAX_PROCESSES 4

/* ─── Data Structures ───────────────────────────────────────── */

/* Page Table Entry */
typedef struct {
  int frame_number; /* -1 means not loaded (page fault)   */
  int valid;        /* 1 = in physical memory             */
  int dirty;        /* 1 = modified                       */
  int referenced;   /* 1 = recently accessed              */
} PageTableEntry;

/* Segment Descriptor */
typedef struct {
  char name[16]; /* e.g. "Code", "Data", "Stack"       */
  int base;      /* base address in physical memory    */
  int limit;     /* size of segment                    */
  int valid;
} SegmentDescriptor;

/* Process Control Block (memory view) */
typedef struct {
  int pid;
  char name[32];
  int num_pages;
  PageTableEntry page_table[MAX_PAGES];
  int num_segments;
  SegmentDescriptor seg_table[MAX_SEGMENTS];
} Process;

/* Physical Memory Frame */
typedef struct {
  int occupied;
  int pid; /* which process owns this frame */
  int page_num;
} Frame;

/* ─── Global State ──────────────────────────────────────────── */
Frame physical_memory[PHYSICAL_MEMORY_SIZE];
Process processes[MAX_PROCESSES];
int num_processes = 0;
int page_faults = 0;
int page_hits = 0;

/* ─── Helper Functions ──────────────────────────────────────── */

void init_physical_memory(void) {
  for (int i = 0; i < PHYSICAL_MEMORY_SIZE; i++) {
    physical_memory[i].occupied = 0;
    physical_memory[i].pid = -1;
    physical_memory[i].page_num = -1;
  }
}

int find_free_frame(void) {
  for (int i = 0; i < PHYSICAL_MEMORY_SIZE; i++)
    if (!physical_memory[i].occupied)
      return i;
  return -1; /* memory full */
}

void print_separator(void) {
  printf("\n%s\n", "════════════════════════════════════════════════");
}

/* ─── Paging Module ─────────────────────────────────────────── */

Process *create_process(int pid, const char *name, int num_pages) {
  if (num_processes >= MAX_PROCESSES) {
    printf("[ERROR] Maximum processes reached.\n");
    return NULL;
  }
  Process *p = &processes[num_processes++];
  p->pid = pid;
  p->num_pages = num_pages;
  strncpy(p->name, name, 31);

  for (int i = 0; i < num_pages; i++) {
    p->page_table[i].frame_number = -1;
    p->page_table[i].valid = 0;
    p->page_table[i].dirty = 0;
    p->page_table[i].referenced = 0;
  }
  printf("[PAGING] Process %d ('%s') created with %d pages (%d KB each).\n",
         pid, name, num_pages, PAGE_SIZE);
  return p;
}

int translate_virtual_address(Process *p, int virtual_address) {
  int page_number = virtual_address / PAGE_SIZE;
  int offset = virtual_address % PAGE_SIZE;

  printf("\n[ADDR TRANSLATION] VA=%d  →  Page=%d, Offset=%d\n", virtual_address,
         page_number, offset);

  if (page_number >= p->num_pages) {
    printf("[FAULT] Segmentation fault: page %d out of range.\n", page_number);
    return -1;
  }

  PageTableEntry *pte = &p->page_table[page_number];

  if (!pte->valid) {
    /* Page Fault */
    page_faults++;
    int frame = find_free_frame();
    if (frame == -1) {
      printf("[FAULT] Page fault on page %d — no free frame! Need eviction.\n",
             page_number);
      return -1;
    }
    pte->frame_number = frame;
    pte->valid = 1;
    pte->referenced = 1;
    physical_memory[frame].occupied = 1;
    physical_memory[frame].pid = p->pid;
    physical_memory[frame].page_num = page_number;
    printf("[PAGE FAULT] Page %d loaded into frame %d. (Total faults: %d)\n",
           page_number, frame, page_faults);
  } else {
    page_hits++;
    pte->referenced = 1;
    printf("[PAGE HIT]  Page %d already in frame %d. (Total hits: %d)\n",
           page_number, pte->frame_number, page_hits);
  }

  int physical_address = pte->frame_number * PAGE_SIZE + offset;
  printf("[RESULT]    Physical Address = %d\n", physical_address);
  return physical_address;
}

void print_page_table(Process *p) {
  printf("\n[PAGE TABLE] Process %d ('%s')\n", p->pid, p->name);
  printf("  %-8s %-12s %-6s %-6s %-10s\n", "Page", "Frame", "Valid", "Dirty",
         "Referenced");
  printf("  %s\n", "─────────────────────────────────────────");
  for (int i = 0; i < p->num_pages; i++) {
    PageTableEntry *e = &p->page_table[i];
    printf("  %-8d %-12s %-6d %-6d %-10d\n", i,
           e->valid ? "Loaded" : "Not loaded", e->valid, e->dirty,
           e->referenced);
  }
}

/* ─── Segmentation Module ───────────────────────────────────── */

void add_segment(Process *p, const char *seg_name, int base, int limit) {
  if (p->num_segments >= MAX_SEGMENTS) {
    printf("[ERROR] Max segments reached for process %d.\n", p->pid);
    return;
  }
  SegmentDescriptor *sd = &p->seg_table[p->num_segments++];
  strncpy(sd->name, seg_name, 15);
  sd->base = base;
  sd->limit = limit;
  sd->valid = 1;
  printf("[SEGMENT] Added segment '%s': base=%d, limit=%d\n", seg_name, base,
         limit);
}

int segment_translate(Process *p, int seg_num, int offset) {
  printf("\n[SEGMENTATION] Process %d, Segment %d, Offset %d\n", p->pid,
         seg_num, offset);

  if (seg_num >= p->num_segments) {
    printf("[FAULT] Invalid segment number %d.\n", seg_num);
    return -1;
  }
  SegmentDescriptor *sd = &p->seg_table[seg_num];
  if (!sd->valid) {
    printf("[FAULT] Segment '%s' not valid/loaded.\n", sd->name);
    return -1;
  }
  if (offset >= sd->limit) {
    printf("[FAULT] Offset %d exceeds segment limit %d for '%s'.\n", offset,
           sd->limit, sd->name);
    return -1;
  }
  int physical = sd->base + offset;
  printf("[OK] Segment '%s': base=%d + offset=%d → Physical=%d\n", sd->name,
         sd->base, offset, physical);
  return physical;
}

void print_segment_table(Process *p) {
  printf("\n[SEGMENT TABLE] Process %d ('%s')\n", p->pid, p->name);
  printf("  %-4s %-12s %-8s %-8s %-6s\n", "Seg", "Name", "Base", "Limit",
         "Valid");
  printf("  %s\n", "──────────────────────────────────────────");
  for (int i = 0; i < p->num_segments; i++) {
    SegmentDescriptor *s = &p->seg_table[i];
    printf("  %-4d %-12s %-8d %-8d %-6d\n", i, s->name, s->base, s->limit,
           s->valid);
  }
}

/* ─── Memory Snapshot ───────────────────────────────────────── */

void print_physical_memory_snapshot(void) {
  printf("\n[PHYSICAL MEMORY SNAPSHOT]\n");
  printf("  Total Frames: %d | Frame Size: %d KB\n\n", PHYSICAL_MEMORY_SIZE,
         PAGE_SIZE);
  int used = 0;
  for (int i = 0; i < PHYSICAL_MEMORY_SIZE; i++) {
    if (physical_memory[i].occupied) {
      printf("  Frame %02d: PID=%d, Page=%d\n", i, physical_memory[i].pid,
             physical_memory[i].page_num);
      used++;
    }
  }
  printf("\n  Used: %d / %d frames (%.1f%%)\n", used, PHYSICAL_MEMORY_SIZE,
         100.0 * used / PHYSICAL_MEMORY_SIZE);
}

void print_statistics(void) {
  print_separator();
  printf("  SIMULATION STATISTICS\n");
  print_separator();
  printf("  Page Faults : %d\n", page_faults);
  printf("  Page Hits   : %d\n", page_hits);
  int total = page_faults + page_hits;
  if (total > 0)
    printf("  Hit Rate    : %.1f%%\n", 100.0 * page_hits / total);
  printf("\n");
}

/* ─── Main Demo ─────────────────────────────────────────────── */

int main(void) {
  printf("\n");
  print_separator();
  printf("  DYNAMIC MEMORY MANAGEMENT SIMULATOR\n");
  printf("  CSE-316 Operating Systems — CA2\n");
  print_separator();

  init_physical_memory();

  /* ── Demo 1: Paging ─── */
  printf("\n╔══════════════════════════════════╗\n");
  printf("║   MODULE 1 : PAGING              ║\n");
  printf("╚══════════════════════════════════╝\n");

  Process *p1 = create_process(1, "WebBrowser", 6);
  Process *p2 = create_process(2, "TextEditor", 4);

  /* Simulate virtual address accesses */
  translate_virtual_address(p1, 0);  /* page 0 — fault */
  translate_virtual_address(p1, 5);  /* page 1 — fault */
  translate_virtual_address(p1, 0);  /* page 0 — hit   */
  translate_virtual_address(p1, 18); /* page 4 — fault */
  translate_virtual_address(p2, 7);  /* page 1 — fault */
  translate_virtual_address(p2, 7);  /* page 1 — hit   */

  print_page_table(p1);
  print_page_table(p2);

  /* ── Demo 2: Segmentation ─── */
  printf("\n╔══════════════════════════════════╗\n");
  printf("║   MODULE 2 : SEGMENTATION        ║\n");
  printf("╚══════════════════════════════════╝\n");

  add_segment(p1, "Code", 100, 200);
  add_segment(p1, "Data", 300, 150);
  add_segment(p1, "Stack", 500, 100);

  segment_translate(p1, 0, 50);  /* Code  segment, valid  */
  segment_translate(p1, 1, 140); /* Data  segment, valid  */
  segment_translate(p1, 1, 200); /* Data  segment, FAULT  */
  segment_translate(p1, 2, 0);   /* Stack segment, valid  */
  segment_translate(p1, 5, 0);   /* Invalid segment       */

  print_segment_table(p1);

  /* ── Demo 3: Physical Memory Snapshot ─── */
  printf("\n╔══════════════════════════════════╗\n");
  printf("║   MODULE 3 : PHYSICAL MEMORY     ║\n");
  printf("╚══════════════════════════════════╝\n");

  print_physical_memory_snapshot();
  print_statistics();

  return 0;
}