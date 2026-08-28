param(
    [string]$OutputDirectory = $PSScriptRoot
)

$ErrorActionPreference = 'Continue'
$snapshot = Get-Date

function One-Line([object]$Value) {
    if ($null -eq $Value) { return '' }
    return (($Value.ToString() -replace "`t", ' ') -replace "[`r`n]+", ' ')
}

$candidateRoots = @(
    'C:\Program Files\Unity',
    'C:\Program Files\Unity Hub',
    'C:\Program Files (x86)\Unity',
    'C:\Program Files (x86)\Unity Hub',
    "$env:LOCALAPPDATA\Programs\Unity Hub",
    "$env:LOCALAPPDATA\Unity"
)

$candidateFiles = @()
foreach ($root in $candidateRoots) {
    if (Test-Path -LiteralPath $root) {
        $candidateFiles += Get-ChildItem -LiteralPath $root -Filter '*.exe' -File -Recurse -ErrorAction SilentlyContinue
    }
}

# Include locally built Unity players. A sibling UnityPlayer.dll is a stronger
# signal than a directory name alone.
$localUnityRoot = 'C:\Users\k\Repository\Unity'
if (Test-Path -LiteralPath $localUnityRoot) {
    foreach ($file in Get-ChildItem -LiteralPath $localUnityRoot -Filter '*.exe' -File -Recurse -ErrorAction SilentlyContinue) {
        $playerDll = Join-Path -Path $file.DirectoryName -ChildPath 'UnityPlayer.dll'
        if (Test-Path -LiteralPath $playerDll) { $candidateFiles += $file }
    }
}

$inventoryLines = @("Path`tName`tCompanyName`tProductName`tFileVersion`tProductVersion`tCategory")
$seenPaths = @{}
foreach ($file in ($candidateFiles | Sort-Object FullName)) {
    if ($seenPaths.ContainsKey($file.FullName)) { continue }
    $version = $file.VersionInfo
    $isOfficial = $version.CompanyName -match 'Unity' -or $version.ProductName -match 'Unity' -or $file.Name -match '^Unity.*\.exe$'
    $hasPlayer = Test-Path -LiteralPath (Join-Path -Path $file.DirectoryName -ChildPath 'UnityPlayer.dll')
    if (-not $isOfficial -and -not $hasPlayer) { continue }
    $seenPaths[$file.FullName] = $true
    $category = if ($hasPlayer -and $file.FullName.StartsWith($localUnityRoot, [System.StringComparison]::OrdinalIgnoreCase)) { 'local Unity player build' } else { 'Unity-supplied component' }
    $inventoryLines += @(
        "$(One-Line $file.FullName)`t$(One-Line $file.Name)`t$(One-Line $version.CompanyName)`t$(One-Line $version.ProductName)`t$(One-Line $version.FileVersion)`t$(One-Line $version.ProductVersion)`t$category"
    )
}
$inventoryPath = Join-Path -Path $OutputDirectory -ChildPath 'unity-executables.tsv'
$inventoryLines | Set-Content -LiteralPath $inventoryPath -Encoding utf8

$referencePath = 'C:\Users\k\Program\nvidiaProfileInspector\Reference.xml'
$referenceLines = @("Id`tName`tGroup`tDescription")
$referenceIds = @{}
if (Test-Path -LiteralPath $referencePath) {
    [xml]$referenceXml = Get-Content -LiteralPath $referencePath -Raw
    foreach ($setting in $referenceXml.CustomSettingNames.Settings.CustomSetting) {
        $id = One-Line $setting.HexSettingID
        if ($id -notmatch '^0x[0-9A-Fa-f]{8}$') { continue }
        $referenceIds[$id.ToLowerInvariant()] = $true
        $referenceLines += "$id`t$(One-Line $setting.UserfriendlyName)`t$(One-Line $setting.GroupName)`t$(One-Line $setting.Description)"
    }
}

# Reference.xml contains NVIDIA Profile Inspector's custom/undocumented IDs.
# The ordinary Control Panel IDs are constants in the application assembly, so
# add those as well to make the effective-setting query complete.
$npiAssemblyPath = 'C:\Users\k\Program\nvidiaProfileInspector\nvidiaProfileInspector.exe'
if (Test-Path -LiteralPath $npiAssemblyPath) {
    try {
        $npiAssembly = [System.Reflection.Assembly]::LoadFrom($npiAssemblyPath)
        $settingType = $npiAssembly.GetType('nvidiaProfileInspector.Native.NvApi.DriverSettings.ESetting')
        $bindingFlags = [System.Reflection.BindingFlags]'Public,Static'
        foreach ($field in $settingType.GetFields($bindingFlags) | Sort-Object Name) {
            if ($field.Name -notmatch '_ID$' -or $field.Name -eq 'INVALID_SETTING_ID') { continue }
            $number = [System.Convert]::ToUInt32($field.GetValue($null))
            $id = '0x{0:X8}' -f $number
            if ($referenceIds.ContainsKey($id.ToLowerInvariant())) { continue }
            $referenceIds[$id.ToLowerInvariant()] = $true
            $friendlyName = ($field.Name -replace '_ID$', '') -replace '_', ' '
            $referenceLines += "$id`t$friendlyName`tNVIDIA built-in setting`tNVIDIA Profile Inspector built-in constant $($field.Name)"
        }
    } catch {
        $referenceLines += "0xFFFFFFFF`tNPI reflection failed`tAudit metadata`t$(One-Line $_.Exception.Message)"
    }
}
$settingReferencePath = Join-Path -Path $OutputDirectory -ChildPath 'setting-reference.tsv'
$referenceLines | Set-Content -LiteralPath $settingReferencePath -Encoding utf8

$machineLines = @()
$machineLines += '=== SNAPSHOT ==='
$machineLines += "Local time: $($snapshot.ToString('o'))"
$machineLines += "UTC time: $($snapshot.ToUniversalTime().ToString('o'))"
$machineLines += "Computer: $env:COMPUTERNAME"
$machineLines += "User: $env:USERNAME"
$machineLines += "PowerShell language mode: $($ExecutionContext.SessionState.LanguageMode)"
$machineLines += ''
$machineLines += '=== NVIDIA-SMI ==='
$smi = Get-Command nvidia-smi.exe -ErrorAction SilentlyContinue
if ($null -ne $smi) {
    $machineLines += (& $smi.Source --query-gpu=name,driver_version,pci.bus_id,display_active,display_mode,memory.total --format=csv,noheader 2>&1 | Out-String).TrimEnd()
    $machineLines += (& $smi.Source 2>&1 | Out-String).TrimEnd()
} else {
    $machineLines += 'nvidia-smi.exe not found'
}
$machineLines += ''
$machineLines += '=== VIDEO CONTROLLERS ==='
$machineLines += (Get-CimInstance Win32_VideoController | Select-Object Name,DriverVersion,DriverDate,PNPDeviceID,AdapterRAM,VideoModeDescription,CurrentRefreshRate | Format-List | Out-String).TrimEnd()
$machineLines += ''
$machineLines += '=== INSTALLED UNITY PRODUCTS (UNINSTALL REGISTRY) ==='
$uninstallRoots = @(
    'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*',
    'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*',
    'HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*'
)
foreach ($uninstallRoot in $uninstallRoots) {
    $entries = Get-ItemProperty -Path $uninstallRoot -ErrorAction SilentlyContinue | Where-Object { $_.DisplayName -match '(?i)(^|[^A-Za-z])Unity([^A-Za-z]|$)' }
    foreach ($entry in $entries) {
        $machineLines += "Name=$(One-Line $entry.DisplayName); Version=$(One-Line $entry.DisplayVersion); Publisher=$(One-Line $entry.Publisher); Location=$(One-Line $entry.InstallLocation); Key=$(One-Line $entry.PSPath)"
    }
}
$machineLines += ''
$machineLines += '=== RUNNING UNITY-RELATED PROCESSES ==='
$unityProcessCount = 0
foreach ($process in Get-Process -ErrorAction SilentlyContinue | Sort-Object ProcessName,Id) {
    $path = ''
    try { $path = $process.Path } catch {}
    if ($process.ProcessName -match 'Unity' -or $path -match '\\Unity( Hub)?\\|\\Unity\\') {
        ++$unityProcessCount
        $machineLines += "PID=$($process.Id); Name=$($process.ProcessName); Path=$(One-Line $path); StartTime=$(try { $process.StartTime.ToString('o') } catch { '' })"
    }
}
if ($unityProcessCount -eq 0) { $machineLines += 'None' }
$machineLines += ''
$machineLines += '=== NVIDIA COMPONENT VERSIONS ==='
$nvidiaExecutables = @(
    'C:\Program Files\NVIDIA Corporation\NVIDIA app\CEF\NVIDIA app.exe',
    'C:\Program Files\NVIDIA Corporation\NVIDIA app\NVIDIA app.exe',
    'C:\Program Files\NVIDIA Corporation\Control Panel Client\nvcplui.exe',
    'C:\Windows\System32\DriverStore\FileRepository'
)
foreach ($path in $nvidiaExecutables) {
    if ((Test-Path -LiteralPath $path -PathType Leaf)) {
        $item = Get-Item -LiteralPath $path
        $machineLines += "Path=$path; FileVersion=$(One-Line $item.VersionInfo.FileVersion); ProductVersion=$(One-Line $item.VersionInfo.ProductVersion)"
    }
}
$machineLines += (Get-AppxPackage -Name '*NVIDIAControlPanel*' -ErrorAction SilentlyContinue | Select-Object Name,Version,PackageFullName,InstallLocation | Format-List | Out-String).TrimEnd()
$machineLines += ''
$machineLines += '=== DRS DATABASE FILES ==='
foreach ($path in @(
    'C:\ProgramData\NVIDIA Corporation\Drs\nvdrsdb0.bin',
    'C:\ProgramData\NVIDIA Corporation\Drs\nvdrsdb1.bin'
)) {
    if (Test-Path -LiteralPath $path) {
        $item = Get-Item -LiteralPath $path
        $hashText = 'unavailable'
        try { $hashText = (Get-FileHash -LiteralPath $path -Algorithm SHA256 -ErrorAction Stop).Hash } catch { $hashText = "unavailable ($($_.Exception.Message))" }
        $machineLines += "Path=$path; Length=$($item.Length); LastWriteTime=$($item.LastWriteTime.ToString('o')); SHA256=$hashText"
    } else {
        $machineLines += "Missing: $path"
    }
}
$machineLines += ''
$machineLines += '=== NVIDIA APP STATE FILES ==='
$nvBackend = "$env:LOCALAPPDATA\NVIDIA Corporation\NVIDIA App\NvBackend"
foreach ($name in @('ApplicationStorage.json','backend.log')) {
    $path = Join-Path -Path $nvBackend -ChildPath $name
    if (Test-Path -LiteralPath $path) {
        $item = Get-Item -LiteralPath $path
        $hashText = 'unavailable'
        try { $hashText = (Get-FileHash -LiteralPath $path -Algorithm SHA256 -ErrorAction Stop).Hash } catch { $hashText = "unavailable ($($_.Exception.Message))" }
        $machineLines += "Path=$path; Length=$($item.Length); LastWriteTime=$($item.LastWriteTime.ToString('o')); SHA256=$hashText"
    } else {
        $machineLines += "Missing: $path"
    }
}
$machineLines += ''
$machineLines += '=== WINDOWS GRAPHICS SCHEDULING ==='
$machineLines += (reg.exe query 'HKLM\SYSTEM\CurrentControlSet\Control\GraphicsDrivers' /v HwSchMode 2>&1 | Out-String).TrimEnd()
$machineLines += ''
$machineLines += '=== WINDOWS PER-APP GPU PREFERENCES ==='
$machineLines += (reg.exe query 'HKCU\Software\Microsoft\DirectX\UserGpuPreferences' 2>&1 | Out-String).TrimEnd()

$machineStatePath = Join-Path -Path $OutputDirectory -ChildPath 'machine-state.txt'
$machineLines | Set-Content -LiteralPath $machineStatePath -Encoding utf8

$registryLines = @()
foreach ($key in @(
    'HKCU\Software\NVIDIA Corporation',
    'HKLM\SOFTWARE\NVIDIA Corporation',
    'HKLM\SOFTWARE\WOW6432Node\NVIDIA Corporation',
    'HKCU\Software\Microsoft\DirectX\UserGpuPreferences'
)) {
    $registryLines += "=== $key ==="
    $raw = reg.exe query $key /s 2>&1
    $matches = $raw | Select-String -Pattern 'unity|ZoomTracks' -CaseSensitive:$false
    if ($null -eq $matches -or $matches.Count -eq 0) {
        $registryLines += 'No Unity/ZoomTracks match.'
    } else {
        $registryLines += ($matches | ForEach-Object { $_.Line })
    }
    $registryLines += ''
}
$registryLines | Set-Content -LiteralPath (Join-Path -Path $OutputDirectory -ChildPath 'registry-unity-search.txt') -Encoding utf8

$applicationStorage = Join-Path -Path $nvBackend -ChildPath 'ApplicationStorage.json'
if (Test-Path -LiteralPath $applicationStorage) {
    Copy-Item -LiteralPath $applicationStorage -Destination (Join-Path -Path $OutputDirectory -ChildPath 'nvidia-app-application-storage.json') -Force
}

$fingerprintDatabase = Join-Path -Path $nvBackend -ChildPath 'ApplicationOntology\data\fingerprint.db'
$fingerprintLines = @()
if (Test-Path -LiteralPath $fingerprintDatabase) {
    $captureFingerprint = $false
    foreach ($line in Get-Content -LiteralPath $fingerprintDatabase) {
        if ($line -match "<Fingerprint name='unity_editor'>") { $captureFingerprint = $true }
        if ($captureFingerprint) { $fingerprintLines += $line }
        if ($captureFingerprint -and $line -match '</Fingerprint>') { break }
    }
}
if ($fingerprintLines.Count -eq 0) { $fingerprintLines += '<!-- Unity Editor fingerprint not found. -->' }
$fingerprintLines | Set-Content -LiteralPath (Join-Path -Path $OutputDirectory -ChildPath 'nvidia-app-unity-editor-fingerprint.xml') -Encoding utf8

$backendLog = Join-Path -Path $nvBackend -ChildPath 'backend.log'
$logLines = @()
if (Test-Path -LiteralPath $backendLog) {
    $lineNumber = 0
    foreach ($line in Get-Content -LiteralPath $backendLog) {
        ++$lineNumber
        if ($line -match '(?i)unity|ZoomTracks') { $logLines += "$lineNumber`:$line" }
    }
}
if ($logLines.Count -eq 0) { $logLines += 'No Unity/ZoomTracks lines found.' }
$logLines | Set-Content -LiteralPath (Join-Path -Path $OutputDirectory -ChildPath 'nvidia-app-unity-log-lines.txt') -Encoding utf8

$captureCoreLog = 'C:\ProgramData\NVIDIA Corporation\ShadowPlay\CaptureCore.log'
$captureCoreLines = @()
if (Test-Path -LiteralPath $captureCoreLog) {
    $lineNumber = 0
    foreach ($line in Get-Content -LiteralPath $captureCoreLog) {
        ++$lineNumber
        if ($line -match '(?i)Unity|ZoomTracks') { $captureCoreLines += "$lineNumber`:$line" }
    }
}
if ($captureCoreLines.Count -eq 0) { $captureCoreLines += 'No Unity/ZoomTracks lines found.' }
$captureCoreLines | Set-Content -LiteralPath (Join-Path -Path $OutputDirectory -ChildPath 'shadowplay-unity-log-lines.txt') -Encoding utf8

$auditExe = Join-Path -Path $OutputDirectory -ChildPath 'unity-drs-audit.exe'
$auditOutput = Join-Path -Path $OutputDirectory -ChildPath 'drs-audit.txt'
if (Test-Path -LiteralPath $auditExe) {
    (& $auditExe $inventoryPath $settingReferencePath 2>&1 | Out-String) | Set-Content -LiteralPath $auditOutput -Encoding utf8
} else {
    "Missing audit executable: $auditExe" | Set-Content -LiteralPath $auditOutput -Encoding utf8
}

Get-ChildItem -LiteralPath $OutputDirectory -File | Where-Object { $_.Name -ne 'artifact-manifest.tsv' } | ForEach-Object {
    $hash = Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256
    "$($_.Name)`t$($_.Length)`t$($_.LastWriteTime.ToString('o'))`t$($hash.Hash)"
} | Sort-Object | Set-Content -LiteralPath (Join-Path -Path $OutputDirectory -ChildPath 'artifact-manifest.tsv') -Encoding utf8
