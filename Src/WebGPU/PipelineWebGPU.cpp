////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#if defined(NESHNY_WEBGPU)
namespace Neshny {

////////////////////////////////////////////////////////////////////////////////
EntityPipeline::EntityPipeline(RunType type, GPUEntity* entity, RenderableBuffer* buffer, BaseCache* cache, std::string_view shader_name, bool replace_main, std::string_view identifer, SSBO* control_ssbo, int iterations, WebGPUPipeline::RenderParams render_params) :
	m_Identifier		( identifer )
	,m_RunType			( type )
	,m_Entity			( entity )
	,m_Buffer			( buffer )
	,m_Cache			( cache )
	,m_ShaderName		( shader_name )
	,m_ReplaceMain		( replace_main )
	,m_Iterations		( iterations )
	,m_ControlSSBO		( control_ssbo )
	,m_RenderParams		( render_params )
{
	if (cache) {
		m_CachesToBind.push_back(cache);
	}
	const auto& limits = Core::Singleton().GetLimits();
	m_LocalSize = iVec3(limits.maxComputeInvocationsPerWorkgroup, 1, 1);
}

////////////////////////////////////////////////////////////////////////////////
EntityPipeline& EntityPipeline::AddEntity(GPUEntity& entity, BaseCache* cache) {
	m_Entities.push_back({ &entity, false });
	if (cache) {
		m_CachesToBind.push_back(cache);
	}
	return *this;
}

////////////////////////////////////////////////////////////////////////////////
EntityPipeline& EntityPipeline::AddCreatableEntity(GPUEntity& entity, BaseCache* cache) {
	m_Entities.push_back({ &entity, true });
	if (cache) {
		m_CachesToBind.push_back(cache);
	}
	return *this;
}

////////////////////////////////////////////////////////////////////////////////
bool EntityPipeline::OutputResults::GetValue(std::string_view name, int& value) const {
	for (const auto& result : p_Results) {
		if (result.first == name) {
			value = result.second;
			return true;
		}
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////
std::string EntityPipeline::GetDataVectorStructCode(const AddedDataVector& data_vect, bool read_only) {

	std::vector<std::string> insertion;
	insertion.push_back(std::format("struct {0} {{", data_vect.p_Name));

	std::vector<std::string> function{ std::format("fn Get{0}(base_index: i32) -> {0} {{ // use io{0}Num for count\n\tvar item: {0};", data_vect.p_Name) };
	std::vector<std::string> members;
	if (read_only) {
		function.push_back(std::format("\tlet index = io{0}Offset + (base_index * {1});", data_vect.p_Name, data_vect.p_NumIntsPerItem));
	} else {
		function.push_back(std::format("\tlet index = Get_io{0}Offset + (base_index * {1});", data_vect.p_Name, data_vect.p_NumIntsPerItem));
	}

	int pos_index = 0;
	for (const auto& member : data_vect.p_Members) {
		members.push_back(std::format("\t{}: {}", member.p_Name, MemberSpec::GetGPUType(member.p_Type)));

		std::string name = member.p_Name;
		if (read_only) {

			if (member.p_Type == MemberSpec::T_INT) {
				function.push_back(std::format("\titem.{0} = b_Control[index + {1}];", name, pos_index));
			} else if (member.p_Type == MemberSpec::T_FLOAT) {
				function.push_back(std::format("\titem.{0} = bitcast<f32>(b_Control[index + {1}]);", name, pos_index));
			} else if (member.p_Type == MemberSpec::T_VEC2) {
				function.push_back(std::format("\titem.{0} = vec2f(bitcast<f32>(b_Control[index + {1}]), bitcast<f32>(b_Control[index + {1} + 1]));", name, pos_index));
			} else if (member.p_Type == MemberSpec::T_VEC3) {
				function.push_back(std::format("\titem.{0} = vec3f(bitcast<f32>(b_Control[index + {1}]), bitcast<f32>(b_Control[index + {1} + 1]), bitcast<f32>(b_Control[index + {1} + 2]));", name, pos_index));
			} else if (member.p_Type == MemberSpec::T_VEC4) {
				function.push_back(std::format("\titem.{0} = vec4f(bitcast<f32>(b_Control[index + {1}]), bitcast<f32>(b_Control[index + {1} + 1]), bitcast<f32>(b_Control[index + {1} + 2]), bitcast<f32>(b_Control[index + {1} + 3]));", name, pos_index));
			} else if (member.p_Type == MemberSpec::T_IVEC2) {
				function.push_back(std::format("\titem.{0} = vec2i(b_Control[index + {1}], b_Control[index + {1} + 1]);", name, pos_index));
			} else if (member.p_Type == MemberSpec::T_IVEC3) {
				function.push_back(std::format("\titem.{0} = vec3i(b_Control[index + {1}], b_Control[index + {1} + 1], b_Control[index + {1} + 2]);", name, pos_index));
			} else if (member.p_Type == MemberSpec::T_IVEC4) {
				function.push_back(std::format("\titem.{0} = vec4i(b_Control[index + {1}], b_Control[index + {1} + 1], b_Control[index + {1} + 2], b_Control[index + {1} + 3]);", name, pos_index));
			}

		} else {
			if (member.p_Type == MemberSpec::T_INT) {
				function.push_back(std::format("\titem.{0} = atomicLoad(&b_Control[index + {1}]);", name, pos_index));
			} else if (member.p_Type == MemberSpec::T_FLOAT) {
				function.push_back(std::format("\titem.{0} = bitcast<f32>(atomicLoad(&b_Control[index + {1}]));", name, pos_index));
			} else if (member.p_Type == MemberSpec::T_VEC2) {
				function.push_back(std::format("\titem.{0} = vec2f(bitcast<f32>(atomicLoad(&b_Control[index + {1}])), bitcast<f32>(atomicLoad(&b_Control[index + {1} + 1])));", name, pos_index));
			} else if (member.p_Type == MemberSpec::T_VEC3) {
				function.push_back(std::format("\titem.{0} = vec3f(bitcast<f32>(atomicLoad(&b_Control[index + {1}])), bitcast<f32>(atomicLoad(&b_Control[index + {1} + 1])), bitcast<f32>(atomicLoad(&b_Control[index + {1} + 2])));", name, pos_index));
			} else if (member.p_Type == MemberSpec::T_VEC4) {
				function.push_back(std::format("\titem.{0} = vec4f(bitcast<f32>(atomicLoad(&b_Control[index + {1}])), bitcast<f32>(atomicLoad(&b_Control[index + {1} + 1])), bitcast<f32>(atomicLoad(&b_Control[index + {1} + 2])), bitcast<f32>(atomicLoad(&b_Control[index + {1} + 3])));", name, pos_index));
			} else if (member.p_Type == MemberSpec::T_IVEC2) {
				function.push_back(std::format("\titem.{0} = vec2i(atomicLoad(&b_Control[index + {1}]), atomicLoad(&b_Control[index + {1} + 1]));", name, pos_index));
			} else if (member.p_Type == MemberSpec::T_IVEC3) {
				function.push_back(std::format("\titem.{0} = vec3i(atomicLoad(&b_Control[index + {1}]), atomicLoad(&b_Control[index + {1} + 1]), atomicLoad(&b_Control[index + {1} + 2]));", name, pos_index));
			} else if (member.p_Type == MemberSpec::T_IVEC4) {
				function.push_back(std::format("\titem.{0} = vec4i(atomicLoad(&b_Control[index + {1}]), atomicLoad(&b_Control[index + {1} + 1]), atomicLoad(&b_Control[index + {1} + 2]), atomicLoad(&b_Control[index + {1} + 3]));", name, pos_index));
			}
		}
		pos_index += member.p_Size / sizeof(int);
	}
	function.push_back("\treturn item;\n};");

	insertion.push_back(JoinStrings(members, ",\n"));
	insertion.push_back("};");
	insertion.push_back(JoinStrings(function, "\n"));

	return JoinStrings(insertion, "\n");
}

////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<Core::CachedPipeline> EntityPipeline::GetCachedPipeline(void) {
	auto existing_pipelines = Core::Singleton().GetPreparedPipelines();
	for (auto& existing : existing_pipelines) {
		if (existing->m_Identifier == m_Identifier) {
			return existing;
		}
	}
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<Core::CachedPipeline> EntityPipeline::Prepare(void) {

	auto result = GetCachedPipeline();
	bool create_new = false;
	if (!result) {
		result = std::make_shared<Core::CachedPipeline>();
		result->m_Identifier = m_Identifier;
		result->m_Pipeline = new WebGPUPipeline();
		create_new = true;
	}

	for (BaseCache* cache : m_CachesToBind) {
		cache->Bind(*this, create_new);
	}

	struct BuffersToAdd {
		WebGPUBuffer& p_Buffer;
		WGPUShaderStageFlags p_VisibilityFlags;
		bool p_ReadOnly;
	};

	std::vector<BuffersToAdd> pipeline_buffers;
	std::vector<std::string> immediate_insertion;
	std::list<std::string> insertion;
	std::vector<std::string> end_insertion;
	std::vector<std::string> insertion_buffers;
	bool entity_processing = m_Entity && (m_RunType == RunType::ENTITY_PROCESS);
	bool is_render = ((m_RunType == RunType::ENTITY_RENDER) || (m_RunType == RunType::BASIC_RENDER));
	WGPUShaderStageFlags vis_flags = is_render ? WGPUShaderStage_Vertex | WGPUShaderStage_Fragment : WGPUShaderStage_Compute;

	if (create_new) {
		if (!m_ImmediateExtraCode.empty()) {
			immediate_insertion.push_back("////////////////");
			immediate_insertion.push_back(m_ImmediateExtraCode);
		}
		immediate_insertion.push_back(std::format("#define ENTITY_OFFSET_INTS {0}", ENTITY_OFFSET_INTS));
	}

	SSBO* control_ssbo = m_ControlSSBO ? m_ControlSSBO : (m_Entity ? m_Entity->GetControlSSBO() : nullptr);
	result->m_ReadRequired = false;
	if (control_ssbo) {
		if (create_new) {
			if (is_render) {
				insertion_buffers.push_back("@group(0) @binding(0) var<storage, read> b_Control: array<i32>;");
			} else {
				insertion_buffers.push_back("@group(0) @binding(0) var<storage, read_write> b_Control: array<atomic<i32>>;");
			}
		}
		pipeline_buffers.push_back({ *control_ssbo, vis_flags, is_render });
		for (const auto& var : m_Vars) {
			if (var.p_ReadBack) {
				result->m_ReadRequired = true;
				break;
			}
		}
	}

	if(!m_Uniform.p_Spec.empty()) { // uniform
		if (create_new) {
			std::vector<std::string> members;
			int size = 0;
			for (const auto& member : m_Uniform.p_Spec) {
				size += member.p_Size;
				members.push_back(std::format("\t{}: {}", member.p_Name, MemberSpec::GetGPUType(member.p_Type)));
			}
			immediate_insertion.push_back("struct UniformStruct {");
			immediate_insertion.push_back(JoinStrings(members, ",\n"));
			immediate_insertion.push_back("};");

			insertion_buffers.push_back(std::format("@group(0) @binding({0}) var<uniform> Uniform: UniformStruct;", insertion_buffers.size()));
			result->m_UniformBuffer = new WebGPUBuffer(WGPUBufferUsage_Uniform, size);
		}

		result->m_UniformBuffer->EnsureSizeBytes(m_Uniform.p_Data.size(), false);
		result->m_UniformBuffer->Write(m_Uniform.p_Data.data(), 0, m_Uniform.p_Data.size());

		pipeline_buffers.push_back({ *result->m_UniformBuffer, vis_flags, true });
	}

	if (create_new) {
		if (m_Entity && is_render) {
			insertion.push_back(std::format("#define Get_ioMaxIndex (b_{0}[3])", m_Entity->GetName()));
		}
		else if (m_Entity) {
			insertion.push_back(std::format("#define Get_ioMaxIndex (atomicLoad(&b_{0}[3]))", m_Entity->GetName()));
		}
		if (entity_processing) {
			m_Vars.push_back({ "ioEntityDeaths" });
			insertion.push_back(std::format("#define ioEntityCount b_{0}[0]", m_Entity->GetName()));
			insertion.push_back(std::format("#define ioEntityFreeCount b_{0}[1]", m_Entity->GetName()));
		}

		if (control_ssbo && (!m_DataVectors.empty())) {
			end_insertion.push_back("//////////////// Data vector helpers");
			for (const auto& data_vect : m_DataVectors) {
				end_insertion.push_back(GetDataVectorStructCode(data_vect, is_render));
				m_Vars.push_back({ data_vect.p_CountVar });
				m_Vars.push_back({ data_vect.p_OffsetVar });
				m_Vars.push_back({ data_vect.p_NumVar });
			}
		}

		result->m_UsingRandom = !is_render;
		if (result->m_UsingRandom) {
			m_Vars.push_back({ "ioRandSeed" });
			insertion.push_back("#include \"Random.wgsl\"\n");
			insertion.push_back("fn RandomRange(min_val: f32, max_val: f32) -> f32 { return GetRandomFromSeed(min_val, max_val, u32(atomicAdd(&ioRandSeed, 1))); }");
			insertion.push_back("fn Random() -> f32 { return GetRandomFromSeed(0.0, 1.0, u32(atomicAdd(&ioRandSeed, 1))); }");
		}
	}

	if(m_Entity) {
		bool input_read_only = is_render;
		pipeline_buffers.push_back({ *m_Entity->GetSSBO(), vis_flags, input_read_only });

		if (create_new) {
			result->m_EntityBufferIndex = insertion_buffers.size();
			insertion_buffers.push_back(std::format("@group(0) @binding({0}) var<storage, {1}> b_{2}: array<{3}>;", insertion_buffers.size(), input_read_only ? "read" : "read_write", m_Entity->GetName(), input_read_only ? "i32" : "atomic<i32>"));
		}

		if (entity_processing && m_Entity->IsDoubleBuffering()) {
			pipeline_buffers.push_back({ *m_Entity->GetOuputSSBO(), vis_flags, false });
			if (create_new) {
				insertion_buffers.push_back(std::format("@group(0) @binding({0}) var<storage, read_write> b_Output{1}: array<i32>;", insertion_buffers.size(), m_Entity->GetName()));
				insertion.push_back(std::string(m_Entity->GetDoubleBufferGPUInsertion()));
			}
		} else if (create_new) {
			insertion.push_back(std::string(m_Entity->GetGPUInsertion()));
		}
		if (create_new) {
			if (!input_read_only) {
				insertion.push_back(std::format("#define {0}_LOOKUP(base, index) (atomicLoad(&b_{0}[(base) + (index)]))", m_Entity->GetName()));
			}

			if (entity_processing) {
				insertion.push_back(m_Entity->GetSpecs().p_GPUInsertion);
			} else {
				insertion.push_back(m_Entity->GetSpecs().p_GPUReadOnlyInsertion);
			}
		}
	}

	if(entity_processing) {
		pipeline_buffers.push_back({ *m_Entity->GetFreeListSSBO(), vis_flags, false });
		if (create_new) {
			insertion_buffers.push_back(std::format("@group(0) @binding({0}) var<storage, read_write> b_FreeList: array<i32>;", insertion_buffers.size()));

			insertion.push_back(std::format("fn Destroy{0}(index: i32) {{", m_Entity->GetName()));
			insertion.push_back(std::format("\tlet base = index * FLOATS_PER_{0} + ENTITY_OFFSET_INTS;", m_Entity->GetName()));
			insertion.push_back(std::format("\t{0}_SET(base, 0, -1);",m_Entity->GetName()));
			insertion.push_back("\tatomicAdd(&ioEntityDeaths, 1);");
			insertion.push_back("\tatomicAdd(&ioEntityCount, -1);");
			insertion.push_back("\tlet free_index = atomicAdd(&ioEntityFreeCount, 1);");
			insertion.push_back("\tb_FreeList[free_index] = index;");
			insertion.push_back("}");
		}
	}

	for (const auto& ssbo: m_SSBOs) {
		bool read_only = ssbo.p_Access == BufferAccess::READ_ONLY;
		if (create_new) {
			int buffer_index = insertion_buffers.size();
			auto type_str = MemberSpec::GetGPUType(ssbo.p_Type);
			if (ssbo.p_Access == BufferAccess::READ_WRITE_ATOMIC) {
				type_str = std::format("atomic<{}>", type_str);
			}
			insertion_buffers.push_back(std::format("@group(0) @binding({0}) var<storage, {1}> {2}: array<{3}>;"
				,insertion_buffers.size()
				,read_only ? "read" : "read_write"
				,ssbo.p_Name
				,type_str
			));
		}
		pipeline_buffers.push_back({ ssbo.p_Buffer, vis_flags, read_only });
	}

	for (const auto& ssbo_spec : m_StructBuffers) {
		bool read_only = ssbo_spec.p_Access == BufferAccess::READ_ONLY;
		if (create_new) {
			std::vector<std::string> members;
			for (const auto& member : ssbo_spec.p_Members) {
				members.push_back(std::format("\t{}: {}", member.p_Name, MemberSpec::GetGPUType(member.p_Type)));
			}
			immediate_insertion.push_back(std::format("struct {0} {{", ssbo_spec.p_StructName));
			immediate_insertion.push_back(JoinStrings(members, ",\n"));
			immediate_insertion.push_back("};");

			int buffer_index = insertion_buffers.size();
			auto type_str = ssbo_spec.p_StructName;
			if (ssbo_spec.p_Access == BufferAccess::READ_WRITE_ATOMIC) {
				type_str = std::format("atomic<{0}>", type_str);
			}
			if (ssbo_spec.p_IsArray) {
				type_str = std::format("array<{0}>", type_str);
			}
			if (ssbo_spec.p_Buffer.GetUsageFlags() & WGPUBufferUsage_Uniform) {
				read_only = true;
				insertion_buffers.push_back(std::format("@group(0) @binding({0}) var<uniform> {1}: {2};"
					,insertion_buffers.size()
					,ssbo_spec.p_Name
					,type_str
				));
			} else {
				insertion_buffers.push_back(std::format("@group(0) @binding({0}) var<storage, {1}> {2}: {3};"
					,insertion_buffers.size()
					,read_only ? "read" : "read_write"
					,ssbo_spec.p_Name
					,type_str
				));
			}
		}

		pipeline_buffers.push_back({ ssbo_spec.p_Buffer, vis_flags, read_only });
	}

	for (auto& entity_spec : m_Entities) {
		if (!entity_spec.p_Entity) {
			continue;
		}
		GPUEntity& entity = *entity_spec.p_Entity;
		if (create_new) {
			insertion.push_back(std::format("//////////////// Entity {0}", entity.GetName()));
			insertion.push_back(std::format("#define io{0}Count b_{0}[0]", entity.GetName()));
		}

		bool read_only_entity = is_render;
		if (read_only_entity) {
			pipeline_buffers.push_back({ *entity.GetSSBO(), vis_flags, true });
			if (create_new) {
				insertion_buffers.push_back(std::format("@group(0) @binding({0}) var<storage, read> b_{1}: array<i32>;", insertion_buffers.size(), entity.GetName()));

				insertion.push_back(std::string(entity.GetGPUInsertion()));
				insertion.push_back(entity.GetSpecs().p_GPUReadOnlyInsertion);
			}
		} else {
			pipeline_buffers.push_back({ *entity.GetSSBO(), vis_flags, false });
			if (create_new) {
				insertion_buffers.push_back(std::format("@group(0) @binding({0}) var<storage, read_write> b_{1}: array<atomic<i32>>;", insertion_buffers.size(), entity.GetName()));

				insertion.push_back(std::string(entity.GetGPUInsertion()));
				// this will replace non-atomic getters and setters with atomic ones
				insertion.push_back(std::format("#define {0}_LOOKUP(base, index) (atomicLoad(&b_{0}[(base) + (index)]))", entity.GetName()));
				insertion.push_back(std::format("#define {0}_SET(base, index, value) atomicStore(&b_{0}[(base) + (index)], (value))", entity.GetName()));
				insertion.push_back(entity.GetSpecs().p_GPUInsertion);
			}
		}

		if (entity_spec.p_Creatable) {

			pipeline_buffers.push_back({ *entity.GetFreeListSSBO(), vis_flags, false });
			if (create_new) {
				insertion_buffers.push_back(std::format("@group(0) @binding({0}) var<storage, read_write> b_Free{1}: array<i32>;", insertion_buffers.size(), entity.GetName()));

				insertion.push_back(std::format("#define io{0}FreeCount b_{0}[1]", entity.GetName()));
				insertion.push_back(std::format("#define io{0}NextId b_{0}[2]", entity.GetName()));
				insertion.push_back(std::format("#define io{0}MaxIndex b_{0}[3]", entity.GetName()));

				auto next_id = std::format("io{0}NextId", entity.GetName());
				auto max_index = std::format("io{0}MaxIndex", entity.GetName());
				auto free_count = std::format("io{0}FreeCount", entity.GetName());

				insertion.push_back(std::format("fn Create{0}(input_item: {0}) {{", entity.GetName()));
				insertion.push_back("\tvar item = input_item;");
				insertion.push_back(std::format("\tlet item_id: i32 = atomicAdd(&{0}, 1);", next_id));

				insertion.push_back("\tvar item_pos: i32 = 0;");
				insertion.push_back(std::format("\tatomicAdd(&io{0}Count, 1);", entity.GetName()));
				insertion.push_back(std::format("\tlet free_count: i32 = atomicAdd(&{0}, -1);", free_count));
				insertion.push_back(std::format("\tatomicMax(&{0}, 0);", free_count)); // ensure this is never negative after completion
				insertion.push_back("\tif(free_count > 0) {");
					insertion.push_back(std::format("\t\titem_pos = b_Free{0}[free_count - 1];", entity.GetName()));
				insertion.push_back("\t} else {");
					insertion.push_back(std::format("\t\titem_pos = atomicAdd(&{0}, 1);", max_index));
				insertion.push_back("\t}");

				insertion.push_back(std::format("\titem.{0} = item_id;", entity.GetIDName()));
				insertion.push_back(std::format("\tSet{0}(item, item_pos);\n}}", entity.GetName()));
			}
		}
	}

	if (create_new) {

		for (const auto& tex : m_Textures) {

	#pragma msg("fix pipeline API to only require dimension on init, then use withTexture pattern")
			auto dim = tex.p_Tex->GetViewDimension();
			std::string tex_spec;
			if (dim == WGPUTextureViewDimension_1D) {
				tex_spec = "texture_1d<f32>";
			} else if (dim == WGPUTextureViewDimension_2D) {
				tex_spec = "texture_2d<f32>";
			} else if (dim == WGPUTextureViewDimension_2DArray) {
				tex_spec = "texture_2d_array<f32>";
			} else if (dim == WGPUTextureViewDimension_Cube) {
				tex_spec = "texture_cube<f32>";
			} else if (dim == WGPUTextureViewDimension_CubeArray) {
				tex_spec = "texture_cube_array<f32>";
			} else if (dim == WGPUTextureViewDimension_3D) {
				tex_spec = "texture_3d<f32>";
			}
			insertion_buffers.push_back(std::format("@group(0) @binding({0}) var {1}: {2};", insertion_buffers.size(), tex.p_Name, tex_spec));
			result->m_Pipeline->AddTexture(tex.p_Tex->GetViewDimension(), tex.p_Tex->GetTextureView());
		}
		for (const auto& sampler : m_Samplers) {
			insertion_buffers.push_back(std::format("@group(0) @binding({0}) var {1}: sampler;", insertion_buffers.size(), sampler.p_Name));
			result->m_Pipeline->AddSampler(*sampler.p_Sampler);
		}

		if (control_ssbo) { // avoid initial validation error for zero-sized buffers
			control_ssbo->EnsureSizeBytes(std::max(int(m_Vars.size()), 1) * sizeof(int));
		}
		for (int i = 0; i < m_Vars.size(); i++) {
			const auto& var = m_Vars[i];
			insertion.push_front(std::format("#define Get_{0} (atomicLoad(&b_Control[{1}]))", var.p_Name, i));
			insertion.push_front(std::format("#define {0} (b_Control[{1}])", var.p_Name, i));
			result->m_VarNames.push_back(var.p_Name);
		}

		insertion.push_back("////////////////");

		if (entity_processing && m_ReplaceMain) {
			end_insertion.push_back(std::format("@compute @workgroup_size({0}, {1}, {2})", m_LocalSize.x, m_LocalSize.y, m_LocalSize.z));
			end_insertion.push_back(std::format(
				"fn main(@builtin(global_invocation_id) global_id: vec3u) {{\n"
				"\tlet item_index = i32(global_id.x);\n"
				"\tif (item_index >= Get_ioMaxIndex) {{ return; }}\n"
				"\tlet item: {0} = Get{0}(item_index);", m_Entity->GetName()));
			if (m_Entity->IsDoubleBuffering()) {
				end_insertion.push_back(std::format("\tif (item.{0} < 0) {{ Set{1}(item, item_index); return; }}", m_Entity->GetIDName(), m_Entity->GetName()));
			} else {
				end_insertion.push_back(std::format("\tif (item.{0} < 0) {{ return; }}", m_Entity->GetIDName()));
			}
			end_insertion.push_back(std::format(
				"\tvar new_item: {0} = item;\n"
				"\tlet should_destroy: bool = {0}Main(item_index, item, &new_item);\n"
				"\tif(should_destroy) {{ Destroy{0}(item_index); }} else {{ Set{0}(new_item, item_index); }}\n"
				"}}", m_Entity->GetName()));
		} else if (m_Entity && m_ReplaceMain && (m_RunType == RunType::ENTITY_RENDER)) {

			/*
			end_insertion +=
				"@vertex\n"
				"void {0}Main(int item_index, {0} item); // forward declaration\n"
				"void main() {\n"
				"\t{0} item = Get{0}(gl_InstanceID);";
			end_insertion.push_back(std::format("\tif (item.{0} < 0) { gl_Position = vec4(0.0, 0.0, 100.0, 0.0); return; }", m_Entity->GetIDName());
			end_insertion.push_back(
				"\t{0}Main(gl_InstanceID, item);\n"
				"}\n#endif\n////////////////");
				*/
		} else if (m_Entity && m_ReplaceMain) {
			end_insertion.push_back(std::format("@compute @workgroup_size({0}, {1}, {2})", m_LocalSize.x, m_LocalSize.y, m_LocalSize.z));
			end_insertion.push_back(std::format(
				"fn main(@builtin(global_invocation_id) global_id: vec3u) {{\n"
				"\tlet item_index = i32(global_id.x);\n"
				"\tif (item_index >= Get_ioMaxIndex) {{ return; }}\n"
				"\tlet item: {0} = Get{0}(item_index);", m_Entity->GetName()));
			end_insertion.push_back(std::format("\tif (item.{0} < 0) {{ return; }}", m_Entity->GetIDName()));
			end_insertion.push_back(std::format(
				"\t{0}Main(item_index, item);\n"
				"}}\n////////////////", m_Entity->GetName()));
		}

		if (!m_ExtraCode.empty()) {
			insertion.push_back("////////////////");
			insertion.push_back(m_ExtraCode);
		}

		insertion.push_front(JoinStrings(insertion_buffers, "\n"));
		insertion.push_front(JoinStrings(immediate_insertion, "\n"));
	}

	if (create_new) {
		for (const auto& buffer_to_add : pipeline_buffers) {
			result->m_Pipeline->AddBuffer(buffer_to_add.p_Buffer, buffer_to_add.p_VisibilityFlags, buffer_to_add.p_ReadOnly);
		}

		std::string insertion_str = JoinStrings(insertion, "\n");
		std::string end_insertion_str;
		if (!end_insertion.empty()) {
			end_insertion_str = "\n//////////\n" + JoinStrings(end_insertion, "\n");
		}

		if (is_render) {
			result->m_Pipeline->FinalizeRender(m_ShaderName, *m_Buffer, m_RenderParams, insertion_str, end_insertion_str);
		} else {
			result->m_Pipeline->FinalizeCompute(m_ShaderName, insertion_str, end_insertion_str);
		}
		Core::Singleton().CachePreparedPipeline(result);
	} else {
		for (int i = 0; i < pipeline_buffers.size(); i++) {
			result->m_Pipeline->ReplaceBuffer(i, pipeline_buffers[i].p_Buffer);
		}
	}

	return result;
}

////////////////////////////////////////////////////////////////////////////////
EntityPipeline::AsyncOutputResults EntityPipeline::RunInternal(int iterations, RTT* rtt, std::optional<std::function<void(const OutputResults& results)>>&& callback) {

	auto prepared = Prepare();
	if (Core::Singleton().GetPipelinePrepareOnlyMode()) {
		return AsyncOutputResults::Empty();
	}

	bool compute = prepared->m_Pipeline->GetType() == WebGPUPipeline::Type::COMPUTE;
	bool entity_processing = m_Entity && (m_RunType == RunType::ENTITY_PROCESS);

	if (!compute && !rtt) {
		throw std::invalid_argument("Render requires an RTT");
	}

	if (m_Entity) {
		iterations = m_Entity->GetMaxCount();
	}

	bool previously_using_temp_frame = prepared->m_TemporaryFrame.get();
	prepared->m_TemporaryFrame = nullptr;
	if (!compute && m_Entity) {
		int time_slider = Core::GetInterfaceData().p_BufferView.p_TimeSlider;
		if (time_slider > 0) {
			prepared->m_TemporaryFrame = BufferViewer::GetStoredFrameAt(m_Entity->GetName(), Core::GetTicks() - time_slider, iterations);
		}
	}

	for(auto& pair: m_Entities) {
		if (pair.p_Entity && pair.p_Entity->IsDoubleBuffering()) {
			prepared->m_Pipeline->ReplaceBuffer(*pair.p_Entity->GetOuputSSBO(), *pair.p_Entity->GetSSBO());
		}
	}

	auto fill_var_data = [this, prepared](std::string_view name, int& value) {
		for (const auto& var: m_Vars) {
			if (var.p_Name== name) {
				value = var.p_Input;
				return;
			}
		}
		int offset = prepared->m_VarNames.size();
		for (const auto& vect : m_DataVectors) {
			if (vect.p_CountVar == name) {
				value = vect.p_NumIntsPerItem * vect.p_NumItems;
				return;
			}
			if (vect.p_NumVar == name) {
				value = vect.p_NumItems;
				return;
			}
			if (vect.p_OffsetVar == name) {
				value = offset;
				return;
			}
			offset += vect.p_NumIntsPerItem * vect.p_NumItems;
		}
	};

	int death_count_index = -1;
	std::vector<int> var_vals(prepared->m_VarNames.size(), 0);
	for (int i = 0; i < prepared->m_VarNames.size(); i++) {
		std::string_view varname = prepared->m_VarNames[i];
		if (entity_processing && (varname == "ioEntityDeaths")) {
			death_count_index = i;
			var_vals[i] = 0;
			continue;
		}
		if (prepared->m_UsingRandom && (varname == "ioRandSeed")) {
			var_vals[i] = RandomInt(0, std::numeric_limits<int>::max());
			continue;
		}

		fill_var_data(varname, var_vals[i]);
	}

	SSBO* control_ssbo = m_Entity ? m_Entity->GetControlSSBO() : m_ControlSSBO;
	if (control_ssbo && !var_vals.empty()) {

		std::vector<int> values = var_vals;
		int control_size = (int)var_vals.size();

		// allocate some space for vectors on the control buffer and copy data over
		for (const auto& data_vect : m_DataVectors) {
			int data_size = data_vect.p_NumIntsPerItem * data_vect.p_NumItems;
			int offset = control_size;
			control_size += data_size;
			values.resize(control_size);
			memcpy((unsigned char*)&(values[offset]), data_vect.p_Data, sizeof(int) * data_size);
		}

		control_ssbo->EnsureSizeBytes(control_size * sizeof(int), false);
		control_ssbo->SetValues(values);
	}

	if (compute) {
		prepared->m_Pipeline->Compute(iterations);
	} else {
		if (!m_Buffer) {
			throw std::invalid_argument("Render buffer not found");
		}
		if (prepared->m_TemporaryFrame.get()) {
			prepared->m_Pipeline->ReplaceBuffer(prepared->m_EntityBufferIndex, *prepared->m_TemporaryFrame.get());
		} else if (m_Entity && (m_Entity->IsDoubleBuffering() || previously_using_temp_frame)) {
			prepared->m_Pipeline->ReplaceBuffer(prepared->m_EntityBufferIndex, *m_Entity->GetSSBO());
		}
		rtt->Render(prepared->m_Pipeline, iterations);
	}
	if (entity_processing) {
		m_Entity->QueueInfoRead();
	}
	for (auto& pair: m_Entities) {
		if (pair.p_Entity && pair.p_Creatable) {
			pair.p_Entity->QueueInfoRead();
		}
	}

	////////////////////////////////////////////////////

	if (entity_processing && m_Entity->IsDoubleBuffering()) {
		WebGPUBuffer::CopyBufferToBuffer(m_Entity->GetSSBO()->Get(), m_Entity->GetOuputSSBO()->Get(), 0, 0, sizeof(EntityInfo));
		prepared->m_Pipeline->ReplaceBuffer(*m_Entity->GetOuputSSBO(), *m_Entity->GetSSBO());
		prepared->m_Pipeline->ReplaceBuffer(*m_Entity->GetSSBO(), *m_Entity->GetOuputSSBO());
		m_Entity->SwapInputOutputSSBOs();
	}

#ifdef NESHNY_ENTITY_DEBUG
	if (entity_processing && (m_Entity->GetDeleteMode() == GPUEntity::DeleteMode::MOVING_COMPACT)) {
		BufferViewer::Checkpoint(std::format("{0} Death", m_Entity->GetName()), "PostRun", *m_Entity->GetFreeListSSBO(), MemberSpec::Type::T_INT);
	}
	if (entity_processing) {
		BufferViewer::Checkpoint(std::format("{0} Control", m_Entity->GetName()), "PostRun", *control_ssbo, MemberSpec::Type::T_INT);
		BufferViewer::Checkpoint(std::format("{0} Free", m_Entity->GetName()), "PostRun", *m_Entity->GetFreeListSSBO(), MemberSpec::Type::T_INT, m_Entity->GetFreeCount());
	}
#endif

	if (control_ssbo && prepared->m_ReadRequired) {
		return control_ssbo->Read<OutputResults>(0, (int)(var_vals.size() * sizeof(int)), [var_names = prepared->m_VarNames, result_callback = std::move(callback)](unsigned char* data, int size, AsyncOutputResults token) -> std::shared_ptr<OutputResults> {
			OutputResults* results = new OutputResults();
			int* int_data = (int*)data;
			for (int i = 0; i < var_names.size(); i++) {
				results->p_Results.push_back({ var_names[i], int_data[i] });
			}
			if (result_callback.has_value()) {
				result_callback.value()(*results);
			}
			return std::shared_ptr<OutputResults>(results);
		});
	}

	return AsyncOutputResults::Empty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
QueryEntities::QueryEntities(GPUEntity& entity) :
	m_Entity(entity)
{
}

////////////////////////////////////////////////////////////////////////////////
QueryEntities& QueryEntities::ByNearestPosition(std::string_view param_name, fVec2 pos) {
	m_Query = QueryType::Position2D;
	m_QueryParam = pos;
	m_ParamName = std::string(param_name);
	return *this;
}

////////////////////////////////////////////////////////////////////////////////
QueryEntities& QueryEntities::ByNearestPosition(std::string_view param_name, fVec3 pos) {
	m_Query = QueryType::Position3D;
	m_QueryParam = pos;
	m_ParamName = std::string(param_name);
	return *this;
}

////////////////////////////////////////////////////////////////////////////////
QueryEntities& QueryEntities::ById(int id) {
	m_Query = QueryType::ID;
	m_QueryParam = id;
	m_ParamName = m_Entity.GetIDName();
	return *this;
}

////////////////////////////////////////////////////////////////////////////////
int QueryEntities::ExecuteQuery(void) {

#pragma msg("finish webGPU version")
	return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
Grid2DCache::Grid2DCache(GPUEntity& entity, std::string_view pos_name) :
	m_Entity		( entity )
	,m_PosName		( pos_name )
#if defined(NESHNY_WEBGPU)
	,m_GridIndices	( WGPUBufferUsage_Storage )
	,m_GridItems	( WGPUBufferUsage_Storage )
	,m_Uniform		( WGPUBufferUsage_Uniform, sizeof(Grid2DCacheUniform) )
#endif
{
}

////////////////////////////////////////////////////////////////////////////////
void Grid2DCache::GenerateCache(iVec2 grid_size, Vec2 grid_min, Vec2 grid_max) {

	m_GridSize = grid_size;
	m_GridMin = grid_min;
	m_GridMax = grid_max;

	m_GridIndices.EnsureSizeBytes(grid_size.x * grid_size.y * 3 * sizeof(int), true);
	m_GridItems.EnsureSizeBytes(m_Entity.GetMaxCount() * sizeof(int), false);

	std::string main_func = std::format("fn {0}Main(item_index: i32, item: {0}) {{ ItemMain(item_index, item.{1}); }}", m_Entity.GetName(), m_PosName);
	std::string base_id = std::format("GridCache2D:{}:{}", m_Entity.GetName(), m_PosName);

	Grid2DCacheUniform uniform{
		grid_size
		,grid_min
		,grid_max
	};

	Neshny::EntityPipeline::IterateEntity(std::format("{}:INDEX", base_id), m_Entity, "GridCache2D", true)
		.AddBuffer("b_Index", m_GridIndices, MemberSpec::T_INT, EntityPipeline::BufferAccess::READ_WRITE_ATOMIC)
		.AddCode("#define PHASE_INDEX")
		.AddCode(main_func)
		.SetUniform(uniform)
		.Run();

	for (int i = 0; i < 2; i++) {
		Neshny::EntityPipeline::IterateEntity(std::format("{}:ALLOC", base_id), m_Entity, "GridCache2D", true)
			.AddBuffer("b_Index", m_GridIndices, MemberSpec::T_INT, EntityPipeline::BufferAccess::READ_WRITE_ATOMIC)
			.AddCode("#define PHASE_ALLOCATE")
			.AddCode(main_func)
			.AddInputVar("AllocationCount", 0)
			.SetUniform(uniform)
			.Run();
	}

	Neshny::EntityPipeline::IterateEntity(std::format("{}:FILL", base_id), m_Entity, "GridCache2D", true)
		.AddBuffer("b_Index", m_GridIndices, MemberSpec::T_INT, EntityPipeline::BufferAccess::READ_WRITE_ATOMIC)
		.AddBuffer("b_Cache", m_GridItems, MemberSpec::T_INT, EntityPipeline::BufferAccess::READ_WRITE)
		.AddCode("#define PHASE_FILL")
		.AddCode(main_func)
		.SetUniform(uniform)
		.Run();

	// gets used by the entity run that uses the buffer
	m_Uniform.SetSingleValue(0, uniform);
}

////////////////////////////////////////////////////////////////////////////////
void Grid2DCache::Bind(EntityPipeline& target_stage, bool initial_creation) {

	auto name = m_Entity.GetName();

	target_stage.AddBuffer(std::format("b_{0}GridIndices", name), m_GridIndices, MemberSpec::Type::T_INT, EntityPipeline::BufferAccess::READ_ONLY);
	target_stage.AddBuffer(std::format("b_{0}GridItems", name), m_GridItems, MemberSpec::Type::T_INT, EntityPipeline::BufferAccess::READ_ONLY);

	target_stage.AddStructBuffer<Grid2DCacheUniform>(std::format("b_{0}GridUniform", name), std::format("{0}GridUniformStruct", name), m_Uniform, EntityPipeline::BufferAccess::READ_ONLY, false);

	if (!initial_creation) {
		return;
	}

	std::string err_msg;
	std::string utils_file = Core::Singleton().LoadEmbedded("GridCache2D.wgsl.template", err_msg);
	if (utils_file.empty()) {
		Core::Log(std::format("could not open 'GridCache2D.wgsl.template' {}", err_msg), ImVec4(1.0, 0.25f, 0.25f, 1.0));
		target_stage.AddCode("#error could not open 'GridCache2D.wgsl.template'");
		return;
	}

	target_stage.AddCode(ReplaceAll(ReplaceAll(utils_file, "<<NAME>>", name), "<<ID>>", m_Entity.GetIDName()));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
Grid3DCache::Grid3DCache(GPUEntity& entity, std::string_view pos_name) :
	m_Entity		( entity )
	,m_PosName		( pos_name )
#if defined(NESHNY_WEBGPU)
	,m_GridIndices	( WGPUBufferUsage_Storage )
	,m_GridItems	( WGPUBufferUsage_Storage )
	,m_Uniform		( WGPUBufferUsage_Uniform, sizeof(Grid3DCacheUniform) )
#endif
{
}

////////////////////////////////////////////////////////////////////////////////
void Grid3DCache::GenerateCache(iVec3 grid_size, Vec3 grid_min, Vec3 grid_max) {

	m_GridSize = grid_size;
	m_GridMin = grid_min;
	m_GridMax = grid_max;

	m_GridIndices.EnsureSizeBytes(grid_size.x * grid_size.y * grid_size.z * 3 * sizeof(int), true);
	m_GridItems.EnsureSizeBytes(m_Entity.GetMaxCount() * sizeof(int), false);

	std::string main_func = std::format("fn {0}Main(item_index: i32, item: {0}) {{ ItemMain(item_index, item.{1}); }}", m_Entity.GetName(), m_PosName);
	std::string base_id = std::format("GridCache3D:{}:{}", m_Entity.GetName(), m_PosName);

	Grid3DCacheUniform uniform;
	uniform.p_GridSize = grid_size;
	uniform.p_GridMin = grid_min.ToFloat3();
	uniform.p_GridMax = grid_max.ToFloat3();

	Neshny::EntityPipeline::IterateEntity(std::format("{}:INDEX", base_id), m_Entity, "GridCache3D", true)
		.AddBuffer("b_Index", m_GridIndices, MemberSpec::T_INT, EntityPipeline::BufferAccess::READ_WRITE_ATOMIC)
		.AddCode("#define PHASE_INDEX")
		.AddCode(main_func)
		.SetUniform(uniform)
		.Run();

	for (int i = 0; i < 2; i++) {
		Neshny::EntityPipeline::IterateEntity(std::format("{}:ALLOC", base_id), m_Entity, "GridCache3D", true)
			.AddBuffer("b_Index", m_GridIndices, MemberSpec::T_INT, EntityPipeline::BufferAccess::READ_WRITE_ATOMIC)
			.AddCode("#define PHASE_ALLOCATE")
			.AddCode(main_func)
			.AddInputVar("AllocationCount", 0)
			.SetUniform(uniform)
			.Run();
	}

	Neshny::EntityPipeline::IterateEntity(std::format("{}:FILL", base_id), m_Entity, "GridCache3D", true)
		.AddBuffer("b_Index", m_GridIndices, MemberSpec::T_INT, EntityPipeline::BufferAccess::READ_WRITE_ATOMIC)
		.AddBuffer("b_Cache", m_GridItems, MemberSpec::T_INT, EntityPipeline::BufferAccess::READ_WRITE)
		.AddCode("#define PHASE_FILL")
		.AddCode(main_func)
		.SetUniform(uniform)
		.Run();

	// gets used by the entity run that uses the buffer
	m_Uniform.SetSingleValue(0, uniform);
}

////////////////////////////////////////////////////////////////////////////////
void Grid3DCache::Bind(EntityPipeline& target_stage, bool initial_creation) {

	auto name = m_Entity.GetName();

	target_stage.AddBuffer(std::format("b_{0}GridIndices", name), m_GridIndices, MemberSpec::Type::T_INT, EntityPipeline::BufferAccess::READ_ONLY);
	target_stage.AddBuffer(std::format("b_{0}GridItems", name), m_GridItems, MemberSpec::Type::T_INT, EntityPipeline::BufferAccess::READ_ONLY);

	target_stage.AddStructBuffer<Grid3DCacheUniform>(std::format("b_{0}GridUniform", name), std::format("{0}GridUniformStruct", name), m_Uniform, EntityPipeline::BufferAccess::READ_ONLY, false);

	if (!initial_creation) {
		return;
	}

	std::string err_msg;
	std::string utils_file = Core::Singleton().LoadEmbedded("GridCache3D.wgsl.template", err_msg);
	if (utils_file.empty()) {
		Core::Log(std::format("could not open 'GridCache3D.wgsl.template' {}", err_msg), ImVec4(1.0, 0.25f, 0.25f, 1.0));
		target_stage.AddCode("#error could not open 'GridCache3D.wgsl.template'");
		return;
	}

	target_stage.AddCode(ReplaceAll(ReplaceAll(utils_file, "<<NAME>>", name), "<<ID>>", m_Entity.GetIDName()));
}


} // namespace Neshny
#endif