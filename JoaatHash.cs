using System;
using System.Text;

namespace FLASC
{
    /// <summary>
    /// Jenkins one-at-a-time hash ("joaat"), as used throughout GTA IV's RAGE audio
    /// metadata (confirmed identical to the algorithm in ivam's HashManager.cpp).
    /// Strings are always lower-cased before hashing.
    /// </summary>
    public static class JoaatHash
    {
        public static uint Compute(string str)
        {
            string key = str.ToLowerInvariant();
            byte[] bytes = Encoding.ASCII.GetBytes(key);

            uint hash = 0;
            foreach (byte b in bytes)
            {
                hash += b;
                hash += hash << 10;
                hash ^= hash >> 6;
            }
            hash += hash << 3;
            hash ^= hash >> 11;
            hash += hash << 15;
            return hash;
        }

        /// <summary>
        /// Produces the same textual representation ivam's JSON uses for a hash field:
        /// the lower-cased sound name if we have one, or a "0xHHHHHHHH" fallback.
        /// Since FLASC always converts *from* a human-authored .ini (where the name is
        /// already known), we practically always have the name and never need the
        /// hex-fallback branch - it exists here only to mirror the "NULL" -> hash 0 case.
        /// </summary>
        public static string ToJsonHashString(string soundName)
        {
            if (string.IsNullOrEmpty(soundName) ||
                soundName.Equals("NULL", StringComparison.OrdinalIgnoreCase))
            {
                // FLASC's historical convention: "NULL" means "write 4 zero bytes",
                // i.e. hash == 0, NOT the joaat hash of the literal word "NULL".
                return "0x00000000";
            }

            return soundName.ToLowerInvariant();
        }
    }
}
