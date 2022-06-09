<div align="center">
  <strong>IMPORTANT: This injector is NOT VAC-Safe! You have been warned!<strong>
    <br/>
  ![16ab986c-2e92-434c-9726-7636ebfe1be0](https://user-images.githubusercontent.com/72025514/172918787-f3763500-46f8-479d-b342-682754b8b8f8.png)
</div>
   
# TrustedInjector
This is a LoadLibrary injector for Counter-Strike: Global Offensive.

## Information
It automatically bypasses trusted mode by removing hooks on various Win32 functions. 

> I made this because KittenPopo's CSGhost doesn't support command line parameters, such a simple thing to implement but oh well...

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
