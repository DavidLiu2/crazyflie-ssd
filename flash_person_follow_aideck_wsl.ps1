param(
  [string]$CrazyflieUri,
  [Parameter(ValueFromRemainingArguments = $true)]
  [string[]]$MakeArgs
)

$ErrorActionPreference = "Stop"

function Convert-ToWslPath {
  param([Parameter(Mandatory = $true)][string]$WindowsPath)

  $fullPath = [System.IO.Path]::GetFullPath($WindowsPath)
  if ($fullPath -match '^([A-Za-z]):\\(.*)$') {
    $drive = $matches[1].ToLowerInvariant()
    $rest = ($matches[2] -replace '\\', '/')
    return "/mnt/$drive/$rest"
  }

  throw "Only drive-letter paths are supported for WSL conversion: $WindowsPath"
}

function Quote-BashArg {
  param([Parameter(Mandatory = $true)][string]$Value)
  return "'" + ($Value -replace "'", "'\"'\"'") + "'"
}

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$scriptDirWsl = Convert-ToWslPath -WindowsPath $scriptDir

$bashParts = @(
  "cd $(Quote-BashArg $scriptDirWsl)",
  "bash ./flash_person_follow_aideck.sh"
)

if ($CrazyflieUri) {
  $bashParts += (Quote-BashArg $CrazyflieUri)
}

foreach ($arg in $MakeArgs) {
  $bashParts += (Quote-BashArg $arg)
}

$bashCommand = ($bashParts -join ' ')

Write-Host "Launching AI-deck build helper through WSL..."
Write-Host "WSL command: $bashCommand"

& wsl.exe bash -lc $bashCommand
if ($LASTEXITCODE -ne 0) {
  exit $LASTEXITCODE
}
