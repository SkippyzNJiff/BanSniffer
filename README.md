# BanSniffer

A Windows utility for monitoring and comparing hardware serial numbers to detect changes in system identifiers.

## Overview

BanSniffer is a command-line tool designed to help system administrators and security professionals track changes in hardware serial numbers. The tool allows you to save a baseline of your system's hardware serials and later compare current serials against the saved baseline to identify any modifications.

## Features

- **Save Current Serials**: Captures and stores current hardware serial numbers
- **Compare Serials**: Compares current hardware serials against previously saved baselines
- **Change Detection**: Identifies which serials have been modified, added, or removed
- **Simple Menu Interface**: Easy-to-use numbered menu system

## Use Cases

- **System Administration**: Monitor hardware changes in managed environments
- **Ban Detection**: Make sure eac doesnt sig ur pc
- **Forensic Analysis**: Track hardware identifier modifications

## How It Works

1. **Baseline Creation**: Use option (2) to save your current hardware serials to a file
2. **System Modification**: Make changes to your system (hardware replacement, serial spoofing, etc.)
3. **Comparison**: Use option (1) to compare current serials against your saved baseline
4. **Analysis**: Review the output to see exactly what changed

## Building the Project

### Prerequisites

- Visual Studio 2019 or later
- Windows SDK

### Build Instructions

1. **Clone the repository**:
   ```cmd
   git clone <your-repo-url>
   cd BanSniffer
   ```

2. **Open in Visual Studio**:
   - Open `BanSniffer.sln` in Visual Studio
   - Or use "Open Folder" to open the project directory

3. **Build the project**:
   - Select your desired configuration (Debug/Release)
   - Choose your target platform (x86/x64)
   - Build → Build Solution (Ctrl+Shift+B)

### Build Configuration

The project is configured to use Visual Studio's built-in build system. No CMake configuration is required.

## Usage

1. **Run the executable**:
   ```cmd
   BanSniffer.exe
   ```

2. **Choose from the menu options**:
   ```
   BanSniffer - Hardware Serial Monitor
   
   [1] Compare current serials to saved serials
   [2] Save current serials
   
   Enter your choice:
   ```

3. **Save baseline serials** (first-time setup):
   - Select option `2`
   - Serials will be saved to a local file for future comparison

4. **Compare serials** (after system changes):
   - Select option `1`
   - View the comparison results showing any changes

## Output Format

When comparing serials, the tool will display:
- **Unchanged**: Serials that remain the same
- **Modified**: Serials that have been changed (shows old → new)
- **Added**: New serials not present in the baseline
- **Removed**: Serials present in baseline but missing now

## File Storage

Serial data is stored locally in the same directory as the executable. The storage format is designed for easy parsing and human readability.

## Security Considerations

- **Administrative Privileges**: May require elevated permissions to access certain hardware information
- **Data Sensitivity**: Serial numbers can be considered sensitive system information
- **File Protection**: Consider protecting saved serial files from unauthorized access

## Supported Hardware

The tool attempts to gather serials from various hardware components including:
- Hard drives and SSDs
- Network adapters
- System board/motherboard
- CPU information
- Memory modules
- Graphics cards

*Note: Available information depends on hardware support and system permissions*

## Troubleshooting

### Common Issues

**"Access Denied" errors**:
- Run the tool as Administrator
- Check Windows permissions for hardware access

**No serials detected**:
- Verify hardware supports serial number reporting
- Save serials

**Comparison shows unexpected changes**:
- Some serials may change after system updates
- Virtual machines may have dynamic serials
- Hardware drivers can affect reported information

## Contributing

When contributing to this project:
1. Follow existing code style and conventions
2. Test changes on multiple Windows versions
3. Ensure compatibility with Visual Studio build system
4. Document any new features or changes

## License

MIT

## Disclaimer

This tool is intended for legitimate system administration and security purposes. Users are responsible for complying with all applicable laws and regulations. The authors are not responsible for any misuse of this software, nor bans from games.

---

**Version**: 1.0  
**Platform**: Windows  
**Architecture**: x86/x64
