param(
  [string]$FilePath,
  [string]$TimestampUrl = "http://timestamp.digicert.com"
)

# TODO: Configure Windows code signing certificate and enable signing in CI.
# Requirements:
# - A valid code signing certificate (.pfx)
# - SIGN_PFX_PATH and SIGN_PFX_PASSWORD set in CI secrets
# - signtool.exe available (from Windows SDK)

if (-not $FilePath) {
  Write-Host "Usage: .\\scripts\\sign_windows.ps1 -FilePath <path-to-exe-or-msi>"
  exit 1
}

$PfxPath = $env:SIGN_PFX_PATH
$PfxPassword = $env:SIGN_PFX_PASSWORD

if (-not $PfxPath -or -not $PfxPassword) {
  Write-Host "Signing disabled: SIGN_PFX_PATH or SIGN_PFX_PASSWORD not set."
  exit 0
}

$Signtool = "signtool.exe"
& $Signtool sign /f $PfxPath /p $PfxPassword /tr $TimestampUrl /td sha256 /fd sha256 $FilePath
