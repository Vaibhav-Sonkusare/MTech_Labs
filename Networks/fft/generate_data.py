import os
import sys

def generate_file(filename, size_mb):
    size_bytes = int(size_mb * 1024 * 1024)
    with open(filename, 'wb') as f:
        # Write in chunks to avoid memory spikes
        chunk_size = 1024 * 1024
        written = 0
        while written < size_bytes:
            to_write = min(chunk_size, size_bytes - written)
            f.write(os.urandom(to_write))
            written += to_write
    print(f"Generated {filename} ({size_mb} MB)")

if __name__ == "__main__":
    if not os.path.exists("test_files"):
        os.makedirs("test_files")
        
    generate_file("test_files/1mb.bin", 1)
    generate_file("test_files/10mb.bin", 10)
    # generate_file("test_files/100mb.bin", 100) # Optional, might take longer
