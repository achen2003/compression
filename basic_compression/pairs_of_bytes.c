#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
Encode 'in' into (count, value) pairs in 'out'
Return the number of bytes written to 'out'
Output needs to be at least 2*n bytes long (think of ABCD)
*/
size_t rle_encode(const unsigned char *in, size_t n, unsigned char *out, uint8_t unit_size) {
    /*
    Encoding style - (number of units, value within the unit), (...), ...
    More effective for things like colors, which we can organize with unit sizes of 3
    */
    if (n == 0) {
        return 0;
    }

    if (unit_size == 0) {
        return SIZE_MAX;
    }
    
    const unsigned char *cur_unit = in;
    int cur_num_units = 0;
    size_t num_bytes_written = 0;

    size_t i;

    for (i = 0; i + unit_size <= n; i += unit_size) { // Make sure there's at least a full unit left to encode
        if (cur_num_units >= 255 || memcmp(in + i, cur_unit, unit_size) != 0) { // Max byte size or new unit value found (...AAAAA | BBB...)
            // Write (count, value) pair into out
            out[num_bytes_written++] = cur_num_units; // The count
            memcpy(out + num_bytes_written, cur_unit, unit_size); // The value

            num_bytes_written += unit_size;
            cur_unit = in + i;
            cur_num_units = 1;
        } else {
            ++cur_num_units;
        }
    }
    
    // Write any remaining chars into a final pair
    if (cur_num_units > 0) { // Edge case
        size_t bytes_left = n - i;
        out[num_bytes_written++] = cur_num_units;
        memcpy(out + num_bytes_written, cur_unit, bytes_left);

        num_bytes_written += bytes_left;
    }

    return num_bytes_written;
}

// (8, A) (3, B) (6, C) (1, D) 

/* 
Decodes n bytes of (count, value) pairs from 'in' into 'out'.
'out_cap' is how many bytes 'out' can hold.
Returns the number of bytes written, or SIZE_MAX on error.
*/
size_t rle_decode(const unsigned char *in, size_t n, unsigned char *out, size_t out_cap, uint8_t unit_size) {
    if ((n % 2) != 0) { // Odd number of bytes, unexpected
        return SIZE_MAX;
    }

    size_t num_bytes_written = 0;

    for (size_t i = 0; i < n; i += 2) {
        if (in[i] > (out_cap - num_bytes_written)) { // Writing would cause overflow
            return SIZE_MAX;
        }

        // in[i] = count, in[i+1] = value 
        memset(out + num_bytes_written, in[i+1], in[i]); // Start at out indexed by num_bytes_written

        num_bytes_written += in[i];
    }

    return num_bytes_written;
}

/*
Writes a header to the encoded/compressed file
Header gives information for original size and unit size
Assumes the file is opened properly
*/
int write_header(FILE *fp, uint64_t original_size, uint8_t unit_size) {
    size_t original_size_written = fwrite(&original_size, sizeof(original_size), 1, fp);
    size_t unit_size_written = fwrite(&unit_size, sizeof(unit_size), 1, fp);

    if (original_size_written != 1 || unit_size_written != 1) {
        printf("Issue when writing header...\n");
        return -1;
    }

    return 0;
}


int main(int argc, char *argv[]) {
    unsigned char *input;
    size_t input_size = 0;
    int use_file = 0;
    uint8_t unit_size = 3;

    if (argc <= 1) {
        printf("No file provided, defaulting to text\n");
        input = (unsigned char *) "AAAAAAABBBBBBBBCCCDDE";
        input_size = strlen((const char*) input);
    } else {
        use_file = 1;

        const char *filename = argv[1];
        FILE *fp = fopen(filename, "rb");
        if (fp == NULL) {
            printf("Failed to open original file...\n");
            return -1;
        }

        if (fseek(fp, 0, SEEK_END) != 0) { // Move file pointer to EOF
            fclose(fp);
            return -1;
        }

        long input_len = ftell(fp); // Get cur pos of fp
        if (input_len < 0) {
            printf("Invalid file length...\n");
            return -1;
        }

        input_size = (size_t)input_len;

        rewind(fp);

        input = malloc(input_size);
        if (input == NULL) {
            printf("Malloc failed...\n");
            return -1;
        }

        size_t elements_read = fread(input, 1, input_size, fp);
        if (elements_read != input_size) {
            printf("File length read != actual file length...\n");
            return -1;
        }

        fclose(fp);
    }

    printf("Number of original bytes: %zu\n", input_size);

    unsigned char *encoded = malloc(2 * input_size);
    if (encoded == NULL) {
        printf("Encode malloc failed...\n");
        return -1;
    }

    size_t bytes_encoded = rle_encode(input, input_size, encoded, unit_size);
    printf("Number of encoded bytes:  %zu\n", bytes_encoded);

    if (argc >= 3) {
        const char *encoded_file = argv[2];
        FILE *fp = fopen(encoded_file, "wb");
        if (fp == NULL) {
            printf("Failed to open encoded file...\n");
            return -1;
        }

        if (write_header(fp, input_size, unit_size) != 0) {
            printf("Error writing header to encoded file...\n");
            return -1;
        }

        size_t encoded_written = fwrite(encoded, 1, bytes_encoded, fp);
        if (encoded_written != bytes_encoded) {
            printf("Issue when writing encoded file...\n");
            return -1;
        }

        fclose(fp);
    }


    unsigned char *decoded = malloc(input_size);
    if (decoded == NULL) {
        printf("Decode malloc failed...\n");
        return -1;
    }

    size_t bytes_decoded = rle_decode(encoded, bytes_encoded, decoded, input_size, unit_size);
    printf("Number of decoded bytes:  %zu\n", bytes_decoded);

    if (memcmp(input, decoded, input_size) != 0) {
        printf("\nDiffers...\n");
    } else {
        printf("\nMatch!\n");
    }

    if (use_file == 1) {
        free(input);
    }

    free(encoded);
    free(decoded);

    return 0;
}