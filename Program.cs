using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;

class Program
{
    static string logPath;

    static readonly HashSet<string> OneByteFields = new HashSet<string> {
        "structType", "field_13E", "field_13F", "field_140", "field_141", "field_15E", "field_15F", "field_160"
    };

    static void Main()
    {
        string exeDir = AppDomain.CurrentDomain.BaseDirectory;
        logPath = Path.Combine(exeDir, "log.txt");
        File.WriteAllText(logPath, $"[{DateTime.Now}] Program has started\n");

        while (true)
        {
            Console.WriteLine("Choose an option:");
            Console.WriteLine("1. Convert ini file to .bin files");
            Console.WriteLine("2. Inject binary data into game.dat16");
            Console.WriteLine("3. Exit");
            Console.Write("Enter number: ");
            var input = Console.ReadLine();

            if (input == "1")
            {
                ConvertIni(exeDir);
            }
            else if (input == "2")
            {
                InjectToGameDat16();
            }
            else if (input == "3")
            {
                Console.WriteLine("Exiting...");
                LogToFile($"[{DateTime.Now}]Program exited by user.");
                break;
            }
            else
            {
                Console.WriteLine("Invalid option. Try again.");
            }
            Console.WriteLine();
        }
    }

    static void ConvertIni(string exeDir)
    {
        Console.Write("Enter the name of the .ini file (without extension): ");
        string iniName = Console.ReadLine()?.Trim();

        if (string.IsNullOrWhiteSpace(iniName))
        {
            Console.WriteLine("INI filename was not entered.");
            LogToFile("INI filename was not entered.");
            return;
        }

        string iniPath = Path.Combine(exeDir, iniName + ".ini");
        if (!File.Exists(iniPath))
        {
            Console.WriteLine($"INI file '{iniName}.ini' not found in the program directory.");
            LogToFile($"INI file '{iniName}.ini' not found.");
            return;
        }

        // Read all lines once
        var allLines = File.ReadAllLines(iniPath);

        // Check for duplicate section names (case-insensitive)
        var sectionCounts = new Dictionary<string, int>(StringComparer.OrdinalIgnoreCase);
        var sectionRegex = new Regex(@"^\s*\[(.+?)\]\s*$");
        foreach (var raw in allLines)
        {
            var line = raw.Trim();
            if (line.Length == 0 || line.StartsWith(";") || line.StartsWith("#"))
                continue;

            var m = sectionRegex.Match(line);
            if (m.Success)
            {
                var name = m.Groups[1].Value.Trim();
                if (sectionCounts.ContainsKey(name))
                    sectionCounts[name]++;
                else
                    sectionCounts[name] = 1;
            }
        }

        var duplicates = sectionCounts
            .Where(kv => kv.Value > 1)
            .Select(kv => $"{kv.Key} (x{kv.Value})")
            .ToList();

        if (duplicates.Count > 0)
        {
            Console.WriteLine("Error: Duplicate section names found in INI file. Conversion aborted.");
            Console.WriteLine("List of duplicate sections:");
            foreach (var dup in duplicates)
                Console.WriteLine(" - " + dup);

            LogToFile("Duplicate sections detected:");
            foreach (var dup in duplicates)
                LogToFile(" - " + dup);

            return;
        }

        // No duplicates — proceed
        var blocks = ParseBlocks(allLines);

        string outputDir = Path.Combine(exeDir, "output");
        if (!Directory.Exists(outputDir))
            Directory.CreateDirectory(outputDir);

        foreach (var block in blocks)
        {
            try
            {
                // Validating structType for each block
                var structTypeLine = block.Value.FirstOrDefault(line => line.StartsWith("structType", StringComparison.OrdinalIgnoreCase));
                if (structTypeLine == null)
                {
                    Console.WriteLine($"Block '{block.Key}' skipped: structType field not found.");
                    LogToFile($"Block '{block.Key}' skipped: structType field not found.");
                    continue;
                }

                var parts = structTypeLine.Split('=', 2);
                if (parts.Length != 2)
                {
                    Console.WriteLine($"Block '{block.Key}' skipped: structType field malformed.");
                    LogToFile($"Block '{block.Key}' skipped: structType field malformed.");
                    continue;
                }

                byte structType;
                if (!byte.TryParse(parts[1].Trim(), out structType))
                {
                    Console.WriteLine($"Block '{block.Key}' skipped: structType field is not a valid byte.");
                    LogToFile($"Block '{block.Key}' skipped: structType field is not a valid byte.");
                    continue;
                }

                if (structType != 0 && structType != 4 && structType != 7)
                {
                    Console.WriteLine($"Block '{block.Key}' skipped: unsupported structType {structType}.");
                    LogToFile($"Block '{block.Key}' skipped: unsupported structType {structType}.");
                    continue;
                }

                CreateBinFileForBlock(block.Key, block.Value, outputDir);
                Console.WriteLine($"File created: output\\{block.Key}.bin");
                LogToFile($"File created: output\\{block.Key}.bin");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error in block {block.Key}: {ex.Message}");
                LogToFile($"Error in block {block.Key}: {ex.Message}");
            }
        }

        Console.WriteLine("Done");
        LogToFile("Finished converting ini file.");
    }

    static void InjectToGameDat16()
    {
        try
        {
            string exeDirectory = AppDomain.CurrentDomain.BaseDirectory;
            string outputDir = Path.Combine(exeDirectory, "output");

            if (!Directory.Exists(outputDir))
            {
                Console.WriteLine("Output folder not found.");
                LogToFile("Output folder not found.");
                return;
            }

            string gameDatPath = Path.Combine(exeDirectory, "game.dat16");
            if (!File.Exists(gameDatPath))
            {
                Console.WriteLine("game.dat16 file not found in the program directory.");
                LogToFile("game.dat16 file not found.");
                return;
            }

            Console.Write("Enter the name of the .bin file from the output folder (without extension): ");
            string binFileName = Console.ReadLine();
            if (string.IsNullOrEmpty(binFileName))
            {
                Console.WriteLine("No filename entered.");
                return;
            }

            string binFilePath = Path.Combine(outputDir, binFileName + ".bin");
            if (!File.Exists(binFilePath))
            {
                Console.WriteLine($"File {binFileName}.bin not found in output folder.");
                LogToFile($"File {binFileName}.bin not found.");
                return;
            }

            byte[] binData = File.ReadAllBytes(binFilePath);

            if (binData.Length == 0)
            {
                Console.WriteLine("Error: .bin file is empty.");
                LogToFile("Empty bin file: " + binFilePath);
                return;
            }

            byte structTypeFromBin = binData[0];
            int expectedLength = structTypeFromBin switch
            {
                0x00 => 353,
                0x04 => 186,
                0x07 => 201,
                _ => -1
            };

            if (expectedLength == -1)
            {
                Console.WriteLine($"Error: Unsupported structType 0x{structTypeFromBin:X2}. Expected 00, 04, or 07.");
                LogToFile($"Unsupported structType: 0x{structTypeFromBin:X2} in file {binFileName}.bin");
                return;
            }

            if (binData.Length != expectedLength)
            {
                Console.WriteLine($"Error: Incorrect .bin file size for structType 0x{structTypeFromBin:X2}. Expected {expectedLength} bytes, but got {binData.Length}.");
                LogToFile($"Incorrect .bin size for structType 0x{structTypeFromBin:X2}. Expected {expectedLength}, got {binData.Length}.");
                return;
            }

            // Get sound name
            Console.Write("Enter sound name (1-8 chars, only A-Z, a-z, 0-9, case-sensitive): ");
            string soundNameInput = Console.ReadLine();

            if (string.IsNullOrEmpty(soundNameInput) ||
                soundNameInput.Length < 1 || soundNameInput.Length > 8 ||
                !soundNameInput.All(c => (c >= 'A' && c <= 'Z') ||
                                         (c >= 'a' && c <= 'z') ||
                                         (c >= '0' && c <= '9')))
            {
                Console.WriteLine("Invalid sound name. Only 1-8 English letters/numbers allowed, no spaces or symbols.");
                LogToFile("Invalid sound name entered: " + soundNameInput);
                return;
            }

            byte[] gameDat = File.ReadAllBytes(gameDatPath);

            // Create search pattern: [name length][name bytes]
            byte nameLength = (byte)soundNameInput.Length;
            byte[] nameBytes = Encoding.ASCII.GetBytes(soundNameInput);
            byte[] namePattern = new byte[1 + nameBytes.Length];
            namePattern[0] = nameLength;
            Array.Copy(nameBytes, 0, namePattern, 1, nameBytes.Length);

            List<long> foundOffsets = new List<long>();

            // Search for header in game.dat
            for (long i = 0; i <= gameDat.LongLength - namePattern.Length; i++)
            {
                bool match = true;
                for (int j = 0; j < namePattern.Length; j++)
                {
                    if (gameDat[i + j] != namePattern[j])
                    {
                        match = false;
                        break;
                    }
                }
                if (match)
                    foundOffsets.Add(i);
            }

            if (foundOffsets.Count == 0)
            {
                Console.WriteLine($"Sound name '{soundNameInput}' not found in game.dat16.");
                LogToFile($"Sound name '{soundNameInput}' not found in game.dat16.");
                return;
            }
            if (foundOffsets.Count > 1)
            {
                Console.WriteLine($"Multiple entries found for sound name '{soundNameInput}'. Aborting.");
                LogToFile($"Multiple entries found for sound name '{soundNameInput}'.");
                return;
            }

            long headerOffset = foundOffsets[0];

            // --- MODIFIED PART START ---
            // Read offset key (4 bytes immediately after [len][name])
            byte[] offsetKeyBytes = new byte[4];
            Array.Copy(gameDat, headerOffset + 1 + nameLength, offsetKeyBytes, 0, 4);
            int containerOffset = BitConverter.ToInt32(offsetKeyBytes, 0);

            // According to file layout, the actual block 'type' is located at containerOffset + 8.
            // So compute blockStart = containerOffset + 8.
            int blockStart = containerOffset + 8;

            // Bounds check
            if (blockStart < 0 || blockStart >= gameDat.Length)
            {
                Console.WriteLine($"Invalid container/block offset (0x{containerOffset:X}) read from header for '{soundNameInput}'.");
                LogToFile($"Invalid container/block offset (0x{containerOffset:X}) for '{soundNameInput}'.");
                return;
            }

            // Check struct type BEFORE injection at the actual block start (not at containerOffset)
            if (gameDat[blockStart] != structTypeFromBin)
            {
                Console.WriteLine($"Struct type mismatch. BIN: 0x{structTypeFromBin:X2}, GAME: 0x{gameDat[blockStart]:X2} at block start 0x{blockStart:X} (container 0x{containerOffset:X}).");
                LogToFile($"Struct type mismatch for '{soundNameInput}'. BIN: 0x{structTypeFromBin:X2}, GAME: 0x{gameDat[blockStart]:X2} at block start 0x{blockStart:X} (container 0x{containerOffset:X}).");
                return;
            }

            // We must preserve the first 5 bytes of the block (type + 4-byte id).
            // Start injection at blockStart + 5 (i.e. do not overwrite bytes [blockStart .. blockStart+4]).
            int targetOffset = blockStart + 5;

            // Ensure we do not overflow the file
            if (targetOffset < 0 || (long)targetOffset + (binData.Length - 5) > gameDat.LongLength)
            {
                Console.WriteLine("Not enough space in game.dat16 to inject the bin data at the target position.");
                LogToFile("Injection failed: not enough space for injection at computed targetOffset.");
                return;
            }

            // Copy from bin starting at index 5 to keep the first 5 bytes unchanged.
            Array.Copy(binData, 5, gameDat, targetOffset, binData.Length - 5);
            // --- MODIFIED PART END ---

            // Save modified game.dat16
            File.WriteAllBytes(gameDatPath, gameDat);

            Console.WriteLine($"Successfully injected {binFileName}.bin for sound '{soundNameInput}' at offset 0x{targetOffset:X}.");
            LogToFile($"Injected {binFileName}.bin for '{soundNameInput}' at offset 0x{targetOffset:X}.");
        }
        catch (Exception ex)
        {
            Console.WriteLine("Error during injection: " + ex.Message);
            LogToFile("Exception in InjectToGameDat16: " + ex);
        }
    }

    static Dictionary<string, List<string>> ParseBlocks(string[] lines)
    {
        var blocks = new Dictionary<string, List<string>>(StringComparer.OrdinalIgnoreCase);
        string currentBlock = null;

        foreach (var lineRaw in lines)
        {
            string line = lineRaw.Trim();
            if (line.Length == 0 || line.StartsWith(";") || line.StartsWith("#"))
                continue;

            var matchSection = Regex.Match(line, @"^\[(.+)]$");
            if (matchSection.Success)
            {
                currentBlock = matchSection.Groups[1].Value;
                if (!blocks.ContainsKey(currentBlock))
                    blocks[currentBlock] = new List<string>();
            }
            else if (currentBlock != null)
            {
                blocks[currentBlock].Add(line);
            }
        }

        return blocks;
    }

    static void CreateBinFileForBlock(string modelName, List<string> lines, string outputDir)
    {
        using var ms = new MemoryStream();
        using var bw = new BinaryWriter(ms);

        var fields = new List<(string key, string value)>();
        foreach (var line in lines)
        {
            var parts = line.Split(new char[] { '=' }, 2);
            if (parts.Length != 2) continue;
            fields.Add((parts[0].Trim(), parts[1].Trim()));
        }

        var structTypeField = fields.Find(f => f.key.Equals("structType", StringComparison.OrdinalIgnoreCase));
        if (structTypeField.key == null)
            throw new Exception("Field structType was not found");

        byte structType = ParseByte(structTypeField.value);
        bw.Write(structType);
        bw.Write(new byte[4]); // field_1 = 00 00 00 00

        if (structType == 0)
        {
            var flags = fields.Find(f => f.key.Equals("flags", StringComparison.OrdinalIgnoreCase));
            if (flags.key != null)
            {
                var flagsBytes = ParseFlags(flags.value);
                if (flagsBytes.Length != 4)
                    throw new Exception("Field flags should be 4 bytes");
                bw.Write(flagsBytes);
            }

            bw.Write((byte)0x00); // 1 byte padding

            foreach (var (key, value) in fields)
            {
                if (key.Equals("structType", StringComparison.OrdinalIgnoreCase) ||
                    key.Equals("field_1", StringComparison.OrdinalIgnoreCase) ||
                    key.Equals("flags", StringComparison.OrdinalIgnoreCase))
                    continue;

                if (OneByteFields.Contains(key))
                {
                    bw.Write(ParseByte(value));
                }
                else if (key.Equals("field_11A", StringComparison.OrdinalIgnoreCase) ||
                         key.Equals("field_12E", StringComparison.OrdinalIgnoreCase) ||
                         key.Equals("field_132", StringComparison.OrdinalIgnoreCase))
                {
                    bw.Write(int.Parse(value));
                }
                else if (key.StartsWith("floatAsInt_multiplier1200_", StringComparison.OrdinalIgnoreCase))
                {
                    float val = ParseFloat(value);
                    bw.Write((int)Math.Round(val * 1200, MidpointRounding.AwayFromZero));
                }
                else if (key.StartsWith("floatAsInt_", StringComparison.OrdinalIgnoreCase))
                {
                    float val = ParseFloat(value);
                    bw.Write((int)Math.Round(val * 100, MidpointRounding.AwayFromZero));
                }
                else if (key.StartsWith("field_", StringComparison.OrdinalIgnoreCase))
                {
                    float f = ParseFloat(value);
                    bw.Write(BitConverter.GetBytes(f));
                }
                else
                {
                    string trimmedValue = value.Trim();
                    if (string.Equals(trimmedValue, "NULL", StringComparison.OrdinalIgnoreCase))
                    {
                        bw.Write(new byte[4]);
                    }
                    else
                    {
                        uint hash = JoaatHash(trimmedValue);
                        bw.Write(hash);
                    }
                }
            }
        }
        else if (structType == 4)
        {
            var flags = fields.Find(f => f.key.Equals("flags", StringComparison.OrdinalIgnoreCase));
            byte flagByte = flags.key != null ? ParseByte(flags.value) : (byte)0;
            bw.Write(flagByte);
            bw.Write(new byte[4]); // 4 bytes padding

            foreach (var (key, value) in fields)
            {
                if (key.Equals("structType", StringComparison.OrdinalIgnoreCase) ||
                    key.Equals("flags", StringComparison.OrdinalIgnoreCase) ||
                    key.Equals("field_1", StringComparison.OrdinalIgnoreCase))
                    continue;

                if (key.StartsWith("floatAsInt_", StringComparison.OrdinalIgnoreCase))
                {
                    short s = (short)Math.Round(ParseFloat(value) * 100, MidpointRounding.AwayFromZero);
                    bw.Write(s);
                }
                else if (new[] {
                "field_46", "field_48", "field_4C", "field_4E",
                "field_52", "field_54", "field_90", "field_92",
                "field_96", "field_98" }.Contains(key))
                {
                    short s = short.Parse(value);
                    bw.Write(s);
                }
                else if (new[] {
                "field_1E", "field_58", "field_5C", "field_64", "field_68" }.Contains(key))
                {
                    float f = ParseFloat(value);
                    bw.Write(f);
                }
                else if (new[] { "field_A8", "field_A9" }.Contains(key))
                {
                    bw.Write(ParseByte(value));
                }
                else
                {
                    string trimmedValue = value.Trim();
                    if (string.Equals(trimmedValue, "NULL", StringComparison.OrdinalIgnoreCase))
                    {
                        bw.Write(new byte[4]);
                    }
                    else
                    {
                        uint hash = JoaatHash(trimmedValue);
                        bw.Write(hash);
                    }
                }
            }
        }
        else if (structType == 7)
        {
            var flags = fields.Find(f => f.key.Equals("flags", StringComparison.OrdinalIgnoreCase));
            byte flagByte = flags.key != null ? ParseByte(flags.value) : (byte)0;
            bw.Write(flagByte);
            bw.Write(new byte[4]); // 4 bytes padding

            foreach (var (key, value) in fields)
            {
                if (key.Equals("structType", StringComparison.OrdinalIgnoreCase) ||
                    key.Equals("flags", StringComparison.OrdinalIgnoreCase) ||
                    key.Equals("field_1", StringComparison.OrdinalIgnoreCase))
                    continue;

                if (key.Equals("field_42", StringComparison.OrdinalIgnoreCase))
                {
                    var bytes = ParseFlags(value);
                    if (bytes.Length != 4)
                        throw new Exception("Field field_42 should be 4 bytes");
                    bw.Write(bytes);
                }
                else if (new[] {
                "field_46", "field_4A", "field_56", "field_5A", "field_66", "field_6A",
                "field_76", "field_7A", "field_8A", "field_8E" }.Contains(key))
                {
                    float f = ParseFloat(value);
                    bw.Write(f);
                }
                else if (key.StartsWith("floatAsInt_", StringComparison.OrdinalIgnoreCase))
                {
                    int i = (int)Math.Round(ParseFloat(value) * 100, MidpointRounding.AwayFromZero);
                    bw.Write(i);
                }
                else if (new[] { "field_9E", "field_A6" }.Contains(key))
                {
                    bw.Write(int.Parse(value));
                }
                else if (new[] { "field_B6", "field_B7", "field_C8" }.Contains(key))
                {
                    bw.Write(ParseByte(value));
                }
                else
                {
                    string trimmedValue = value.Trim();
                    if (string.Equals(trimmedValue, "NULL", StringComparison.OrdinalIgnoreCase))
                    {
                        bw.Write(new byte[4]);
                    }
                    else
                    {
                        uint hash = JoaatHash(trimmedValue);
                        bw.Write(hash);
                    }
                }
            }
        }
        else
        {
            throw new Exception($"Unsupported structType: {structType}");
        }

        var outPath = Path.Combine(outputDir, $"{modelName}.bin");
        File.WriteAllBytes(outPath, ms.ToArray());
    }

    static byte ParseByte(string val)
    {
        val = val.Trim();
        if (val.StartsWith("0x", StringComparison.OrdinalIgnoreCase))
            return Convert.ToByte(val.Substring(2), 16);
        if (byte.TryParse(val, out byte b))
            return b;
        throw new Exception($"Unable to convert '{val}' into byte");
    }

    static float ParseFloat(string val)
    {
        val = val.Trim();
        if (float.TryParse(val, NumberStyles.Float, CultureInfo.InvariantCulture, out float f))
            return f;
        throw new Exception($"Unable to convert '{val}' into float");
    }

    static byte[] ParseFlags(string val)
    {
        // 4 bytes little-endian
        val = val.Trim().ToLower();
        if (val.StartsWith("0x"))
            val = val.Substring(2);
        if (val.Length > 8)
            throw new Exception("Field flags is too long");

        // adding 00 up to 8 symbols (4 bytes)
        val = val.PadLeft(8, '0');

        // parsing bytes into little-endian
        var bytes = new byte[4];
        for (int i = 0; i < 4; i++)
        {
            string bStr = val.Substring(val.Length - 2 * (i + 1), 2);
            bytes[i] = Convert.ToByte(bStr, 16);
        }
        return bytes;
    }

    static uint JoaatHash(string key)
    {
        uint hash = 0;
        foreach (char c in key)
        {
            hash += char.ToLowerInvariant(c);
            hash += (hash << 10);
            hash ^= (hash >> 6);
        }
        hash += (hash << 3);
        hash ^= (hash >> 11);
        hash += (hash << 15);
        return hash;
    }

    static void LogToFile(string message)
    {
        try
        {
            string timestamp = DateTime.Now.ToString("[dd/MM/yyyy HH:mm:ss] ");
            File.AppendAllText(logPath, timestamp + message + Environment.NewLine);
        }
        catch
        {
            // ignore logging exceptions
        }
    }
}