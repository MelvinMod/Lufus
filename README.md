Lufus: The Linux USB Formatting Utility
==========================================
The same [Rufus](https://github.com/pbatard/rufus/), but ported to Linux.

![Rufus logo](https://raw.githubusercontent.com/pbatard/rufus/master/res/icons/rufus-128.png)

Lufus is a utility that helps format and create bootable USB flash drives, but on Linux now!

Features
--------

* Format USB, flash card and virtual drives to FAT/FAT32/NTFS/UDF/exFAT/ReFS/ext2/ext3 (and more)
* [DD](https://github.com/coreutils/coreutils/blob/master/src/dd.c) writing method
* Create DOS bootable USB drives using [FreeDOS](https://www.freedos.org) or MS-DOS
* Create BIOS or UEFI bootable drives, including [UEFI bootable NTFS](https://github.com/pbatard/uefi-ntfs)
* Create bootable drives from bootable ISOs (Windows, Linux, etc.)
* Create bootable drives from bootable disk images, including compressed ones
* Create Windows 11 installation drives for PCs that don't have TPM or Secure Boot
* Create [Windows To Go](https://en.wikipedia.org/wiki/Windows_To_Go) drives
* Create VHD/DD, VHDX and FFU images of an existing drive
* Create persistent Linux partitions
* Windows bloatware removal feature
* Option to migrate your settings and saved settings from Linux to Windows
* Compute MD5, SHA-1, SHA-256 and SHA-512 checksums of the selected image
* Perform runtime validation of UEFI bootable media
* Improve Windows installation experience by automatically setting up OOBE parameters (local account, privacy options, etc.)
* Perform bad blocks checks, including detection of "fake" flash drives
* Download official Microsoft Windows 8, Windows 10 or Windows 11 retail ISOs
* Download [UEFI Shell](https://github.com/pbatard/UEFI-Shell) ISOs
* Modern and familiar UI, with [38 languages natively supported](https://github.com/pbatard/rufus/wiki/FAQ#What_languages_are_natively_supported_by_Rufus)
* Small footprint. No installation required.
* Portable. Secure Boot compatible.
* 100% [Free Software](https://www.gnu.org/philosophy/free-sw) ([GPL v3](https://www.gnu.org/licenses/gpl-3.0))

Compilation
-----------

Use [gcc](https://github.com/gcc-mirror/gcc) and then invoke the `.sln` or `configure`/`make` respectively.

#### Visual Studio

Lufus (the same as Rufus) is an OSI compliant Open Source project. You are entitled to
download and use the *freely available* [Visual Studio Community Edition](https://www.visualstudio.com/vs/community/)
to build, run or develop for Lufus. As per the Visual Studio Community Edition license,
this applies regardless of whether you are an individual or a corporate user.

Additional information
----------------------
Enhancements/Bugs
-----------------

Please use the [GitHub issue tracker](https://github.com/MelvinMod/Lufus/issues)
for reporting problems or suggesting new features.
