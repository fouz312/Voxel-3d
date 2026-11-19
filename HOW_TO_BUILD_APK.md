# How to make the Android APK (full details)

GitHub builds the APK for you in the cloud. You do not need Android Studio or a computer with special software.
It takes about 10-20 minutes the first time. Everything is click-and-wait.

## Before you start
- A GitHub account and your repository with the Voxel3D files uploaded (see `HOW_TO_LAUNCH.md`).
- An Android phone or tablet with Android 7.0 or newer (OpenGL ES 2 - basically any phone from the last 8 years).
- The repository must contain `.github/workflows/build-android.yml` and `scripts/build_android.sh`.

## Step 1 - check the repository
1. Open your repository on github.com.
2. Make sure you can see these at the top level: `main.cpp`, `game.h`, `art.h`, `CMakeLists.txt`, and the folders `.github`, `scripts`, `tests`.
3. Click `.github` -> `workflows`. You should see `build-android.yml`.
   If `.github` is missing, the upload skipped hidden folders - upload again using Codespaces (see `HOW_TO_LAUNCH.md`, Option B).
4. Your default branch must be `main` (Settings -> Branches).
5. Make sure Actions are allowed: Settings -> Actions -> General -> choose **Allow all actions and reusable workflows** -> Save.

## Step 2 - start the build
1. Click the **Actions** tab at the top of the repository.
2. If you see a green button "I understand my workflows, go ahead and enable them", click it.
3. In the left sidebar click **Build Voxel3D Android APK**.
4. On the right, click **Run workflow** (a grey dropdown button), keep Branch: `main`, then click the green **Run workflow** button.
5. After a few seconds a new line appears with a yellow spinning circle. Click it to watch progress.
   The steps it runs, in order: logic tests -> Java -> Android SDK -> NDK/CMake -> Gradle -> **Build the APK** (the long one, 8-15 min).

## Step 3 - download the APK
1. When the circle turns into a green check, open that run (click its name).
2. Scroll to the bottom to the **Artifacts** box. Click **Voxel3D-debug-apk**. A zip downloads.
3. Unzip it. Inside is `app-debug.apk`.
   - On a phone: GitHub's mobile page sometimes hides Artifacts. In Chrome tap the three dots -> **Desktop site**, then scroll down.
   - You can also download on a computer and send the file to your phone (USB cable, Google Drive, email to yourself).
   - Phone tip: if the zip cannot be opened, use the Files app (long-press the zip -> Extract).

## Step 4 - install it on your phone
1. Tap `app-debug.apk` in your Files/Downloads app.
2. Android will say it cannot install apps from this source. Tap **Settings**, switch on **Allow from this source**, press back.
3. Tap **Install**.
4. If Google Play Protect says "App blocked / unsafe": tap **More details** -> **Install anyway**.
   (This is normal. The APK is signed with a debug key because you built it yourself; it is not on the Play Store.)
5. Tap **Open**. The first start can show a black screen for a few seconds while the world is generated.

## Step 5 - playing on the phone
- Turn the phone sideways (landscape). Create a world from the title screen (name/seed are automatic on touch).
- Left thumb = move stick (push to the edge to sprint); drag the right side to look.
- Buttons: MINE (hold), PLACE (tap, hold to eat/block), JUMP, SNEAK, FLY (Creative), BAG (inventory/crafting), VIEW (camera), II (pause).
- Settings -> Render Distance **Low** and Vibrant Visuals **OFF** if it runs slowly.
- Worlds are saved in the app's private storage and survive closing the app. Uninstalling the app deletes them.

## If something goes wrong
**The workflow does not appear in the Actions tab**
- `.github/workflows/build-android.yml` is missing or the default branch is not `main`. See Step 1.

**A red X appears**
1. Open the failed run and click the step with the red X (usually **Build the APK**).
2. Click it to expand and scroll to the very end. Copy the last ~60 lines.
3. Also download the artifact **Voxel3D-android-build-logs** at the bottom of the run (only exists after a failure).
4. Send me the lines. Android build errors are usually fixed in one round.
   Most likely suspects: raylib's Android build options, a missing library name, or `std::filesystem` on the NDK.

| Message in the log | Meaning | Fix |
|---|---|---|
| `Could not find native_app_glue` / `android_native_app_glue.h not found` | NDK path not found | send me the log |
| `undefined symbol: ...` / `multiple definition` | link problem | send me the log |
| `License for package ... not accepted` | SDK license | re-run the workflow; send log if it repeats |
| `Execution failed for task ':app:...'` | Gradle task failed | send the 60 lines above it |
| `No space left on device` | runner full | re-run |

**"App not installed"** - an older copy signed with another key exists: uninstall it first, then install again. Also check you have free storage.

**The app opens and closes immediately** - tell me your phone model and Android version. If you know `adb`, run `adb logcat | grep -i -E "raylib|voxel|AndroidRuntime"` and send the output.

**Slow / hot phone** - Low render distance, Vibrant OFF, close other apps.

## Honest status
The game logic is tested on a computer. The Android packaging has been written carefully (and several bugs from the original template were fixed), but
I cannot run Android builds myself, so the **first build may need one fix round**. Send me the log and I will correct it.

## Want your own signed release (Play Store)?
A debug APK is fine for personal use. A Play Store release needs a signing key and an `.aab` bundle - tell me and I will add a signed release workflow.
