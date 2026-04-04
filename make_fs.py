import struct

SECTOR_SIZE = 512
TOTAL_SECTORS = 200
FS_TABLE_SECTOR = 21
DATA_START = 22
MAX_FILES = 16
FILENAME_LEN = 12

# 🔥 FILE LIST
files = [
    ("hello.txt", b"Hello from LeosFS!\n"),
    ("app.bin", open("app.bin", "rb").read()),
]

img = bytearray(SECTOR_SIZE * TOTAL_SECTORS)

entries = []
current_sector = DATA_START

for name, data in files:
    size = (len(data) + 511) // 512

    # Write file data
    for i in range(size):
        chunk = data[i*512:(i+1)*512]
        start = (current_sector + i) * SECTOR_SIZE
        img[start:start+len(chunk)] = chunk

    # Create file entry
    name_bytes = name.encode()[:FILENAME_LEN]
    name_bytes += b'\x00' * (FILENAME_LEN - len(name_bytes))

    entry = struct.pack("<12sii", name_bytes, current_sector, size)
    entries.append(entry)

    current_sector += size

# Fill remaining entries
while len(entries) < MAX_FILES:
    entries.append(struct.pack("<12sii", b'\x00'*12, 0, 0))

# Write file table
table = b''.join(entries)
start = FS_TABLE_SECTOR * SECTOR_SIZE
img[start:start+len(table)] = table

# Save image
with open("fs.img", "wb") as f:
    f.write(img)

print("fs.img created with multiple files")
