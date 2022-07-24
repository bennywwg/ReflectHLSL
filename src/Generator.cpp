#include "Generator.hpp"

namespace ReflectHLSL {
	void DefinesContext::ReplaceDefine(std::string& inout) const {
		const std::string from = "#define";
		const std::string to = "#undef";
		size_t start_pos = 0;
		while ((start_pos = inout.find(from, start_pos)) != std::string::npos) {
			inout.replace(start_pos, from.length(), to);
			start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
		}
	}
	std::string DefinesContext::GetDefs() const {
		std::string res;
		for (std::string str : Defines) {
			if (str.find("#define") == std::string::npos) continue;
			res += str + "\n";
		}
		return res;
	}
	std::string DefinesContext::GetUndefs() const {
		std::string res;
		for (std::string str : Defines) {
			if (str.find("#define") == std::string::npos) continue;
			ReplaceDefine(str);
			res += str + "\n";
		}
		return res;
	}
	std::string Generate(GenerationContext const& ctx, DefinesContext const& dctx) {
		const std::string part0 =
			"template<\n"
			"	typename VectorConfig,\n"
			"	typename BufferConfig,\n"
			"	typename Context\n"
			">\n"
			"struct Generator {\n"
			"	using float1 = float;\n"
			"	using float2 = VectorConfig::template Vector<2, float>::typename Type;\n"
			"	using float3 = VectorConfig::template Vector<3, float>::typename Type;\n"
			"	using float4 = VectorConfig::template Vector<4, float>::typename Type;\n"
			"	using int1 = int32_t;\n"
			"	using int2 = VectorConfig::template Vector<2, int32_t>::typename Type;\n"
			"	using int3 = VectorConfig::template Vector<3, int32_t>::typename Type;\n"
			"	using int4 = VectorConfig::template Vector<4, int32_t>::typename Type;\n"
			"	using uint1 = uint32_t;\n"
			"	using uint2 = VectorConfig::template Vector<2, uint32_t>::typename Type;\n"
			"	using uint3 = VectorConfig::template Vector<3, uint32_t>::typename Type;\n"
			"	using uint4 = VectorConfig::template Vector<4, uint32_t>::typename Type;\n"
			"	using double1 = double;\n"
			"	using double2 = VectorConfig::template Vector<2, double>::typename Type;\n"
			"	using double3 = VectorConfig::template Vector<3, double>::typename Type;\n"
			"	using double4 = VectorConfig::template Vector<4, double>::typename Type;\n"
			"	using float4x4 = VectorConfig::template Matrix<4, float>::typename Type;\n"
			"\n"
			"	template<typename T>\n"
			"	using StructuredBuffer = BufferConfig::template Buffer<T>::typename Type;\n"
			"\n"
			"	template<typename T>\n"
			"	using RWStructuredBuffer = BufferConfig::template RWBuffer<T>::typename Type;\n"
			"\n"
			"	struct Program {\n";

		return part0 +
			dctx.GetDefs() +
			ctx.Output +
			dctx.GetUndefs() +
			"	};\n"
			"};\n";
	}
}