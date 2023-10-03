const http = require('http');
const fs = require('fs');

const service_dir = "./build-web";
const port = 8000;
const refresh_milliseconds = 5000;

// const max_age = 1;
const max_age = 10;
// const max_age = 2419200;

function crawlFiles(file_list, dir, prefix) {
    fs.readdirSync(dir).forEach(function(filename) {
        var filepath = dir + "/" + filename;
        var ext = filename.substr(filename.lastIndexOf("."));
        var stats = fs.statSync(filepath);
        if(!stats.isDirectory()) {

            var file = fs.readFileSync(filepath);
            var file_obj = { filename: filename, filepath: filepath, extension: ext, cache_control: `public, max-age=${max_age}` };
            if(ext === ".html") {
                file_obj.content_type = "text/html";
                file_obj.file = file + "";
            } else if(ext === ".css") {
                file_obj.content_type = "text/css";
                file_obj.file = file + "";
            } else if(ext === ".jpg") {
                file_obj.content_type = "image/jpg";
                file_obj.file = file;
            } else if(ext === ".png") {
                file_obj.content_type = "image/png";
                file_obj.file = file;
            } else if(ext === ".woff") {
                file_obj.content_type = "font/woff";
                file_obj.file = file;
            } else if(ext === ".woff2") {
                file_obj.content_type = "font/woff2";
                file_obj.file = file;
            } else if(ext === ".js") {
                file_obj.content_type = "application/javascript";
                file_obj.encoding = "utf-8";
                file_obj.file = file + "";
            } else if(ext === ".data") {
                file_obj.content_type = "application/octet-stream";
                file_obj.encoding = "binary";
                file_obj.file = file;
            } else if(ext === ".wasm") {
                file_obj.content_type = "application/wasm";
                file_obj.encoding = "binary";
                file_obj.file = file;
            } else {
                file_obj.content_type = "text/script";
                file_obj.file = file + "";
            }
            if(filename === "index.html") {
                file_list[prefix] = file_obj;
                if(prefix !== "/") {
                    file_list[prefix.slice(0, prefix.length - 1)] = { redirect: prefix };
                }
            }
            file_list[prefix + filename] = file_obj;
        } else {
            crawlFiles(file_list, dir + "/" + filename, prefix + filename + "/");
        }
    });
}

const prefix = "/";

let static_data = { files: {} };
crawlFiles(static_data.files, service_dir, prefix);
// console.log("Initial files", static_data);
// TODO: make this interval AFTER crawlFiles completes to avoid stacking events
setInterval(() => {
    const new_static_files = {};
	try {
		crawlFiles(new_static_files, service_dir, prefix);
	} catch(err) {
		console.log("Could not crawl files");
	}
    static_data.files = new_static_files;
}, refresh_milliseconds);

module.exports = () => {
    const server = http.createServer({}, function (request, response) {

        var url = request.url;
        if(url.indexOf("?") !== -1) {
            url = url.slice(0, url.indexOf("?"));
        }

        if(request.method === "GET") {
            const file = static_data.files[url];
            if(file !== undefined) {

                if(file.redirect) {
                    response.writeHead(302, {
                        "Location": file.redirect,
                        "Cache-Control": `public, max-age=${max_age}`
                    });
                    response.end();
                    return;
                }

                const headers = {
                    "Content-Type": file.content_type,
                    "Cache-Control": file.cache_control,
                    "Cross-Origin-Embedder-Policy": "require-corp",
                    "Cross-Origin-Opener-Policy": "same-origin"
                };
                let encoding = "binary";
                if(file.encoding) {
                    encoding = file.encoding;
                    headers["Content-Type"] += "; charset=" + file.encoding;
                }
                response.writeHead(200, headers);
                response.end(file.file, encoding);
                return;
            }
        }
        response.writeHead(404);
        response.end("404 Not Found\n");

    }).listen(port);

    console.log(`Listening on port ${port}`);
}