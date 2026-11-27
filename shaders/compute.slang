[[vk_binding(0, 0)]]
StructuredBuffer<float> vec_a;

[[vk_binding(1, 0)]]
StructuredBuffer<float> vec_b;

[[vk_binding(2, 0)]]
RWStructuredBuffer<float> vec_c;

[[vk_push_constant]]
ConstantBuffer<uint> size;

[[numthreads(64, 1, 1)]]
[[shader("compute")]]
void main(uint3 id: SV_DispatchThreadID) {
    if (id.x < size) {
        vec_c[id.x] = vec_a[id.x] + vec_b[id.x];
    }
}
