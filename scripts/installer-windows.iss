; Inno Setup script for KiraWavTar
; Compile with: iscc scripts\installer-windows.iss

#define AppBinary "..\build\artifact\bin\KiraWAVTar.exe"

#ifndef AppVersion
  #if FileExists(AppBinary)
    #define AppVersion GetStringFileInfo(AppBinary, "ProductVersion")
    #define VersionInfoVersion GetVersionNumbersString(AppBinary)
  #else
    #define AppVersion "0.0.0"
    #define VersionInfoVersion "0.0.0.0"
  #endif
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
LicenseFile=..\LICENSE
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern
VersionInfoVersion={#VersionInfoVersion}
ChangesEnvironment=yes

[Files]
Source: "..\build\artifact\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\build\artifact\bin\vc_redist.x64.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall; Check: VCRedistExists

[Icons]
Name: "{group}\KiraWavTar"; Filename: "{app}\bin\KiraWAVTar.exe"
Name: "{autodesktop}\KiraWavTar"; Filename: "{app}\bin\KiraWAVTar.exe"; Tasks: desktopicon

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "zh_CN"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"
Name: "zh_TW"; MessagesFile: "compiler:Languages\ChineseTraditional.isl"
Name: "ja"; MessagesFile: "compiler:Languages\Japanese.isl"


[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"
Name: "addtopath"; Description: "Add CLI tool (kirawavtar-cli) to &PATH"; GroupDescription: "CLI integration:"; Flags: unchecked

[Run]
Filename: "{tmp}\vc_redist.x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "Installing Visual C++ Redistributable..."; Flags: waituntilterminated; Check: VCRedistExists
Filename: "{app}\bin\KiraWAVTar.exe"; Description: "Launch KiraWavTar"; Flags: nowait postinstall skipifsilent

[Code]
function VCRedistExists: Boolean;
begin
  Result := FileExists(ExpandConstant('{src}\..\build\artifact\bin\vc_redist.x64.exe'));
end;

// Read the user PATH from the registry. Returns empty string if not found.
function GetUserPath: String;
var
  Path: String;
begin
  if not RegQueryStringValue(HKCU, 'Environment', 'Path', Path) then
    Path := '';
  Result := Path;
end;

// Split a string by a separator into an array.
procedure Explode(var Dest: TArrayOfString; const S, Delim: String);
var
  Tmp: String;
  P: Integer;
begin
  SetArrayLength(Dest, 0);
  Tmp := S;
  repeat
    P := Pos(Delim, Tmp);
    if P > 0 then
    begin
      SetArrayLength(Dest, GetArrayLength(Dest) + 1);
      Dest[GetArrayLength(Dest) - 1] := Copy(Tmp, 1, P - 1);
      Tmp := Copy(Tmp, P + Length(Delim), Length(Tmp));
    end else
    begin
      SetArrayLength(Dest, GetArrayLength(Dest) + 1);
      Dest[GetArrayLength(Dest) - 1] := Tmp;
    end;
  until P = 0;
end;

// Check if Dir already exists in a semicolon-separated PATH string (case-insensitive).
function IsDirInPath(const Path, Dir: String): Boolean;
var
  Entries: TArrayOfString;
  I: Integer;
  Entry: String;
begin
  Result := False;
  if Path = '' then
    Exit;
  Explode(Entries, Path, ';');
  for I := 0 to GetArrayLength(Entries) - 1 do
  begin
    Entry := Trim(Entries[I]);
    // Strip trailing backslash for comparison
    if (Length(Entry) > 0) and (Entry[Length(Entry)] = '\') then
      Entry := Copy(Entry, 1, Length(Entry) - 1);
    if CompareText(Entry, Dir) = 0 then
    begin
      Result := True;
      Exit;
    end;
  end;
end;

procedure AddToPath;
var
  BinDir, Path: String;
begin
  BinDir := ExpandConstant('{app}\bin');
  Path := GetUserPath;
  if not IsDirInPath(Path, BinDir) then
  begin
    if Path <> '' then
      Path := Path + ';';
    Path := Path + BinDir;
    RegWriteExpandStringValue(HKCU, 'Environment', 'Path', Path);
  end;
end;

procedure RemoveFromPath;
var
  BinDir, Path, NewPath: String;
  Entries: TArrayOfString;
  I: Integer;
  Entry: String;
begin
  BinDir := ExpandConstant('{app}\bin');
  Path := GetUserPath;
  if Path = '' then
    Exit;
  Explode(Entries, Path, ';');
  NewPath := '';
  for I := 0 to GetArrayLength(Entries) - 1 do
  begin
    Entry := Trim(Entries[I]);
    // Strip trailing backslash for comparison
    if (Length(Entry) > 0) and (Entry[Length(Entry)] = '\') then
      Entry := Copy(Entry, 1, Length(Entry) - 1);
    if CompareText(Entry, BinDir) <> 0 then
    begin
      if NewPath <> '' then
        NewPath := NewPath + ';';
      NewPath := NewPath + Entries[I];
    end;
  end;
  RegWriteExpandStringValue(HKCU, 'Environment', 'Path', NewPath);
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep = ssPostInstall) and IsTaskSelected('addtopath') then
    AddToPath;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
    RemoveFromPath;
end;
