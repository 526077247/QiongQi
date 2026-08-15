$paths = @(
  "HKLM:\SOFTWARE\EpicGames\Unreal Engine\5.5",
  "HKLM:\SOFTWARE\EpicGames\Unreal Engine\5.4",
  "HKLM:\SOFTWARE\EpicGames\Unreal Engine",
  "HKCU:\SOFTWARE\Epic Games\Unreal Engine\5.5"
)
foreach ($p in $paths) {
  $item = Get-ItemProperty $p -ErrorAction SilentlyContinue
  if ($item) {
    Write-Output "REG: $p"
    $item.PSObject.Properties | ForEach-Object { Write-Output "  $($_.Name) = $($_.Value)" }
  }
}
Write-Output "--- Epic folders ---"
Get-ChildItem "C:\Program Files\Epic Games" -Directory -ErrorAction SilentlyContinue | ForEach-Object { Write-Output $_.FullName }
Get-ChildItem "D:\Epic Games","E:\Epic Games","G:\Epic Games" -Directory -ErrorAction SilentlyContinue | ForEach-Object { Write-Output $_.FullName }
Write-Output "--- UE_ folders on drives ---"
Get-ChildItem "C:\","D:\","E:\","G:\","F:\" -Directory -ErrorAction SilentlyContinue | Where-Object { $_.Name -like "UE*" } | ForEach-Object { Write-Output $_.FullName }
