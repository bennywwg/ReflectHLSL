template<
	typename VectorConfig,
	typename BufferConfig,
	typename TextureConfig,
	typename Context
>
struct Generator {
	using float1 = float;
	using float2 = VectorConfig::template Vector<2, float>::typename Type;
	using float3 = VectorConfig::template Vector<3, float>::typename Type;
	using float4 = VectorConfig::template Vector<4, float>::typename Type;
	using int1 = int32_t;
	using int2 = VectorConfig::template Vector<2, int32_t>::typename Type;
	using int3 = VectorConfig::template Vector<3, int32_t>::typename Type;
	using int4 = VectorConfig::template Vector<4, int32_t>::typename Type;
	using uint = uint32_t;
	using uint1 = uint32_t;
	using uint2 = VectorConfig::template Vector<2, uint32_t>::typename Type;
	using uint3 = VectorConfig::template Vector<3, uint32_t>::typename Type;
	using uint4 = VectorConfig::template Vector<4, uint32_t>::typename Type;
	using double1 = double;
	using double2 = VectorConfig::template Vector<2, double>::typename Type;
	using double3 = VectorConfig::template Vector<3, double>::typename Type;
	using double4 = VectorConfig::template Vector<4, double>::typename Type;
	using float4x4 = VectorConfig::template Matrix<4, float>::typename Type;

	template<typename T>
	using StructuredBuffer = BufferConfig::template Buffer<T>::typename Type;

	template<typename T>
	using RWStructuredBuffer = BufferConfig::template RWBuffer<T>::typename Type;

	template<typename T>
	using Texture2D = TextureConfig::template Texture2D<T>::typename Type;

	template<typename T>
	using RWTexture2D = TextureConfig::template RWTexture2D<T>::typename Type;

	struct Program {
		struct BufType {
			int i;
			float f;
		};
		StructuredBuffer<BufType> Buffer0; // : register
		StructuredBuffer<BufType> Buffer1; // : register
		RWStructuredBuffer<BufType> BufferOut; // : register

		// void
		// - uint3 DTid
		static constexpr uint3 InvokeSize = uint3(1, 1, 1);

		inline Program(Context& ctx)
		: Buffer0(ctx, "Buffer0", "t0")
		, Buffer1(ctx, "Buffer1", "t1")
		, BufferOut(ctx, "BufferOut", "u0", "s", 4)
		{ }
	};
};
