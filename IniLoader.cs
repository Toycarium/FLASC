using System;
using System.Collections.Generic;
using System.IO;

namespace FLASC
{
    public sealed class IniSection
    {
        public string Name { get; }
        public List<KeyValuePair<string, string>> Entries { get; } = new List<KeyValuePair<string, string>>();

        public IniSection(string name)
        {
            Name = name;
        }

        public string? Get(string key)
        {
            foreach (var kv in Entries)
                if (string.Equals(kv.Key, key, StringComparison.OrdinalIgnoreCase))
                    return kv.Value;
            return null;
        }
    }

    public static class IniLoader
    {
        /// <summary>
        /// Parses the whole .ini file into an ordered list of sections, each with an
        /// ordered list of key/value pairs (order matters: for gameAutomobile/gameHeli/
        /// gameBoat, field order in the .ini is the same as the field's byte offset order
        /// in the binary struct).
        /// </summary>
        public static List<IniSection> Load(string path)
        {
            var sections = new List<IniSection>();
            IniSection? current = null;

            foreach (var rawLine in File.ReadLines(path))
            {
                string line = rawLine.Trim();
                if (line.Length == 0)
                    continue;

                if (line.StartsWith("[", StringComparison.Ordinal) && line.EndsWith("]", StringComparison.Ordinal))
                {
                    current = new IniSection(line.Substring(1, line.Length - 2).Trim());
                    sections.Add(current);
                    continue;
                }

                int eq = line.IndexOf('=');
                if (eq < 0 || current == null)
                    continue;

                string key = line.Substring(0, eq).Trim();
                string value = line.Substring(eq + 1).Trim();
                current.Entries.Add(new KeyValuePair<string, string>(key, value));
            }

            return sections;
        }
    }
}
