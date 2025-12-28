#!/usr/bin/sh
dxc  patch.hlsl -T ps_6_7  -spirv -Fo frag.spv -E fsMain -fspv-entrypoint-name=main -O3
dxc  patch.hlsl -T vs_6_7  -spirv -Fo vert.spv -E vsMain -fspv-entrypoint-name=main -O3