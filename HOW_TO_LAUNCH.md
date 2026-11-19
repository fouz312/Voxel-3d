# How to launch Voxel3D from GitHub (step by step)

This is a native C++ game. GitHub does not "run" it in the browser. Instead, GitHub **builds** it for you
(GitHub Actions) and gives you a file to download: a Windows `.exe`, a Linux program, or an Android `.apk`.

## 1. Put the files in your repository

You need these files at the **top level** of the repository (not inside an extra folder):
`main.cpp`, `game.h`, `art.h`, `CMakeLists.txt`, and the folders `.github/`, `.devcontainer/`, `scripts/`, `tests/`, `legacy/`.

### Option A - from a computer (web browser)
1. Unzip `Voxel3D_v4.zip`. Open the `proj` folder inside it.
2. Open your repository page on github.com.
3. Click **Add file -> Upload files**.
4. Drag **everything inside `proj`** into the page (select all files AND folders; do not drag the `proj` folder itself).
   - Mac: press `Cmd+Shift+.` in Finder to show the `.github` and `.devcontainer` folders so you can select them.
5. Type a message such as `Voxel3D v3`, keep **Commit directly to the main branch**, click **Commit changes**.
6. Check that the repo page now shows `main.cpp`, `game.h`, `art.h`, `.github` ...

### Option B - from a phone or any device (Codespaces, no installs)
1. On the repository page tap **Code -> Codespaces -> Create codespace on main**. Wait for the editor to open.
2. Upload the zip: in the left **Explorer** panel, right-click an empty area -> **Upload...** -> choose `Voxel3D_v4.zip`.
3. Open the terminal (menu -> Terminal -> New Terminal) and run:
```bash
unzip -o Voxel3D_v4.zip
cp -r proj/. .
rm -rf proj Voxel3D_v4.zip
git add -A
git commit -m "Voxel3D v3"
git push
```

> The workflows only appear if your default branch is called `main` (check **Settings -> Branches**).

## 2. Build it (GitHub does this for you)

1. Open the **Actions** tab of your repository. If GitHub asks, click **I understand my workflows, enable them**.
2. In the left list pick a workflow, then **Run workflow -> Run workflow** (green button):
   - **Build Voxel3D (Windows)** -> gives `Voxel3D-windows-x64.zip`
   - **Build Voxel3D (Linux)** -> gives `Voxel3D-linux-x64.tar.gz` (also runs all game-logic tests first)
   - **Build Voxel3D Android APK** -> gives `app-debug.apk`
3. Wait a few minutes (first run is slowest). A green check means success.
4. Click the finished run, scroll to **Artifacts** at the bottom, click the name to download.

## 3. Play it

- **Windows:** unzip, double-click `Voxel3D.exe`. If Windows SmartScreen warns, click **More info -> Run anyway** (the file is unsigned because you built it yourself).
  Saves are created in a `saves` folder next to the exe.
- **Linux:** `tar xzf Voxel3D-linux-x64.tar.gz && ./Voxel3D` (needs a desktop with OpenGL 3.3).
- **Android:** (full guide: `HOW_TO_BUILD_APK.md`) unzip the artifact, send `app-debug.apk` to your phone (Drive, USB, etc.), open it, allow **Install unknown apps** when asked,
  install, and launch. Landscape works best. Saves live in the app's private storage.
- **In Codespaces:** you can build and run tests there (`cmake -S . -B build && cmake --build build -j2`, then `g++ -std=c++17 -I. tests/test_logic.cpp -o t && ./t`).
  To *see* the game there, open the **Ports** tab, open port **6080** (password `vscode`), then in that desktop's terminal run `./build/Voxel3D`. It is software-rendered, so expect it to be slow.

## 4. If a build fails (red X)

1. Open the failed run, click the red step (for example **Build**).
2. Copy the last ~40 lines of the log.
3. Send them to me; compiler errors usually take one round to fix.

## 5. Updating later
Upload/replace changed files the same way (Option A or B) and commit. Every push to `main` rebuilds automatically.
