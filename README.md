# Discord Profile Tool

`dcpf` is a Windows command-line tool for keeping Discord profile setups in one place. A profile is a JSON file, and its avatar/banner files live in a matching folder under `Assets`.

It does not log into Discord or change anything on a Discord account.

## What it does

- Create, view, edit, move, and delete profiles
- Keep profiles in folders
- Copy avatar and banner files into the library
- Check the 10 MiB avatar and 8 MiB banner limits
- Compress large PNG, WebP, and GIF files when FFmpeg/Gifsicle are available

## Requirements

- Windows 10 or later
- `dcpf.exe`, or a Visual Studio C++ setup with the Desktop development with C++ workload if you want to build it yourself
- FFmpeg and Gifsicle only when an image is too large. The tool will ask before downloading either one.

## Build

Open `DcProfilesTool.slnx` in Visual Studio and build the `x64` Debug or Release configuration.

The executable is produced as:

```text
x64\Release\dcpf.exe
```

## Usage

For the full command list:

```powershell
.\dcpf.exe -Help
```

Create a folder and a profile:

```powershell
.\dcpf.exe -CreateFolder -Path "Games"
.\dcpf.exe -CreateProfile -Path "Games" -Name "Fun Game" `
  -Avatar "C:\Images\avatar.png" `
  -Banner "C:\Images\banner.jpg" `
  -PrimaryColor "#FCEE0A" `
  -SecondaryColor "#00F0FF"
```

Show a profile:

```powershell
.\dcpf.exe -ShowProfile -Path "Games\Fun Game.json"
```

Edit a profile:

```powershell
.\dcpf.exe -EditProfile -Path "Games\Fun Game.json" `
  -ProfileEffect "Sick" `
  -Avatar "C:\Images\new-avatar.gif"
```

List the library:

```powershell
.\dcpf.exe -List
.\dcpf.exe -List -Path "Games" -All
```

Move or delete a profile:

```powershell
.\dcpf.exe -MoveProfile -Path "Games\Fun Game.json" `
  -Destination "Favorites\Fun Game.json"

.\dcpf.exe -DeleteProfile -Path "Favorites\Fun Game.json"
```

Move, edit, and delete commands ask before doing anything.

## Profile library layout

```text
ProfileLibrary/
└── Games/
    └── Fun Game.json

Assets/
└── A1B2C3D4/
    ├── avatar.png
    └── banner.jpg
```

The JSON file stores the asset filenames. If you rename an asset by hand, update the JSON too.

## Supported image formats

`PNG`, `JPG`, `JPEG`, `GIF`, and `WebP` are supported.

Files already under the limit are copied without changes. Large PNG and WebP files can be compressed with FFmpeg. Large GIFs use FFmpeg and Gifsicle. Large JPEGs are recompressed with FFmpeg, starting at high quality and lowering quality only if needed.

## A note on files

- Folder and profile paths stay inside `ProfileLibrary`; `..` and absolute paths are rejected.
- Profiles are saved through a temporary file first, so an interrupted write is less likely to damage a JSON file.
- Back up `ProfileLibrary` and `Assets` before deleting lots of profiles or editing files manually.

## License


