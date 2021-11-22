# TrustedInjector
This is a LoadLibrary injector for Counter-Strike: Global Offensive.

## Information
It automatically bypasses trusted mode by removing hooks on various Win32 functions.

## Usage
To inject a DLL, bypassing trusted mode automatically.
>Note: You can also invoke this behavior by dragging and dropping the DLL onto the executable.
```powershell
PS C:\Example> .\TrustedInjector.exe  C:\Osiris.dll
```
Or to only disable trusted mode, without injecting a DLL.
```powershell
PS C:\Example> .\TrustedInjector.exe  bypass
```
## Compiling
I used Visual Studio 2022, so compile with that in x86 mode.

## Credits
 - me (lol)