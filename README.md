<div align="center">
  <img src="branding/dili/dili.svg" alt="Dili icon" width="160" />
  <h1 align="center">Dili</h1>
  <h4 align="center">A simple game streaming host for Moonlight, made for Linux.</h4>
</div>

## About

Dili lets you play the games on your Linux PC on your TV, phone or tablet, using any
[Moonlight](https://moonlight-stream.org) client. It is based on
[Sunshine](https://github.com/LizardByte/Sunshine) and adds the features that make streaming feel effortless,
similar to what [Apollo](https://github.com/ClassicOldSong/Apollo) offers on Windows.

The name comes from the Albanian word *dielli*, the sun.

> **Status: early preview.** Dili is tested on Bazzite with KDE Plasma 6.7 and an AMD graphics card.
> Expect rough edges, and please report what you find.

## What Dili adds

- **A virtual screen for every stream.** Each device gets its own screen, created on the fly and sized to match it:
  4K at 60 Hz on a TV, 120 Hz on a phone, or any other resolution and refresh rate the device asks for.
  Your real monitors stay untouched, and the screen disappears when the stream ends.
- **Games open where you play.** While streaming, new windows, games and Steam Big Picture appear on the streamed
  screen. Windows you open at the PC stay on your monitor.
- **Taskbar on the streamed screen** (optional). The streamed screen becomes the main screen during a stream and
  hands it back afterwards.
- **Controller mouse mode.** Hold **Start** for one second to use your controller as a mouse: right stick moves the
  pointer, LB clicks, RB right-clicks, left stick scrolls. Works on every device, including Apple TV.
- **A simple, clean interface** in light and dark mode: Home with live status, Displays, Quality presets and
  Devices with easy 4-digit pairing.
- **First-run setup wizard** that takes care of controller access, network access and screen sharing permission,
  with one password prompt at most.
- **Autostart** with a single switch.

Everything goes through KDE's official screen sharing, so Dili does not need to weaken any security settings.

## Requirements

- KDE Plasma 6.6 or newer on Wayland (virtual screens with custom refresh rates need KWin 6.6+)
- A graphics card with hardware video encoding (tested with AMD through VA-API)
- A Moonlight client on the device you want to stream to

## Building from source

Dili is not packaged yet. On an immutable system like Bazzite, the easiest way is to build it in a
[Distrobox](https://distrobox.it):

```bash
distrobox create --name sunshine-dev --image registry.fedoraproject.org/fedora:43
distrobox enter sunshine-dev
git clone --recurse-submodules https://github.com/Disresbek/dili-stream.git ~/dev/dili
cd ~/dev/dili
git checkout virtual-display
./scripts/linux_build.sh --skip-cuda
```

For AMD hardware encoding inside the box, install the full VA-API driver from RPM Fusion
(`mesa-va-drivers-freeworld`). After building, install it with `sudo cmake --install build`.

## Using Dili

1. Start Dili and open **https://localhost:47990** in your browser.
2. The setup wizard walks you through the rest.
3. Open Moonlight on your device, select your PC, and pair it on the **Devices** page with the 4-digit code.

## Credits and license

Dili is a fork of [Sunshine](https://github.com/LizardByte/Sunshine) by LizardByte and its contributors, whose
work makes all of this possible. The virtual display idea is inspired by
[Apollo](https://github.com/ClassicOldSong/Apollo).

Like Sunshine, Dili is licensed under the [GNU General Public License v3.0](LICENSE).
