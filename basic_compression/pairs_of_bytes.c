#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
Encode 'in' into (count, value) pairs in 'out'
Return the number of bytes written to 'out'
Output needs to be at least 2*n bytes long (think of ABCD --> A1B1C1D1)
*/
size_t rle_encode(const unsigned char *in, size_t n, unsigned char *out) {
    if (n == 0) {
        return 0;
    }
    
    unsigned char cur_char = in[0];
    int cur_num_chars = 0;
    size_t num_bytes_written = 0;
    
    for (size_t i = 0; i < n; ++i) {
        if (cur_num_chars >= 255 || in[i] != cur_char) { // Max byte size or new char found (...AAAAABBB...)
            out[num_bytes_written++] = cur_num_chars;
            out[num_bytes_written++] = cur_char;

            cur_char = in[i];
            cur_num_chars = 1;
        } else {
            ++cur_num_chars;
        }
    }
    
    // Write any remaining chars into a final pair
    out[num_bytes_written++] = cur_num_chars;
    out[num_bytes_written++] = cur_char;

    return num_bytes_written;
}

// (8, A) (3, B) (6, C) (1, D) 

/* 
Decodes n bytes of (count, value) pairs from 'in' into 'out'.
'out_cap' is how many bytes 'out' can hold.
Returns the number of bytes written, or SIZE_MAX on error.
*/
size_t rle_decode(const unsigned char *in, size_t n, unsigned char *out, size_t out_cap) {
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



int main(int argc, char *argv[]) {
    unsigned char *input;
    size_t input_size = 0;

    if (argc <= 1) {
        input = (unsigned char *) "AAAAAAABBBBBBBBCCCDDE";
        input_size = strlen((const char*) input);
    } else {
        const char *filename = argv[1];
        FILE *fp = fopen(filename, "rb");
        if (fp == NULL) {
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

    size_t bytes_encoded = rle_encode(input, input_size, encoded);

    // for (size_t i = 0; i < bytes_encoded; i += 2) {
    //     printf("(%d, %c) ", encoded[i], encoded[i + 1]);
    // }

    printf("Number of encoded bytes:  %zu\n", bytes_encoded);

    unsigned char *decoded = malloc(input_size);
    if (decoded == NULL) {
        printf("Decode malloc failed...\n");
        return -1;
    }

    size_t bytes_decoded = rle_decode(encoded, bytes_encoded, decoded, input_size);

    // for (size_t i = 0; i < bytes_decoded; ++i) {
    //     printf("%c", decoded[i]);
    // }

    printf("Number of decoded bytes:  %zu\n", bytes_decoded);

    if (memcmp(input, decoded, input_size) != 0) {
        printf("\nDiffers...\n");
    } else {
        printf("\nMatch!\n");
    }

    free(input);

    return 0;
}