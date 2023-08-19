//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Test {

#pragma pack(push)
#pragma pack (1)

	struct GPUThing {
		static GPUThing Init(int i) {
			return GPUThing{
				-1
				,i
				,i * 0.001f
				,Neshny::fVec2(i * 1.5, i * 2.5)
				,Neshny::fVec3(i * 3.5, i * 4.5, i * 5.5)
				,Neshny::fVec4(i * 6.5, i * 7.5, i * 8.5, i * 9.5)
				,Neshny::iVec2(i * 10, i * 20)
				,Neshny::iVec3(i * 30, i * 40, i * 50)
				,Neshny::iVec4(i * 60, i * 70, i * 80, i * 90)
			};
		}
		static void Add(GPUThing& item, float v) {
			item.p_Float += v;
			item.p_TwoDim += Neshny::fVec2(v, v);
			item.p_ThreeDim += Neshny::fVec3(v, v, v);
			item.p_FourDim += Neshny::fVec4(v, v, v, v);
			const int int_val = int(floor(v));
			item.p_IntTwoDim += Neshny::iVec2(int_val, int_val);
			item.p_IntThreeDim += Neshny::iVec3(int_val, int_val, int_val);
			item.p_IntFourDim += Neshny::iVec4(int_val, int_val, int_val, int_val);
		}
		bool CloseEnough(const GPUThing& other) {
			double delta = 0.0001;

			std::vector<bool> test = {
				(fabs(p_Float - other.p_Float) < delta),
				(fabs(p_TwoDim.x - other.p_TwoDim.x) < delta),
				(fabs(p_TwoDim.y - other.p_TwoDim.y) < delta),
				(fabs(p_ThreeDim.x - other.p_ThreeDim.x) < delta),
				(fabs(p_ThreeDim.y - other.p_ThreeDim.y) < delta),
				(fabs(p_ThreeDim.z - other.p_ThreeDim.z) < delta),
				(fabs(p_FourDim.x - other.p_FourDim.x) < delta),
				(fabs(p_FourDim.y - other.p_FourDim.y) < delta),
				(fabs(p_FourDim.z - other.p_FourDim.z) < delta),
				(fabs(p_FourDim.w - other.p_FourDim.w) < delta),
				(p_IntTwoDim.x == other.p_IntTwoDim.x),
				(p_IntTwoDim.y == other.p_IntTwoDim.y),
				(p_IntThreeDim.x == other.p_IntThreeDim.x),
				(p_IntThreeDim.y == other.p_IntThreeDim.y),
				(p_IntThreeDim.z == other.p_IntThreeDim.z),
				(p_IntFourDim.x == other.p_IntFourDim.x),
				(p_IntFourDim.y == other.p_IntFourDim.y),
				(p_IntFourDim.z == other.p_IntFourDim.z),
				(p_IntFourDim.w == other.p_IntFourDim.w)
			};

			bool result = 
				(fabs(p_Float - other.p_Float) < delta) &&
				(fabs(p_TwoDim.x - other.p_TwoDim.x) < delta) &&
				(fabs(p_TwoDim.y - other.p_TwoDim.y) < delta) &&
				(fabs(p_ThreeDim.x - other.p_ThreeDim.x) < delta) &&
				(fabs(p_ThreeDim.y - other.p_ThreeDim.y) < delta) &&
				(fabs(p_ThreeDim.z - other.p_ThreeDim.z) < delta) &&
				(fabs(p_FourDim.x - other.p_FourDim.x) < delta) &&
				(fabs(p_FourDim.y - other.p_FourDim.y) < delta) &&
				(fabs(p_FourDim.z - other.p_FourDim.z) < delta) &&
				(fabs(p_FourDim.w - other.p_FourDim.w) < delta) &&
				(p_IntTwoDim.x == other.p_IntTwoDim.x) &&
				(p_IntTwoDim.y == other.p_IntTwoDim.y) &&
				(p_IntThreeDim.x == other.p_IntThreeDim.x) &&
				(p_IntThreeDim.y == other.p_IntThreeDim.y) &&
				(p_IntThreeDim.z == other.p_IntThreeDim.z) &&
				(p_IntFourDim.x == other.p_IntFourDim.x) &&
				(p_IntFourDim.y == other.p_IntFourDim.y) &&
				(p_IntFourDim.z == other.p_IntFourDim.z) &&
				(p_IntFourDim.w == other.p_IntFourDim.w);
			if (!result) {
				int rlk = 0;
			}
			return result;
		}
		int				p_Id;
		int				p_Int;
		float			p_Float;
		Neshny::fVec2	p_TwoDim;
		Neshny::fVec3	p_ThreeDim;
		Neshny::fVec4	p_FourDim;
		Neshny::iVec2	p_IntTwoDim;
		Neshny::iVec3	p_IntThreeDim;
		Neshny::iVec4	p_IntFourDim;
	};

	struct Uniform {
		int				p_Mode;
		float			p_Value;
	};


#pragma pack(pop)
}

namespace meta {
	template<> inline auto registerMembers<Test::GPUThing>() {
		return members(
			member("Id", &Test::GPUThing::p_Id)
			,member("Int", &Test::GPUThing::p_Int)
			,member("Float", &Test::GPUThing::p_Float)
			,member("TwoDim", &Test::GPUThing::p_TwoDim)
			,member("ThreeDim", &Test::GPUThing::p_ThreeDim)
			,member("FourDim", &Test::GPUThing::p_FourDim)
			,member("IntTwoDim", &Test::GPUThing::p_IntTwoDim)
			,member("IntThreeDim", &Test::GPUThing::p_IntThreeDim)
			,member("IntFourDim", &Test::GPUThing::p_IntFourDim)
		);
	}
	template<> inline auto registerMembers<Test::Uniform>() {
		return members(
			member("Mode", &Test::Uniform::p_Mode)
			,member("Value", &Test::Uniform::p_Value)
		);
	}
}

namespace Test {

	////////////////////////////////////////////////////////////////////////////////
	void CompareEntities(QString msg_prefix, std::vector<GPUThing> expected, std::vector<GPUThing> actual) {
		Expect("Size mismatch", expected.size() <= actual.size());

		auto sort_func = [](const GPUThing& a, const GPUThing& b) { return a.p_Id > b.p_Id; };
		std::sort(expected.begin(), expected.end(), sort_func);
		std::sort(actual.begin(), actual.end(), sort_func);

		for (int i = 0; i < expected.size(); i++) {
			if (expected[i].p_Id < 0) {
				break;
			}
			Expect(msg_prefix + " -> Item ID mismatch", expected[i].p_Id == actual[i].p_Id);
			Expect(msg_prefix + " -> Item mismatch", expected[i].CloseEnough(actual[i]));
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void GPUEntityTest(Neshny::GPUEntity::DeleteMode mode) {

		Neshny::GPUEntity entities("Thing", mode, &GPUThing::p_Id, "Id");
		entities.Init(1000);

		const int initial_count = 50;
		std::vector<GPUThing> expected;
		for (int i = 0; i < initial_count; i++) {
			GPUThing thing = GPUThing::Init(i);
			entities.AddInstance(&thing);
			expected.push_back(thing);
		}

		const float in_value = 123.4f;
		const int div_val = 7;


		// add in_value to most values
#if defined(NESHNY_GL)
		Neshny::PipelineStage::ModifyEntity(entities, "UnitTestEntity", true).Run([in_value](Neshny::GLShader* prog) {
			glUniform1i(prog->GetUniform("uMode"), 0);
			glUniform1f(prog->GetUniform("uValue"), in_value);
		});
#elif defined(NESHNY_WEBGPU)
		auto executable = Neshny::PipelineStage::ModifyEntity(entities, "UnitTestEntity", true).AddInputOutputVar("uCheckVal").Prepare<Uniform>();
		int uCheckVal = 0;
		Uniform uniform{ 0, in_value };
		executable->Run(uniform, { { "uCheckVal", &uCheckVal } });
#endif

		for (auto& item : expected) {
			GPUThing::Add(item, in_value);
		}
		std::vector<GPUThing> gpu_values;
		entities.GetSSBO()->GetValues(gpu_values, initial_count);

		CompareEntities("After first add", expected, gpu_values);

		// add in_value to most values again to check double buffering works
#if defined(NESHNY_GL)
		Neshny::PipelineStage::ModifyEntity(entities, "UnitTestEntity", true).Run([in_value](Neshny::GLShader* prog) {
			glUniform1i(prog->GetUniform("uMode"), 0);
			glUniform1f(prog->GetUniform("uValue"), in_value);
		});
#elif defined(NESHNY_WEBGPU)
		executable->Run(uniform, { { "uCheckVal", &uCheckVal } });
#endif

		for (auto& item : expected) {
			GPUThing::Add(item, in_value);
		}
		gpu_values.clear();
		entities.GetSSBO()->GetValues(gpu_values, initial_count);

		CompareEntities("After second add", expected, gpu_values);

		// delete every nth item
#if defined(NESHNY_GL)
		Neshny::PipelineStage::ModifyEntity(entities, "UnitTestEntity", true).Run([div_val](Neshny::GLShader* prog) {
			glUniform1i(prog->GetUniform("uMode"), 1);
			glUniform1f(prog->GetUniform("uValue"), div_val);
		});
#elif defined(NESHNY_WEBGPU)
		uniform.p_Mode = 1;
		uniform.p_Value = div_val;
		executable->Run(uniform, { { "uCheckVal", &uCheckVal } });
#endif

		if(mode == Neshny::GPUEntity::DeleteMode::STABLE_WITH_GAPS) {
			for (auto& item : expected) {
				if (item.p_Int % div_val == 0) {
					item.p_Id = -1;
				}
			}
		} else {
			for (int i = 0; i < 2; i++) { // to avoid skipping the moved ones
				for (auto iter = expected.begin(); iter != expected.end(); iter++) {
					if (iter->p_Int % div_val == 0) {
						RemoveUnordered(expected, iter);
					}
				}
			}
		}

		gpu_values.clear();
		entities.GetSSBO()->GetValues(gpu_values, mode == Neshny::GPUEntity::DeleteMode::STABLE_WITH_GAPS ? initial_count : entities.GetCount());

		CompareEntities("After deletion", expected, gpu_values);

		for (int i = 0; i < 10; i++) {
			GPUThing thing = GPUThing::Init(i + 1000);
			entities.AddInstance(&thing);

			bool found = false;
			for (auto& item : expected) {
				if (item.p_Id < 0) {
					item = thing;
					found = true;
					break;
				}
			}
			if (!found) {
				expected.push_back(thing);
			}
		}

		gpu_values.clear();
		entities.GetSSBO()->GetValues(gpu_values, entities.GetCount());

		CompareEntities("After adding in new values", expected, gpu_values);
	}

	////////////////////////////////////////////////////////////////////////////////
	void UnitTest_GPUEntityStable(void) {
		GPUEntityTest(Neshny::GPUEntity::DeleteMode::STABLE_WITH_GAPS);
    }

	////////////////////////////////////////////////////////////////////////////////
	void UnitTest_GPUEntityMoving(void) {
		GPUEntityTest(Neshny::GPUEntity::DeleteMode::MOVING_COMPACT);
    }

	////////////////////////////////////////////////////////////////////////////////
	void UnitTest_Compute(void) {

#if defined(NESHNY_WEBGPU)
		Neshny::WebGPUPipeline pipe;
		Neshny::WebGPUBuffer buffer(WGPUBufferUsage_Storage);
		Neshny::WebGPUBuffer atomic_buffer(WGPUBufferUsage_Storage, sizeof(int));
		const int num = 1024;
		const int first_pass = 13;
		const int second_pass = 601;
		std::vector<float> values;
		for (int i = 0; i < num; i++) {
			values.push_back(i * 10000);
		}
		buffer.EnsureSizeBytes(num * sizeof(float));
		buffer.SetValues(values);

		Neshny::WebGPUBuffer uniform_buffer(WGPUBufferUsage_Uniform, sizeof(unsigned int));

		pipe
			.AddBuffer(buffer, WGPUShaderStage_Compute, false)
			.AddBuffer(uniform_buffer, WGPUShaderStage_Compute, true)
			.AddBuffer(atomic_buffer, WGPUShaderStage_Compute, false)
			.FinalizeCompute("UnitTest");

		uniform_buffer.SetSingleValue(0, first_pass);
		pipe.Compute(first_pass, Neshny::iVec3(256, 1, 1));

		int atomic_result_first = atomic_buffer.GetSingleValue<int>(0);
		Expect("Atomic value mismatch", atomic_result_first == first_pass);

		std::vector<float> out_values;
		buffer.GetValues(out_values, num);

		for (int i = 0; i < num; i++) {
			float expected = values[i];
			float actual = out_values[i];
			if (i < first_pass) {
				expected += i + 0.25;
			}
			Expect("Value mismatch", expected == actual);
		}

		uniform_buffer.SetSingleValue(0, second_pass);
		pipe.Compute(second_pass, Neshny::iVec3(256, 1, 1));

		int atomic_result_second = atomic_buffer.GetSingleValue<int>(0);
		Expect("Atomic value mismatch", atomic_result_second == (first_pass + second_pass));

		out_values.clear();
		buffer.GetValues(out_values, num);

		for (int i = 0; i < num; i++) {
			float expected = values[i];
			float actual = out_values[i];
			if (i < first_pass) {
				expected += i + 0.25;
			}
			if (i < second_pass) {
				expected += i + 0.25;
			}
			Expect("Value mismatch", expected == actual);
		}
#endif
	}

} // namespace Test