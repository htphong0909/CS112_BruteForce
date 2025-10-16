import os
import subprocess
from pathlib import Path

SRC = Path("src")
BIN = Path("bin")
LOGS = Path("logs")

GPP = "g++"
CFLAGS = ["-static", "-Os", "-Wall"]
LIBS = ["-lgdi32"]

def build_missing():
    if not SRC.exists():
        print(f"[ERROR] Không thấy thư mục {SRC}")
        return 1
    BIN.mkdir(exist_ok=True)
    LOGS.mkdir(exist_ok=True)

    # Liệt kê goBT*.cpp
    sources = sorted(SRC.glob("goBT*.cpp"))
    if not sources:
        print(f"[INFO] Không có file khớp: {SRC / 'goBT*.cpp'}")
        return 0

    # Kiểm tra g++ có trong PATH
    try:
        subprocess.run([GPP, "--version"], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except Exception:
        print('[ERROR] Không tìm thấy "g++" trong PATH.')
        return 1

    built = 0
    for cpp in sources:
        name = cpp.stem                 # goBT12
        exe  = BIN / f"{name}.exe"
        xx   = name.replace("goBT", "") # 12
        runner = BIN / f"run_{name}.cmd"

        if not exe.exists():
            print(f"[BUILD] {cpp} -> {exe}")
            cmd = [GPP, *CFLAGS, "-o", str(exe), str(cpp), *LIBS]
            try:
                subprocess.run(cmd, check=True)
            except subprocess.CalledProcessError:
                print(f"[ERROR] Build failed: {cpp}")
                continue
            make_runner(runner, name, xx)
            print(f"[OK]    Built {exe}")
            print(f"[OK]    Created admin runner {runner}\n")
            built += 1
        else:
            if not runner.exists():
                make_runner(runner, name, xx)
                print(f"[SKIP] {exe} đã tồn tại. Tạo mới runner: {runner}")
            else:
                print(f"[SKIP] {exe} đã tồn tại. Bỏ qua.")

    print("\n===== Done =====")
    return 0

def make_runner(runner_path: Path, base_name: str, xx: str):
    # Runner auto-elevate bằng fltmc, ghi stderr vào logs\<xx>.log
    runner_path.write_text(
        "@echo off\n"
        "rem ==== Auto-elevate to Administrator (UAC) ====\n"
        "fltmc >nul 2>&1\n"
        "if %errorlevel% neq 0 (\n"
        "  powershell -NoProfile -ExecutionPolicy Bypass -Command "
        "\"Start-Process -FilePath 'cmd.exe' -Verb RunAs -WorkingDirectory '%cd%' "
        "-ArgumentList @('/c','\"%~f0\"','%*')\"\n"
        "  exit /b\n"
        ")\n"
        "rem ==== You are admin now; run target and log stderr ====\n"
        "setlocal\n"
        "set \"BIN=%~dp0\"\n"
        "set \"ROOT=%BIN%..\"\n"
        "set \"LOGDIR=%ROOT%\\logs\"\n"
        "if not exist \"%LOGDIR%\" mkdir \"%LOGDIR%\"\n"
        f"\"%BIN%{base_name}.exe\" %* 2> \"%LOGDIR%\\{xx}.log\"\n",
        encoding="utf-8",
    )

if __name__ == "__main__":
    raise SystemExit(build_missing())
