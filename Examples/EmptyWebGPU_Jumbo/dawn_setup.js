///////////////////////////////////////////////////////////////////////////////
const util = require('util');
const fs = require('fs');
const exec = util.promisify(require('child_process').exec);

const search_str = "DAWN_PATH";
const lib_files = [
    ""
];

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
        await exec("git pull", { cwd: dawn_path });
    }

    console.log("Fetching dependencies...");
    await exec(`python tools/fetch_dawn_dependencies.py --use-test-deps`, { cwd: dawn_path });

    console.log("Running cmake...");
    await exec(`cmake . -B build-release -G "Visual Studio 16 2019" -A x64`, { cwd: dawn_path });

    console.log("Building...");
    await exec(`cmake --build build-release --config=RELEASE`, { cwd: dawn_path });

    console.log("Copying lib files...");
}

run().then(() => {
    console.log("Completed Successfully");
}).catch(err => {
    console.error("Error!");
    console.error(err);
});