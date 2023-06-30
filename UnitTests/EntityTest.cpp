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
}

namespace Test {

    void UnitTest_GPUEntityStable(void) {

#if defined(NESHNY_GL)

		Neshny::GPUEntity entities("Thing", Neshny::GPUEntity::DeleteMode::STABLE_WITH_GAPS, &GPUThing::p_Id, "Id");
		entities.Init(1000);

		const int initial_count = 50;
		std::vector<GPUThing> expected;
		for (int i = 0; i < initial_count; i++) {
			GPUThing thing = GPUThing::Init(i);
			entities.AddInstance(&thing);
			expected.push_back(thing);
		}

		// add in_value to most values
		const float in_value = 123.4f;
		const int div_val = 7;
		Neshny::PipelineStage::ModifyEntity(entities, "UnitTestEntity", true).Run([in_value](Neshny::GLShader* prog) {
			glUniform1i(prog->GetUniform("uMode"), 0);
			glUniform1f(prog->GetUniform("uValue"), in_value);
		});

		for (auto& item : expected) {
			GPUThing::Add(item, in_value);
		}

		std::vector<GPUThing> gpu_values;
		entities.GetSSBO()->GetValues(gpu_values, initial_count);

		// delete every nth item
		Neshny::PipelineStage::ModifyEntity(entities, "UnitTestEntity", true).Run([div_val](Neshny::GLShader* prog) {
			glUniform1i(prog->GetUniform("uMode"), 1);
			glUniform1f(prog->GetUniform("uValue"), div_val);
		});

		for (auto& item : expected) {
			if (item.p_Int % div_val == 0) {
				item.p_Id = -1;
			}
		}

		gpu_values.clear();
		entities.GetSSBO()->GetValues(gpu_values, initial_count);

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

		Expect("Size mismatch", expected.size() == gpu_values.size());
		for (int i = 0; i < (int)gpu_values.size(); i++) {
			const auto& a = expected[i];
			const auto& b = gpu_values[i];
			bool later_a = a.p_Int >= 1000;
			bool later_b = b.p_Int >= 1000;
			Expect("Batch mismatch", later_a == later_b);
		}

#elif defined(NESHNY_WEBGPU)

#endif
    }

    void UnitTest_GPUEntityMoving(void) {

#if defined(NESHNY_GL)

		Neshny::GPUEntity entities("Thing", Neshny::GPUEntity::DeleteMode::MOVING_COMPACT, &GPUThing::p_Id, "Id");
		entities.Init(1000);

		const int initial_count = 50;
		std::vector<GPUThing> expected;
		for (int i = 0; i < initial_count; i++) {
			GPUThing thing = GPUThing::Init(i);
			entities.AddInstance(&thing);
			expected.push_back(thing);
		}

		// add in_value to most values
		const float in_value = 123.4f;
		const int div_val = 7;
		Neshny::PipelineStage::ModifyEntity(entities, "UnitTestEntity", true).Run([in_value](Neshny::GLShader* prog) {
			glUniform1i(prog->GetUniform("uMode"), 0);
			glUniform1f(prog->GetUniform("uValue"), in_value);
		});

		for (auto& item : expected) {
			GPUThing::Add(item, in_value);
		}

		std::vector<GPUThing> gpu_values;
		entities.GetSSBO()->GetValues(gpu_values, initial_count);

		// delete every nth item
		Neshny::PipelineStage::ModifyEntity(entities, "UnitTestEntity", true).Run([div_val](Neshny::GLShader* prog) {
			glUniform1i(prog->GetUniform("uMode"), 1);
			glUniform1f(prog->GetUniform("uValue"), div_val);
		});

		for (int i = 0; i < 2; i++) { // to avoid skipping the moved ones
			for (auto iter = expected.begin(); iter != expected.end(); iter++) {
				if (iter->p_Int % div_val == 0) {
					RemoveUnordered(expected, iter);
				}
			}
		}

		gpu_values.clear();
		entities.GetSSBO()->GetValues(gpu_values, entities.GetCount());

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

		Expect("Size mismatch", expected.size() == gpu_values.size());
		for (int i = 0; i < (int)gpu_values.size(); i++) {
			const auto& a = expected[i];
			const auto& b = gpu_values[i];
			bool later_a = a.p_Int >= 1000;
			bool later_b = b.p_Int >= 1000;
			Expect("Batch mismatch", later_a == later_b);
		}

#elif defined(NESHNY_WEBGPU)

#endif
    }


} // namespace Test