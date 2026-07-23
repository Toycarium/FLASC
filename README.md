# FLA Sound Converter
A tool for converting GTA IV FLA vehicle audio data into the **ivam** format, and for driving ivam itself to parse and build audio metadata files.

## Overview
FLASC converts vehicle audio settings for **GTA IV** from the community-defined **FLA `.ini`** format into **ivam**-compatible `.json`, allowing these settings to be accurately edited and rebuilt into `game.dat16` audio metadata through the ivam tool. It can also merge `.json` files together, and drive ivam itself directly — parsing audio metadata files into `.json` and building them back — without needing to run ivam separately from the command line.

Version 2.0 fully replaces the previous RAGEAT-based binary patching workflow with this ivam-first approach.

## Usage
 
1. Run `FLASC.exe`.
2. Choose an option from the menu.

### Menu options
 
| Option | Description |
|---|---|
| 1. Convert `.ini` to `.json` | Converts a FLA-format `.ini` vehicle audio file into an ivam-compatible `.json`. |
| 2. Merge `.json` files | Merges two ivam-compatible `.json` files into a single output file. |
| 3. Parse files (ivam) | Runs ivam's own parse step, decompiling recognised audio metadata files into `.json` right next to each one. |
| 4. Build files (ivam) | Runs ivam's own build step, reassembling a `.json` back into its matching binary file. |

## Related Projects
- **[GTAAudioMetadataTool_V2](https://github.com/ook3D/GTAAudioMetadataTool_V2)** — the target format and tool this project outputs for, and the tool FLASC drives directly for options 3 and 4
- **[fastman92_limit_adjuster](https://github.com/fastman92/fastman92_limit_adjuster)** — source of the legacy `.ini` format this project converts from

## Credits
- **ook3D** — creator of ivam, used as the authoritative schema reference for audio metadata field definitions
- **fastman92** — creator of the FLA `.ini` format that this tool converts from
- **Monkeypolice188** — for community GTA V audio field research referenced during the field-mapping effort
- **IzzyClap** — for the original byte-mapping scheme translating FLA `.ini` fields into RAGE Audio Toolkit hex values
- **Claude** — assisted with research, development, and documentation
