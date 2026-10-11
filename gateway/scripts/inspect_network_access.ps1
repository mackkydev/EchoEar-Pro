$ErrorActionPreference = "Stop"

$outputDirectory = "D:\2026-Projects\EchoEar-Pro\gateway\output"
$outputPath = Join-Path $outputDirectory "network-access-diagnostic.txt"

New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("=== Network profiles ===")
Get-NetConnectionProfile | ForEach-Object {
    $lines.Add(("Name={0}; Interface={1}; Category={2}; IPv4={3}; IPv6={4}" -f $_.Name, $_.InterfaceAlias, $_.NetworkCategory, $_.IPv4Connectivity, $_.IPv6Connectivity))
}

$lines.Add("")
$lines.Add("=== EchoEar firewall rule ===")
$rules = Get-NetFirewallRule -DisplayName "EchoEar Gemini Gateway" -ErrorAction SilentlyContinue
if ($null -eq $rules) {
    $lines.Add("Rule not found")
} else {
    foreach ($rule in $rules) {
        $port = $rule | Get-NetFirewallPortFilter
        $address = $rule | Get-NetFirewallAddressFilter
        $lines.Add(("Enabled={0}; Direction={1}; Action={2}; Profile={3}; Protocol={4}; LocalPort={5}; RemoteAddress={6}" -f $rule.Enabled, $rule.Direction, $rule.Action, $rule.Profile, $port.Protocol, $port.LocalPort, ($address.RemoteAddress -join ',')))
    }
}

$lines.Add("")
$lines.Add("=== Listening port ===")
Get-NetTCPConnection -LocalPort 8765 -State Listen -ErrorAction SilentlyContinue | ForEach-Object {
    $lines.Add(("LocalAddress={0}; LocalPort={1}; PID={2}" -f $_.LocalAddress, $_.LocalPort, $_.OwningProcess))
}

$lines | Set-Content -LiteralPath $outputPath -Encoding UTF8
