# Usage: crop.ps1 <in.png> <out.png> <x> <y> <w> <h> [scale]
param($In, $Out, [int]$X, [int]$Y, [int]$W, [int]$H, [double]$Scale = 2)
Add-Type -AssemblyName System.Drawing
$src = [System.Drawing.Bitmap]::FromFile((Resolve-Path $In).Path)
$dst = New-Object System.Drawing.Bitmap ([int]($W*$Scale)), ([int]($H*$Scale))
$g = [System.Drawing.Graphics]::FromImage($dst)
$g.InterpolationMode = 'HighQualityBicubic'
$g.DrawImage($src, (New-Object System.Drawing.Rectangle 0,0,$dst.Width,$dst.Height), $X, $Y, $W, $H, 'Pixel')
$dst.Save((Join-Path (Get-Location) $Out), [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $dst.Dispose(); $src.Dispose()
