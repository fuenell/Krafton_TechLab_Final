# Visual Studio Pre-Build Event용 코드 생성 스크립트

# ---------------------------------------------------------
# [핵심] 파이썬 print() 실행 시 한글 깨짐 방지
# ---------------------------------------------------------
$env:PYTHONUTF8 = "1"
$env:PYTHONIOENCODING = "utf-8"

# 배치파일의 %~dp0 (현재 스크립트 위치)
$ScriptDir = $PSScriptRoot

# 경로 설정
$EmbeddedPython = Join-Path $ScriptDir "..\..\ThirdParty\Python\python.exe"
$GenerateScript = Join-Path $ScriptDir "generate.py"
$EnsureScript   = Join-Path $ScriptDir "EnsurePython.ps1"

# 1. 프로젝트 내장 Python 체크
if (Test-Path $EmbeddedPython) {
    # 경로 정규화 후 실행
    $ExecPath = (Resolve-Path $EmbeddedPython).Path
    & $ExecPath $GenerateScript $args
    exit $LASTEXITCODE
}

# 2. 시스템 Python 체크
if (Get-Command "py" -ErrorAction SilentlyContinue) {
    & py $GenerateScript $args
    exit $LASTEXITCODE
}

# 3. Python 없음 - 자동 다운로드 시도
Write-Host "[CodeGen] Python not found. Attempting automatic setup..."
& $EnsureScript

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Failed to setup Python automatically."
    Write-Host "Please install Python manually or check network connection."
    exit 1
}

# 4. 다운로드된 Python으로 재시도
if (Test-Path $EmbeddedPython) {
    $ExecPath = (Resolve-Path $EmbeddedPython).Path
    & $ExecPath $GenerateScript $args
    exit $LASTEXITCODE
}

Write-Host "[ERROR] Python setup succeeded but executable not found!"
exit 1