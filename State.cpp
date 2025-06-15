#include "State.h"
#include "lfs.h"
#include <cstring>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

// Static buffer for the FS
static uint8_t lfs_read_buf[64];
static uint8_t lfs_prog_buf[64];
static uint8_t lfs_lookahead_buf[16];

// Speicherbereich aus custom_flash.ld
#define FS_BASE_ADDR (2 * 1024 * 1024 - 64 * 1024) // letzte 64 KB
#define FS_SIZE      (64 * 1024)

static int lfs_read(const struct lfs_config *c, lfs_block_t block,
                    lfs_off_t off, void *buffer, lfs_size_t size) {
    std::memcpy(buffer, (const void*)(XIP_BASE + FS_BASE_ADDR + block * c->block_size + off), size);
    return 0;
}

static int lfs_prog(const struct lfs_config *c, lfs_block_t block,
                    lfs_off_t off, const void *buffer, lfs_size_t size) {
    uint32_t addr = FS_BASE_ADDR + block * c->block_size + off;
    uint32_t ints = save_and_disable_interrupts();
    flash_range_program(addr, (const uint8_t*)buffer, size);
    restore_interrupts(ints);
    return 0;
}

static int lfs_erase(const struct lfs_config *c, lfs_block_t block) {
    uint32_t addr = FS_BASE_ADDR + block * c->block_size;
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(addr, c->block_size);
    restore_interrupts(ints);
    return 0;
}

static int lfs_sync(const struct lfs_config *c) {
    return 0;
}

static struct lfs_config cfg = {
    // block device operations
    .read  = lfs_read,
    .prog  = lfs_prog,
    .erase = lfs_erase,
    .sync  = lfs_sync,

    // block device configuration
    .read_size = 16,
    .prog_size = 16,
    .block_size = 4096,
    .block_count = FS_SIZE / 4096,
    .block_cycles = 500,

    // cache configuration
    .cache_size = 16,
    .lookahead_size = sizeof(lfs_lookahead_buf),

    // buffers
    .read_buffer = lfs_read_buf,
    .prog_buffer = lfs_prog_buf,
    .lookahead_buffer = lfs_lookahead_buf,
};

static lfs_t lfs;
static bool fs_mounted = false;

static void init_fs() {
    if (fs_mounted) return;

    int err = lfs_mount(&lfs, &cfg);
    if (err) {
        lfs_format(&lfs, &cfg);
        lfs_mount(&lfs, &cfg);
    }
    fs_mounted = true;
}

// Konstruktor
State::State(const std::string& path) : m_path(path), m_state(CLOSE) {
    init_fs();
    load();
}

State::STATE State::GetState() const {
    return m_state;
}

bool State::SetState(STATE state) {
    if (m_state != state) {
        m_state = state;
        save();
        return true;
    }
    return false;
}

void State::load() {
    lfs_file_t file;
    if (lfs_file_open(&lfs, &file, m_path.c_str(), LFS_O_RDONLY) >= 0) {
        uint8_t val = 0;
        if (lfs_file_read(&lfs, &file, &val, 1) == 1) {
            m_state = static_cast<STATE>(val);
        }
        lfs_file_close(&lfs, &file);
    }
}

void State::save() {
    lfs_file_t file;
    if (lfs_file_open(&lfs, &file, m_path.c_str(), LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) >= 0) {
        uint8_t val = static_cast<uint8_t>(m_state);
        lfs_file_write(&lfs, &file, &val, 1);
        lfs_file_close(&lfs, &file);
    }
}
