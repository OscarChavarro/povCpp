import sys

# Read both files and compare byte by byte
with open('render_test/chess_legacy.tga', 'rb') as f:
    legacy = f.read()
with open('render_test/chess_antlr.tga', 'rb') as f:
    antlr = f.read()

# TGA header is usually 18 bytes, then image data
# For a 600x450 image with 32-bit RGBA: 600*450*4 = 1080000 bytes
# But POV-Ray may use different formats

# Skip the first 18 bytes (TGA header) and compare image data
header_size = 18
image_start = header_size

# Find where differences start and their frequency
diff_count = 0
first_diff_at = -1
diff_sections = []

for i in range(len(legacy)):
    if legacy[i] != antlr[i]:
        if first_diff_at == -1:
            first_diff_at = i
        diff_count += 1
        if len(diff_sections) == 0 or i - diff_sections[-1][1] > 100:
            diff_sections.append([i, i])
        else:
            diff_sections[-1][1] = i

print(f"Total bytes: {len(legacy)}")
print(f"Different bytes: {diff_count} ({100*diff_count/len(legacy):.1f}%)")
print(f"First difference at byte: {first_diff_at}")
print(f"Number of diff sections: {len(diff_sections)}")
print("\nFirst 5 diff sections:")
for i, (start, end) in enumerate(diff_sections[:5]):
    print(f"  Section {i}: bytes {start}-{end} ({end-start} bytes)")
