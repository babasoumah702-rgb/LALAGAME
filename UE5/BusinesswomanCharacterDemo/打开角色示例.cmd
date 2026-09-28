@echo off
setlocal
set "PROJECT=%~dp0BusinesswomanCharacterDemo.uproject"
set "EDITOR=%~dp0..\..\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"

if not exist "%EDITOR%" set "EDITOR=%ProgramFiles%\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"

if not exist "%EDITOR%" (
  echo Unreal Engine 5.8 was not found.
  echo Open BusinesswomanCharacterDemo.uproject manually with Unreal Engine 5.8.
  pause
  exit /b 1
)

start "" "%EDITOR%" "%PROJECT%" "/Game/orc_character/Women_Motified_Ultimate/FinalGameTest/L_BusinesswomanGameTest"
