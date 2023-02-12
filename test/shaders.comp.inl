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
#define NUM_LIGHTS 3
#define SHADOW_DEPTH_BIAS 0.00005f
#define SHADOW_DEPTH_BIAS \
            54580
		Texture2D shadowMap; // : register
		Texture2D diffuseMap; // : register
		Texture2D normalMap; // : register
		SamplerState sampleWrap; // : register
		SamplerState sampleClamp; // : register
		int val = 5;
		bruh d = { { 4, 5, 6 }, 4, { 4, 5, 6 } };
		struct Blah {
		};
		struct LightState {
			float3 position;
			float3 direction;
			float4 color;
			float4 falloff;
			float4x4 view;
			float4x4 projection;
			struct Inner {
			};
		};
		struct SceneConstantBuffer { // : register
			float4x4 model;
			float4x4 view;
			float4x4 projection;
			float4 ambientColor;
			bool sampleShadowMap;
			LightState lights[NUM_LIGHTS];
		};
		struct PSInput {
			float4 position; // : SV_POSITION
			float4 worldpos; // : POSITION
			float2 uv; // : TEXCOORD0
			float3 normal; // : NORMAL
			float3 tangent; // : TANGENT
		};

		// float3
		// - float2 vTexcoord
		// - float3 vVertNormal
		// - float3 vVertTangent
		static constexpr uint3 InvokeSize = uint3(1, 1, 1);

		// float4
		// - float3 vLightPos
		// - float3 vLightDir
		// - float4 vLightColor
		// - float4 vFalloffs
		// - float3 vPosWorld
		// - float3 vPerPixelNormal
		static constexpr uint3 InvokeSize = uint3(1, 1, 1);

		// float4
		// - int lightIndex
		// - float4 vPosWorld
		static constexpr uint3 InvokeSize = uint3(1, 1, 1);

		// PSInput
		// - float3 position
		// - float3 normal
		// - float2 uv
		// - float3 tangent
		static constexpr uint3 InvokeSize = uint3(1, 1, 1);

		// float4
		// - PSInput input
		static constexpr uint3 InvokeSize = uint3(1, 1, 1);

		inline Program(Context& ctx)
		{ }
#undef NUM_LIGHTS 3
#undef SHADOW_DEPTH_BIAS 0.00005f
#undef SHADOW_DEPTH_BIAS \
            54580
	};
};
