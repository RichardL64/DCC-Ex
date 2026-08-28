INPUT_FILENAME = "petscii_ascii_mk2_pf.64c"  # Make sure this matches your file name
OUTPUT_FILENAME = "petscii_16x16_font.h"

def upscale_glyph(char_bytes):
    """
    Takes an 8-byte array (8x8 pixels, 1 bit per pixel)
    and scales it up 2x to a 32-byte array (16x16 pixels).
    """
    expanded = bytearray()

    for row in char_bytes:
        wide_row = 0
        for bit in range(8):
            if row & (1 << (7 - bit)):
                wide_row |= (3 << (14 - (bit * 2)))

        high_byte = (wide_row >> 8) & 0xFF
        low_byte = wide_row & 0xFF

        expanded.append(high_byte)
        expanded.append(low_byte)
        expanded.append(high_byte)
        expanded.append(low_byte)

    return expanded

def main():
    with open(INPUT_FILENAME, "rb") as f:
        raw_bytes = f.read()

    if (len(raw_bytes) - 8) % 8 == 0:
        print("Detected 8-byte file header. Stripping header bytes...")
        raw_bytes = raw_bytes[8:]
    elif (len(raw_bytes) - 2) % 8 == 0:
        print("Detected 2-byte file header. Stripping header bytes...")
        raw_bytes = raw_bytes[2:]
    else:
        print(f"Warning: File size ({len(raw_bytes)} bytes) is not a standard multiple after checking headers!")

    num_chars = len(raw_bytes) // 8

    with open(OUTPUT_FILENAME, "w") as out:
        out.write(f"// Auto-generated 16x16 upscaled font from binary {INPUT_FILENAME}\n")
        out.write("const PROGMEM unsigned char petscii_16x16[] = {\n")

        total_bytes_written = 0
        for i in range(num_chars):
            char_bytes = raw_bytes[i*8 : (i+1)*8]
            upscaled = upscale_glyph(char_bytes)

            # Determine a safe printable representation for the comment
            if 32 <= i <= 126:
                char_repr = chr(i)
                if char_repr == '\\':
                    char_repr = '\\\\'
                elif char_repr == "'":
                    char_repr = "\\'"
                comment = f" // Char {i} ('{char_repr}')"
            else:
                comment = f" // Char {i}"

            # Write the 32 bytes for this character block
            for b_idx, b in enumerate(upscaled):
                if b_idx == 0:
                    out.write(" ")
                out.write(f"0x{b:02X},")
                if b_idx < len(upscaled) - 1:
                    out.write(" ")

            out.write(f"{comment}\n")
            total_bytes_written += len(upscaled)

        out.write("};\n")

    print(f"Successfully processed {num_chars} characters into {OUTPUT_FILENAME} ({total_bytes_written} total bytes).")

if __name__ == "__main__":
    main()