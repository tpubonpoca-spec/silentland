# dppbotcpp Release Files

## Executables

### dppbotcpp_studio.exe
Main GUI application with Win32 controls.
- Multi-pack hero analysis
- Merge workflow with priority control
- VPK export

### dppbotcpp_modern.exe (WIP)
Modern GUI with custom black minimalist design.
- Direct2D rendering
- Custom window frame
- Smooth animations
- Work in progress - full functionality coming soon

### dppbotcpp_cli.exe
Command-line interface for automation.
- `scan` - Analyze VPK pack
- `extract` - Export hero from pack

## Required DLLs

These DLLs are required for all executables:
- `libgcc_s_seh-1.dll` - GCC runtime
- `libstdc++-6.dll` - C++ standard library
- `libwinpthread-1.dll` - Threading support

## Usage

### GUI
Double-click `dppbotcpp_studio.exe` to launch the main application.

### CLI Examples
```powershell
# Scan a pack
.\dppbotcpp_cli.exe scan --vpk C:\path\to\pak_dir.vpk

# Extract a hero
.\dppbotcpp_cli.exe extract --vpk C:\path\to\pak_dir.vpk --hero shadow_fiend --output C:\output
```

## Recent Improvements (2026-05-20)
- Fixed merged manifest generation
- Added external VPK chunk support (*_000.vpk, *_001.vpk)
- Configurable merge precedence with Move Up/Down buttons
- Started modern black minimalist GUI redesign
