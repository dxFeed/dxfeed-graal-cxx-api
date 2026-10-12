param(
  [Parameter(Mandatory)][string]$Supervisor,
  [Parameter(Mandatory)][string]$Child,
  [Parameter(Mandatory)][string]$LogDirectory,
  [string[]]$ChildArguments = @(),
  [ValidateSet('none', 'dashboard')][string]$Drive = 'none',
  [switch]$HostWheel,
  [int]$ExpectedExit = 0,
  [int]$DeadlineMs = 15000,
  [int]$QuitMs = 4500,
  [string]$Terminal = 'wt.exe'
)
$ErrorActionPreference = 'Stop'
$Supervisor = (Resolve-Path -LiteralPath $Supervisor).Path
$Child = (Resolve-Path -LiteralPath $Child).Path
if (Test-Path -LiteralPath $LogDirectory) { throw 'Use a fresh log directory to preserve evidence' }
New-Item -ItemType Directory -Path $LogDirectory | Out-Null
$LogDirectory = (Resolve-Path -LiteralPath $LogDirectory).Path
$title = 'portals-native-' + [guid]::NewGuid().ToString()
$log = Join-Path $LogDirectory 'supervisor.log'
Add-Type @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
public static class PortalsNativeWindow {
  [StructLayout(LayoutKind.Sequential)] public struct Point { public int X, Y; }
  [StructLayout(LayoutKind.Sequential)] public struct Rect { public int Left, Top, Right, Bottom; }
  public delegate bool Callback(IntPtr window, IntPtr parameter);
  [DllImport("user32.dll")] static extern bool EnumWindows(Callback callback, IntPtr parameter);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] static extern int GetWindowText(IntPtr window, System.Text.StringBuilder text, int count);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr window, out uint pid);
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr window, uint message, IntPtr w, IntPtr l);
  [DllImport("user32.dll")] static extern bool GetClientRect(IntPtr window, out Rect rectangle);
  [DllImport("user32.dll")] static extern bool ClientToScreen(IntPtr window, ref Point point);
  public static bool PostWheel(IntPtr window, out Point point) {
    point = new Point();
    Rect rectangle;
    if (!GetClientRect(window, out rectangle) || rectangle.Right <= 0 || rectangle.Bottom <= 0) return false;
    // Use the table body of this launcher's fixed 120x40 fixture, below WT's tab strip.
    point.X = (int)(rectangle.Right * 8.5 / 120);
    point.Y = (int)(rectangle.Bottom * 9.5 / 40);
    if (!ClientToScreen(window, ref point)) return false;
    var position = new IntPtr((point.Y << 16) | (point.X & 65535));
    return PostMessage(window, 0x020A, new IntPtr(-120 << 16), position);
  }
  public static IntPtr[] Find(string title) {
    var result = new List<IntPtr>();
    EnumWindows((window, parameter) => {
      var text = new System.Text.StringBuilder(512);
      GetWindowText(window, text, text.Capacity);
      if (text.ToString() == title) result.Add(window);
      return true;
    }, IntPtr.Zero);
    return result.ToArray();
  }
}
'@
function Get-TerminalIdentity {
  @(Get-Process WindowsTerminal,OpenConsole -ErrorAction SilentlyContinue | ForEach-Object {
    [pscustomobject]@{ Id = $_.Id; Started = $_.StartTime.ToUniversalTime().ToString('o'); Path = $_.Path }
  })
}
function Quote-Argument([string]$value) {
  # Windows CRT quoting, including trailing backslashes and embedded quotes.
  '"' + [regex]::Replace([regex]::Replace($value, '(\\*)"', '$1$1\"'), '(\\+)$', '$1$1') + '"'
}
$baseline = @(Get-TerminalIdentity)
$baseline | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $LogDirectory 'baseline.json')
Get-FileHash -Algorithm SHA256 -LiteralPath $Supervisor,$Child | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $LogDirectory 'hashes.json')
$arguments = @('-w', $title, '--size', '120,40', 'new-tab', '--title', $title, '--suppressApplicationTitle',
  '--startingDirectory', (Split-Path -Parent $Supervisor), $Supervisor, '--log', $log, '--title', $title,
  '--deadline-ms', "$DeadlineMs", '--quit-ms', "$QuitMs", '--expected-exit', "$ExpectedExit", '--drive', $Drive,
  '--', $Child) + $ChildArguments
$command = ($arguments | ForEach-Object { Quote-Argument $_ }) -join ' '
$started = [datetime]::UtcNow
$launcher = Start-Process -FilePath $Terminal -ArgumentList $command -WindowStyle Hidden -PassThru
$until = $started.AddMilliseconds($DeadlineMs + 12000)
$runtime = @{}
$ownWindowSeen = $false
$hostWheelAttempted = $false
$hostWheelSubmitted = $false
$hostWheelDelayMs = 2200
do {
  foreach ($window in [PortalsNativeWindow]::Find($title)) {
    $ownWindowSeen = $true
    [uint32]$runtimePid = 0
    [void][PortalsNativeWindow]::GetWindowThreadProcessId($window, [ref]$runtimePid)
    if (-not $runtime.ContainsKey($runtimePid)) {
      $process = Get-Process -Id $runtimePid
      $runtime[$runtimePid] = [pscustomobject]@{ Id = $runtimePid; Path = $process.Path;
        Version = $process.FileVersion; Started = $process.StartTime.ToUniversalTime().ToString('o') }
    }
  }
  if ($HostWheel -and -not $hostWheelAttempted -and ([datetime]::UtcNow - $started).TotalMilliseconds -ge $hostWheelDelayMs) {
    $windows = @([PortalsNativeWindow]::Find($title))
    if ($windows.Count -eq 1) {
      $hostWheelAttempted = $true
      [uint32]$hostPid = 0
      [void][PortalsNativeWindow]::GetWindowThreadProcessId($windows[0], [ref]$hostPid)
      $point = [PortalsNativeWindow+Point]::new()
      $hostWheelSubmitted = [PortalsNativeWindow]::PostWheel($windows[0], [ref]$point)
      [ordered]@{ kind = 'WM_MOUSEWHEEL'; hwnd = $windows[0].ToInt64(); pid = $hostPid;
        screen_x = $point.X; screen_y = $point.Y; delta = -120; submitted = $hostWheelSubmitted;
        evidence = 'Host submission only; verify input trace and semantic result separately'
      } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $LogDirectory 'host-input.json')
    }
  }
  $text = if (Test-Path -LiteralPath $log) { Get-Content -Raw -LiteralPath $log } else { '' }
  if ($text -match '(PASS|FAIL): native application supervisor' -and [PortalsNativeWindow]::Find($title).Count -eq 0) { break }
  Start-Sleep -Milliseconds 100
} while ([datetime]::UtcNow -lt $until)
$forcedCleanup = $false
foreach ($window in [PortalsNativeWindow]::Find($title)) {
  $forcedCleanup = $true
  # Close this uniquely named fixture window only; never stop a shared WT process.
  [void][PortalsNativeWindow]::PostMessage($window, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero)
}
if ($forcedCleanup) { Start-Sleep -Milliseconds 1500 }
$after = @(Get-TerminalIdentity)
$preserved = @($baseline | Where-Object {
  $original = $_
  @($after | Where-Object { $_.Id -eq $original.Id -and $_.Started -eq $original.Started }).Count -eq 1
}).Count -eq $baseline.Count
$ownPids = @([regex]::Matches($text, '(?:harness|child)_pid=(\d+)') | ForEach-Object { [int]$_.Groups[1].Value })
$alive = @($ownPids | ForEach-Object { Get-Process -Id $_ -ErrorAction SilentlyContinue })
$gone = [PortalsNativeWindow]::Find($title).Count -eq 0
$passed = $text -match 'PASS: native application supervisor' -and $ownWindowSeen -and $gone -and
  $alive.Count -eq 0 -and $preserved -and -not $forcedCleanup -and (-not $HostWheel -or $hostWheelSubmitted)
$runtime.Values | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $LogDirectory 'runtime.json')
$after | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $LogDirectory 'after.json')
[ordered]@{ pass = $passed; title = $title; command = $command; expected_child_exit = $ExpectedExit;
  baseline_preserved = $preserved; own_window_seen = $ownWindowSeen; own_window_gone = $gone;
  own_pid_alive = $alive.Count; forced_cleanup = $forcedCleanup; launcher_pid = $launcher.Id
  host_wheel_requested = [bool]$HostWheel; host_wheel_submitted = $hostWheelSubmitted
  host_wheel_attempted = $hostWheelAttempted
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $LogDirectory 'launch.json')
Get-Content -LiteralPath $log -ErrorAction SilentlyContinue
if ($passed) { exit 0 }
exit 1
