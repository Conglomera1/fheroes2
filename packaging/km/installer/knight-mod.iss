; Build with ISCC.exe /DPayloadDir=<staged package> knight-mod.iss.
#ifndef PayloadDir
  #error Specify a staging directory via /DPayloadDir
#endif

[Setup]
AppId=KnightMod
AppName=Knight Mod
AppVersion=1.1b
AppPublisher=Conglomera
AppPublisherURL=https://github.com/Conglomera1/fheroes2
SetupArchitecture=x86
MinVersion=6.1
DefaultDirName={code:FindFheroes2Directory}
DisableDirPage=no
DirExistsWarning=no
DefaultGroupName=fheroes2
UsePreviousGroup=no
DisableProgramGroupPage=yes
OutputBaseFilename=fheroes2_KM-1.1b_Windows_Setup
SetupIconFile={#PayloadDir}\launcher.ico
UninstallDisplayIcon={app}\KnightModLauncher.exe
UninstallFilesDir={app}\Knight Mod\uninstall
LicenseFile={#PayloadDir}\common\Knight Mod\docs\LICENCE.txt
WizardStyle=modern
SolidCompression=yes
RestartApplications=no
ShowLanguageDialog=yes
LanguageDetectionMethod=uilanguage

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"

[Messages]
english.SelectDirDesc=Choose an existing fheroes2 installation.
french.SelectDirDesc=Choisissez une installation existante de fheroes2.
english.SelectDirLabel3=Select the fheroes2 folder in which to create the Knight Mod subfolder. Click Browse to select another installation.
french.SelectDirLabel3=Le sous-dossier Knight Mod sera créé dans le dossier fheroes2 sélectionné. Cliquez sur Parcourir pour choisir une autre installation.

[CustomMessages]
english.InvalidGame=This folder must contain a Windows x86 or x64 installation of fheroes2 1.1.17 or later. Select the folder that contains fheroes2.exe and files\data\resurrection.h2d.
french.InvalidGame=Ce dossier doit contenir une installation Windows x86 ou x64 de fheroes2 1.1.17 ou d'une version ultérieure. Sélectionnez le dossier qui contient fheroes2.exe et files\data\resurrection.h2d.
english.InvalidRuntime=The library %1 is missing or its architecture differs from fheroes2.exe. Choose a complete fheroes2 installation.
french.InvalidRuntime=La bibliothèque %1 est absente ou son architecture diffère de celle de fheroes2.exe. Choisissez une installation complète de fheroes2.
english.UnsupportedGame=This version of Windows cannot run the x64 build. Select a 32-bit fheroes2 installation.
french.UnsupportedGame=Cette version de Windows ne peut pas exécuter le build x64. Sélectionnez une installation 32 bits de fheroes2.
english.Readme=Open the Readme
french.Readme=Ouvrir le Readme
english.Guide=Testing and Balance Guide
french.Guide=Guide de test et d'équilibrage
english.Uninstall=Uninstall Knight Mod
french.Uninstall=Désinstaller Knight Mod
english.RemovePrevious=Uninstall the previous Knight Mod installation before continuing. Its uninstaller can keep your settings and saved games.
french.RemovePrevious=Désinstallez d’abord la version précédente de Knight Mod. Son désinstalleur peut conserver les réglages et les sauvegardes.
english.RemoveSettings=Also delete Knight Mod settings for this Windows user? Choose No to keep them for a future installation.
french.RemoveSettings=Supprimer également les réglages de Knight Mod pour cet utilisateur Windows ? Choisissez Non pour les conserver en vue d'une future installation.
english.RemoveSettingsError=Some Knight Mod settings could not be deleted.
french.RemoveSettingsError=Certains réglages de Knight Mod n'ont pas pu être supprimés.

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; Flags: unchecked

[Files]
Source: "{#PayloadDir}\x86\fheroes2KM.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: InstallGameX86
Source: "{#PayloadDir}\x64\fheroes2KM.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: InstallGameX64
Source: "{#PayloadDir}\common\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#PayloadDir}\en.language"; DestDir: "{app}\Knight Mod"; DestName: "KnightModLauncher.language"; Flags: ignoreversion; Languages: english
Source: "{#PayloadDir}\fr.language"; DestDir: "{app}\Knight Mod"; DestName: "KnightModLauncher.language"; Flags: ignoreversion; Languages: french

[Icons]
Name: "{group}\Knight Mod\Knight Mod Launcher"; Filename: "{app}\KnightModLauncher.exe"; WorkingDir: "{app}"
Name: "{group}\Knight Mod\Readme"; Filename: "{app}\Knight Mod\docs\Readme.txt"; Languages: english
Name: "{group}\Knight Mod\Readme"; Filename: "{app}\Knight Mod\docs\Readme_FR.txt"; Languages: french
Name: "{group}\Knight Mod\{cm:Guide}"; Filename: "{app}\Knight Mod\docs\Testing_and_Balance_Guide.pdf"; Languages: english
Name: "{group}\Knight Mod\{cm:Guide}"; Filename: "{app}\Knight Mod\docs\Guide_de_Test_et_Equilibrage.pdf"; Languages: french
Name: "{group}\Knight Mod\{cm:Uninstall}"; Filename: "{uninstallexe}"; IconFilename: "{sys}\shell32.dll"; IconIndex: 31
Name: "{autodesktop}\Knight Mod Launcher"; Filename: "{app}\KnightModLauncher.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[InstallDelete]
Type: files; Name: "{commonprograms}\Knight Mod\Knight Mod Launcher.lnk"
Type: files; Name: "{commonprograms}\Knight Mod\Readme.lnk"
Type: files; Name: "{commonprograms}\Knight Mod\Testing and Balance Guide.lnk"
Type: files; Name: "{commonprograms}\Knight Mod\Guide de test et d'équilibrage.lnk"
Type: files; Name: "{commonprograms}\Knight Mod\Uninstall Knight Mod.lnk"
Type: files; Name: "{commonprograms}\Knight Mod\Désinstaller Knight Mod.lnk"
Type: dirifempty; Name: "{commonprograms}\Knight Mod"

[Run]
Filename: "{app}\Knight Mod\docs\Readme.txt"; Description: "{cm:Readme}"; Flags: postinstall shellexec skipifsilent unchecked runasoriginaluser; Languages: english
Filename: "{app}\Knight Mod\docs\Readme_FR.txt"; Description: "{cm:Readme}"; Flags: postinstall shellexec skipifsilent unchecked runasoriginaluser; Languages: french

[Code]
const
  MachineX86 = $014C;
  MachineX64 = $8664;
  Fheroes2UninstallKey = 'Software\Microsoft\Windows\CurrentVersion\Uninstall\fheroes2_is1';

var
  GameMachine: Integer;

function ReadWord(const Bytes: AnsiString; Index: Integer): Integer;
begin
  Result := Ord(Bytes[Index]) + 256 * Ord(Bytes[Index + 1]);
end;

function PEMachine(const Filename: String; ExpectedLibrary: Boolean): Integer;
var
  Stream: TFileStream;
  Header: AnsiString;
  Offset: Int64;
  Machine, Magic, Characteristics: Integer;
begin
  Result := 0;
  if not FileExists(Filename) then
    Exit;

  try
    Stream := TFileStream.Create(Filename, fmOpenRead or fmShareDenyNone);
    try
      SetLength(Header, 64);
      if Stream.Read(Header, 64) <> 64 then
        Exit;
      if Copy(Header, 1, 2) <> 'MZ' then
        Exit;

      Offset := Int64(ReadWord(Header, 61)) + Int64(ReadWord(Header, 63)) * 65536;
      if (Offset < 64) or (Offset > Stream.Size - 26) then
        Exit;
      Stream.Seek(Offset, soFromBeginning);

      SetLength(Header, 26);
      if Stream.Read(Header, 26) <> 26 then
        Exit;
      if (Copy(Header, 1, 4) <> 'PE'#0#0) or (ReadWord(Header, 7) = 0) then
        Exit;

      Machine := ReadWord(Header, 5);
      Characteristics := ReadWord(Header, 23);
      Magic := ReadWord(Header, 25);
      if ((Characteristics and $0002) = 0) or (((Characteristics and $2000) <> 0) <> ExpectedLibrary) then
        Exit;
      if ((Machine = MachineX86) and (Magic = $10B)) or ((Machine = MachineX64) and (Magic = $20B)) then
        Result := Machine;
    finally
      Stream.Free;
    end;
  except
    Log('Unable to read PE header: ' + Filename);
  end;
end;

function SupportedGame(const Filename: String): Boolean;
var
  VersionHigh, VersionLow: Cardinal;
begin
  Result := False;
  if not GetVersionNumbers(Filename, VersionHigh, VersionLow) then
    Exit;
  Result := (VersionHigh > $00010001) or ((VersionHigh = $00010001) and (VersionLow >= $00110000));
end;

function DestinationError(const Directory: String; var Machine: Integer): String;
var
  Root, GameExe, LibraryName: String;
  Libraries: TArrayOfString;
  Index: Integer;
begin
  Root := AddBackslash(Directory);
  GameExe := Root + 'fheroes2.exe';
  Machine := PEMachine(GameExe, False);
  Result := CustomMessage('InvalidGame');
  if (Machine = 0) or not SupportedGame(GameExe) then
    Exit;
  if not FileExists(Root + 'files\data\resurrection.h2d') then
    Exit;
  if (Machine = MachineX64) and not IsX64Compatible then
  begin
    Result := CustomMessage('UnsupportedGame');
    Exit;
  end;

  Libraries := ['SDL2.dll', 'SDL2_mixer.dll', 'SDL2_image.dll', 'zlib1.dll'];
  for Index := 0 to GetArrayLength(Libraries) - 1 do
  begin
    LibraryName := Libraries[Index];
    if PEMachine(Root + LibraryName, True) <> Machine then
    begin
      Result := FmtMessage(CustomMessage('InvalidRuntime'), [LibraryName]);
      Exit;
    end;
  end;
  Result := '';
end;

function RegisteredFheroes2Directory(RootKey: Integer): String;
var
  Directory: String;
  Machine: Integer;
begin
  Result := '';
  if RegQueryStringValue(RootKey, Fheroes2UninstallKey, 'Inno Setup: App Path', Directory) then
  begin
    if DestinationError(Directory, Machine) = '' then
      Result := Directory;
  end;
end;

function FindFheroes2Directory(Unused: String): String;
var
  Machine: Integer;
begin
  Result := RegisteredFheroes2Directory(HKCU);
  if Result = '' then
    Result := RegisteredFheroes2Directory(HKLM32);
  if (Result = '') and IsWin64 then
    Result := RegisteredFheroes2Directory(HKLM64);
  if Result <> '' then
    Exit;

  if IsWin64 then
  begin
    Result := ExpandConstant('{commonpf64}\fheroes2');
    if DestinationError(Result, Machine) = '' then
      Exit;
  end;
  Result := ExpandConstant('{commonpf32}\fheroes2');
end;

function NextButtonClick(CurPageID: Integer): Boolean;
var
  Error: String;
  Machine: Integer;
begin
  Result := True;
  if CurPageID = wpSelectDir then
  begin
    Error := DestinationError(WizardDirValue, Machine);
    if Error <> '' then
    begin
      SuppressibleMsgBox(Error, mbError, MB_OK, IDOK);
      Result := False;
    end;
  end;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  PreviousUninstaller: TFindRec;
  Root: String;
begin
  Result := DestinationError(WizardDirValue, GameMachine);
  if Result <> '' then
    Exit;

  Root := AddBackslash(WizardDirValue);
  if FindFirst(Root + 'docs\Knight Mod\uninstall\unins*.exe', PreviousUninstaller) then
  begin
    FindClose(PreviousUninstaller);
    Result := CustomMessage('RemovePrevious');
  end
  else if FileExists(Root + 'Uninstall_KM.bat') then
    Result := CustomMessage('RemovePrevious');
end;

function InstallGameX86: Boolean;
begin
  Result := GameMachine = MachineX86;
end;

function InstallGameX64: Boolean;
begin
  Result := GameMachine = MachineX64;
end;

function RemoveSetting(const Filename: String): Boolean;
begin
  Result := not FileExists(Filename) or DeleteFile(Filename);
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  SettingsDir: String;
  RemovedIni, RemovedLanguage: Boolean;
begin
  if (CurUninstallStep <> usPostUninstall) or UninstallSilent then
    Exit;

  SettingsDir := ExpandConstant('{userappdata}\fheroes2\');
  if not FileExists(SettingsDir + 'knight_mod.ini') and
     not FileExists(SettingsDir + 'knight_mod_launcher.language') then
    Exit;

  if SuppressibleMsgBox(CustomMessage('RemoveSettings'), mbConfirmation, MB_YESNO or MB_DEFBUTTON2, IDNO) = IDYES then
  begin
    RemovedIni := RemoveSetting(SettingsDir + 'knight_mod.ini');
    RemovedLanguage := RemoveSetting(SettingsDir + 'knight_mod_launcher.language');
    if not RemovedIni or not RemovedLanguage then
      MsgBox(CustomMessage('RemoveSettingsError'), mbError, MB_OK);
  end;
end;
