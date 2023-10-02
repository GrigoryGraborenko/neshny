///////////////////////////////////////////////////////////////////////////////
const util = require('util');
const fs = require('fs');
const exec = util.promisify(require('child_process').exec);

const search_str = "DAWN_PATH";
const required_files = [
    "absl_int128.lib",
    "absl_raw_logging_internal.lib",
    "absl_str_format_internal.lib",
    "absl_strerror.lib",
    "absl_strings.lib",
    "absl_strings_internal.lib",
    "absl_throw_delegate.lib",
    "dawn_common.lib",
    "dawn_headers.lib",
    "dawn_native.lib",
    "dawn_platform.lib",
    "dawn_proc.lib",
    "dawn_utils.lib",
    "dawncpp.lib",
    "dawncpp_headers.lib",
    "SPIRV-Tools.lib",
    "SPIRV-Tools-opt.lib",
    "tint.lib",
    "tint_diagnostic_utils.lib",
    "tint_utils_io.lib",
    "tint_val.lib",
    "webgpu_dawn.lib"
];

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

/*
function crawlRequiredFiles(file_list, dir) {
    fs.readdirSync(dir).forEach(function(filename) {
        var filepath = dir + "/" + filename;
        var stats = fs.statSync(filepath);
        if(!stats.isDirectory()) {
            if (required_files.includes(filename)) {
                if (file_list[filename]) {
                    file_list[filename].paths.push(filepath);
                } else {
                    file_list[filename] = { paths: [filepath] };
                }
            }
        } else {
            crawlRequiredFiles(file_list, dir + "/" + filename);
        }
    });
}
*/

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

async function run() {
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
    if (fs.statSync(`${dawn_path}/dawn/.git`, { throwIfNoEntry: false }) === undefined) {
        console.log("Cloning dawn repo...");
        await exec("git clone https://dawn.googlesource.com/dawn", { cwd: dawn_path });
    } else {
        dawn_path += "/dawn";
        console.log("Found existing dawn repo, updating...");
        // await exec("git pull", { cwd: dawn_path });
    }
/*
    console.log("Fetching dependencies...");
    await exec(`python tools/fetch_dawn_dependencies.py --use-test-deps`, { cwd: dawn_path });

    console.log("Running cmake...");
    await exec(`cmake . -B build-release -G "Visual Studio 17 2022" -A x64`, { cwd: dawn_path });
    await exec(`cmake . -B build-debug -G "Visual Studio 17 2022" -A x64`, { cwd: dawn_path });

    console.log("Building debug...");
    await exec(`cmake --build build-debug --config=DEBUG`, { cwd: dawn_path });
    console.log("Building release...");
    await exec(`cmake --build build-release --config=RELEASE`, { cwd: dawn_path });
*/
    console.log("Copying lib files...");

    fs.mkdirSync('./external/WebGPU/lib/Debug', { recursive: true });
    fs.mkdirSync('./external/WebGPU/lib/Release', { recursive: true });
    
    const all_header_list = [];
    crawlExtensionFiles(all_header_list, ".h", `${dawn_path}/src`);
    crawlExtensionFiles(all_header_list, ".h", `${dawn_path}/include`);
    crawlExtensionFiles(all_header_list, ".h", `${dawn_path}/build-debug/gen/include`);

    let debug_lib_list = [];
    let release_lib_list = [];
    crawlExtensionFiles(debug_lib_list, ".lib", `${dawn_path}/build-debug`);
    crawlExtensionFiles(release_lib_list, ".lib", `${dawn_path}/build-release`);

    copyAllFilesStructureTo(all_header_list, "./external/WebGPU");

    copyAllFilesTo(debug_lib_list.map(file_obj => file_obj.path), "./external/WebGPU/lib/Debug");
    copyAllFilesTo(release_lib_list.map(file_obj => file_obj.path), "./external/WebGPU/lib/Release");

    /*
    const copy_list = [];
    required_files.forEach(filename => {
        if(!file_list[filename]) {
            console.error("ERROR - CANNOT FIND " + filename);
            return;
        }
        const paths = file_list[filename].paths;
        if(paths.length != 1) {
            console.error("ERROR - TOO MANY PATHS FOR " + filename);
            return;
        }
        copy_list.push(paths[0]);
    });
    copyAllFilesTo(copy_list, "./external/WebGPU/lib/Debug");
    */
}

run().then(() => {
    console.log("Completed Successfully");
}).catch(err => {
    console.error("Error!");
    console.error(err);
});