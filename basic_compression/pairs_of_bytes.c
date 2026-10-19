#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
Encode 'in' into (count, value) pairs in 'out'
Return the number of bytes written to 'out'
Output needs to be at least 2*n bytes long (think of ABCD) (for unite size of 1)
*/
size_t rle_encode(const unsigned char *in, size_t original_size, unsigned char *out, uint8_t unit_size) {
    /*
    Encoding style - (number of units, value within the unit), (...), ...
    More effective for things like colors, which we can organize with unit sizes of 3
    */
    if (original_size == 0) {
        return 0;
    }

    if (unit_size == 0) {
        return SIZE_MAX;
    }
    
    const unsigned char *cur_unit = in; // Index pointer, move when value changes
    int cur_num_units = 0;
    size_t num_bytes_written = 0;

    size_t i;

    for (i = 0; i + unit_size <= original_size; i += unit_size) { // Make sure there's at least a full unit left to encode
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
    
    // Final run: a normal pair, same as in the loop
    // Writing a (count, value) pair is only triggered if a new unit_size value is found
    // This never happens for the last pair, so it must be separately written to out
    if (cur_num_units > 0) {
        out[num_bytes_written++] = cur_num_units;
        memcpy(out + num_bytes_written, cur_unit, unit_size);

        num_bytes_written += unit_size;
    }

    // Leftover bytes that don't fill a unit: copied as-is, always
    size_t bytes_left = original_size - i;
    memcpy(out + num_bytes_written, in + i, bytes_left);
    num_bytes_written += bytes_left;

    return num_bytes_written;
}

// (8, A B C) => 8 repeats of bytes A B C

/* 
Decodes n bytes of (count, value) pairs from 'in' into 'out'.
'original_size' is the size of the data before encoding.
Returns the number of bytes written, or SIZE_MAX on error.
*/
size_t rle_decode(const unsigned char *in, size_t n, unsigned char *out, size_t original_size, uint8_t unit_size) {
    if (unit_size == 0) { 
        return SIZE_MAX;
    }

    // Pairs are unit_size + 1 bytes, w/ n - i leftover bytes
    size_t num_leftover_bytes = original_size % unit_size;

    if (((n - num_leftover_bytes) % (unit_size + 1)) != 0) { // Check for unusual byte count
        return SIZE_MAX;
    }


    size_t num_bytes_written = 0;
    size_t i;

    for (i = 0; i + unit_size < n; i += (unit_size + 1)) { // Increment by (count, value) pair
        if ((in[i] * unit_size) > (original_size - num_bytes_written)) { // Writing would cause overflow
            return SIZE_MAX;
        }

        // in[i] = count, in[i+1] = value extending unit_size bytes
        for (size_t j = 0; j < in[i]; ++j) {
            memcpy(out + num_bytes_written, in + i + 1, unit_size);
            num_bytes_written += unit_size;
        }
    }

    // Copy leftover bytes at the end directly to the output address
    memcpy(out + num_bytes_written, in + i, num_leftover_bytes);
    num_bytes_written += num_leftover_bytes;

    if (num_bytes_written != original_size) {
        printf("Decode malfunctioned...\n");
        return SIZE_MAX;
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

    // ./rle.exe encode red.bmp red.rle
    // ./rle.exe decode red.rle red_out.bmp

    if (argc < 4) {
        printf("No file provided, defaulting to text\n");
        input = (unsigned char *) "AAAAAAABBBBBBBBCCCDDE";
        input_size = strlen((const char*) input);
    } else {
        if (strcmp(argv[1], "encode") == 0) { //Encode path ==================================
            // Read the original file and put it into an input buffer
            const char *original_file = argv[2];
            FILE *fp_orig = fopen(original_file, "rb");
            if (fp_orig == NULL) {
                printf("Failed to open original file...\n");
                return -1;
            }

            if (fseek(fp_orig, 0, SEEK_END) != 0) { // Move file pointer to EOF
                fclose(fp_orig);
                return -1;
            }

            long input_len = ftell(fp_orig); // Get current position of fp_orig
            if (input_len < 0) {
                printf("Invalid file length...\n");
                return -1;
            }

            input_size = (size_t)input_len;

            rewind(fp_orig);

            input = malloc(input_size);
            if (input == NULL) {
                printf("Malloc failed...\n");
                return -1;
            }

            size_t elements_read = fread(input, 1, input_size, fp_orig);
            if (elements_read != input_size) {
                printf("File length read != actual file length...\n");
                return -1;
            }

            fclose(fp_orig);

            printf("Number of original bytes: %zu\n", input_size);
            
            // Encode the original data
            unsigned char *encoded = malloc(2 * input_size);
            if (encoded == NULL) {
                printf("Encode malloc failed...\n");
                return -1;
            }

            size_t bytes_encoded = rle_encode(input, input_size, encoded, unit_size);
            printf("Number of encoded bytes:  %zu\n", bytes_encoded);

            // Write encoded data to a file
            const char *encoded_file = argv[3];
            FILE *fp_enc = fopen(encoded_file, "wb");
            if (fp_enc == NULL) {
                printf("Failed to open encoded file...\n");
                return -1;
            }

            if (write_header(fp_enc, input_size, unit_size) != 0) {
                printf("Error writing header to encoded file...\n");
                return -1;
            }

            size_t encoded_written = fwrite(encoded, 1, bytes_encoded, fp_enc);
            if (encoded_written != bytes_encoded) {
                printf("Issue when writing encoded file...\n");
                return -1;
            }

            fclose(fp_enc);
        } else if (strcmp(argv[1], "decode") == 0) { // Decode path ==================================
            unsigned char *decoded = malloc(input_size);
            if (decoded == NULL) {
                printf("Decode malloc failed...\n");
                return -1;
            }

            size_t bytes_decoded = rle_decode(encoded, bytes_encoded, decoded, input_size, unit_size);
            printf("Number of decoded bytes:  %zu\n", bytes_decoded);
        } else {
            printf("Please use the format: ./rle.exe --encode/--decode <file_to_enc/dec> <file_out>\n");
            return -1;
        }
    }

    

    if (memcmp(input, decoded, input_size) != 0) {
        printf("\nDiffers...\n");
    } else {
        printf("\nMatch!\n");
    }


    free(input);
    free(encoded);
    free(decoded);

    return 0;
}