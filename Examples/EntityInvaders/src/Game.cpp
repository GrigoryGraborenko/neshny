////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
Game::Game(void) :
	m_Enemies("Enemy", &GPUEnemy::p_Id, "Id")

	,m_EnemyProjectiles("EnemyProjectile", &GPUEnemyProjectile::p_Id, "Id", true)
	,m_PlayerDrones("PlayerDrone", &GPUPlayerDrone::p_Id, "Id", false)
	,m_PlayerProjectiles("PlayerProjectile", &GPUPlayerProjectile::p_Id, "Id", false)
	,m_Debris("Debris", &GPUDebris::p_Id, "Id", false)

	,m_EnemyCache(m_Enemies, "Pos")
	,m_DroneCache(m_PlayerDrones, "Pos")

	,m_MiscUniform(WGPUBufferUsage_Uniform, sizeof(MiscUniform))

	,m_TotalDuration(0)
{
}

////////////////////////////////////////////////////////////////////////////////
Game::~Game(void) {
}

////////////////////////////////////////////////////////////////////////////////
bool Game::Init(void) {

	//UnitTester::Execute();

	m_Enemies.Init(100000);
	m_EnemyProjectiles.Init(1000000);
	m_PlayerDrones.Init(100000);
	m_PlayerProjectiles.Init(1000000);
	m_Debris.Init(1000000);

	WebGPURenderBuffer* render_buffer = Core::GetBuffer("Square");

	WebGPUPipeline::RenderParams render_params;
	render_params.p_DepthWriteEnabled = true;
	m_ShipRenderPipeline
		.AddBuffer(m_MiscUniform, WGPUShaderStage_Vertex | WGPUShaderStage_Fragment, true)
		.FinalizeRender("ShipRender", *render_buffer, render_params);

	RandomSeed(0);
	const double spread = 10.0;
	{
		std::vector<Vec2> positions;
		for (int i = 0; i < 0; i++) {
			positions.emplace_back(Random(-spread, spread), Random(-spread, spread));
		}
		AddEnemies(positions);
	}

	{
		std::vector<Vec2> positions;
		for (int i = 0; i < 2; i++) {
			positions.emplace_back(Random(-spread, spread), Random(-spread * 0.1, 0));
		}
		AddDrones(positions);
	}

	for (int i = 0; i < 1; i++) {
		fVec2 anchor_pos(0.0, 0.0);
		m_MapMetaState.p_EnemyMovement.push_back({
			iVec2(RandomInt(0, std::numeric_limits<int>::max()), RandomInt(0, std::numeric_limits<int>::max()))
			,fVec2(-8, 5) // anchor pos
			,fVec2(1.0, 3.0) // angular rate
			,fVec2(0.0, 10.0) // addition
			,fVec2(0.25, 1.5) // radius
			//,fVec2(0.0, 0.0) // radius
			,0.5 // spacing
			,10 // per row
			,3 // subgroup size
			//,0.05 // subgroup deviation
			,0.0 // subgroup deviation
			,0.05 // index deviation
		});

		m_MapMetaState.p_EnemyMovement.push_back({
			iVec2(RandomInt(0, std::numeric_limits<int>::max()), RandomInt(0, std::numeric_limits<int>::max()))
			,fVec2(8, 2) // anchor pos
			,fVec2(1.0, 3.0) // angular rate
			,fVec2(0.0, 10.0) // addition
			,fVec2(0.25, 1.5) // radius
			//,fVec2(0.0, 0.0) // radius
			,0.0 // spacing
			,10 // per row
			,1 // subgroup size
			//,0.05 // subgroup deviation
			,0.0 // subgroup deviation
			,0.2 // index deviation
		});

	}

	m_SkillLevels.resize(g_Skills.size(), 0);

	/////////////////////////

	return true;
}

////////////////////////////////////////////////////////////////////////////////
void Game::AddEnemies(std::vector<Vec2> positions) {

	const double vr = 1.0;
	std::vector<GPUEnemy> enemies;
	for (Vec2 pos: positions) {
		enemies.push_back(GPUEnemy{
			-1,				//p_Id
			0,				//p_Type
			0,				//p_Flags
			100,			//p_Health
			fVec2(pos),		//p_Pos
			//fVec2(0.0, 0.0),//p_Velocity
			fVec2(Random(-vr, vr), Random(-vr, vr)),//p_Velocity
			0.0,			//p_Cooldown
		});
	}
	m_Enemies.AddInstances(enemies);
}

////////////////////////////////////////////////////////////////////////////////
void Game::AddDrones(std::vector<Vec2> positions) {

	const double vr = 0.1;
	std::vector<GPUPlayerDrone> drones;
	for (Vec2 pos : positions) {
		drones.push_back(GPUPlayerDrone{
			-1,											//p_Id
			0,											//p_Type
			0,											//p_Flags
			(int)drones.size(),								// p_Index
			100,										//p_Health
			fVec2(pos),									//p_Pos
			//fVec2(0.0, 0.0),							//p_Velocity
			fVec2(Random(-vr, vr), Random(-vr, vr)),	//p_Velocity
			0.0,										//p_Cooldown
		});
	}
	m_PlayerDrones.AddInstances(drones);
}

////////////////////////////////////////////////////////////////////////////////
void Game::AddPlayerProjectiles(std::vector<fVec2> positions) {

	std::vector<GPUPlayerProjectile> projectiles;
	for (fVec2 pos : positions) {
		projectiles.push_back(GPUPlayerProjectile{
			-1,			// p_Id
			0,			// p_Type
			0,			// p_Flags
			5,			// p_Lifespan
			100,		// p_Damage
			pos,		// p_Pos
			fVec2(0.0, m_GameMetaState.p_MainGunSpeed)	// p_Velocity
		});
	}

	m_PlayerProjectiles.AddInstances(projectiles);
}

////////////////////////////////////////////////////////////////////////////////
void Game::MouseButton(int button, bool is_down) {
	m_Mice[button] = is_down;
}

////////////////////////////////////////////////////////////////////////////////
void Game::MouseMove(Vec2 delta) {
	if ((m_LastWidth < 0) || (m_LastHeight < 0)) {
		return;
	}
	auto mouse = ImGui::GetMousePos();
	m_LastMouseWorld = m_Cam.ScreenToWorld(Vec2(mouse.x, mouse.y), m_LastWidth, m_LastHeight);

	if (m_Mice[2]) {
		m_Cam.Pan(m_LastWidth, delta.x, delta.y);
	}
}

////////////////////////////////////////////////////////////////////////////////
void Game::MouseWheel(bool up) {

	const double ZOOM_SPEED = 0.1;
	double new_zoom = m_Cam.p_Zoom * (1.0 + (up ? -1 : 1) * ZOOM_SPEED);

	if ((m_LastWidth <= 0) || (m_LastHeight <= 0)) {
		m_Cam.p_Zoom = new_zoom;
		return;
	}
	m_Cam.Zoom(new_zoom, m_LastMouseWorld, m_LastWidth, m_LastHeight);
}

////////////////////////////////////////////////////////////////////////////////
void Game::GameStateTick(double delta_seconds) {

	double energy_rate = GetEnergyRate();
	double energy_max = GetEnergyMax();

	m_GameState.p_Energy = std::min(energy_max, m_GameState.p_Energy + energy_rate * delta_seconds);

	m_GameState.p_ShipPos = m_LastMouseWorld.Clamped(m_MapSize * -0.5, m_MapSize * 0.5);

	if ((m_GameState.p_EnemySpawnCooldown <= 0.0) && (m_Enemies.GetLastKnownCount() < m_MapMetaState.p_EnemyMax)) {
		const double spawn_min_buffer = 0.1;
		const double spawn_max_buffer = 0.5;
		AddEnemies({ Vec2(Random(m_MapSize.x * -0.5, m_MapSize.x * 0.5), Random(m_MapSize.y * (0.5 + spawn_min_buffer), m_MapSize.y * (0.5 + spawn_max_buffer))) });
		m_GameState.p_EnemySpawnCooldown = m_MapMetaState.p_EnemySpawnCooldown;
	} else if (m_GameState.p_EnemySpawnCooldown > 0.0) {
		m_GameState.p_EnemySpawnCooldown = std::max(0.0, m_GameState.p_EnemySpawnCooldown - delta_seconds);
	}

	if (m_Mice[0]) {
		if ((m_GameState.p_AutoFireCooldown <= 0) && (m_GameState.p_Energy >= m_GameMetaState.p_MainGunEnergy)) {
			AddPlayerProjectiles({ fVec2(m_GameState.p_ShipPos) });
			m_GameState.p_Energy -= m_GameMetaState.p_MainGunEnergy;

			auto splode_sound = Core::GetResource<SoundFile>("../../assets/sounds/explode.mp3");
			if (splode_sound.IsValid()) {
				splode_sound->Play();
			}

		}
		if (m_GameState.p_AutoFireCooldown < 0.0) {
			m_GameState.p_AutoFireCooldown = AUTOFIRE_COOLDOWN;
		} else {
			m_GameState.p_AutoFireCooldown = std::max(0.0, m_GameState.p_AutoFireCooldown - delta_seconds);
		}
	} else {
		m_GameState.p_AutoFireCooldown = -1;
	}

	//if (m_Mice[0] && (m_GameState.p_Energy >= m_GameMetaState.p_MainGunEnergy)) {
	//	AddPlayerProjectiles({ fVec2(m_GameState.p_ShipPos) });
	//	m_GameState.p_Energy -= m_GameMetaState.p_MainGunEnergy;
	//}
}

////////////////////////////////////////////////////////////////////////////////
bool Game::Tick(double delta_seconds, int tick) {

	auto splode_sound = Core::GetResource<SoundFile>("../../assets/sounds/explode.mp3");

	if (tick <= 0) {
		return true;
	}

	if (!m_Playing) {
		return false;
	}

	DebugTiming debug_func("Engine::Tick");

	GameStateTick(delta_seconds);

#pragma msg("move total seconds over to core")
	m_TotalDuration += std::chrono::duration<double>(delta_seconds);
	double total_seconds = m_TotalDuration.count();

	MoveUniform move;
	move.p_TotalSeconds = float(total_seconds);
	move.p_DeltaSeconds = float(delta_seconds);
	move.p_MapSize = m_MapSize;
	move.p_ShipPos = fVec2(m_GameState.p_ShipPos);


	/////////////////////////

	Neshny::EntityPipeline::ModifyEntity(SrcStr(), m_Enemies, "EnemyMove", true, {})
		.AddCreatableEntity(m_EnemyProjectiles)
		.AddDataVector("Movement", m_MapMetaState.p_EnemyMovement)
		.AddDataVector("EnemySpec", g_EnemySpecs)
		.AddInputOutputVar("Deaths", 0)
		.SetUniform(move)
		.Run([this](const EntityPipeline::OutputResults& results) {
			int deaths = 0;
			results.GetValue("Deaths", deaths);
			m_GameState.p_TotalKills += deaths;
		});

	const int grid_cache_div = 1;
	m_EnemyCache.Generate2DCache(iVec2(m_MapSize.x / grid_cache_div, m_MapSize.y / grid_cache_div), m_MapSize * -0.5, m_MapSize * 0.5);
	m_DroneCache.Generate2DCache(iVec2(m_MapSize.x / grid_cache_div, m_MapSize.y / grid_cache_div), m_MapSize * -0.5, m_MapSize * 0.5);

	Neshny::EntityPipeline::ModifyEntity(SrcStr(), m_EnemyProjectiles, "EnemyProjectileMove", true)
		.AddEntity(m_PlayerDrones, &m_DroneCache)
		.SetUniform(move)
		//.AddEntity(m_PlayerDrones)
		.Run();

	Neshny::EntityPipeline::ModifyEntity(SrcStr(), m_PlayerDrones, "PlayerDroneMove", true)
		.AddCreatableEntity(m_PlayerProjectiles)
		.SetUniform(move)
		.Run();

	Neshny::EntityPipeline::ModifyEntity(SrcStr(), m_PlayerProjectiles, "PlayerProjectileMove", true)
		.AddEntity(m_Enemies, &m_EnemyCache)
		//.AddEntity(m_Enemies)
		.SetUniform(move)
		.Run();

	/////////////////////////////////////////

	MiscUniform misc_uniform = {
		m_Cam.Get4x4Matrix(m_LastWidth, m_LastHeight).ToGPU()
		,fVec2(m_GameState.p_ShipPos)
	};
	m_MiscUniform.EnsureSizeBytes(sizeof(MiscUniform), false);
	m_MiscUniform.SetSingleValue(0, misc_uniform);

	BufferViewer::Checkpoint("Tick", m_Enemies);
	BufferViewer::Checkpoint("Tick", m_EnemyProjectiles);
	BufferViewer::Checkpoint("Tick", m_PlayerDrones);
	BufferViewer::Checkpoint("Tick", m_PlayerProjectiles);

	//m_Playing = false;

	return true;
}

////////////////////////////////////////////////////////////////////////////////
void Game::Key(int key, bool is_down) {
	if (!is_down) {
		return;
	}
	if (key == SDLK_F4) {
		//Core::Singleton().UnloadAllResources();
	}
	if (key == SDLK_F5) {
		Core::Singleton().UnloadAllPipelinesShaders();
	}
	if (key == SDLK_SPACE) {
		m_Playing = !m_Playing;
	}
}

////////////////////////////////////////////////////////////////////////////////
void Game::Render(WebGPURTT& rtt, int width, int height) {

	DebugTiming debug_func("Engine::Render");

	m_LastWidth = width;
	m_LastHeight = height;

	auto sprites = Core::GetResource<TextureTileset>("../../assets/images/sprites.png", TextureTileset::Params{ 128, 128, true });
	//auto background = Core::GetResource<Texture2D>("../../assets/images/background_stars.png");
	//if ((!sprites.IsValid()) || (!background.IsValid())) {
	if (!sprites.IsValid()) {
		return;
	}

	WebGPURenderBuffer* square_buffer = Core::GetBuffer("Square");
	WebGPUPipeline::RenderParams render_params;
	render_params.p_DepthWriteEnabled = true;

	auto view_persp = m_Cam.Get4x4Matrix(width, height);
	RenderUniform uniform;
	uniform.p_ViewPerspective = view_persp.ToGPU();

	rtt.Render(&m_ShipRenderPipeline);

	Neshny::EntityPipeline::RenderEntity("EnemyRender", m_Enemies, "EnemyRender", true, square_buffer, render_params)
		.AddTexture("Sprites", &(sprites->Get()))
		.AddSampler("SmoothSampler", Core::GetSampler(WGPUAddressMode_Repeat))
		.SetUniform(uniform)
		.Render(&rtt);
	Neshny::EntityPipeline::RenderEntity(SrcStr(), m_PlayerDrones, "PlayerDroneRender", true, square_buffer, render_params)
		.AddTexture("Sprites", &(sprites->Get()))
		.AddSampler("SmoothSampler", Core::GetSampler(WGPUAddressMode_Repeat))
		.SetUniform(uniform)
		.Render(&rtt);
	Neshny::EntityPipeline::RenderEntity(SrcStr(), m_EnemyProjectiles, "EnemyProjectileRender", true, square_buffer, render_params)
		.SetUniform(uniform)
		.Render(&rtt);
	Neshny::EntityPipeline::RenderEntity(SrcStr(), m_PlayerProjectiles, "PlayerProjectileRender", true, square_buffer, render_params)
		.SetUniform(uniform)
		.Render(&rtt);

	ImGui::SetCursorPosY(100);
	auto hovered = BufferViewer::GetHoveredValue();

	if (hovered.has_value() && std::holds_alternative<fVec2>(*hovered)) {
		fVec2 v = std::get<fVec2>(*hovered);
		SimpleRender2D::Circle(v, 0.5, Vec4(0.5, 0.5, 1.0, 1.0));
	}

	double total_seconds = m_TotalDuration.count();

	if(1) {
		const int wid_slider = 200;
		auto& mve = m_MapMetaState.p_EnemyMovement[0];
		ImGui::SetNextItemWidth(wid_slider);
		ImGui::SliderFloat("Min angular rate", &mve.p_MinMaxAngleMult.x, 0.0, 5.0);
		ImGui::SetNextItemWidth(wid_slider);
		ImGui::SliderFloat("Max angular rate", &mve.p_MinMaxAngleMult.y, 0.0, 5.0);

		ImGui::SetNextItemWidth(wid_slider);
		ImGui::SliderFloat("Min radius", &mve.p_MinMaxRadius.x, 0.0, 10.0);
		ImGui::SetNextItemWidth(wid_slider);
		ImGui::SliderFloat("Max radius", &mve.p_MinMaxRadius.y, 0.0, 10.0);

		ImGui::SetNextItemWidth(wid_slider);
		ImGui::SliderInt("Per Row", &mve.p_PerRow, 1, 30);
		ImGui::SetNextItemWidth(wid_slider);
		ImGui::SliderFloat("Spacing", &mve.p_Spacing, 0.0, 10.0);

		ImGui::SetNextItemWidth(wid_slider);
		ImGui::SliderInt("Subgroup Size", &mve.p_SubGroupSize, 1, 100);
		ImGui::SetNextItemWidth(wid_slider);
		ImGui::SliderFloat("Subgroup Deviation", &mve.p_SubGroupDeviation, 0.0, 1.0);
		ImGui::SetNextItemWidth(wid_slider);
		ImGui::SliderFloat("Index Deviation", &mve.p_IndexDeviation, 0.0, 1.0);
	}

	{
		int index = 0;
		for (const auto& skill_spec : g_Skills) {
			int level = m_SkillLevels[index];
			double value = skill_spec.GetValue(level);
			double cost = skill_spec.GetCost(level);
			auto button_buffer = std::format("Upgrade###{}", index);
			auto info_buffer = std::format("{} [{}]: {:.1f} (next cost: {:.1f})", skill_spec.p_Name, level, value, cost);
			if (ImGui::Button(button_buffer.c_str())) {
				m_SkillLevels[index]++;
			}
			ImGui::SameLine(0, 10.0);
			ImGui::Text("%s", info_buffer.c_str());
			index++;
		}
	}

	ImGui::Text("Seconds %.3f", total_seconds);
	ImGui::Text("m_Enemies %i", m_Enemies.GetLastKnownCount());
	ImGui::Text("m_EnemyProjectiles %i", m_EnemyProjectiles.GetLastKnownCount());
	ImGui::Text("m_PlayerDrones %i", m_PlayerDrones.GetLastKnownCount());
	ImGui::Text("m_PlayerProjectiles %i", m_PlayerProjectiles.GetLastKnownCount());
	ImGui::Text("Ship Pos %.3f, %.3f", m_LastMouseWorld.x, m_LastMouseWorld.y);
	ImGui::Text("Energy %.3f", m_GameState.p_Energy);

	ImGui::Text("KILLS: %i", m_GameState.p_TotalKills);

	{
		const int energy_left = 20;
		const int energy_bottom = 20;
		const int energy_width = 50;
		const int energy_height = 300;

		float energy_frac = m_GameState.p_Energy / GetEnergyMax();

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		draw_list->AddRectFilled(ImVec2(energy_left, height - energy_bottom - energy_height), ImVec2(energy_left + energy_width, height - energy_bottom), IM_COL32(0, 64, 0, 255));
		draw_list->AddRectFilled(ImVec2(energy_left, height - energy_bottom - int(energy_height * energy_frac)), ImVec2(energy_left + energy_width, height - energy_bottom), IM_COL32(0, 128, 0, 255));
	}

	SimpleRender2D::Square(m_MapSize * -0.5, m_MapSize * 0.5, Vec4(1.0, 0.0, 0.0, 0.8));
	SimpleRender2D::Texture(m_MapSize * -0.5, m_MapSize * 0.5, "../../assets/images/background_stars.png", 0.9);
	//SimpleRender2D::Texture(m_MapSize * -0.5, m_MapSize * 0.5, "../../assets/images/background_bright.png", 0.9);

	SimpleRender2D::Render(rtt, view_persp, width, height);
}