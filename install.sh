
#!/bin/bash
set -e

BUILD_TYPE="${1:-Release}"

echo "building terminyl (${BUILD_TYPE})..."
cmake -S . -B build -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
cmake --build build --parallel

printf "One of the lessons of war is that institutions, while powerful and long-lasting, are often not insuperably rigid _if the emergency in great enough_.\n\n\n\n\n"
sudo cmake --install build

echo "done, run 'terminyl' to use."
