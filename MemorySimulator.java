/**
 * MemorySimulator.java
 * Dynamic Memory Management Simulator — Java Implementation
 * CSE-316 Operating Systems — CA2 Project
 *
 * Simulates: Paging, Segmentation, Virtual Memory, Clock Algorithm
 * Compile: javac MemorySimulator.java
 * Run:     java MemorySimulator
 */

import java.util.*;

public class MemorySimulator {

    // ─────────────────────── CONSTANTS ───────────────────────
    static final int PAGE_SIZE  = 4096;
    static final int NUM_FRAMES = 16;
    static final int NUM_PAGES  = 32;
    static final int TLB_SIZE   = 4;
    static final int NUM_SEGS   = 8;

    // ─────────────────────── INNER CLASSES ───────────────────────

    static class PageTableEntry {
        int     pageNumber;
        Integer frameNumber;
        boolean valid, dirty, referenced;
        int     lastUsed;

        PageTableEntry(int pageNumber) {
            this.pageNumber = pageNumber;
        }
    }

    static class Frame {
        int     frameId;
        boolean occupied;
        Integer processId;
        Integer pageNumber;

        Frame(int frameId) { this.frameId = frameId; }
    }

    static class TLBEntry {
        int     pageNumber, frameNumber;
        boolean valid;
        TLBEntry(int p, int f) { pageNumber = p; frameNumber = f; valid = true; }
    }

    static class Segment {
        int segId, base, limit, processId;
        Segment(int id, int base, int limit, int pid) {
            this.segId = id; this.base = base; this.limit = limit; this.processId = pid;
        }
    }

    static class Stats {
        int pageFaults, tlbHits, tlbMisses, totalAccesses, evictions, segFaults;
    }

    // ─────────────────────── TLB (LRU via LinkedHashMap) ───────────────────────

    static class TLB {
        private final LinkedHashMap<Integer, TLBEntry> cache =
            new LinkedHashMap<>(TLB_SIZE, 0.75f, true) {
                protected boolean removeEldestEntry(Map.Entry<Integer, TLBEntry> eldest) {
                    return size() > TLB_SIZE;
                }
            };

        Integer lookup(int page) {
            TLBEntry e = cache.get(page);
            return (e != null && e.valid) ? e.frameNumber : null;
        }

        void insert(int page, int frame) {
            cache.put(page, new TLBEntry(page, frame));
        }

        void invalidate(int page) {
            if (cache.containsKey(page)) cache.get(page).valid = false;
        }

        void display() {
            System.out.println("\n  ┌─── TLB ────────────────────────┐");
            int i = 0;
            for (TLBEntry e : cache.values()) {
                System.out.printf("  │ Slot %d: Pg %2d → Fr %2d  [%s]  │%n",
                    i++, e.pageNumber, e.frameNumber, e.valid ? "✓" : "✗");
            }
            if (cache.isEmpty()) System.out.println("  │  (empty)                       │");
            System.out.println("  └────────────────────────────────┘");
        }
    }

    // ─────────────────────── CLOCK ALGORITHM ───────────────────────

    static class ClockReplacer {
        private int hand = 0;
        private final Frame[] frames;
        private final PageTableEntry[] pageTable;

        ClockReplacer(Frame[] frames, PageTableEntry[] pageTable) {
            this.frames = frames; this.pageTable = pageTable;
        }

        int pickVictim() {
            while (true) {
                Frame f = frames[hand];
                if (f.occupied) {
                    PageTableEntry pte = pageTable[f.pageNumber];
                    if (pte.referenced) {
                        pte.referenced = false;   // second chance
                        System.out.printf("  ○ Clock: giving page %d a second chance%n", f.pageNumber);
                    } else {
                        int victim = hand;
                        hand = (hand + 1) % NUM_FRAMES;
                        return victim;
                    }
                }
                hand = (hand + 1) % NUM_FRAMES;
            }
        }
    }

    // ─────────────────────── MEMORY MANAGER ───────────────────────

    static class MemoryManager {
        PageTableEntry[] pageTable = new PageTableEntry[NUM_PAGES];
        Frame[]          frames    = new Frame[NUM_FRAMES];
        Map<Integer, Segment> segTable = new HashMap<>();
        TLB              tlb       = new TLB();
        Stats            stats     = new Stats();
        ClockReplacer    clock;
        int              timer     = 0;

        MemoryManager() {
            for (int i = 0; i < NUM_PAGES; i++)  pageTable[i] = new PageTableEntry(i);
            for (int i = 0; i < NUM_FRAMES; i++) frames[i]    = new Frame(i);
            clock = new ClockReplacer(frames, pageTable);
        }

        // ── helpers ──

        int findFreeFrame() {
            for (Frame f : frames) if (!f.occupied) return f.frameId;
            return -1;
        }

        int evictFrame() {
            int victim = clock.pickVictim();
            int evictedPage = frames[victim].pageNumber;
            System.out.printf("  ⚡ Evicting page %d from frame %d (Clock)%n", evictedPage, victim);
            pageTable[evictedPage].valid = false;
            pageTable[evictedPage].frameNumber = null;
            tlb.invalidate(evictedPage);
            stats.evictions++;
            return victim;
        }

        // ── paging ──

        Integer translate(int pid, int virtualAddr) {
            if (virtualAddr < 0 || virtualAddr >= NUM_PAGES * PAGE_SIZE) {
                System.out.printf("  ✗ Address %d out of range%n", virtualAddr);
                return null;
            }
            int pageNum = virtualAddr / PAGE_SIZE;
            int offset  = virtualAddr % PAGE_SIZE;
            stats.totalAccesses++;
            int t = ++timer;

            System.out.printf("%n  ▶ VA=%d  (page=%d, offset=%d)%n", virtualAddr, pageNum, offset);

            // TLB check
            Integer frameNum = tlb.lookup(pageNum);
            if (frameNum != null) {
                stats.tlbHits++;
                int pa = frameNum * PAGE_SIZE + offset;
                System.out.printf("  ✓ TLB HIT → Frame %d, PA=%d%n", frameNum, pa);
                pageTable[pageNum].lastUsed = t;
                return pa;
            }

            stats.tlbMisses++;
            PageTableEntry pte = pageTable[pageNum];

            if (!pte.valid) {
                stats.pageFaults++;
                System.out.printf("  ⚠ PAGE FAULT on page %d%n", pageNum);
                int freeFrame = findFreeFrame();
                if (freeFrame < 0) freeFrame = evictFrame();

                frames[freeFrame].occupied  = true;
                frames[freeFrame].processId = pid;
                frames[freeFrame].pageNumber = pageNum;
                pte.valid = true;
                pte.frameNumber = freeFrame;
                System.out.printf("  ↳ Page %d loaded into frame %d%n", pageNum, freeFrame);
            } else {
                System.out.printf("  ○ TLB MISS — page %d in table, frame %d%n", pageNum, pte.frameNumber);
            }

            pte.lastUsed  = t;
            pte.referenced = true;
            tlb.insert(pageNum, pte.frameNumber);
            int pa = pte.frameNumber * PAGE_SIZE + offset;
            System.out.printf("  ✓ PA=%d%n", pa);
            return pa;
        }

        // ── segmentation ──

        void createSegment(int segId, int pid, int size) {
            int base = segId * 12000;
            segTable.put(segId, new Segment(segId, base, size, pid));
            System.out.printf("  ✓ Segment %d | PID=%d base=%d limit=%d%n", segId, pid, base, size);
        }

        Integer segTranslate(int segId, int offset) {
            Segment seg = segTable.get(segId);
            if (seg == null) { System.out.printf("  ✗ Invalid segment %d%n", segId); return null; }
            if (offset >= seg.limit) {
                stats.segFaults++;
                System.out.printf("  ✗ SEGFAULT: offset %d >= limit %d in seg %d%n", offset, seg.limit, segId);
                return null;
            }
            int pa = seg.base + offset;
            System.out.printf("  ✓ Seg %d offset=%d → PA=%d%n", segId, offset, pa);
            return pa;
        }

        // ── display ──

        void printPageTable() {
            System.out.println("\n  ╔══ PAGE TABLE ══════════════════════╗");
            System.out.println("  ║  Page  Valid  Frame  Dirty  Ref    ║");
            System.out.println("  ╠════════════════════════════════════╣");
            for (PageTableEntry p : pageTable) {
                if (p.valid || p.pageNumber < 6) {
                    System.out.printf("  ║  %4d  %-5s  %-5s  %-5s  %-5s  ║%n",
                        p.pageNumber,
                        p.valid ? "Y" : "N",
                        p.frameNumber != null ? p.frameNumber.toString() : "—",
                        p.dirty ? "Y" : "N",
                        p.referenced ? "Y" : "N");
                }
            }
            System.out.println("  ╚════════════════════════════════════╝");
        }

        void printFrames() {
            System.out.println("\n  Physical Frames:");
            for (int row = 0; row < NUM_FRAMES; row += 4) {
                StringBuilder line = new StringBuilder("  ");
                for (int c = 0; c < 4; c++) {
                    Frame f = frames[row + c];
                    if (f.occupied)
                        line.append(String.format("[F%02d P%d pg%02d]  ", f.frameId, f.processId, f.pageNumber));
                    else
                        line.append(String.format("[F%02d FREE    ]  ", f.frameId));
                }
                System.out.println(line);
            }
        }

        void printStats() {
            int total = Math.max(stats.totalAccesses, 1);
            double hr  = (double) stats.tlbHits / total * 100;
            double eat = hr/100 * 10 + (1 - hr/100) * 110;
            System.out.println("\n  ╔══ STATISTICS ════════════════════╗");
            System.out.printf("  ║  Total Accesses : %-17d║%n", stats.totalAccesses);
            System.out.printf("  ║  Page Faults    : %-17d║%n", stats.pageFaults);
            System.out.printf("  ║  TLB Hits       : %d (%.1f%%)%14s║%n", stats.tlbHits, hr, "");
            System.out.printf("  ║  TLB Misses     : %-17d║%n", stats.tlbMisses);
            System.out.printf("  ║  Evictions      : %-17d║%n", stats.evictions);
            System.out.printf("  ║  Segfaults      : %-17d║%n", stats.segFaults);
            System.out.printf("  ║  EAT            : %.1f ns%11s║%n", eat, "");
            long used = Arrays.stream(frames).filter(f -> f.occupied).count();
            System.out.printf("  ║  Frames Used    : %d/%d (%.0f%%)%10s║%n",
                used, NUM_FRAMES, (double)used/NUM_FRAMES*100, "");
            System.out.println("  ╚══════════════════════════════════╝");
        }
    }

    // ─────────────────────────── MAIN ───────────────────────────

    public static void main(String[] args) {
        System.out.println("╔════════════════════════════════════════════════╗");
        System.out.println("║  Dynamic Memory Management Simulator — Java    ║");
        System.out.println("║  CSE-316 Operating Systems — CA2               ║");
        System.out.println("╚════════════════════════════════════════════════╝");

        MemoryManager mm = new MemoryManager();

        System.out.println("\n\n══ PAGING (Clock Replacement) ═══════════════");
        mm.translate(1, 0);
        mm.translate(1, 4096);
        mm.translate(1, 100);      // TLB hit
        mm.translate(2, 8200);
        mm.translate(1, 16384);
        mm.translate(3, 20480);
        mm.translate(1, 4196);     // TLB hit

        mm.tlb.display();
        mm.printPageTable();
        mm.printFrames();

        System.out.println("\n\n══ SEGMENTATION ═════════════════════════════");
        mm.createSegment(0, 1, 6000);
        mm.createSegment(1, 2, 2500);
        mm.createSegment(2, 1, 800);
        mm.segTranslate(0, 3000);
        mm.segTranslate(1, 2499);
        mm.segTranslate(2, 1000);  // segfault

        mm.printStats();

        // Stress test
        System.out.println("\n\n══ STRESS TEST (50 random accesses) ═════════");
        Random rng = new Random(42);
        for (int i = 0; i < 50; i++) {
            mm.translate(rng.nextInt(4) + 1, rng.nextInt(NUM_PAGES * PAGE_SIZE));
        }
        mm.printStats();
    }
}
