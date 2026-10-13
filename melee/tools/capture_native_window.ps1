param(
    [Parameter(Mandatory = $true)][int]$GameProcessId,
    [Parameter(Mandatory = $true)][string]$OutputPath
)

Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class NativeWindowCapture {
    [StructLayout(LayoutKind.Sequential)]
    public struct Rect { public int Left, Top, Right, Bottom; }
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr window, out Rect rect);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr window, IntPtr dc, uint flags);
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
}
'@
[NativeWindowCapture]::SetProcessDPIAware() | Out-Null
$game = Get-Process -Id $GameProcessId -ErrorAction Stop
$bounds = New-Object NativeWindowCapture+Rect
if (-not [NativeWindowCapture]::GetWindowRect($game.MainWindowHandle, [ref]$bounds)) {
    throw 'Could not read the game window bounds.'
}
$bitmap = New-Object System.Drawing.Bitmap(($bounds.Right - $bounds.Left), ($bounds.Bottom - $bounds.Top))
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
try {
    $dc = $graphics.GetHdc()
    try {
        if (-not [NativeWindowCapture]::PrintWindow($game.MainWindowHandle, $dc, 2)) {
            throw 'Could not capture the game window.'
        }
    } finally {
        $graphics.ReleaseHdc($dc)
    }
    $bitmap.Save([IO.Path]::GetFullPath($OutputPath), [System.Drawing.Imaging.ImageFormat]::Png)
} finally {
    $graphics.Dispose()
    $bitmap.Dispose()
}
