#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

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

/* 
Decodes n bytes of (count, value) pairs from 'in' into 'out'.
'out_cap' is how many bytes 'out' can hold.
Returns the number of bytes written, or SIZE_MAX on error.
*/
size_t rle_decode(const unsigned char *in, size_t n, unsigned char *out, size_t out_cap) {
    if ((n % 2) != 0) { // Odd number of bytes, unexpected
        return SIZE_MAX;
    }

    int cur_pair = 0;
    size_t num_bytes_written = 0;

    for (size_t i = 0; i < n; i += 2) {
        for (size_t j = 0; j < in[cur_pair]) {
            out[num_bytes_written++] =
        }

        ++cur_pair;
    }
}

int main() {
    const unsigned char input[] = "AAAAAAAABBBCCCCCCD";
    unsigned char output[2 * sizeof(input)];

    size_t encoded_size = rle_encode(input, sizeof(input) - 1, output);

    for (size_t i = 0; i < encoded_size; i += 2) {
        printf("(%d, %c) ", output[i], output[i + 1]);
    }

    return 0;
}