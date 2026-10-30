; Groove Grabber - Windows installer (Inno Setup 6)
;
;   iscc /DAppVersion=1.0.0 /DArtefacts="C:\path\to\build\GrooveGrabber_artefacts\Release" packaging\windows\GrooveGrabber.iss
;
; Installs:
;   VST3        -> C:\Program Files\Common Files\VST3\Groove Grabber.vst3
;   Standalone  -> C:\Program Files\Groove Grabber\Groove Grabber.exe  (+ Start menu / desktop shortcut)

#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef Artefacts
  #define Artefacts "..\..\build\GrooveGrabber_artefacts\Release"
#endif

[Setup]
AppId={{3F9C2A71-8B4D-4E6A-9C15-7D2E8B1A4F63}
AppName=Groove Grabber
AppVersion={#AppVersion}
AppPublisher=Grabber Audio
DefaultDirName={autopf}\Groove Grabber
DefaultGroupName=Groove Grabber
DisableProgramGroupPage=yes
OutputBaseFilename=Groove-Grabber-{#AppVersion}-Windows-Setup
SetupIconFile=icon.ico
UninstallDisplayIcon={app}\Groove Grabber.exe
ChangesAssociations=yes
Compression=lzma2/max
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern

[Types]
Name: "full";   Description: "Plug-in + standalone app"
Name: "custom"; Description: "Choose components"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plug-in (Cubase, Ableton Live, FL Studio, Studio One, Reaper, Bitwig ...)"; Types: full custom
Name: "app";  Description: "Standalone app"; Types: full custom

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Components: app

[Files]
Source: "{#Artefacts}\VST3\Groove Grabber.vst3\*"; DestDir: "{commoncf64}\VST3\Groove Grabber.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Artefacts}\Standalone\Groove Grabber.exe"; DestDir: "{app}"; Components: app; Flags: ignoreversion

[Icons]
Name: "{group}\Groove Grabber"; Filename: "{app}\Groove Grabber.exe"; Components: app
Name: "{autodesktop}\Groove Grabber"; Filename: "{app}\Groove Grabber.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\Groove Grabber.exe"; Description: "Launch Groove Grabber"; Flags: nowait postinstall skipifsilent; Components: app

[Messages]
FinishedLabel=Groove Grabber is installed.%n%nRescan plug-ins in your DAW and put Groove Grabber on an instrument track. The first time, open the LIBRARY page and add the folders where your drum kits and one shots live.
