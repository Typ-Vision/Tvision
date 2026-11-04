extern "C" __declspec(dllimport) int CameraRuntimeDependency();
extern "C" __declspec(dllexport) int CameraRuntimeFixture(){return CameraRuntimeDependency();}
