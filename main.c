#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>

static const char *program_name = "samplemem";
static const char *usage = "Usage: %s <block_device> <block_size> <value_to_check> <n_samples>\n";


static void print_usage() {
    fprintf(stderr, usage, program_name);
}


static void handle_invalid_param(const char *str, const char *description) {
    print_usage();
    fprintf(stderr, "invalid %s - %s", description, str);
    exit(1);
}


static uintmax_t param_uint(const char *str, const char *description, uintmax_t max) {
    char *end;
    errno = 0;
	const uintmax_t value = strtoumax(str, &end, 0);
	if (*end || errno) {
        fprintf(stderr, "invalid %s - %s", description, str);
		handle_invalid_param(str, description);
	} else if ((value > max) || (value == UINTMAX_MAX && errno == ERANGE)) {
        fprintf(stderr, "%s too large - %ju", description, value);
		handle_invalid_param(str, description);
    }
	return value;
}


static char *format_elapsed_time(time_t elapsed_time, char *buf) {
	int	hr, min, sec;
	sec = elapsed_time % 60;
	elapsed_time /= 60;
	min = elapsed_time % 60;
	hr = elapsed_time / 60;
	if (hr)
		sprintf(buf, "%d:%02d:%02d", hr, min, sec);
	else
		sprintf(buf, "%d:%02d", min, sec);
	return buf;
}


static int check_block(uint8_t *block, size_t size, uint8_t value) {
    for (size_t i = 0; i < size; ++i) {
        if (block[i] != value) {
            return 0;
            break;
        }
    }
    return 1;
}

static size_t sample_memory(const char *block_device, size_t block_size, 
        uint8_t value_to_check, size_t n_samples) {
    // Open the block device
    FILE *block_device_file = fopen(block_device, "rb");
    if (block_device_file == NULL) {
        perror("Error opening block device");
        exit(1);
    }
    // Get the size of the block device
    fseek(block_device_file, 0, SEEK_END);
    const size_t block_count = ftell(block_device_file) / block_size;
    fseek(block_device_file, 0, SEEK_SET);
    uint8_t *block = (unsigned char *)malloc(block_size);
    if (block == NULL) {
        perror("Error allocating memory");
        fclose(block_device_file);
        exit(1);
    }
    const size_t block_stride = block_count / n_samples;
    size_t bad_block_count = 0;
    size_t progress_update_ctr = 10;
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    // Loop over the blocks
    for (size_t current_block = 0, current_check = 0; 
            current_block < block_count; 
            current_block += block_stride, ++current_check) {
        // Read the n-th block from the block device
        fseek(block_device_file, current_block * block_size, SEEK_SET);
        const size_t bytes_read = fread(block, 1, block_size, block_device_file);
        if (bytes_read != block_size) {
            printf("Couldn't read block %zu!\n", current_block);
            continue;
        }
        // Check if every byte in the block matches the given value
        const int block_match = check_block(block, block_size, value_to_check);
        if (!block_match) {
            printf("%zu\n", current_block);
            bad_block_count++;
        }
        if (progress_update_ctr-- == 0) {
            struct timespec end_time;
            clock_gettime(CLOCK_MONOTONIC, &end_time);
            const double elapsed_time = (end_time.tv_sec - start_time.tv_sec) 
                    + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
            const double time_per_check = elapsed_time / (current_check + 1);
            char elapsed_time_buf[32];
            format_elapsed_time((time_t)elapsed_time, elapsed_time_buf);
            const double progress = ((double)(current_check + 1) / n_samples * 100);
            // Iterations until next progress update
            progress_update_ctr = 1.0 / time_per_check;
            fprintf(stderr, "%.2f%% done, %s elapsed. (%zu errors)\r", 
                    progress, elapsed_time_buf, bad_block_count);
        }
    }    
    fclose(block_device_file);
    free(block);
    return bad_block_count;
}


int main(int argc, char *argv[]) {
    // Command line arguments
    if (argc != 5) {
        print_usage();
        return 1;
    }
    const char *block_device = argv[1];
    const size_t block_size = (size_t)param_uint(argv[2], "block_size", SIZE_MAX);
    const uint8_t value_to_check = (uint8_t)param_uint(argv[3], "value_to_check", UINT8_MAX);
    const size_t n_samples = (size_t)param_uint(argv[4], "n_samples", SIZE_MAX);
    // Run check
    const size_t bad_block_count = sample_memory(block_device, block_size, value_to_check, n_samples);
    if (bad_block_count == 0) {
        printf("\nNo bad blocks found.\n");
        return 0;
    } else {
        printf("\nFound %zu bad blocks!\n", bad_block_count);
        return 2;
    }
}
