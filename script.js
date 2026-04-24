const TOTAL_PAGES = 64;
let memory = new Array(TOTAL_PAGES).fill(null); // null means free, number means process ID

// DOM Elements
const memoryGrid = document.getElementById('memoryGrid');
const btnAllocate = document.getElementById('btnAllocate');
const btnDeallocate = document.getElementById('btnDeallocate');
const inputProcessId = document.getElementById('processId');
const inputProcessSize = document.getElementById('processSize');
const terminalLog = document.getElementById('terminalLog');
const statFree = document.getElementById('statFree');
const statUsed = document.getElementById('statUsed');

// Initialize Memory Grid
function initMemoryGrid() {
    memoryGrid.innerHTML = '';
    for (let i = 0; i < TOTAL_PAGES; i++) {
        const block = document.createElement('div');
        block.className = 'memory-block';
        block.id = `page-${i}`;
        block.title = `Physical Page: ${i}\nStatus: Free`;
        memoryGrid.appendChild(block);
    }
    updateStats();
}

function logToTerminal(message, type = 'info') {
    const entry = document.createElement('div');
    entry.className = `log-entry ${type}`;
    
    const time = new Date().toLocaleTimeString([], { hour12: false });
    entry.textContent = `[${time}] ${message}`;
    
    terminalLog.appendChild(entry);
    terminalLog.scrollTop = terminalLog.scrollHeight;
}

function updateStats() {
    const usedCount = memory.filter(page => page !== null).length;
    const freeCount = TOTAL_PAGES - usedCount;
    
    statFree.textContent = freeCount;
    statUsed.textContent = usedCount;
}

function renderMemory() {
    for (let i = 0; i < TOTAL_PAGES; i++) {
        const block = document.getElementById(`page-${i}`);
        const pid = memory[i];
        
        if (pid !== null) {
            block.className = 'memory-block allocated';
            block.textContent = `P${pid}`;
            block.title = `Physical Page: ${i}\nStatus: Allocated to Process ${pid}`;
            // Randomize color based on process ID to distinguish them easily
            const hue = (pid * 137.5) % 360;
            block.style.backgroundColor = `hsl(${hue}, 70%, 40%)`;
        } else {
            block.className = 'memory-block';
            block.textContent = '';
            block.title = `Physical Page: ${i}\nStatus: Free`;
            block.style.backgroundColor = '';
        }
    }
    updateStats();
}

function highlightPages(pages) {
    pages.forEach(pageIdx => {
        const block = document.getElementById(`page-${pageIdx}`);
        block.classList.add('highlight');
        setTimeout(() => block.classList.remove('highlight'), 1000);
    });
}

function allocateMemory() {
    const pid = parseInt(inputProcessId.value);
    const size = parseInt(inputProcessSize.value);

    if (isNaN(pid) || isNaN(size) || size <= 0) {
        logToTerminal('Invalid Process ID or Size.', 'error');
        return;
    }

    if (memory.includes(pid)) {
        logToTerminal(`Process ${pid} is already in memory.`, 'error');
        return;
    }

    const freePages = memory.reduce((acc, curr, idx) => {
        if (curr === null) acc.push(idx);
        return acc;
    }, []);

    if (freePages.length < size) {
        logToTerminal(`Not enough memory. Process ${pid} needs ${size} pages, but only ${freePages.length} are free.`, 'error');
        return;
    }

    // Paging allows non-contiguous allocation
    const allocatedPages = freePages.slice(0, size);
    
    allocatedPages.forEach(pageIdx => {
        memory[pageIdx] = pid;
    });

    logToTerminal(`Allocated ${size} pages for Process ${pid}. Pages: [${allocatedPages.join(', ')}]`, 'success');
    renderMemory();
    highlightPages(allocatedPages);
}

function deallocateMemory() {
    const pid = parseInt(inputProcessId.value);
    
    if (isNaN(pid)) {
        logToTerminal('Invalid Process ID for deallocation.', 'error');
        return;
    }

    const pagesToFree = [];
    memory.forEach((val, idx) => {
        if (val === pid) {
            pagesToFree.push(idx);
            memory[idx] = null;
        }
    });

    if (pagesToFree.length === 0) {
        logToTerminal(`Process ${pid} not found in memory.`, 'error');
        return;
    }

    logToTerminal(`Deallocated Process ${pid}. Freed ${pagesToFree.length} pages: [${pagesToFree.join(', ')}]`, 'success');
    renderMemory();
    highlightPages(pagesToFree);
}

// Event Listeners
btnAllocate.addEventListener('click', allocateMemory);
btnDeallocate.addEventListener('click', deallocateMemory);

// Initialization
window.onload = () => {
    initMemoryGrid();
    logToTerminal('Memory visualizer initialized. Paging system ready.', 'system');
};
