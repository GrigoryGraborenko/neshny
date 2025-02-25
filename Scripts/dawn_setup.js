///////////////////////////////////////////////////////////////////////////////
const util = require('util');
const fs = require('fs');
const exec = util.promisify(require('child_process').exec);
const os = require('os');

const DAWN_VERSION = `chromium/6630`;
const DAWN_URL = `https://dawn.googlesource.com/dawn`;
const search_str = "DAWN_PATH";

// https://nodejs.org/api/os.html#os_os_type
const is_windows = os.type() == "Windows_NT";
const is_mac = os.type() == "Darwin";
const is_linux = os.type() == "Linux";
const lib_ext = is_windows ? ".lib" : ".a";

const required_files = [
    "SPIRV-Tools-opt.lib",
    "SPIRV-Tools.lib",
    "absl_int128.lib",
    "absl_raw_logging_internal.lib",
    "absl_str_format_internal.lib",
    "absl_strerror.lib",
    "absl_strings.lib",
    "absl_strings_internal.lib",
    "absl_throw_delegate.lib",
	"absl_city.lib",
	"absl_hash.lib",
	"absl_low_level_hash.lib",
	"absl_raw_hash_set.lib",
    "dawn_common.lib",
    "dawn_headers.lib",
    "dawn_native.lib",
    "dawn_platform.lib",
    "dawn_proc.lib",
    "dawn_utils.lib",
    "dawncpp.lib",
    "dawncpp_headers.lib",
    "tint_api.lib",
    "tint_api_common.lib",
    "tint_api_options.lib",
    "tint_cmd_common.lib",
    "tint_lang_core.lib",
    "tint_lang_core_constant.lib",
    "tint_lang_core_intrinsic.lib",
    "tint_lang_core_ir.lib",
    "tint_lang_core_ir_transform.lib",
    "tint_lang_core_type.lib",
    "tint_lang_hlsl_validate.lib",
    "tint_lang_hlsl_writer.lib",
    "tint_lang_hlsl_writer_ast_printer.lib",
    "tint_lang_hlsl_writer_ast_raise.lib",
    "tint_lang_hlsl_writer_common.lib",

    "tint_lang_hlsl.lib",
    "tint_lang_hlsl_intrinsic.lib",
    "tint_lang_hlsl_type.lib",
    "tint_lang_hlsl_ir.lib",
    "tint_lang_hlsl_writer_raise.lib",
    "tint_lang_hlsl_writer_printer.lib",
    "tint_lang_core_ir_transform_common.lib",
    "tint_lang_spirv_reader_lower.lib",

    "tint_lang_spirv.lib",
    "tint_lang_spirv_intrinsic.lib",
    "tint_lang_spirv_ir.lib",
    "tint_lang_spirv_reader.lib",
    "tint_lang_spirv_reader_ast_lower.lib",
    "tint_lang_spirv_reader_ast_parser.lib",
    "tint_lang_spirv_reader_common.lib",
	"tint_lang_spirv_reader_parser.lib",
	"tint_lang_spirv_validate.lib",
    "tint_lang_spirv_type.lib",
    "tint_lang_spirv_writer.lib",
    "tint_lang_spirv_writer_ast_printer.lib",
    "tint_lang_spirv_writer_ast_raise.lib",
    "tint_lang_spirv_writer_common.lib",
    "tint_lang_spirv_writer_printer.lib",
    "tint_lang_spirv_writer_raise.lib",
    "tint_lang_wgsl.lib",
    "tint_lang_wgsl_ast.lib",
    "tint_lang_wgsl_ast_transform.lib",
    "tint_lang_wgsl_helpers.lib",
    "tint_lang_wgsl_inspector.lib",
    "tint_lang_wgsl_intrinsic.lib",
    "tint_lang_wgsl_ir.lib",
    "tint_lang_wgsl_program.lib",
    "tint_lang_wgsl_reader.lib",
    "tint_lang_wgsl_reader_lower.lib",
    "tint_lang_wgsl_reader_parser.lib",
    "tint_lang_wgsl_reader_program_to_ir.lib",
    "tint_lang_wgsl_resolver.lib",
    "tint_lang_wgsl_sem.lib",
    "tint_lang_wgsl_writer.lib",
    "tint_lang_wgsl_writer_ast_printer.lib",
    "tint_lang_wgsl_writer_ir_to_program.lib",
    "tint_lang_wgsl_writer_raise.lib",
    "tint_lang_wgsl_writer_syntax_tree_printer.lib",
	"tint_lang_wgsl_features.lib",
    "tint_utils_debug.lib",
    "tint_utils_diagnostic.lib",
    "tint_utils_generator.lib",
    "tint_utils_ice.lib",
    "tint_utils_id.lib",
    "tint_utils_reflection.lib",
    "tint_utils_result.lib",
    "tint_utils_rtti.lib",
    "tint_utils_strconv.lib",
    "tint_utils_symbol.lib",
    "tint_utils_text.lib",
    "webgpu_dawn.lib"
];

if (is_mac) {
    for(var i = 0; i < required_files.length; i++) {
        required_files.splice(i, 1, "lib" + required_files[i].replaceAll('.lib', '.a'));
    }
}

function crawlExtensionFiles(file_list, ext, dir, root_dir = undefined) {
    if (!root_dir) {
        root_dir = dir;
    }
    fs.readdirSync(dir).forEach(function(filename) {
        var filepath = dir + "/" + filename;
        var stats = fs.statSync(filepath);
        if(!stats.isDirectory()) {
            if (filename.substring(filename.lastIndexOf(".")) === ext) {
                file_list.push({ name: filename, path: filepath, dir: dir, reldir: dir.replaceAll(root_dir, "") });
            }
        } else {
            crawlExtensionFiles(file_list, ext, dir + "/" + filename, root_dir);
        }
    });
}

function crawlRequiredFiles(file_list, dir) {
    fs.readdirSync(dir).forEach(function(filename) {
        var filepath = dir + "/" + filename;
        var stats = fs.statSync(filepath);
        if(!stats.isDirectory()) {
            if (required_files.includes(filename)) {
                file_list.push(filepath);
            }
        } else {
            crawlRequiredFiles(file_list, dir + "/" + filename);
        }
    });
}

function copyAllFilesTo(file_list, dir) {
    file_list.forEach(path => {
        const path_index = Math.max(path.lastIndexOf("/"), path.lastIndexOf("\\"));
        const file = path.substring(path_index);
        fs.copyFileSync(path, `${dir}/${file}`);
    });
}

const created_paths = {};
function copyAllFilesStructureTo(file_obj_list, dest_dir) {
    file_obj_list.forEach(file_obj => {

        const new_dir = `${dest_dir}${file_obj.reldir}`;
        if (!created_paths[new_dir]) {
            created_paths[new_dir] = true;
            fs.mkdirSync(new_dir, { recursive: true });
        }

        const dest_file = `${new_dir}/${file_obj.name}`;
        fs.copyFileSync(file_obj.path, dest_file);
    });
}

async function run(all_libs, update_existing, only_copy) {
    try {
        await exec("git --version");
    } catch (err) {
        throw "Git is not installed";
    }

    const user_settings = fs.readFileSync('UserSettings.cmake', 'utf8');
    const dawn_line = user_settings.split('\n').find(line => line.includes(search_str));
    if (!dawn_line) {
        throw `Cannot find settings entry for ${search_str}`;
    }

    let dawn_path = dawn_line.substring(dawn_line.indexOf("\"") + 1, dawn_line.lastIndexOf("\""));
    if (dawn_path.length == 0) {
        throw `Cannot find dawn path - make sure to use quotes`;
    }
    if (fs.statSync(dawn_path, { throwIfNoEntry: false }) === undefined) {
        console.log(`Making new directory for Dawn at ${dawn_path}`);
        fs.mkdirSync(dawn_path, { recursive: true });
    }
	let build = false;
    if (fs.statSync(`${dawn_path}/dawn/.git`, { throwIfNoEntry: false }) === undefined) {
        console.log("Cloning dawn repo...");
        await exec(`git clone ${DAWN_URL}`, { cwd: dawn_path });
        await exec(`git checkout ${DAWN_VERSION}`, { cwd: dawn_path });
        dawn_path += "/dawn";
		build = true;
    } else if (update_existing && (!only_copy)) {
        console.log("Found existing dawn repo, updating...");
        dawn_path += "/dawn";
        await exec(`git fetch`, { cwd: dawn_path });
        await exec(`git checkout ${DAWN_VERSION}`, { cwd: dawn_path });
		build = true;
    } else {
        console.log("Found existing dawn repo (run with -update to ensure version)");
        dawn_path += "/dawn";
	}

	if (build) {
		console.log("Fetching dependencies...");
		await exec(`python tools/fetch_dawn_dependencies.py --use-test-deps`, { cwd: dawn_path });

        console.log("Running cmake...");
        if (is_windows) {
            await exec(`cmake . -B build-debug -G "Visual Studio 17 2022" -A x64`, { cwd: dawn_path });
            await exec(`cmake . -B build-release -G "Visual Studio 17 2022" -A x64`, { cwd: dawn_path });
        } else {
            await exec(`cmake . -B build-debug -G "Unix Makefiles"`, { cwd: dawn_path });
            await exec(`cmake . -B build-release -G "Unix Makefiles"`, { cwd: dawn_path });
        }

		console.log("Building debug...");
		await exec(`cmake --build build-debug --config=DEBUG`, { cwd: dawn_path });
		console.log("Building release...");
		await exec(`cmake --build build-release --config=RELEASE`, { cwd: dawn_path });
	}

    fs.mkdirSync('./external/WebGPU/lib/Debug', { recursive: true });
    fs.mkdirSync('./external/WebGPU/lib/Release', { recursive: true });
    
    let debug_lib_list = [];
    let release_lib_list = [];
    if (all_libs) {
        console.log("Copying all lib files...");
        crawlExtensionFiles(debug_lib_list, lib_ext, `${dawn_path}/build-debug`);
        crawlExtensionFiles(release_lib_list, lib_ext, `${dawn_path}/build-release`);
        copyAllFilesTo(debug_lib_list.map(file_obj => file_obj.path), "./external/WebGPU/lib/Debug");
        copyAllFilesTo(release_lib_list.map(file_obj => file_obj.path), "./external/WebGPU/lib/Release");
    } else {
        console.log("Copying required lib files only (run with -all to copy all)...");
        crawlRequiredFiles(debug_lib_list, `${dawn_path}/build-debug`);
        crawlRequiredFiles(release_lib_list, `${dawn_path}/build-release`);
        copyAllFilesTo(debug_lib_list, "./external/WebGPU/lib/Debug");
        copyAllFilesTo(release_lib_list, "./external/WebGPU/lib/Release");
    }

    console.log("Copying header files...");
    const all_header_list = [];
    crawlExtensionFiles(all_header_list, ".h", `${dawn_path}/src`);
    crawlExtensionFiles(all_header_list, ".h", `${dawn_path}/include`);
    crawlExtensionFiles(all_header_list, ".h", `${dawn_path}/build-debug/gen/include`);
    copyAllFilesStructureTo(all_header_list, "./external/WebGPU");
}

const commands = [
    {
        arg: "-help",
        desc: "List commands",
        opt: "help"
    },
    {
        arg: "-all",
        desc: "Copy all libraries instead of just the required ones. Use if still getting missing function linker errors.",
        opt: "all"
    },
    {
        arg: "-update",
        desc: "Update the repo",
        opt: "update"
    },
    {
        arg: "-copy-only",
        desc: "Only copy, do not build",
        opt: "copy"
    },
];

module.exports = () => {

    const args = process.argv.slice(2);

    const arg_commands = {};
    commands.forEach(command => {
        if(args.includes(command.arg)) {
            arg_commands[command.opt] = true;
        }
    });
    
    if (arg_commands.help) {
        console.log("\nCommands available:\n");
        commands.forEach(command => {
            console.log(`\n\t${command.arg}`);
            console.log(`\t${command.desc}`);
        });
        console.log("\n");
        return;
    }

    run(arg_commands.all, arg_commands.update, arg_commands.copy).then(() => { // TODO: make update a command line arg
        console.log("Completed Successfully");
    }).catch(err => {
        console.error("Error!");
        console.error(err);
    });
}