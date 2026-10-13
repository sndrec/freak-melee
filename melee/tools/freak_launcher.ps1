param(
    [string] $Iso,
    [string] $Channel,
    [switch] $CheckOnly,
    [switch] $Rollback
)

$ErrorActionPreference = 'Stop'
$install = Split-Path -Parent $MyInvocation.MyCommand.Path
$versions = Join-Path $install 'versions'
$statePath = Join-Path $install 'state.json'
$mutex = New-Object System.Threading.Mutex($false, 'Local\FreakMeleeFriendLauncher')

function Test-ReleaseId([string] $id) {
    return $id -match '^[a-zA-Z0-9._-]{1,64}$' -and $id -ne '.' -and $id -ne '..'
}

function Assert-ReleaseId([string] $id) {
    if (-not (Test-ReleaseId $id)) { throw "Invalid release ID: $id" }
}

function Get-Sha256([string] $path) {
    $stream = [IO.File]::OpenRead($path)
    $hash = [Security.Cryptography.SHA256]::Create()
    try {
        return [BitConverter]::ToString($hash.ComputeHash($stream)).Replace('-', '').ToLowerInvariant()
    } finally {
        $hash.Dispose()
        $stream.Dispose()
    }
}

function Save-State($state) {
    $temporary = "$statePath.$([guid]::NewGuid().ToString('N')).tmp"
    $backup = "$statePath.$([guid]::NewGuid().ToString('N')).bak"
    try {
        [IO.File]::WriteAllText($temporary, ($state | ConvertTo-Json -Depth 3),
                                [Text.UTF8Encoding]::new($false))
        if (Test-Path -LiteralPath $statePath) {
            [IO.File]::Replace($temporary, $statePath, $backup)
        } else {
            [IO.File]::Move($temporary, $statePath)
        }
    } finally {
        if (Test-Path -LiteralPath $temporary) { Remove-Item -LiteralPath $temporary }
        if (Test-Path -LiteralPath $backup) { Remove-Item -LiteralPath $backup }
    }
}

function Read-Location([string] $location) {
    if ($location -match '^https://') {
        return (Invoke-WebRequest -Uri $location -UseBasicParsing -TimeoutSec 120).Content
    }
    if ($location -match '^[a-z]+://') { throw 'Only HTTPS update channels are supported.' }
    return [IO.File]::ReadAllText([IO.Path]::GetFullPath($location))
}

function Resolve-Archive([string] $manifestLocation, [string] $archive) {
    if ($archive -notmatch '^freak-melee-[a-zA-Z0-9._-]{1,64}\.zip$') {
        throw 'The manifest has an invalid archive name.'
    }
    if ($manifestLocation -match '^https://') {
        return ([Uri]::new([Uri] $manifestLocation, $archive)).AbsoluteUri
    }
    return Join-Path (Split-Path -Parent ([IO.Path]::GetFullPath($manifestLocation))) $archive
}

function Test-FileEntry($entry) {
    $path = [string] $entry.path
    if (-not $path -or $path.Contains('\') -or $path.StartsWith('/') -or
        $path.Contains(':') -or ($path -split '/') -contains '..' -or
        ($path -split '/') -contains '.' -or
        [string] $entry.sha256 -notmatch '^[0-9a-fA-F]{64}$' -or
        [long] $entry.size -lt 0) {
        throw "Invalid release file entry: $path"
    }
}

function Test-Version([string] $directory, $manifest) {
    foreach ($entry in @($manifest.files)) {
        $path = Join-Path $directory ([string] $entry.path).Replace('/', '\')
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { return $false }
        $file = Get-Item -LiteralPath $path
        if ($file.Length -ne [long] $entry.size -or
            (Get-Sha256 $path) -ne ([string] $entry.sha256)) { return $false }
    }
    return $true
}

function Install-Update([string] $location, $state) {
    $manifest = (Read-Location $location) | ConvertFrom-Json
    $release = [string] $manifest.release
    Assert-ReleaseId $release
    if ($manifest.sha256 -notmatch '^[0-9a-fA-F]{64}$' -or
        [long] $manifest.size -le 0 -or @($manifest.files).Count -lt 3) {
        throw 'The release manifest is incomplete.'
    }
    $names = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
    foreach ($entry in @($manifest.files)) {
        Test-FileEntry $entry
        if (-not $names.Add([string] $entry.path)) { throw 'Duplicate release file.' }
    }
    if (-not $names.Contains('melee_native.exe') -or
        -not $names.Contains('ui/freak_shell.rml') -or
        -not $names.Contains('freak_launcher.ps1')) {
        throw 'The release is missing its game, UI, or launcher.'
    }
    if ($release -eq $state.blocked) {
        Write-Host "Skipping rolled-back release $release until a newer build is published."
        return $state
    }
    if ($release -eq $state.current) {
        if (-not (Test-Version (Join-Path $versions $release) $manifest)) {
            throw "Installed release $release no longer matches the manifest."
        }
        return $state
    }

    $target = Join-Path $versions $release
    if (-not (Test-Path -LiteralPath $target)) {
        $scratch = Join-Path $install ('.download-' + [guid]::NewGuid().ToString('N'))
        $archiveFile = Join-Path $scratch 'release.zip'
        $staging = Join-Path $scratch 'unpacked'
        New-Item -ItemType Directory -Path $staging -Force | Out-Null
        try {
            $source = Resolve-Archive $location ([string] $manifest.archive)
            Write-Host "Downloading Freak Melee $release..."
            if ($source -match '^https://') {
                Invoke-WebRequest -Uri $source -OutFile $archiveFile -UseBasicParsing -TimeoutSec 600
            } else {
                Copy-Item -LiteralPath $source -Destination $archiveFile
            }
            if ((Get-Item -LiteralPath $archiveFile).Length -ne [long] $manifest.size -or
                (Get-Sha256 $archiveFile) -ne ([string] $manifest.sha256)) {
                throw 'Downloaded release hash or size does not match the manifest.'
            }
            Add-Type -AssemblyName System.IO.Compression.FileSystem
            $zip = [IO.Compression.ZipFile]::OpenRead($archiveFile)
            try {
                $entries = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
                foreach ($entry in $zip.Entries) {
                    if (-not $names.Contains($entry.FullName) -or
                        -not $entries.Add($entry.FullName)) {
                        throw 'Archive contains an unexpected or duplicate entry.'
                    }
                }
                if ($entries.Count -ne $names.Count) { throw 'Archive is missing files.' }
            } finally { $zip.Dispose() }
            [IO.Compression.ZipFile]::ExtractToDirectory($archiveFile, $staging)
            if (-not (Test-Version $staging $manifest)) {
                throw 'Extracted release is incomplete or corrupt.'
            }
            Move-Item -LiteralPath $staging -Destination $target
        } finally {
            if (Test-Path -LiteralPath $scratch) {
                Remove-Item -LiteralPath $scratch -Recurse -Force
            }
        }
    } elseif (-not (Test-Version $target $manifest)) {
        throw "Installed release $release is incomplete; refusing to activate it."
    }
    Copy-Item -LiteralPath (Join-Path $target 'freak_launcher.ps1') `
              -Destination (Join-Path $install 'freak_launcher.ps1') -Force
    $newState = [pscustomobject]@{
        current = $release; previous = $state.current; blocked = $null
    }
    Save-State $newState
    Write-Host "Activated Freak Melee $release. Previous build retained."
    return $newState
}

try {
    if (-not $mutex.WaitOne(0)) {
        throw 'Freak Melee is already running from this launcher. Close it before updating.'
    }
    if (-not (Test-Path -LiteralPath $statePath)) {
        throw 'No installed release found. Run the friend installer first.'
    }
    $state = Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
    Assert-ReleaseId ([string] $state.current)
    if ($state.previous) { Assert-ReleaseId ([string] $state.previous) }
    if ($state.blocked) { Assert-ReleaseId ([string] $state.blocked) }
    if ($Rollback) {
        if (-not $state.previous) { throw 'No previous build is available.' }
        $previousGame = Join-Path (Join-Path $versions $state.previous) 'melee_native.exe'
        if (-not (Test-Path -LiteralPath $previousGame -PathType Leaf)) {
            throw "Previous build $($state.previous) is missing."
        }
        $old = $state.current
        $state.current = $state.previous
        $state.previous = $old
        $state | Add-Member -NotePropertyName blocked -NotePropertyValue $old -Force
        Save-State $state
        Write-Host "Restored Freak Melee $($state.current)."
    } else {
        $channelFile = Join-Path $install 'channel.txt'
        if (-not $Channel -and (Test-Path -LiteralPath $channelFile)) {
            $Channel = (Get-Content -LiteralPath $channelFile -Raw).Trim()
        }
        if ($Channel) {
            try { $state = Install-Update $Channel $state }
            catch { Write-Warning "Update unavailable: $_. Using the installed build." }
        }
    }
    if ($CheckOnly) { exit 0 }

    $gameDir = Join-Path $versions ([string] $state.current)
    $game = Join-Path $gameDir 'melee_native.exe'
    if (-not (Test-Path -LiteralPath $game -PathType Leaf) -or
        -not (Test-Path -LiteralPath (Join-Path $gameDir 'ui/freak_shell.rml') -PathType Leaf)) {
        throw "Installed build $($state.current) is missing its game or UI. Try -Rollback."
    }
    $settings = Join-Path $env:LOCALAPPDATA 'Freak Melee'
    $isoSetting = Join-Path $settings 'iso-path.txt'
    if (-not $Iso -and (Test-Path -LiteralPath $isoSetting)) {
        $Iso = (Get-Content -LiteralPath $isoSetting -Raw).Trim()
    }
    if (-not $Iso -or -not (Test-Path -LiteralPath $Iso -PathType Leaf)) {
        Add-Type -AssemblyName System.Windows.Forms
        $picker = New-Object System.Windows.Forms.OpenFileDialog
        $picker.Title = 'Select your Melee USA v1.02 ISO'
        $picker.Filter = 'Disc images (*.iso)|*.iso|All files (*.*)|*.*'
        if ($picker.ShowDialog() -ne [System.Windows.Forms.DialogResult]::OK) {
            throw 'A Melee USA v1.02 ISO is required to launch.'
        }
        $Iso = $picker.FileName
    }
    New-Item -ItemType Directory -Path $settings -Force | Out-Null
    [IO.File]::WriteAllText($isoSetting, [IO.Path]::GetFullPath($Iso))
    $process = New-Object System.Diagnostics.Process
    $process.StartInfo.FileName = $game
    $process.StartInfo.WorkingDirectory = $gameDir
    $process.StartInfo.Arguments = '--iso "' + $Iso.Replace('"', '') + '"'
    $process.StartInfo.UseShellExecute = $false
    if (-not $process.Start()) { throw 'Could not launch Freak Melee.' }
    $process.WaitForExit()
    exit $process.ExitCode
} finally {
    try { $mutex.ReleaseMutex() } catch [System.ApplicationException] { }
    $mutex.Dispose()
}
