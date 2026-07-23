#pragma once

#include <string>
#include <vector>

namespace flasc {
namespace IvamRunner {

// True if "ivam.exe" exists directly inside `folder`.
bool ExistsIn(const std::string& folder);

// One file ivam.exe itself would recognise and act on, and whether the
// output it would produce for it currently exists.
struct ProcessedFile {
    std::string inputName;          // e.g. "game.dat16"
    std::string expectedOutputName; // e.g. "game.dat16.json" or "game.dat16.GEN"
    bool outputExists = false;
};

// Finds every file in `folder` that ivam.exe itself would recognise and act
// on, using the exact same filename-suffix rules ivam.exe uses internally
// (confirmed from its own source - GlobFiles/ProcessMetadataFiles in
// main.cpp: "categories.dat15", "sounds.dat15", "game.dat16", "effects.dat11",
// "curves.dat12", or anything ending in "speech.dat", matched case-insensitively).
// For each one, checks whether the output ivam would produce for it - X.json
// for parse, X.GEN for build - exists right now. Because ivam *overwrites*
// existing output rather than only ever creating new files, this is what
// actually tells you what got processed - diffing the folder before/after
// misses every file that already had an output sitting there from a
// previous run.
std::vector<ProcessedFile> CheckProcessed(const std::string& folder, bool genMode);

struct RunResult {
    bool started = false;   // false if the process couldn't even be launched
    int exitCode = -1;
    std::string error;      // populated when started == false
};

// Runs "ivam.exe" from inside ivamFolder, with its current working directory
// also set to ivamFolder - ivam always operates on "." for whatever files it
// finds there (it takes no file-path argument at all), so ivam.exe and every
// file it needs (Hashes.txt, the .dat files to process) all have to live in
// this one folder together. If genMode is true, passes "gen" as the single
// argument (build/serialise direction); otherwise runs with no arguments
// (parse/deserialise direction). Blocks until ivam exits.
RunResult Run(const std::string& ivamFolder, bool genMode);

// Returns the filenames (not full paths) of regular files directly inside dir.
std::vector<std::string> ListFiles(const std::string& dir);

} // namespace IvamRunner
} // namespace flasc
