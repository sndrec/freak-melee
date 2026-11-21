param(
    [string] $Iso,
    [string] $Channel,
    [switch] $CheckOnly,
    [switch] $Rollback,
    [switch] $Hidden,
    [switch] $Mute,
    [switch] $AgentMcp
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$install = Split-Path -Parent $MyInvocation.MyCommand.Path
$versions = Join-Path $install 'versions'
$statePath = Join-Path $install 'state.json'
$releaseKeyModulus = '@FREAK_MELEE_RELEASE_KEY@'
$launcherIssuedMs = [long]'@FREAK_MELEE_RELEASE_TIME@'
$mutex = New-Object System.Threading.Mutex($false, 'Local\FreakMeleeFriendLauncher')

function Test-ReleaseId([string] $id) {
    return $id -match '^[a-zA-Z0-9._-]{1,64}\z' -and -not $id.EndsWith('.') -and
           $id -notmatch '^(CON|PRN|AUX|NUL|COM[0-9]|LPT[0-9])(\.|$)'
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

function Get-UpdateResponse($client, [Uri] $uri, $completion, $token) {
    for ($hops = 0; $hops -le 5; ++$hops) {
        if (-not $uri.IsAbsoluteUri -or $uri.Scheme -ne 'https' -or -not $uri.Host -or $uri.UserInfo) {
            throw [Security.SecurityException]::new('The update service requires an HTTPS URL without credentials.')
        }
        $response = $client.GetAsync($uri, $completion, $token).GetAwaiter().GetResult()
        try {
            if ([int]$response.StatusCode -in 301, 302, 303, 307, 308) {
                if (-not $response.Headers.Location) { throw 'The update redirect is missing its destination.' }
                $uri = [Uri]::new($uri, $response.Headers.Location)
            } else {
                $response.EnsureSuccessStatusCode() | Out-Null
                $result = $response
                $response = $null
                return $result
            }
        } finally {
            if ($response) { $response.Dispose() }
        }
    }
    throw 'Too many update redirects.'
}

function Read-UpdateBytes([string] $location, [int] $maximum) {
    if ($location -match '^https://') {
        Add-Type -AssemblyName System.Net.Http
        $handler = [Net.Http.HttpClientHandler]::new()
        $handler.AllowAutoRedirect = $false
        $client = [Net.Http.HttpClient]::new($handler)
        $response = $null
        try {
            $client.MaxResponseContentBufferSize = $maximum
            $client.Timeout = [TimeSpan]::FromMinutes(2)
            $response = Get-UpdateResponse $client ([Uri]$location) ([Net.Http.HttpCompletionOption]::ResponseContentRead) ([Threading.CancellationToken]::None)
            return ,$response.Content.ReadAsByteArrayAsync().GetAwaiter().GetResult()
        } finally {
            if ($response) { $response.Dispose() }
            $client.Dispose()
        }
    }
    if ($location -match '^[a-z]+://') { throw 'Only HTTPS update channels are supported.' }
    $stream = [IO.File]::OpenRead([IO.Path]::GetFullPath($location))
    try {
        if ($stream.Length -gt $maximum) { throw 'Update metadata is too large.' }
        $bytes = [byte[]]::new($stream.Length)
        $offset = 0
        while ($offset -lt $bytes.Length) {
            $read = $stream.Read($bytes, $offset, $bytes.Length - $offset)
            if (-not $read) { throw 'Incomplete update metadata.' }
            $offset += $read
        }
        return ,$bytes
    } finally { $stream.Dispose() }
}

function Resolve-Archive([string] $manifestLocation, [string] $archive) {
    if ($archive -notmatch '^freak-melee-[a-zA-Z0-9._-]{1,64}\.zip\z') {
        throw 'The manifest has an invalid archive name.'
    }
    if ($manifestLocation -match '^https://') {
        return ([Uri]::new([Uri] $manifestLocation, $archive)).AbsoluteUri
    }
    return Join-Path (Split-Path -Parent ([IO.Path]::GetFullPath($manifestLocation))) $archive
}

function Receive-Archive([uri] $source, [string] $path, [long] $expectedBytes) {
    Add-Type -AssemblyName System.Net.Http
    $handler = [Net.Http.HttpClientHandler]::new()
    $handler.AllowAutoRedirect = $false
    $client = [Net.Http.HttpClient]::new($handler)
    $timeout = [Threading.CancellationTokenSource]::new([TimeSpan]::FromMinutes(10))
    $response = $inputStream = $outputStream = $null
    $receivedBytes = 0L
    $buffer = [byte[]]::new(128KB)
    $refresh = [Diagnostics.Stopwatch]::StartNew()
    $progressShown = $false
    $ProgressPreference = 'Continue'
    try {
        $response = Get-UpdateResponse $client $source ([Net.Http.HttpCompletionOption]::ResponseHeadersRead) $timeout.Token
        $inputStream = $response.Content.ReadAsStreamAsync().GetAwaiter().GetResult()
        $outputStream = [IO.File]::Open($path, [IO.FileMode]::CreateNew)
        while ($true) {
            $count = $inputStream.ReadAsync($buffer, 0, $buffer.Length,
                                            $timeout.Token).GetAwaiter().GetResult()
            if ($count -eq 0) { break }
            $receivedBytes += $count
            if ($receivedBytes -gt $expectedBytes) { throw 'Update exceeds the manifest size.' }
            $outputStream.Write($buffer, 0, $count)
            # The default PowerShell download progress redraws on each chunk.
            if ($refresh.ElapsedMilliseconds -ge 500) {
                Write-Progress -Activity 'Downloading Freak Melee' -Status (
                    '{0:N1} of {1:N1} MB' -f ($receivedBytes / 1000000), ($expectedBytes / 1000000)
                ) -PercentComplete ([int] (100 * $receivedBytes / $expectedBytes))
                $progressShown = $true
                $refresh.Restart()
            }
        }
    } finally {
        if ($outputStream) { $outputStream.Dispose() }
        if ($inputStream) { $inputStream.Dispose() }
        if ($response) { $response.Dispose() }
        $timeout.Dispose()
        $client.Dispose()
        if ($progressShown) { Write-Progress -Activity 'Downloading Freak Melee' -Completed }
    }
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
    foreach ($part in $path.Split('/')) {
        if (-not $part -or $part.EndsWith('.') -or $part.EndsWith(' ') -or
            $part.IndexOfAny([IO.Path]::GetInvalidFileNameChars()) -ge 0 -or
            $part -match '^(CON|PRN|AUX|NUL|COM[0-9\u00b9\u00b2\u00b3]|LPT[0-9\u00b9\u00b2\u00b3])(\.|$)') {
            throw "Invalid release file name: $path"
        }
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
    $manifestBytes = Read-UpdateBytes $location 4MB
    $sha = [Security.Cryptography.SHA256]::Create()
    try { $digest = [BitConverter]::ToString($sha.ComputeHash($manifestBytes)).Replace('-', '').ToLowerInvariant() }
    finally { $sha.Dispose() }
    $signatureName = "manifest-$digest.sig"
    if ($location -match '^https://') {
        $signatureLocation = ([Uri]::new([Uri]$location, $signatureName)).AbsoluteUri
    } else {
        $signatureLocation = Join-Path ([IO.Path]::GetDirectoryName([IO.Path]::GetFullPath($location))) $signatureName
    }
    $signature = Read-UpdateBytes $signatureLocation 384
    $key = [Security.Cryptography.RSA]::Create()
    try {
        $key.FromXmlString("<RSAKeyValue><Modulus>$releaseKeyModulus</Modulus><Exponent>AQAB</Exponent></RSAKeyValue>")
        if ($signature.Length -ne 384 -or -not $key.VerifyData($manifestBytes, $signature,
                [Security.Cryptography.HashAlgorithmName]::SHA256, [Security.Cryptography.RSASignaturePadding]::Pkcs1)) {
            throw 'The release signature is invalid.'
        }
    } finally { $key.Dispose() }
    $manifest = ([Text.UTF8Encoding]::new($false, $true)).GetString($manifestBytes) | ConvertFrom-Json
    $release = [string] $manifest.release
    Assert-ReleaseId $release
    if ($manifest.issued_ms -isnot [long] -and $manifest.issued_ms -isnot [int]) { throw 'Invalid release timestamp.' }
    $issued = [long]$manifest.issued_ms
    if ($issued -lt $launcherIssuedMs -or $issued -lt [long]$state.latest_ms) { throw 'The channel returned an older release.' }
    if ($issued -eq [long]$state.latest_ms -and $release -ne $state.current -and $release -ne $state.blocked) {
        throw 'That release timestamp already belongs to another build.'
    }
    if ($manifest.sha256 -notmatch '^[0-9a-fA-F]{64}$' -or
        [long] $manifest.size -le 0 -or [long] $manifest.size -gt 1GB -or
        @($manifest.files).Count -lt 4 -or @($manifest.files).Count -gt 8192) {
        throw 'The release manifest is incomplete.'
    }
    $names = [Collections.Generic.Dictionary[string,long]]::new([StringComparer]::OrdinalIgnoreCase)
    $expanded = 0L
    foreach ($entry in @($manifest.files)) {
        Test-FileEntry $entry
        if ([long] $entry.size -gt 1GB - $expanded) { throw 'Expanded release is too large.' }
        $expanded += [long] $entry.size
        $names.Add([string] $entry.path, [long] $entry.size)
    }
    if (-not $names.ContainsKey('melee_native.exe') -or
        -not $names.ContainsKey('native_content_tool.exe') -or
        -not $names.ContainsKey('ui/freak_shell.rml') -or
        -not $names.ContainsKey('freak_launcher.ps1')) {
        throw 'The release is missing its game, Workshop tool, UI, or launcher.'
    }
    if ($release -eq $state.blocked) {
        Write-Host "Skipping rolled-back release $release until a newer build is published."
        return $state
    }
    if ($release -eq $state.current) {
        try { $verified = Test-Version (Join-Path $versions $release) $manifest }
        catch { throw [Security.SecurityException]::new("Could not verify installed release $release.", $_.Exception) }
        if (-not $verified) {
            throw [Security.SecurityException]::new("Installed release $release failed verification. Use -Rollback or reinstall it.")
        }
        if ($issued -gt [long]$state.latest_ms) {
            $state | Add-Member -NotePropertyName latest_ms -NotePropertyValue $issued -Force
            Save-State $state
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
            $phase = [Diagnostics.Stopwatch]::StartNew()
            if ($source -match '^https://') {
                Receive-Archive $source $archiveFile ([long] $manifest.size)
            } else {
                Copy-Item -LiteralPath $source -Destination $archiveFile
            }
            Write-Host ("Downloaded {0:N1} MB in {1:N1}s. Verifying and installing..." -f
                        ($manifest.size / 1000000), $phase.Elapsed.TotalSeconds)
            $phase.Restart()
            if ((Get-Item -LiteralPath $archiveFile).Length -ne [long] $manifest.size -or
                (Get-Sha256 $archiveFile) -ne ([string] $manifest.sha256)) {
                throw 'Downloaded release hash or size does not match the manifest.'
            }
            Add-Type -AssemblyName System.IO.Compression.FileSystem
            $zip = [IO.Compression.ZipFile]::OpenRead($archiveFile)
            try {
                $entries = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
                foreach ($entry in $zip.Entries) {
                    if (-not $names.ContainsKey($entry.FullName) -or
                        -not $entries.Add($entry.FullName) -or
                        $entry.Length -ne $names[$entry.FullName]) {
                        throw 'Archive file list or sizes differ from the manifest.'
                    }
                }
                if ($entries.Count -ne $names.Count) { throw 'Archive is missing files.' }
                $buffer = [byte[]]::new(128KB)
                foreach ($entry in $zip.Entries) {
                    $destination = Join-Path $staging $entry.FullName
                    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destination)) | Out-Null
                    $inputStream = $entry.Open()
                    try {
                        $outputStream = [IO.File]::Open($destination, [IO.FileMode]::CreateNew)
                        try {
                            $remaining = $names[$entry.FullName]
                            while (($read = $inputStream.Read($buffer, 0, $buffer.Length)) -gt 0) {
                                if ($read -gt $remaining) { throw 'Expanded file exceeds the manifest size.' }
                                $outputStream.Write($buffer, 0, $read)
                                $remaining -= $read
                            }
                            if ($remaining -ne 0) { throw 'Expanded release file is incomplete.' }
                        } finally { $outputStream.Dispose() }
                    } finally { $inputStream.Dispose() }
                }
            } finally { $zip.Dispose() }
            if (-not (Test-Version $staging $manifest)) {
                throw 'Extracted release is incomplete or corrupt.'
            }
            Move-Item -LiteralPath $staging -Destination $target
            Write-Host ("Verified and installed in {0:N1}s." -f $phase.Elapsed.TotalSeconds)
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
        current = $release; previous = $state.current; blocked = $null; latest_ms = $issued
    }
    Save-State $newState
    Write-Host "Activated Freak Melee $release. Previous build retained."
    return $newState
}

try {
    if ($AgentMcp) {
        # Agents attach independently of the player's launcher. Select the
        # current version without updating, taking the game mutex or opening UI.
        if (-not (Test-Path -LiteralPath $statePath)) {
            throw 'No installed release found. Extract the setup download first.'
        }
        $agentState = Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
        Assert-ReleaseId ([string] $agentState.current)
        $agentDirectory = Join-Path $versions ([string] $agentState.current)
        $agentTool = Join-Path $agentDirectory 'native_content_tool.exe'
        if (-not (Test-Path -LiteralPath $agentTool -PathType Leaf)) {
            throw 'This installed build is missing its Workshop authoring tool.'
        }
        $agentProcess = New-Object System.Diagnostics.Process
        $agentProcess.StartInfo.FileName = $agentTool
        $agentProcess.StartInfo.Arguments = 'mcp'
        $agentProcess.StartInfo.WorkingDirectory = $agentDirectory
        $agentProcess.StartInfo.UseShellExecute = $false
        $agentProcess.StartInfo.CreateNoWindow = $true
        $agentProcess.StartInfo.RedirectStandardInput = $true
        $agentProcess.StartInfo.RedirectStandardOutput = $true
        $agentProcess.StartInfo.RedirectStandardError = $true
        if (-not $agentProcess.Start()) { throw 'Could not start the Workshop MCP server.' }
        # Forward bytes, without PowerShell's native text-pipeline conversion.
        # Output runs concurrently so a reply cannot block an incoming request.
        $agentOutput = $agentProcess.StandardOutput.BaseStream.CopyToAsync([Console]::OpenStandardOutput())
        $agentErrors = $agentProcess.StandardError.BaseStream.CopyToAsync([Console]::OpenStandardError())
        $agentInput = [Console]::OpenStandardInput()
        $agentBuffer = New-Object byte[] 8192
        try {
            while (($agentBytes = $agentInput.Read($agentBuffer, 0, $agentBuffer.Length)) -gt 0) {
                $agentProcess.StandardInput.BaseStream.Write($agentBuffer, 0, $agentBytes)
                $agentProcess.StandardInput.BaseStream.Flush()
            }
        } finally {
            $agentProcess.StandardInput.Close()
        }
        $agentProcess.WaitForExit()
        [System.Threading.Tasks.Task]::WaitAll(@($agentOutput, $agentErrors))
        exit $agentProcess.ExitCode
    }
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
            if ($Channel -and $Channel -notmatch '^https://') {
                throw [Security.SecurityException]::new('The installed update channel must use HTTPS.')
            }
        }
        if ($Channel) {
            try { $state = Install-Update $Channel $state }
            catch [Security.SecurityException] { throw }
            catch { Write-Warning "Update unavailable: $_. Using the installed build." }
        }
    }
    if ($CheckOnly) { exit 0 }

    $gameDir = Join-Path $versions ([string] $state.current)
    $game = Join-Path $gameDir 'melee_native.exe'
    if (-not (Test-Path -LiteralPath $game -PathType Leaf) -or
        -not (Test-Path -LiteralPath (Join-Path $gameDir 'native_content_tool.exe') -PathType Leaf) -or
        -not (Test-Path -LiteralPath (Join-Path $gameDir 'ui/freak_shell.rml') -PathType Leaf)) {
        throw "Installed build $($state.current) is missing its game, Workshop tool or UI. Try -Rollback."
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
    if ($Hidden) { $process.StartInfo.Arguments += ' --hidden' }
    if ($Mute) { $process.StartInfo.Arguments += ' --mute' }
    $process.StartInfo.UseShellExecute = $false
    if (-not $process.Start()) { throw 'Could not launch Freak Melee.' }
    $process.WaitForExit()
    exit $process.ExitCode
} finally {
    try { $mutex.ReleaseMutex() } catch [System.ApplicationException] { }
    $mutex.Dispose()
}
