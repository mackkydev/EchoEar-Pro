$ErrorActionPreference = "Stop"

$ruleName = "EchoEar Gemini Gateway"
$existing = Get-NetFirewallRule -DisplayName $ruleName -ErrorAction SilentlyContinue

if ($null -eq $existing) {
    New-NetFirewallRule `
        -DisplayName $ruleName `
        -Direction Inbound `
        -Action Allow `
        -Protocol TCP `
        -LocalPort 8765 `
        -Profile Private | Out-Null
} else {
    Set-NetFirewallRule `
        -DisplayName $ruleName `
        -Enabled True `
        -Direction Inbound `
        -Action Allow `
        -Profile Private | Out-Null
}
