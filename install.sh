
#!/bin/bash
set -e

BUILD_TYPE="${1:-Release}"

echo "building terminyl (${BUILD_TYPE})..."
cmake -S . -B build -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
cmake --build build --parallel

printf "\n\nOne of the lessons of war is that institutions, while powerful and long-lasting, are often not insuperably rigid \x1b[3mif the emergency is great enough\x1b[0m.\n\n"
sudo cmake --install build

echo "done, run 'terminyl <source file>' to use."
