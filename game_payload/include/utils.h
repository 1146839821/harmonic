#pragma once

#include <windows.h>
#include <stdint.h>

#define UTILS_COUNT(arr) (sizeof(arr) / sizeof(*arr))

struct file_mapping {
    HANDLE file;
    HANDLE mapping;
    unsigned char *data;
};

void utils_map_file(const wchar_t *path, struct file_mapping *map);
void utils_unmap_file(struct file_mapping *map);

int utils_path_exists(const wchar_t *filePath);
uint32_t utils_file_crc32c(const wchar_t *filePath);

void utils_create_parent_dirs(const wchar_t *path);

void utils_save_to_file(const wchar_t *filePath, const void *buf, size_t length);

char utils_env_enabled(const char *env);

void utils_write_protected_memory(void *addr, const void *buf, size_t size);

void utils_suspend_other_threads();

HMODULE utils_load_module_patched(wchar_t *path);
void utils_hook_address(void *addr, void *target);
