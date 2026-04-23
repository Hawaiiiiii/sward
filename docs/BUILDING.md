# Building the Local Asset-Backed Workspace

This repository keeps the **publishable** R&D layer in git, while the actual private game inputs and generated translation outputs remain local-only.

> [!IMPORTANT]
> A full runtime build requires locally owned game files under `UnleashedRecompLib/private/`. Those files must never be committed.

## 1. Clone the Repository

Clone this repository with submodules:

```bash
git clone --recurse-submodules https://github.com/Hawaiiiiii/sward.git
```

### Windows

If you skipped `--recurse-submodules`, run:

```powershell
.\update_submodules.bat
```

## 2. Add the Required Private Files

Place the following files inside `./UnleashedRecompLib/private/`:

- `default.xex`
- `default.xexp`
- `shader.ar`

These files are sourced from your own legally acquired copy and local install workflow.

> [!NOTE]
> This repository does not ship those files, extracted assets, or generated PPC/shader outputs. The full research workspace expects them to exist locally only.

## 3. Install Dependencies

### Windows

Install [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/) with:

- Desktop development with C++
- C++ Clang Compiler for Windows
- C++ CMake tools for Windows

### Linux

For Debian-based distros:

```bash
sudo apt install autoconf automake libtool pkg-config curl cmake ninja-build clang clang-tools libgtk-3-dev
```

For Arch-based distros:

```bash
sudo pacman -S base-devel ninja lld clang gtk3
```

> [!TIP]
> If you are iterating on reverse-engineering or UI/UX work, `RelWithDebInfo` is usually the best balance between usable debugging and acceptable compile times.

## 4. Configure and Build

### Windows

1. Open the repository in Visual Studio and let CMake configure.
2. Choose one of:
   - `x64-Clang-Debug`
   - `x64-Clang-RelWithDebInfo`
   - `x64-Clang-Release`
3. Switch to **CMake Targets View**.
4. Set `UnleashedRecomp` as the startup target.
5. Set `currentDir` in the generated launch configuration to the root of your local game directory.
6. Start the target. The first full asset-backed build may trigger local generation of translated code and shader outputs.

### Linux

Configure:

```bash
cmake . --preset linux-relwithdebinfo
```

Build:

```bash
cmake --build ./out/build/linux-relwithdebinfo --target UnleashedRecomp
```

Run:

```bash
./out/build/linux-relwithdebinfo/UnleashedRecomp/UnleashedRecomp
```

## 5. CI Expectations

> [!NOTE]
> GitHub Actions in this repository can run in two modes:
>
> - Full build mode when private asset secrets are configured
> - Publishable-scope validation mode when they are not

That split is intentional. The repo should stay green for tooling/docs changes without exposing proprietary material.
