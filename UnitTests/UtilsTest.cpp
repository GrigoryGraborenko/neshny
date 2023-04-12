//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Test {

    void UnitTest_RoundPowerTwo(void) {
        Expect("RoundUpPowerTwo of 0 should be 0", RoundUpPowerTwo(0) == 0);
        Expect("RoundUpPowerTwo of 1 should be 1", RoundUpPowerTwo(1) == 1);
        Expect("RoundUpPowerTwo of 2 should be 2", RoundUpPowerTwo(2) == 2);
        Expect("RoundUpPowerTwo of 3 should be 4", RoundUpPowerTwo(3) == 4);
        Expect("RoundUpPowerTwo of 4 should be 4", RoundUpPowerTwo(4) == 4);
        Expect("RoundUpPowerTwo of 5 should be 8", RoundUpPowerTwo(5) == 8);
        Expect("RoundUpPowerTwo of 31 should be 32", RoundUpPowerTwo(31) == 32);
        Expect("RoundUpPowerTwo of 45 should be 64", RoundUpPowerTwo(45) == 64);
        Expect("RoundUpPowerTwo of 64 should be 64", RoundUpPowerTwo(64) == 64);
    }

    void UnitTest_DefaultMemoryManagement(void) {

		class TestEngine : public Neshny::IEngine {
		public:
			TestEngine(void) {}
			~TestEngine(void) {}

			virtual void			MouseButton(int button, bool is_down) {}
			virtual void			MouseMove(Neshny::Vec2 delta, bool occluded) {}
			virtual void			MouseWheel(bool up) {}
			virtual void			Key(int key, bool is_down) {}

			virtual bool			Init(void) { return true; }
			virtual void			ExitSignal(void) {}
			virtual bool			ShouldExit(void) { return true; }
			virtual bool			Tick(qint64 delta_nanoseconds, int tick) { return true; }
			virtual void			Render(int width, int height) {}

			inline void				SetMaxMem(qint64 max_mem, qint64 max_gpu_mem) { m_MaxMemory = max_mem; m_MaxGPUMemory = max_gpu_mem; }
		};

		struct TestResource {
			qint64	p_Memory;
			qint64	p_GPUMemory;
			float	p_MinutesAge;
			bool	p_ExpectDelete;
		};

		TestEngine* engine = new TestEngine;
		constexpr float minutes_to_ticks = 30 * 60;
		QString found_err = QString();

		auto Meg = [](int megabytes) { return (qint64)megabytes * 1024ll * 1024ll; };

		auto test_scenario = [engine, minutes_to_ticks, &found_err](QString scenario, qint64 max_ram, qint64 max_gpu_ram, const std::vector<TestResource>& resources) {
			engine->SetMaxMem(max_ram, max_gpu_ram);

			qint64 curr_ram = 0;
			qint64 curr_gpu_ram = 0;
			for (const auto& resource : resources) {
				curr_ram += resource.p_Memory;
				curr_gpu_ram += resource.p_GPUMemory;
			}

			engine->ManageResources(Neshny::ResourceManagementToken(
				[&resources, minutes_to_ticks](std::vector<Neshny::ResourceManagementToken::ResourceEntry>& entries) {
					entries.clear();
					entries.reserve(resources.size());
					int ind = 0;
					for (const auto& resource : resources) {
						entries.emplace_back(nullptr, QString("%1").arg(ind++), resource.p_Memory, resource.p_GPUMemory, resource.p_MinutesAge * minutes_to_ticks);
					}
				}
				,[&resources, scenario, &found_err](const std::vector<Neshny::ResourceManagementToken::ResourceEntry>& entries) {

					if (entries.empty()) {
						for(int ind = 0; ind < resources.size(); ind++) {
							if (resources[ind].p_ExpectDelete) {
								found_err = QString("%1: expected DELETION in resource %2").arg(scenario).arg(ind);
								return;
							}
						}
					}
					for (const auto& entry : entries) {
						int ind = entry.p_Id.toInt();
						const auto& resource = resources[ind];
						if (resource.p_ExpectDelete != entry.p_FlagForDeletion) {
							found_err = QString("%1: expected %2 in resource %3").arg(scenario).arg(resource.p_ExpectDelete ? "DELETION" : "PRESERVATION").arg(ind);
						}
					}
				}
			), curr_ram, curr_gpu_ram);

			Expect(found_err, found_err.isNull());
		};

		test_scenario("Enough memory", Meg(1000), Meg(1000), {
			{ Meg(10), Meg(10), 0.0, false }
			,{ Meg(20), Meg(20), 1.0, false }
			,{ Meg(80), Meg(80), 10.0, false }
		});

		test_scenario("Large old resource", Meg(100), Meg(100), {
			{ Meg(10), Meg(10), 0.0, false }
			,{ Meg(20), Meg(20), 1.0, false }
			,{ Meg(80), Meg(80), 10.0, true }
		});

		test_scenario("Moderate very old resource", Meg(100), Meg(100), {
			{ Meg(50), Meg(50), 0.0, false }
			,{ Meg(40), Meg(40), 10.0, true }
			,{ Meg(50), Meg(50), 0.0, false }
		});
		
		test_scenario("GPU only", Meg(10000), Meg(100), {
			{ Meg(50), Meg(110), 0.0, true }
			,{ Meg(50), Meg(110), 0.0, true }
			,{ Meg(50), Meg(5), 1.0, false }
			,{ Meg(400), Meg(40), 0.0, false }
			,{ Meg(500), Meg(40), 0.0, false }
		});

		test_scenario("RAM only with bias towards old", Meg(100), Meg(1000), {
			{ Meg(10), 0, 0.5, false }
			,{ Meg(10), 0, 0.5, false }
			,{ Meg(10), 0, 0.5, false }
			,{ Meg(50), 0, 0.5, false }
			,{ Meg(40), 0, 3.0, true }
		});

		delete engine;
    }

	void UnitTest_Random(void) {

		// checks the random number generator to ensure it's fair
		// this also tests the validity of implementing this rng generator in the GPU
		struct u64_32x2 { uint32_t high; uint32_t low; };

		auto ToComposite = [](uint64_t input) -> u64_32x2 {
			return { uint32_t(input >> 32u), uint32_t(input & 0xFFFFFFFFU) };
		};

		auto FromComposite = [](u64_32x2 composite)->uint64_t{
			return (uint64_t)composite.low + (((uint64_t)composite.high) << 32u);
		};

		auto Add64 = [&](u64_32x2 a, u64_32x2 b) -> u64_32x2 {
			uint32_t rl = a.low + b.low;
			uint32_t rh = rl < a.low;
			u64_32x2 low_sum{ rh, rl };
			uint32_t high_sum = a.high + b.high;
			return { high_sum + low_sum.high, low_sum.low };
		};

		auto Mul64_32 = [&](uint32_t a, uint32_t b) -> u64_32x2{
			uint32_t ah = a >> 16, al = a & 0xFFFFU;
			uint32_t bh = b >> 16, bl = b & 0xFFFFU;
			uint32_t rl = al * bl;
			uint32_t rm1 = ah * bl;
			uint32_t rm2 = al * bh;
			uint32_t rh = ah * bh;

			uint32_t rm1h = rm1 >> 16, rm1l = (rm1 & 0xFFFFU) << 16;
			uint32_t rm2h = rm2 >> 16, rm2l = (rm2 & 0xFFFFU) << 16;
			uint32_t high = rh + rm1h + rm2h;
			uint32_t low = rl + rm1l;
			high += low < rl ? 1 : 0;
			low += rm2l;
			high += low < rm2l ? 1 : 0;
			return { high, low };
		};

		auto Mul64 = [&](u64_32x2 a, u64_32x2 b) -> u64_32x2{
			u64_32x2 a0b0 = Mul64_32(a.low, b.low);
			uint32_t a0b1 = a.low * b.high;
			uint32_t a1b0 = a.high * b.low;
			return { a0b1 + a1b0 + a0b0.high, a0b0.low };
		};

		auto Xor64 = [](u64_32x2 a, u64_32x2 b)->u64_32x2{
			return { a.high ^ b.high, a.low ^ b.low };
		};

		auto LeftShift64 = [&](u64_32x2 a, int num)->u64_32x2{
			if (num >= 32) {
				uint32_t new_low = a.high >> (num - 32);
				return { 0, new_low };
			}
			uint32_t new_low = (a.low >> num) | (a.high << (32 - num));
			uint32_t new_high = (a.high >> num);
			return { new_high, new_low };
		};

		auto TestSum = [&](uint64_t a, uint64_t b) -> bool{
			u64_32x2 comp_a = ToComposite(a);
			u64_32x2 comp_b = ToComposite(b);

			u64_32x2 comp_sum = Add64(comp_a, comp_b);
			uint64_t native_sum = a + b;
			uint64_t result_sum = FromComposite(comp_sum);
			return native_sum == result_sum;
		};

		auto TestMult = [&](uint64_t a, uint64_t b) -> bool {
			u64_32x2 comp_a = ToComposite(a);
			u64_32x2 comp_b = ToComposite(b);
			u64_32x2 comp_mul = Mul64(comp_a, comp_b);
			uint64_t native_mul = a * b;
			uint64_t result_mul = FromComposite(comp_mul);
			return native_mul == result_mul;
		};

		u64_32x2 g_32State = ToComposite(0x4d595df4d0f33173);
		const u64_32x2 g_32Inc = ToComposite(1442695040888963407u | 1u);
		const u64_32x2 g_32Mult = ToComposite(6364136223846793005ULL);

		auto pcg32_random_r_32 = [&](u64_32x2* rng) ->uint32_t {
			u64_32x2 oldstate = *rng;
			// Advance internal state
			*rng = Add64(Mul64(oldstate, g_32Mult), g_32Inc);

			uint32_t xorshifted = LeftShift64(Xor64(LeftShift64(oldstate, 18), oldstate), 27).low;
			uint32_t rot = LeftShift64(oldstate, 59).low;

			return (xorshifted >> rot) | (xorshifted << (((rot ^ 0xFFFFFFFF) + 1) & 31));
		};

		auto TestRand32 = [&]() -> uint32_t {
			return pcg32_random_r_32(&g_32State);
		};

		std::vector<std::pair<uint64_t, uint64_t>> test_pairs = {
			{ 458840393324832221LL, 6364136223846793005ULL }
			,{ 0x4d595df4d0f33173, 0x4d595df4345173 }
			,{ 0x33454465465, 0x4433333345173 }
			,{ 0x235, 0x173 }
			,{ 0x2355465464534, 0x435453454353473 }
		};
		for (auto pair : test_pairs) {
			Expect("32 bit simulation of 64 bit addition", TestSum(pair.first, pair.second));
			Expect("32 bit simulation of 64 bit multiplication", TestMult(pair.first, pair.second));
		}

		RandomGenerator gen(false);

		std::set<unsigned int> set_vals;
		std::vector<unsigned int> vals;
		int duplicates = 0;

		for (int i = 0; i < 10000; i++) {
			unsigned int val = gen.Next();
			unsigned int val32 = TestRand32();
			Expect("32 bit simulation of 64 bit random num generation", val == val32);
			vals.push_back(val);
			bool dup = !set_vals.insert(val).second;
			if (dup) {
				duplicates++;
			}
		}

		Expect("Small number of duplicates", duplicates < 5);

		const int num_buckets = 100;
		std::vector<int> buckets;
		for (int i = 0; i < num_buckets; i++) {
			buckets.push_back(0);
		}

		const int num_iters = 100000;
		double sum = 0;
		for (int i = 0; i < num_iters; i++) {
			double ran = Random();
			buckets[ran * num_buckets]++;
			sum += ran;
		}
		sum /= num_iters;

		const double avg_spread = 0.01;
		Expect("Average random num to be close to 0.5", fabs(sum - 0.5) < avg_spread);

		int min_bucket = buckets[0];
		int max_bucket = min_bucket;
		for (int bucket_num : buckets) {
			min_bucket = std::min(min_bucket, bucket_num);
			max_bucket = std::max(max_bucket, bucket_num);
		}
		double expected_bucket_num = (double)num_iters / num_buckets;
		const double bucket_max_spread = 0.2;

		int bucket_min_bound = floor(expected_bucket_num * (1.0 - bucket_max_spread));
		int bucket_max_bound = ceil(expected_bucket_num * (1.0 + bucket_max_spread));

		Expect("Minimum bucket count to be greater", min_bucket > bucket_min_bound);
		Expect("Maximum bucket count to be less than", max_bucket < bucket_max_bound);
	}

} // namespace Test