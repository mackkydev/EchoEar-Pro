$ErrorActionPreference = 'Stop'

$gatewayRoot = Split-Path -Parent $PSScriptRoot
$pythonPath = Join-Path $gatewayRoot '.venv\Scripts\python.exe'
$testScript = Join-Path $PSScriptRoot 'gemini_smoke_test.py'
$outputPath = Join-Path $gatewayRoot 'output\gemini-thai.wav'

if (-not (Test-Path -LiteralPath $pythonPath)) {
    throw 'Gateway environment is missing. Run: python -m venv .venv'
}

$secureKey = Read-Host 'Paste GEMINI_API_KEY (input is hidden)' -AsSecureString
$keyPointer = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($secureKey)
try {
    $env:GEMINI_API_KEY = [Runtime.InteropServices.Marshal]::PtrToStringBSTR($keyPointer)
    & $pythonPath $testScript --output $outputPath
    if ($LASTEXITCODE -ne 0) {
        throw "Gemini smoke test failed with exit code $LASTEXITCODE"
    }
}
finally {
    Remove-Item Env:GEMINI_API_KEY -ErrorAction SilentlyContinue
    [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($keyPointer)
}
