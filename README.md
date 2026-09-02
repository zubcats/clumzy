# Clumzy

Clumzy is Zub's fork of [clumsy](https://jagt.github.io/clumsy/). Same job: make your Windows network worse on purpose — lag, drop, throttle, duplicate — so you can test apps on a bad connection.

Hotkeys, presets, hotspot/local toggle, disconnect, and a bandwidth cap with KB/s–MB/s switch are included.

* No installation.
* No proxy setup or changes in the app you're testing.
* System-wide capture, including localhost.
* Start and stop anytime while other programs keep running.

Original clumsy by Chen Tao. This tree keeps that MIT license.

## Download

GitHub Actions builds a portable zip on every push to `main`:

https://github.com/zubcats/clumzy/releases/latest/download/Clumzy-windows-x64.zip

Unzip and run `Clumzy.exe` as administrator. Leave `clumzy_engine.dll`, the WinDivert files, and `presets.ini` in the same folder.

See [jagt.github.io/clumsy](http://jagt.github.io/clumsy) for background and local build notes.

## License

MIT
