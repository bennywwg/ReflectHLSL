template<
	typename VectorConfig,
	typename BufferConfig
>
struct Generator {
	using float1 = float;
	using float2 = VectorConfig::template Vector<2, float>::Type;
	using float3 = VectorConfig::template Vector<3, float>::Type;
	using float4 = VectorConfig::template Vector<4, float>::Type;
	using int1 = int32_t;
	using int2 = VectorConfig::template Vector<2, int32_t>::Type;
	using int3 = VectorConfig::template Vector<3, int32_t>::Type;
	using int4 = VectorConfig::template Vector<4, int32_t>::Type;
	using uint1 = uint32_t;
	using uint2 = VectorConfig::template Vector<2, uint32_t>::Type;
	using uint3 = VectorConfig::template Vector<3, uint32_t>::Type;
	using uint4 = VectorConfig::template Vector<4, uint32_t>::Type;
	using double1 = double;
	using double2 = VectorConfig::template Vector<2, double>::Type;
	using double3 = VectorConfig::template Vector<3, double>::Type;
	using double4 = VectorConfig::template Vector<4, double>::Type;
	using float4x4 = VectorConfig::template Matrix<4, float>::Type;

	template<typename T>
	using StructuredBuffer = BufferConfig::template Buffer<T>::Type;

	template<typename T>
	using RWStructuredBuffer = BufferConfig::template RWBuffer<T>::Type;

	struct Program {
		struct BufType {
			int i;
			float f;
		};
		StructuredBuffer<BufType> Buffer0; // : register(t0)
		StructuredBuffer<BufType> Buffer1; // : register(t1)
		RWStructuredBuffer<BufType> BufferOut; // : register(u0)
		static constexpr uint3 CSMainInvokeSize = uint3(1, 1, 1);
	};
};
