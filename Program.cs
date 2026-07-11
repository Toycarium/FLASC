using System;
using System.Collections.Generic;
using System.IO;
using FLASC.Json;

namespace FLASC
{
    class Program
    {
        static void Main()
        {
            string exeDir = AppDomain.CurrentDomain.BaseDirectory;

            while (true)
            {
                Console.WriteLine();
                Console.WriteLine("FLASC");
                Console.WriteLine("1) Convert from .ini to .json (FLA -> ivam)");
                Console.WriteLine("2) Exit");
                Console.Write("> ");

                string? choice = Console.ReadLine();
                if (choice == "1")
                {
                    ConvertIniToJson(exeDir);
                }
                else if (choice == "2")
                {
                    break;
                }
                else
                {
                    Console.WriteLine("Unrecognised option.");
                }
            }
        }

        static void ConvertIniToJson(string exeDir)
        {
            Console.Write("Path to .ini file: ");
            string? input = Console.ReadLine();
            string? path = ResolveIniPath(input, exeDir);
            if (path == null)
            {
                Console.WriteLine("File not found.");
                return;
            }

            var sections = IniLoader.Load(path);
            var root = new JObject();

            int ok = 0, failed = 0;
            foreach (var section in sections)
            {
                try
                {
                    root[section.Name.ToUpperInvariant()] = VehicleJsonBuilder.BuildVehicleObject(section);
                    ok++;
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"  [{section.Name}] FAILED: {ex.Message}");
                    failed++;
                }
            }

            string outputDir = Path.Combine(exeDir, "output");
            Directory.CreateDirectory(outputDir);
            string outputPath = Path.Combine(outputDir, Path.GetFileNameWithoutExtension(path) + ".json");

            File.WriteAllText(outputPath, root.ToJsonString());

            Console.WriteLine($"Converted {ok} section(s), {failed} failed.");
            Console.WriteLine($"Written to: {outputPath}");
        }

        /// <summary>
        /// Resolves what the user typed into an actual .ini file path. Tries, in order:
        ///  1) exactly what they typed (relative to the current working directory, or absolute)
        ///  2) the same, with ".ini" appended, if they left the extension off
        ///  3) the same two attempts again, but next to the .exe itself, so a user who just
        ///     types a bare name (e.g. "myVehicles") finds "myVehicles.ini" sitting alongside
        ///     the program without having to type a full path.
        /// </summary>
        static string? ResolveIniPath(string? input, string exeDir)
        {
            if (string.IsNullOrWhiteSpace(input))
                return null;

            input = input.Trim().Trim('"');

            var candidates = new List<string> { input };
            if (!input.EndsWith(".ini", StringComparison.OrdinalIgnoreCase))
                candidates.Add(input + ".ini");

            // Also try each candidate resolved next to the executable.
            int count = candidates.Count;
            for (int i = 0; i < count; i++)
                candidates.Add(Path.Combine(exeDir, candidates[i]));

            foreach (string candidate in candidates)
            {
                if (File.Exists(candidate))
                    return candidate;
            }

            return null;
        }
    }
}
