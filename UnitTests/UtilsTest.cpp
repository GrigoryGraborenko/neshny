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

} // namespace Test