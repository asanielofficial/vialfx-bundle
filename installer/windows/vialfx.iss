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
Name: "full"; Description: "Full installation (all plugins, VST3 and CLAP)"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plugins (C:\Program Files\Common Files\VST3)"; Types: full custom
Name: "clap"; Description: "CLAP plugins (C:\Program Files\Common Files\CLAP)"; Types: full custom
Name: "vst3\chorus"; Description: "Vial Chorus"; Types: full custom
Name: "vst3\compressor"; Description: "Vial Compressor"; Types: full custom
Name: "vst3\delay"; Description: "Vial Delay"; Types: full custom
Name: "vst3\distortion"; Description: "Vial Distortion"; Types: full custom
Name: "vst3\eq"; Description: "Vial EQ"; Types: full custom
Name: "vst3\filter"; Description: "Vial Filter"; Types: full custom
Name: "vst3\flanger"; Description: "Vial Flanger"; Types: full custom
Name: "vst3\phaser"; Description: "Vial Phaser"; Types: full custom
Name: "vst3\reverb"; Description: "Vial Reverb"; Types: full custom
Name: "clap\chorus"; Description: "Vial Chorus"; Types: full custom
Name: "clap\compressor"; Description: "Vial Compressor"; Types: full custom
Name: "clap\delay"; Description: "Vial Delay"; Types: full custom
Name: "clap\distortion"; Description: "Vial Distortion"; Types: full custom
Name: "clap\eq"; Description: "Vial EQ"; Types: full custom
Name: "clap\filter"; Description: "Vial Filter"; Types: full custom
Name: "clap\flanger"; Description: "Vial Flanger"; Types: full custom
Name: "clap\phaser"; Description: "Vial Phaser"; Types: full custom
Name: "clap\reverb"; Description: "Vial Reverb"; Types: full custom

[Files]
Source: "{#SourceDir}\LICENSE"; DestDir: "{commonappdata}\VialFX"; Flags: ignoreversion
Source: "{#SourceDir}\VST3\Vial Chorus.vst3\*"; DestDir: "{commoncf64}\VST3\Vial Chorus.vst3"; Components: vst3\chorus; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\VST3\Vial Compressor.vst3\*"; DestDir: "{commoncf64}\VST3\Vial Compressor.vst3"; Components: vst3\compressor; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\VST3\Vial Delay.vst3\*"; DestDir: "{commoncf64}\VST3\Vial Delay.vst3"; Components: vst3\delay; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\VST3\Vial Distortion.vst3\*"; DestDir: "{commoncf64}\VST3\Vial Distortion.vst3"; Components: vst3\distortion; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\VST3\Vial EQ.vst3\*"; DestDir: "{commoncf64}\VST3\Vial EQ.vst3"; Components: vst3\eq; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\VST3\Vial Filter.vst3\*"; DestDir: "{commoncf64}\VST3\Vial Filter.vst3"; Components: vst3\filter; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\VST3\Vial Flanger.vst3\*"; DestDir: "{commoncf64}\VST3\Vial Flanger.vst3"; Components: vst3\flanger; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\VST3\Vial Phaser.vst3\*"; DestDir: "{commoncf64}\VST3\Vial Phaser.vst3"; Components: vst3\phaser; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\VST3\Vial Reverb.vst3\*"; DestDir: "{commoncf64}\VST3\Vial Reverb.vst3"; Components: vst3\reverb; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\CLAP\Vial Chorus.clap"; DestDir: "{commoncf64}\CLAP"; Components: clap\chorus; Flags: ignoreversion
Source: "{#SourceDir}\CLAP\Vial Compressor.clap"; DestDir: "{commoncf64}\CLAP"; Components: clap\compressor; Flags: ignoreversion
Source: "{#SourceDir}\CLAP\Vial Delay.clap"; DestDir: "{commoncf64}\CLAP"; Components: clap\delay; Flags: ignoreversion
Source: "{#SourceDir}\CLAP\Vial Distortion.clap"; DestDir: "{commoncf64}\CLAP"; Components: clap\distortion; Flags: ignoreversion
Source: "{#SourceDir}\CLAP\Vial EQ.clap"; DestDir: "{commoncf64}\CLAP"; Components: clap\eq; Flags: ignoreversion
Source: "{#SourceDir}\CLAP\Vial Filter.clap"; DestDir: "{commoncf64}\CLAP"; Components: clap\filter; Flags: ignoreversion
Source: "{#SourceDir}\CLAP\Vial Flanger.clap"; DestDir: "{commoncf64}\CLAP"; Components: clap\flanger; Flags: ignoreversion
Source: "{#SourceDir}\CLAP\Vial Phaser.clap"; DestDir: "{commoncf64}\CLAP"; Components: clap\phaser; Flags: ignoreversion
Source: "{#SourceDir}\CLAP\Vial Reverb.clap"; DestDir: "{commoncf64}\CLAP"; Components: clap\reverb; Flags: ignoreversion

[Messages]
FinishedLabel=VialFX-Bundle is installed. Rescan plugins in your DAW to see the Vial effects.
