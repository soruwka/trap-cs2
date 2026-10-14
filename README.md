# Nocturne

Nocturne is an experimental Windows x64 C++17 project using Direct3D 11,
Dear ImGui, FreeType, and MinHook. It was previously named trap-cs2.

The existing code integrates with CS2 and contains player-visualization features.
It is intended for educational use on community servers without VAC enabled;
it makes no claim of being undetected or compatible with current game versions.

## Build prerequisites

- Windows 10 or later (x64).
- Visual Studio 2022 with **Desktop development with C++**, MSVC v143, and a
  Windows 10/11 SDK.
- The legacy **DirectX SDK (June 2010)** for D3DX11. Set `DXSDK_DIR` to its
  installation directory, including a trailing backslash, and restart Visual
  Studio after changing the environment variable.

Open `Nocturne.sln`, choose **Debug | x64** or **Release | x64**, and build.
Alternatively, use a Visual Studio Developer Command Prompt:

```bat
msbuild Nocturne.sln /m /p:Configuration=Debug /p:Platform=x64
msbuild Nocturne.sln /m /p:Configuration=Release /p:Platform=x64
```

FreeType is built from the vendored sources automatically. No downloaded or
manually copied `freetype.lib` is required. Outputs are written to
`x64\Debug\Nocturne.dll` and `x64\Release\Nocturne.dll` respectively.
32-bit configurations are not supported.

## Checks

Portable C++17 unit tests cover vector arithmetic, keybinding dispatch, and
feature ownership without loading the DLL or running a game:

```sh
python3 -m unittest discover -s tests -v
```

The test runner needs Python 3 and a GCC/Clang-compatible C++17 compiler (`CXX`,
or `c++` by default).
It also validates the Visual Studio manifests and branding. These checks do not
replace a native Windows build or runtime tests.

## Known limitations

- Module signatures, interface versions, schema layouts, and hard-coded engine
  offsets are version-dependent and have not been validated against a current
  CS2 installation.
- Startup and shutdown cross loader, render, and game callback threads. Safe
  callback quiescence, graphics-context ownership, and full DLL unloading need a
  separate Windows lifecycle review before production use.
- Constructors install callbacks and consult global objects before the owning
  `unique_ptr` assignment completes, creating initialization-order and early
  callback risks.
- Pattern parsing treats zero bytes as wildcards and pads signatures with extra
  wildcards; missing matches can flow into unchecked pointer dereferences.
- The entity cache exposes shared mutable state without consistent locking;
  stale pointers and concurrent updates remain runtime risks.
- The UI includes placeholder tabs; they do not represent implemented features.

Do not treat this maintenance pass or a successful build as a guarantee that all
runtime issues are fixed. Original and third-party license notices are retained.
