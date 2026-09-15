import os
import sys
import shutil
import platform
import subprocess

def print_color(text, color_code):
    # ANSI escape sequences for colors
    # 92 is Green, 91 is Red, 93 is Yellow, 0 is Reset
    if sys.stdout.isatty():
        print(f"\033[{color_code}m{text}\033[0m")
    else:
        print(text)

def check_command(cmd, name):
    path = shutil.which(cmd)
    if path:
        print_color(f"[OK] {name} found: {path}", 92)
        try:
            version_output = subprocess.check_output([cmd, "--version"], stderr=subprocess.STDOUT).decode('utf-8')
            version = version_output.split('\n')[0].strip()
            print(f"     Version: {version}")
        except Exception:
            pass
        return True
    else:
        print_color(f"[FAIL] {name} not found. Please install {cmd}.", 91)
        return False

def check_terminal():
    try:
        cols, rows = os.get_terminal_size()
        print_color(f"[OK] Terminal dynamically resizable (Current: {cols}x{rows})", 92)
        return True
    except Exception:
        print_color("[WARN] Could not determine terminal size. Make sure you run this in a real terminal.", 93)
        return False

def main():
    # Enable Windows ANSI support if needed
    if os.name == 'nt':
        os.system('color')
        
    print("==========================================")
    print("        Gyro Environment Diagnostics      ")
    print("==========================================")
    print(f"Operating System: {platform.system()} {platform.release()} ({platform.architecture()[0]})")
    print("------------------------------------------")

    missing = False

    # Check compiler
    has_gcc = check_command("gcc", "GCC Compiler")
    has_clang = False
    if not has_gcc:
        has_clang = check_command("clang", "Clang Compiler")
    
    if not has_gcc and not has_clang:
        missing = True
        print_color("     -> Recommendation:", 93)
        if platform.system() == "Windows":
            print("        Install MSYS2 (https://www.msys2.org/) and run: pacman -S mingw-w64-ucrt-x86_64-gcc")
        elif platform.system() == "Linux":
            print("        Run: sudo apt install build-essential")
        elif platform.system() == "Darwin":
            print("        Run: xcode-select --install")

    # Check make
    has_make = check_command("make", "GNU Make")
    if not has_make:
        missing = True
        print_color("     -> Recommendation:", 93)
        if platform.system() == "Windows":
            print("        Install MSYS2 and run: pacman -S make")
        elif platform.system() == "Linux":
            print("        Run: sudo apt install make")

    # Check terminal
    check_terminal()

    print("------------------------------------------")
    if missing:
        print_color("ERROR: Your system is missing required dependencies to build Gyro.", 91)
        sys.exit(1)
    else:
        print_color("SUCCESS: Your system is ready to compile and run Gyro!", 92)
        print("Next steps: Run 'make' to build all demos.")

if __name__ == '__main__':
    main()
