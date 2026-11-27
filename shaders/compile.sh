#!/usr/bin/sh
set -eu
cd "$(dirname "$0")"
slangc shader.slang -profile spirv_1_3 -matrix-layout-row-major -o shader.spv -O3
slangc post.slang -profile spirv_1_3 -o post.spv -O3
slangc compute.slang -profile spirv_1_3 -o compute.spv -O3
