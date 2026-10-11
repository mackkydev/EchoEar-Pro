$ErrorActionPreference = "Stop"

$rule = Get-NetFirewallRule -DisplayName "EchoEar Gemini Gateway" -ErrorAction SilentlyContinue
if ($null -eq $rule) {
    New-NetFirewallRule `
        -DisplayName "EchoEar Gemini Gateway" `
        -Direction Inbound `
        -Action Allow `
        -Protocol TCP `
        -LocalPort 8765 `
        -Profile Private,Public `
        -RemoteAddress 192.168.1.0/24 | Out-Null
} else {
    $rule | Set-NetFirewallRule `
        -Enabled True `
        -Direction Inbound `
        -Action Allow `
        -Profile Private,Public `
        -RemoteAddress 192.168.1.0/24 | Out-Null
}
