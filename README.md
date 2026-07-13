# FLA Sound Converter

A tool for converting GTA IV FLA vehicle audio data into the **ivam** format.

## Overview

FLASC converts vehicle audio settings for **GTA IV** from the community-defined **FLA `.ini`** format into **ivam**-compatible `.json`, allowing these settings to be accurately edited and rebuilt into `game.dat16` audio metadata through the ivam tool.

Version 2.0 fully replaces the previous RAGEAT-based binary patching workflow with this ivam-first approach.

## Features

- Converts FLA `.ini` vehicle audio settings into ivam-compatible `.json` format

## Related Projects

- **ivam** — [GTAAudioMetadataTool_V2](https://github.com/ook3D/GTAAudioMetadataTool_V2) — the target format and tool this project outputs for
- **fastman92 Limit Adjuster (FLA)** — [fastman92_limit_adjuster](https://github.com/fastman92/fastman92_limit_adjuster) — source of the legacy `.ini` format this project converts from

## Credits

- **ook3D** — creator of ivam, used as the authoritative schema reference for audio metadata field definitions
- **fastman92** — creator of the FLA `.ini` format that this tool converts from
- **Monkeypolice188** — for community GTA V audio field research referenced during the field-mapping effort
- **IzzyClap** — for the original byte-mapping scheme translating FLA `.ini` fields into RAGE Audio Toolkit hex values
- **Claude** — assisted with research, development, and documentation
