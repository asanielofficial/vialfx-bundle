; VialFX-Bundle Windows installer (Inno Setup 6).
; Build:  iscc /DSourceDir=<dir containing VST3\ and CLAP\> /DOutputDir=<dir> installer\windows\vialfx.iss
#ifndef SourceDir
  #define SourceDir "..\..\VialFX-Bundle"
#endif
#ifndef OutputDir
  #define OutputDir "."
#endif
#define AppVersion "0.1.0"

[Setup]
AppId={{6B0E7C52-2F43-4A57-9D5C-1A7F4E5B9C31}
AppName=VialFX-Bundle
AppVersion={#AppVersion}
AppPublisher=VialFX
AppPublisherURL=https://github.com/asanielofficial/vialfx-bundle
DefaultDirName={commoncf64}\VialFX
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
LicenseFile={#SourceDir}\LICENSE
OutputDir={#OutputDir}
OutputBaseFilename=VialFX-Bundle-Setup-windows
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallFilesDir={commonappdata}\VialFX\Uninstall
UninstallDisplayName=VialFX-Bundle (Vital-based effects)

[Types]
Name: "full"; Description: "Full installation (all nine plugins)"
Name: "custom"; Description: "Custom installation (choose plugins)"; Flags: iscustom

[Components]
Name: "chorus"; Description: "Vial Chorus"; Types: full custom
Name: "compressor"; Description: "Vial Compressor"; Types: full custom
Name: "delay"; Description: "Vial Delay"; Types: full custom
Name: "distortion"; Description: "Vial Distortion"; Types: full custom
Name: "eq"; Description: "Vial EQ"; Types: full custom
Name: "filter"; Description: "Vial Filter"; Types: full custom
Name: "flanger"; Description: "Vial Flanger"; Types: full custom
Name: "phaser"; Description: "Vial Phaser"; Types: full custom
Name: "reverb"; Description: "Vial Reverb"; Types: full custom

[Tasks]
Name: "vst3"; Description: "Install VST3 versions (C:\Program Files\Common Files\VST3)"; GroupDescription: "Plugin formats:"
Name: "clap"; Description: "Install CLAP versions (C:\Program Files\Common Files\CLAP)"; GroupDescription: "Plugin formats:"

[Files]
Source: "{#SourceDir}\LICENSE"; DestDir: "{commonappdata}\VialFX"; Flags: ignoreversion
Source: "{#SourceDir}\VST3\Vial Chorus.vst3\*"; DestDir: "{commoncf64}\VST3\Vial Chorus.vst3"; Components: chorus; Tasks: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\VST3\Vial Compressor.vst3\*"; DestDir: "{commoncf64}\VST3\Vial Compressor.vst3"; Components: compressor; Tasks: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\VST3\Vial Delay.vst3\*"; DestDir: "{commoncf64}\VST3\Vial Delay.vst3"; Components: delay; Tasks: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\VST3\Vial Distortion.vst3\*"; DestDir: "{commoncf64}\VST3\Vial Distortion.vst3"; Components: distortion; Tasks: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\VST3\Vial EQ.vst3\*"; DestDir: "{commoncf64}\VST3\Vial EQ.vst3"; Components: eq; Tasks: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\VST3\Vial Filter.vst3\*"; DestDir: "{commoncf64}\VST3\Vial Filter.vst3"; Components: filter; Tasks: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\VST3\Vial Flanger.vst3\*"; DestDir: "{commoncf64}\VST3\Vial Flanger.vst3"; Components: flanger; Tasks: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\VST3\Vial Phaser.vst3\*"; DestDir: "{commoncf64}\VST3\Vial Phaser.vst3"; Components: phaser; Tasks: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\VST3\Vial Reverb.vst3\*"; DestDir: "{commoncf64}\VST3\Vial Reverb.vst3"; Components: reverb; Tasks: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\CLAP\Vial Chorus.clap"; DestDir: "{commoncf64}\CLAP"; Components: chorus; Tasks: clap; Flags: ignoreversion
Source: "{#SourceDir}\CLAP\Vial Compressor.clap"; DestDir: "{commoncf64}\CLAP"; Components: compressor; Tasks: clap; Flags: ignoreversion
Source: "{#SourceDir}\CLAP\Vial Delay.clap"; DestDir: "{commoncf64}\CLAP"; Components: delay; Tasks: clap; Flags: ignoreversion
Source: "{#SourceDir}\CLAP\Vial Distortion.clap"; DestDir: "{commoncf64}\CLAP"; Components: distortion; Tasks: clap; Flags: ignoreversion
Source: "{#SourceDir}\CLAP\Vial EQ.clap"; DestDir: "{commoncf64}\CLAP"; Components: eq; Tasks: clap; Flags: ignoreversion
Source: "{#SourceDir}\CLAP\Vial Filter.clap"; DestDir: "{commoncf64}\CLAP"; Components: filter; Tasks: clap; Flags: ignoreversion
Source: "{#SourceDir}\CLAP\Vial Flanger.clap"; DestDir: "{commoncf64}\CLAP"; Components: flanger; Tasks: clap; Flags: ignoreversion
Source: "{#SourceDir}\CLAP\Vial Phaser.clap"; DestDir: "{commoncf64}\CLAP"; Components: phaser; Tasks: clap; Flags: ignoreversion
Source: "{#SourceDir}\CLAP\Vial Reverb.clap"; DestDir: "{commoncf64}\CLAP"; Components: reverb; Tasks: clap; Flags: ignoreversion

[Messages]
FinishedLabel=VialFX-Bundle is installed. Rescan plugins in your DAW to see the Vial effects.
