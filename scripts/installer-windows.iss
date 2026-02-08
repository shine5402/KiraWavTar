; Inno Setup script for KiraWavTar
; Compile with: iscc /DAppVersion=1.5.0 scripts\installer-windows.iss

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif

[Setup]
AppName=KiraWavTar
AppVersion={#AppVersion}
AppPublisher=shine5402
AppPublisherURL=https://github.com/shine5402/KiraWavTar
DefaultDirName={autopf}\KiraWavTar
DefaultGroupName=KiraWavTar
UninstallDisplayIcon={app}\bin\KiraWAVTar.exe
OutputDir=..\build
OutputBaseFilename=KiraWavTar-{#AppVersion}-setup
SetupIconFile=..\assets\icon.ico
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern

[Files]
Source: "..\build\artifact\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\build\artifact\bin\vc_redist.x64.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall; Check: VCRedistExists

[Icons]
Name: "{group}\KiraWavTar"; Filename: "{app}\bin\KiraWAVTar.exe"
Name: "{autodesktop}\KiraWavTar"; Filename: "{app}\bin\KiraWAVTar.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"

[Run]
Filename: "{tmp}\vc_redist.x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "Installing Visual C++ Redistributable..."; Flags: waituntilterminated; Check: VCRedistExists
Filename: "{app}\bin\KiraWAVTar.exe"; Description: "Launch KiraWavTar"; Flags: nowait postinstall skipifsilent

[Code]
function VCRedistExists: Boolean;
begin
  Result := FileExists(ExpandConstant('{src}\..\build\artifact\bin\vc_redist.x64.exe'));
end;
