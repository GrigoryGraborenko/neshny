//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreCompiledHeader.h"

#include "Engine.h"

#include "EmbeddedFiles.cpp"

int main(int argc, char* argv[]) {

	QCoreApplication::addLibraryPath("plugins");
	QApplication a(argc, argv);

	Engine engine;

	QOpenGLWindow window;
	window.show();
	
	window.makeCurrent();

	bool err = gladLoadGL() == 0;
	if (err) {
		fprintf(stderr, "Failed to initialize OpenGL loader!\n");
		return 1;
	}

	window.setWindowState(Qt::WindowMaximized);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	QtImGui::initialize(&window, &engine);
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	window.doneCurrent();
	

	Neshny::Singleton().SetEmbeddableFileLoader([](QString path, QString& err_msg) -> QByteArray {

#ifdef _DEBUG
		QFile file;
		for (auto prefix : g_ShaderBaseDirs) {
			file.setFileName(QString("%1\\%2").arg(prefix.c_str()).arg(path));
			if (file.open(QIODevice::ReadOnly)) {
				break;
			}
		}
		if (!file.isOpen()) {
			err_msg = "File error - " + file.errorString(); // TODO: better error handling than just last file error
			return QByteArray();
		}
		return file.readAll();
#endif

		auto byte_path = path.toLocal8Bit();
		auto found = g_EmbeddedFiles.find(std::string(byte_path.data()));
		if (found == g_EmbeddedFiles.end()) {
			err_msg = "Cannot find embedded file error - " + path;
			return QByteArray();
		}
		return QByteArray((char*)found->second.p_Data, found->second.p_Size);
	});

	bool result = Neshny::Singleton().QTLoop(&window, &engine);

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui::DestroyContext();

	return result ? 0 : -1;
}

#include "Engine.cpp"