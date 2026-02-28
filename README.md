# Bump (Arena) Allocator Project in C

A custom linear/bump allocator using direct `mmap` arenas, implemented in portable C.

Built as a personal learning and portfolio project to deeply understand:
- Low-level memory management and syscalls (`mmap`, `munmap`)
- Allocator trade-offs (speed, locality, fragmentation, reset semantics)
- Real hardware performance impact (cache misses, TLB behavior, allocation overhead)
- Cross-platform differences (macOS libsystem_malloc vs Linux glibc)

## Features

- Direct `mmap`-backed arenas (no brk/sbrk fallback)
- Proper alignment support
- O(1) reset for bulk deallocation
- Portable across macOS and Linux
- Detailed microbenchmarking:
  - Wall-clock timing
  - RSS delta as fragmentation proxy (`getrusage`)
  - Hardware counters via Linux `perf` (cycles, instructions, cache misses, TLB misses)

## Why Bump Allocators?

Bump allocators shine in workloads with:
- Short-lived, sequentially accessed objects
- Bulk deallocation (reset instead of individual free)
- Need for perfect spatial/temporal locality and predictable memory use

Real-world examples: game engines (per-frame arenas), request handlers, compilers (temporary AST nodes), parsers.

They sacrifice flexibility for near-zero overhead, zero fragmentation during lifetime, and excellent cache/TLB behavior.

## Cross-Platform Benchmark Results

Workload: **10 million allocations of 64 bytes** (alloc + touch + reset/free), single-threaded.

Two configurations were tested:

### Test 1 – Large Arena (2 GiB capacity)
Full 10M allocations completed on capable machines.

| Platform                          | Bump ns/alloc | malloc+free ns/op | Speedup (bump vs malloc) | RSS delta Bump | RSS delta malloc | Notes |
|-----------------------------------|---------------|-------------------|---------------------------|----------------|------------------|-------|
| Apple M1/M2 macOS (libsystem_malloc) | ~13 ns       | ~26 ns           | ~2.0×                    | N/A            | N/A              | Apple's malloc highly tuned for small fixed-size |
| Linux x86_64 server (glibc)      | ~31–33 ns    | ~14–15 ns        | malloc ~2.2× faster      | ~618–624 MB    | 0 KB             | glibc fastbins very efficient in ideal case |
| AWS t3.micro (Ubuntu 24.04, glibc) | N/A          | N/A              | N/A                      | N/A            | N/A              | Insufficient RAM (test skipped) |

### Test 2 – Medium Arena (512 MiB capacity)
Early exhaustion at ~8,000,000 allocations (~512 MiB theoretical usage after alignment/padding).

| Platform                          | Bump ns/alloc (8M succ.) | malloc+free ns/op (10M succ.) | Speedup (bump vs malloc) | RSS delta Bump | RSS delta malloc | Notes |
|-----------------------------------|---------------------------|-------------------------------|---------------------------|----------------|------------------|-------|
| Apple M1/M2 macOS (libsystem_malloc) | ~13 ns                   | ~26 ns                       | ~2.0×                    | N/A            | N/A              | Consistent behavior |
| Linux x86_64 server (glibc)      | ~32 ns                   | ~14 ns                       | malloc ~2.3× faster      | N/A            | 0 KB             | malloc unaffected by size |
| AWS t3.micro (Ubuntu 24.04, glibc) | ~39 ns                   | ~16 ns                       | malloc ~2.4× faster      | N/A            | N/A              | Burstable CPU slows bump more |

**Key observations**:
- M1 Mac: Bump consistently ~2× faster libsystem_malloc excellent, but bump's simple bump wins slightly.
- Linux server & t3.micro: malloc+free wins ~2.2–2.4× per-thread fastbins make small fixed-size alloc/free extremely cheap.
- t3.micro: Bump slowest (~39 ns) burstable CPU / lower sustained performance hurts tight pointer-bump loops.
- Arena exhaustion at ~8M allocs (512 MiB) shows real capacity limits and alignment overhead.

## Detailed Hardware Counter Analysis (Linux x86_64 server)

Measured with `sudo perf stat` on isolated loops (bump-only and malloc-only) for 10M × 64-byte allocations (2 GiB arena).

### Full Metrics Overview

| Category                  | Metric                          | Bump Arena                  | malloc + free              | Winner & Insight |
|---------------------------|---------------------------------|-----------------------------|----------------------------|------------------|
| Wall Time                 | ns per op                       | ~33 ns                     | ~14–15 ns                 | malloc (fastbins dominate) |
| Allocation Overhead       | Est. cycles per op (4 GHz)      | ~132 cycles                | ~56–60 cycles             | malloc (fewer instructions) |
| Cache Behavior            | Cache references                | ~2.006 billion             | ~20,278                   | malloc (minimal accesses) |
|                           | Cache miss rate                 | ~0.70%                     | ~10.07%                   | **bump** (superior relative locality) |
|                           | Absolute cache misses           | ~14.1 million              | ~2,042                    | malloc (low total pressure) |
|                           | L1-dcache-load-misses           | ~14.1 million              | ~18,439                   | malloc (tiny hot set) |
|                           | LLC-load-misses                 | 559,296                    | 443                       | malloc (very low) |
| TLB Behavior              | dTLB-loads                      | ~217 million               | ~370 million              | bump (fewer loads despite large arena) |
|                           | dTLB miss rate                  | ~0.12%                     | ~0.00016%                 | malloc (tiny working set) |
| Fragmentation Proxy       | RSS delta (KB)                  | ~618,000–624,000           | 0                         | malloc (no growth); **bump** predictable & zero frag |

**Interpretation**:
- Bump allocator delivers **excellent relative cache locality** (0.70% miss rate despite billions of references) and **predictable memory footprint** with zero fragmentation.
- glibc malloc minimizes **total cache/TLB pressure** and instruction count via aggressive per-thread fast paths winning in this perfect-case microbenchmark.
- Bump advantages shine in reset-heavy, variable-size, multi-threaded, or fragmentation-inducing workloads.

## Trade-offs

| Aspect               | Bump Arena                          | glibc malloc                        |
|----------------------|-------------------------------------|-------------------------------------|
| Alloc speed (this test) | ~33 ns                             | ~14–15 ns                          |
| Free/reset           | O(1) bulk reset                    | Individual free (fast here)        |
| Fragmentation        | Zero during lifetime               | None here, but grows in bad cases  |
| Locality             | Excellent (sequential)             | Variable (scattered after frees)   |
| Memory overhead      | Full arena mapped                  | Per-chunk metadata + free lists    |
| Flexibility          | Fixed-size arenas, reset semantics | General purpose                    |
| Best for             | Game frames, request handlers, parsers | General small allocations          |

## Build & Run

```bash
# Build
make bench

# Run (capacity in bytes, e.g. 2 GiB)
./bench 2147483648

# On Linux – hardware counters (may need sudo on shared servers)
sudo perf stat -e cycles,instructions,cache-misses,dTLB-load-misses,LLC-load-misses ./bench 2147483648