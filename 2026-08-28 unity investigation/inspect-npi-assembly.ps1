$ErrorActionPreference = 'Stop'
$assemblyPath = 'C:\Users\k\Program\nvidiaProfileInspector\nvidiaProfileInspector.exe'
$assembly = [System.Reflection.Assembly]::LoadFrom($assemblyPath)
$lines = @()
foreach ($type in $assembly.GetTypes() | Sort-Object FullName) {
    if ($type.FullName -notmatch 'DriverSettings|ConstantSettings|SettingMeta|Native.NvApi') { continue }
    $lines += "TYPE`t$($type.FullName)"
    $flags = [System.Reflection.BindingFlags]'Public,NonPublic,Static,Instance,DeclaredOnly'
    foreach ($field in $type.GetFields($flags) | Sort-Object Name) {
        $value = ''
        if ($field.IsStatic) {
            try {
                $rawValue = $field.GetValue($null)
                if ($field.FieldType.IsEnum) {
                    $value = "$rawValue [0x$([System.Convert]::ToUInt64($rawValue).ToString('X'))]"
                } else {
                    $value = $rawValue
                }
            } catch { $value = "<error: $($_.Exception.Message)>" }
        }
        $lines += "FIELD`t$($field.Name)`t$($field.FieldType.FullName)`t$value"
    }
    foreach ($property in $type.GetProperties($flags) | Sort-Object Name) {
        $lines += "PROPERTY`t$($property.Name)`t$($property.PropertyType.FullName)"
    }
}
$lines | Set-Content -LiteralPath (Join-Path -Path $PSScriptRoot -ChildPath 'npi-assembly-settings.txt') -Encoding utf8
