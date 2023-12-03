//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Test {

#pragma pack(push)
#pragma pack (1)

	struct Uniform {
		int				p_Mode;
		float			p_Value;
	};

	struct DataItem {
		int				p_Int;
		float			p_Float;
		Neshny::fVec2	p_TwoDim;
		Neshny::fVec3	p_ThreeDim;
		Neshny::fVec4	p_FourDim;
		Neshny::iVec2	p_IntTwoDim;
		Neshny::iVec3	p_IntThreeDim;
		Neshny::iVec4	p_IntFourDim;
	};

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
		static void Add(GPUThing& item, DataItem& v) {
			item.p_Float += v.p_Float;
			item.p_TwoDim += v.p_TwoDim;
			item.p_ThreeDim += v.p_ThreeDim;
			item.p_FourDim += v.p_FourDim;
			item.p_IntTwoDim += v.p_IntTwoDim;
			item.p_IntThreeDim += v.p_IntThreeDim;
			item.p_IntFourDim += v.p_IntFourDim;
		}

		bool CloseEnough(const GPUThing& other) {
			double delta = 0.0001;

			std::vector<bool> test = {
				//(p_Id == other.p_Id),
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
				//(p_Id == other.p_Id) &&
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
				int brk = 0;
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

	struct GPUOther {

		bool CloseEnough(const GPUOther& other) {
			double delta = 0.0001;
			return
				//(p_Id == other.p_Id) &&
				(p_ParentIndex == other.p_ParentIndex) &&
				(fabs(p_Float - other.p_Float) < delta) &&
				(fabs(p_FourDim.x - other.p_FourDim.x) < delta) &&
				(fabs(p_FourDim.y - other.p_FourDim.y) < delta) &&
				(fabs(p_FourDim.z - other.p_FourDim.z) < delta) &&
				(fabs(p_FourDim.w - other.p_FourDim.w) < delta);

		}

		int				p_Id;
		int				p_ParentIndex;
		float			p_Float;
		Neshny::fVec4	p_FourDim;
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
	template<> inline auto registerMembers<Test::GPUOther>() {
		return members(
			member("Id", &Test::GPUOther::p_Id)
			,member("ParentIndex", &Test::GPUOther::p_ParentIndex)
			,member("Float", &Test::GPUOther::p_Float)
			,member("FourDim", &Test::GPUOther::p_FourDim)
		);
	}
	template<> inline auto registerMembers<Test::Uniform>() {
		return members(
			member("Mode", &Test::Uniform::p_Mode)
			,member("Value", &Test::Uniform::p_Value)
		);
	}
	template<> inline auto registerMembers<Test::DataItem>() {
		return members(
			member("Int", &Test::DataItem::p_Int)
			,member("Float", &Test::DataItem::p_Float)
			,member("TwoDim", &Test::DataItem::p_TwoDim)
			,member("ThreeDim", &Test::DataItem::p_ThreeDim)
			,member("FourDim", &Test::DataItem::p_FourDim)
			,member("IntTwoDim", &Test::DataItem::p_IntTwoDim)
			,member("IntThreeDim", &Test::DataItem::p_IntThreeDim)
			,member("IntFourDim", &Test::DataItem::p_IntFourDim)
		);
	}
}

namespace Test {

	////////////////////////////////////////////////////////////////////////////////
	template <class T>
	void CompareEntities(QString msg_prefix, std::vector<T> expected, std::vector<T> actual, std::function<bool(const T&, const T&)> sort_func = [](const T& a, const T& b) { return b.p_Id > a.p_Id; }) {
		Expect("Size mismatch", expected.size() <= actual.size());

		std::sort(expected.begin(), expected.end(), sort_func);
		std::sort(actual.begin(), actual.end(), sort_func);

		for (int i = 0; i < expected.size(); i++) {
			if (expected[i].p_Id < 0) {
				break;
			}
			//Expect(msg_prefix + " -> Item ID mismatch", expected[i].p_Id == actual[i].p_Id);
			Expect(msg_prefix + " -> Item mismatch", expected[i].CloseEnough(actual[i]));
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void GPUEntityTest(Neshny::GPUEntity::DeleteMode mode) {

		Neshny::GPUEntity entities("Thing", mode, &GPUThing::p_Id, "Id");
		entities.Init(1000);

		Neshny::GPUEntity other_entities("Other", mode, &GPUOther::p_Id, "Id");
		other_entities.Init(1000);

		const int initial_count = 50;
		std::vector<GPUThing> expected;
		std::vector<GPUOther> other_expected;
		for (int i = 0; i < initial_count; i++) {
			GPUThing thing = GPUThing::Init(i);
			entities.AddInstance(&thing);
			expected.push_back(thing);
		}

		std::vector<int> buffer_values;
		for (int i = 0; i < initial_count; i++) {
			buffer_values.push_back(i * 3 + 123);
		}
#if defined(NESHNY_GL)
		Neshny::GLSSBO test_buffer((int)buffer_values.size() * sizeof(int), (unsigned char*)buffer_values.data());
#elif defined(NESHNY_WEBGPU)
		Neshny::WebGPUBuffer test_buffer(WGPUBufferUsage_Storage, (unsigned char*)buffer_values.data(), buffer_values.size() * sizeof(int));
#endif

		const float in_value = 123.4f;
		const int div_val = 7;

		std::vector<DataItem> data_items;
		for (int i = 0; i < 5; i++) {
			data_items.push_back({
				RandomInt(-10, 10)
				,float(Random(-10, 10))
				,Neshny::fVec2(Random(-10, 10), Random(-10, 10))
				,Neshny::fVec3(Random(-10, 10), Random(-10, 10), Random(-10, 10))
				,Neshny::fVec4(Random(-10, 10), Random(-10, 10), Random(-10, 10), Random(-10, 10))
				,Neshny::iVec2(RandomInt(-10, 10), RandomInt(-10, 10))
				,Neshny::iVec3(RandomInt(-10, 10), RandomInt(-10, 10), RandomInt(-10, 10))
				,Neshny::iVec4(RandomInt(-10, 10), RandomInt(-10, 10), RandomInt(-10, 10), RandomInt(-10, 10))
			});
		}

		// add in_value to most values
#if defined(NESHNY_GL)
		Neshny::PipelineStage::ModifyEntity(entities, "UnitTestEntity", true, { "CREATE_OTHER" })
		.AddDataVector("DataItem", data_items)
		.AddCreatableEntity(other_entities)
		.AddBuffer("b_TestBuffer", test_buffer, Neshny::MemberSpec::T_INT, Neshny::PipelineStage::BufferAccess::READ_ONLY)
		.Run([in_value](Neshny::GLShader* prog) {
			glUniform1i(prog->GetUniform("uMode"), 0);
			glUniform1f(prog->GetUniform("uValue"), in_value);
		});
#elif defined(NESHNY_WEBGPU)
		auto executable = Neshny::PipelineStage::ModifyEntity(entities, "UnitTestEntity", true, { "CREATE_OTHER" })
			.AddInputOutputVar("uCheckVal")
			.AddDataVector<DataItem>("DataItem")
			.AddCreatableEntity(other_entities)
			.AddBuffer("b_TestBuffer", test_buffer, Neshny::MemberSpec::T_INT, Neshny::PipelineStage::BufferAccess::READ_ONLY)
			.Prepare<Uniform>();
		int uCheckVal = 0;
		Uniform uniform{ 0, in_value };
		executable
			->WithDataVector("DataItem", data_items)
			.Run(uniform, { { "uCheckVal", &uCheckVal } });
#endif

		int parent_index = 0;
		for (auto& item : expected) {
			if (item.p_Int % 2 == 0) {
				other_expected.push_back({
					(int)other_expected.size(),
					parent_index,
					item.p_Float,
					Neshny::fVec4(item.p_TwoDim.x, item.p_TwoDim.y, item.p_FourDim.z, item.p_FourDim.w)
				});
			}
			GPUThing::Add(item, in_value);
			item.p_IntTwoDim.x += buffer_values[parent_index];

			parent_index++;
		}
		std::vector<GPUThing> gpu_values;
		entities.GetSSBO()->GetValues(gpu_values, initial_count);

		CompareEntities("After first add", expected, gpu_values);

		std::vector<GPUOther> gpu_other;
		other_entities.GetSSBO()->GetValues(gpu_other, other_entities.GetMaxIndex());

		auto other_sort = [](const GPUOther& a, const GPUOther& b) { 
			if (a.p_Id < 0) {
				return false;
			}
			if (b.p_Id < 0) {
				return true;
			}
			return b.p_ParentIndex > a.p_ParentIndex;
		};
		CompareEntities<GPUOther>("Secondary Entity: After first add", other_expected, gpu_other, other_sort);

		// add in_value to most values again to check double buffering works
#if defined(NESHNY_GL)
		Neshny::PipelineStage::ModifyEntity(entities, "UnitTestEntity", true)
		.AddDataVector("DataItem", data_items)
		.AddEntity(other_entities)
		.Run([float_val = float(data_items.size())](Neshny::GLShader* prog) {
			glUniform1i(prog->GetUniform("uMode"), 1);
			glUniform1f(prog->GetUniform("uValue"), float_val);
		});
		Neshny::PipelineStage::ModifyEntity(other_entities, "UnitTestEntity", true, { "RUN_OTHER" })
		.AddDataVector("DataItem", data_items)
		.AddEntity(entities)
		.Run();

#elif defined(NESHNY_WEBGPU)
		uniform.p_Mode = 1;
		uniform.p_Value = float(data_items.size());
		executable
			->WithDataVector("DataItem", data_items)
			.Run(uniform, { { "uCheckVal", &uCheckVal } });

		auto other_executable = Neshny::PipelineStage::ModifyEntity(other_entities, "UnitTestEntity", true)
			.AddInputOutputVar("uCheckVal")
			.AddDataVector<DataItem>("DataItem")
			.AddEntity(entities)
			.Prepare<Uniform>();
		other_executable
			->WithDataVector("DataItem", data_items)
			.Run(uniform, { { "uCheckVal", &uCheckVal } });

#endif

		for (auto& item : expected) {
			GPUThing::Add(item, data_items[item.p_Int % data_items.size()]);
		}
		for (auto& item : other_expected) {
			auto& parent = expected[item.p_ParentIndex];
			if (parent.p_Int % 3 == 1) {
				item.p_Id = -1;
			}
		}
		if (mode == Neshny::GPUEntity::DeleteMode::MOVING_COMPACT) {
			for (int i = 0; i < 2; i++) { // to avoid skipping the moved ones
				for (auto iter = other_expected.begin(); iter != other_expected.end(); iter++) {
					if (iter->p_Id < 0) {
						RemoveUnordered(other_expected, iter);
					}
				}
			}
		}

		gpu_values.clear();
		entities.GetSSBO()->GetValues(gpu_values, initial_count);

		CompareEntities("After second add", expected, gpu_values);

		gpu_other.clear();
		other_entities.GetSSBO()->GetValues(gpu_other, other_entities.GetMaxIndex());

		CompareEntities<GPUOther>("After second add and other delete", other_expected, gpu_other, other_sort);

		// delete every nth item
#if defined(NESHNY_GL)
		Neshny::PipelineStage::ModifyEntity(entities, "UnitTestEntity", true)
		.AddDataVector("DataItem", data_items)
		.AddEntity(other_entities)
		.Run([div_val](Neshny::GLShader* prog) {
			glUniform1i(prog->GetUniform("uMode"), 2);
			glUniform1f(prog->GetUniform("uValue"), div_val);
		});
#elif defined(NESHNY_WEBGPU)
		uniform.p_Mode = 2;
		uniform.p_Value = div_val;
		executable
			->WithDataVector("DataItem", data_items)
			.Run(uniform, { { "uCheckVal", &uCheckVal } });
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
	void UnitTest_GPUEntityCache(void) {

		const int prey_count = 50;
		const int hunter_count = 20;

		const float map_radius = 50;
		const int grids = 10;
		const float find_radius = 10;

		const float find_radius_sqr = find_radius * find_radius;

		RandomSeed(0);

		Neshny::GPUEntity prey_entities("Prey", Neshny::GPUEntity::DeleteMode::STABLE_WITH_GAPS, &GPUThing::p_Id, "Id");
		prey_entities.Init(1000);

		Neshny::GPUEntity hunter_entities("Hunter", Neshny::GPUEntity::DeleteMode::STABLE_WITH_GAPS, &GPUOther::p_Id, "Id");
		hunter_entities.Init(1000);

		Neshny::Grid2DCache cache(prey_entities, "TwoDim");

		std::vector<GPUThing> expected_prey;
		for (int i = 0; i < prey_count; i++) {
			GPUThing prey = GPUThing::Init(i);
			prey.p_Float = Random(0.0, 10);
			prey.p_TwoDim = Neshny::fVec2(Random(-map_radius, map_radius), Random(-map_radius, map_radius));
			prey_entities.AddInstance(&prey);
			expected_prey.push_back(prey);
		}

		std::vector<GPUOther> expected_hunters;
		for (int i = 0; i < hunter_count; i++) {
			float x = Random(-map_radius, map_radius);
			float y = Random(-map_radius, map_radius);
			GPUOther hunter{
				-1,
				0,
				0,
				Neshny::fVec4(x, y, 0.0, 0.0)
			};
			hunter_entities.AddInstance(&hunter);
			expected_hunters.push_back(hunter);
		}

		for (auto& hunter : expected_hunters) {
			Neshny::fVec2 hunter_pos(hunter.p_FourDim.x, hunter.p_FourDim.y);
			for (const auto& prey : expected_prey) {
				float dist_sqr = (prey.p_TwoDim - hunter_pos).LengthSquared();
				if (dist_sqr < find_radius_sqr) {
					hunter.p_Float += prey.p_Float;
					hunter.p_ParentIndex++;
				}
			}
		}

		// run the generation algorithm
		cache.GenerateCache(Neshny::iVec2(grids, grids), Neshny::Vec2(-map_radius, -map_radius), Neshny::Vec2(map_radius, map_radius));

#if defined(NESHNY_GL)
		// TODO: test here as well
#elif defined(NESHNY_WEBGPU)
		auto executable = Neshny::PipelineStage::ModifyEntity(hunter_entities, "UnitTestCache", true, { "TEST_CACHE" })
			.AddEntity(prey_entities, &cache)
			.Prepare<Uniform>();
		Uniform uniform{ 0, find_radius };
		executable->Run(uniform);
#endif

		std::vector<GPUOther> gpu_values;
		hunter_entities.GetSSBO()->GetValues(gpu_values, hunter_count);

		CompareEntities<GPUOther>("Cache lookup", expected_hunters, gpu_values);
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
		Expect("Atomic value mismatch after first pass", atomic_result_first == first_pass);

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
		Expect("Atomic value mismatch after second pass", atomic_result_second == (first_pass + second_pass));

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