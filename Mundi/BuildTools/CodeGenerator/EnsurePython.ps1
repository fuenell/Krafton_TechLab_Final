# Python Embeddable Package 자동 다운로드 및 설정
# 프로젝트 빌드 시 필요한 Python 환경을 자동으로 구성합니다.

param(
    [string]$PythonVersion = "3.11.9",
    [string]$ProjectRoot = (Split-Path $PSScriptRoot -Parent | Split-Path -Parent)
)

$ErrorActionPreference = "Stop"

$PythonDir = Join-Path $ProjectRoot "ThirdParty\Python"
$PythonExe = Join-Path $PythonDir "python.exe"

# ---------------------------------------------------------
# [중요] 한글 경로 인코딩 에러 방지 설정
# ---------------------------------------------------------
$env:PYTHONUTF8 = "1"
$env:PYTHONIOENCODING = "utf-8"

# 1. Python이 없으면 다운로드 및 설치
if (-not (Test-Path $PythonExe)) {
    Write-Host "[Setup] Python not found. Downloading Python $PythonVersion (embeddable)..."

    $PythonUrl = "https://www.python.org/ftp/python/$PythonVersion/python-$PythonVersion-embed-amd64.zip"
    $ZipFile = Join-Path $env:TEMP "python-embed.zip"

    try {
        if (-not (Test-Path $PythonDir)) {
            New-Item -ItemType Directory -Force -Path $PythonDir | Out-Null
        }

        # Python 다운로드
        Write-Host "[Download] $PythonUrl"
        Invoke-WebRequest -Uri $PythonUrl -OutFile $ZipFile -UseBasicParsing

        # 압축 해제
        Write-Host "[Extract] Extracting to $PythonDir"
        Expand-Archive -Path $ZipFile -DestinationPath $PythonDir -Force

        # get-pip.py 다운로드
        $GetPipUrl = "https://bootstrap.pypa.io/get-pip.py"
        $GetPipPath = Join-Path $PythonDir "get-pip.py"
        Write-Host "[Download] get-pip.py"
        Invoke-WebRequest -Uri $GetPipUrl -OutFile $GetPipPath -UseBasicParsing

        # python311._pth 파일 수정 (site-packages 활성화)
        $PthFile = Get-ChildItem -Path $PythonDir -Filter "python*._pth" | Select-Object -First 1
        if ($PthFile) {
            $PthContent = Get-Content $PthFile.FullName
            $PthContent = $PthContent -replace "^#import site", "import site"
            $PthContent | Set-Content $PthFile.FullName
            Write-Host "[Config] Enabled site-packages in $($PthFile.Name)"
        }

        # pip 설치
        Write-Host "[Install] Installing pip..."
        & $PythonExe $GetPipPath --no-warn-script-location 2>&1 | Out-Null

        # 임시 파일 정리
        Remove-Item $ZipFile -Force -ErrorAction SilentlyContinue
        Remove-Item $GetPipPath -Force -ErrorAction SilentlyContinue
        
    } catch {
        Write-Error "[ERROR] Failed to download/setup Python: $_"
        exit 1
    }
} else {
    Write-Host "[OK] Python already exists at $PythonExe"
}

# 2. 패키지 설치 (Python이 있어도 무조건 실행)
try {
    $RequirementsFile = Join-Path $PSScriptRoot "requirements.txt"
    if (Test-Path $RequirementsFile) {
        Write-Host "[Install] Checking/Installing packages from requirements.txt..."
        
        # pip 실행 시에도 인코딩 강제 적용
        & $PythonExe -m pip install -r $RequirementsFile --upgrade --no-warn-script-location 2>&1 | Out-Null
        
        Write-Host "[Install] Package check complete."
    } else {
        Write-Warning "[Warning] requirements.txt not found at $RequirementsFile"
    }
    
    exit 0
} catch {
    Write-Error "[ERROR] Failed to install packages: $_"
    exit 1
}