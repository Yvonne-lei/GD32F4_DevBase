$ErrorActionPreference = 'Stop'

# 工具链已复制到 ASCII 路径 D:\arm\gcc，绕开中文用户名的编码问题
$GCC = 'D:\arm\gcc\bin\arm-none-eabi-gcc.exe'
$OBJCOPY = 'D:\arm\gcc\bin\arm-none-eabi-objcopy.exe'

$OBJDIR = $PSScriptRoot
$PROJ = Split-Path $PSScriptRoot -Parent
# 输出到 ASCII 路径，避免中文项目路径导致的 map/elf 写入失败
$OUTDIR = 'D:\arm\out'
New-Item -ItemType Directory -Path $OUTDIR -Force | Out-Null
$OUT = Join-Path $OUTDIR 'GD32F4_DevBase.elf'
# 链接脚本直接读中文路径（与 .o 一样，ld 可正常读取）
$LD = Join-Path $PROJ 'User\GD32F407xG.ld'

$objs = @(Get-ChildItem -Path $OBJDIR -Filter '*.o' | ForEach-Object { $_.FullName })
Write-Output ("link: {0} object files" -f $objs.Count)

$args = @(
  '-mcpu=cortex-m4', '-mfpu=fpv4-sp-d16', '-mfloat-abi=softfp', '-mthumb',
  '-T', $LD,
  '-Wl,--gc-sections', '-Wl,--print-memory-usage',
  '--specs=nosys.specs', '--specs=nano.specs',
  '-o', $OUT
) + $objs + @('-lm')

& $GCC @args
if ($LASTEXITCODE -ne 0) {
  Write-Error ("link failed, exit code {0}" -f $LASTEXITCODE)
  exit 1
}
Write-Output ("link OK: {0}" -f $OUT)

& $OBJCOPY -O ihex $OUT (Join-Path $OUTDIR 'GD32F4_DevBase.hex')
& $OBJCOPY -O binary $OUT (Join-Path $OUTDIR 'GD32F4_DevBase.bin')
Write-Output "generated .elf / .hex / .bin"
