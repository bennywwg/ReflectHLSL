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
		struct PixelShaderInput {
			min16float4 pos; // : SV_POSITION
			min16float3 color; // : COLOR0
			min16float2 texCoord; // : TEXCOORD1
		};
		Texture2D<float3> tex; // : t0
		SamplerState samp; // : s0

		// min16float4
		// - PixelShaderInput input
		inline Program(Context& ctx)
		{ }
	};
};
