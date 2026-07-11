using System;
using System.Collections.Generic;
using System.Globalization;
using System.Text;

namespace FLASC.Json
{
    /// <summary>
    /// Minimal, write-only JSON DOM. FLASC never needs to *read* JSON - the ivam-format
    /// JSON we produce is only ever written, not parsed back in - so this intentionally
    /// skips parsing entirely. That buys two things over System.Text.Json.Nodes:
    ///   (a) a noticeably smaller AOT binary, since none of System.Text.Json's
    ///       reader/writer/DOM machinery needs to be pulled in at all, and
    ///   (b) exact control over number formatting - in particular, always emitting a
    ///       trailing ".0" for float fields, matching what ivam itself writes, which
    ///       System.Text.Json's default double formatting does not do for whole numbers.
    /// </summary>
    public abstract class JNode
    {
        public abstract void Write(StringBuilder sb, int indent);

        public string ToJsonString()
        {
            var sb = new StringBuilder();
            Write(sb, 0);
            return sb.ToString();
        }

        protected static void WriteIndent(StringBuilder sb, int indent) => sb.Append(' ', indent * 2);
    }

    /// <summary>An ordered set of key/value pairs. Write-only: keys are appended in the
    /// order they're assigned, duplicates are not checked for (callers are expected not
    /// to assign the same key twice, same as ivam's own ordered_json usage).</summary>
    public sealed class JObject : JNode
    {
        private readonly List<(string Key, JNode Value)> _members = new List<(string, JNode)>();

        public JNode this[string key]
        {
            set => _members.Add((key, value));
        }

        public override void Write(StringBuilder sb, int indent)
        {
            if (_members.Count == 0)
            {
                sb.Append("{}");
                return;
            }

            sb.Append("{\n");
            for (int i = 0; i < _members.Count; i++)
            {
                WriteIndent(sb, indent + 1);
                JString.WriteEscaped(sb, _members[i].Key);
                sb.Append(": ");
                _members[i].Value.Write(sb, indent + 1);
                if (i < _members.Count - 1)
                    sb.Append(',');
                sb.Append('\n');
            }
            WriteIndent(sb, indent);
            sb.Append('}');
        }
    }

    public sealed class JString : JNode
    {
        private readonly string _value;
        public JString(string value) { _value = value; }

        public override void Write(StringBuilder sb, int indent) => WriteEscaped(sb, _value);

        internal static void WriteEscaped(StringBuilder sb, string value)
        {
            sb.Append('"');
            foreach (char c in value)
            {
                switch (c)
                {
                    case '"': sb.Append("\\\""); break;
                    case '\\': sb.Append("\\\\"); break;
                    case '\b': sb.Append("\\b"); break;
                    case '\f': sb.Append("\\f"); break;
                    case '\n': sb.Append("\\n"); break;
                    case '\r': sb.Append("\\r"); break;
                    case '\t': sb.Append("\\t"); break;
                    default:
                        if (c < 0x20)
                        {
                            sb.Append("\\u");
                            sb.Append(((int)c).ToString("x4", CultureInfo.InvariantCulture));
                        }
                        else
                        {
                            sb.Append(c);
                        }
                        break;
                }
            }
            sb.Append('"');
        }
    }

    /// <summary>Any whole-number field (Int8/Int16/Int32/UInt32 - held as long internally
    /// so a UInt32 nametableOffset never overflows).</summary>
    public sealed class JInt : JNode
    {
        private readonly long _value;
        public JInt(long value) { _value = value; }

        public override void Write(StringBuilder sb, int indent) =>
            sb.Append(_value.ToString(CultureInfo.InvariantCulture));
    }

    /// <summary>A float field. Always writes with a decimal point (e.g. "640.0", never
    /// "640"), matching ivam's own formatting.</summary>
    public sealed class JFloat : JNode
    {
        private readonly double _value;
        public JFloat(double value) { _value = value; }

        public override void Write(StringBuilder sb, int indent)
        {
            if (double.IsNaN(_value) || double.IsInfinity(_value))
                throw new InvalidOperationException($"Cannot represent {_value} as JSON.");

            // .NET's default double.ToString() already gives the shortest round-trippable
            // form (e.g. "640" for 640.0, "2799.9996" for 2799.9996) - force a decimal
            // point onto whole numbers so float fields always *look* like floats.
            string text = _value.ToString(CultureInfo.InvariantCulture);
            if (text.IndexOfAny(FloatMarkers) < 0)
                text += ".0";
            sb.Append(text);
        }

        private static readonly char[] FloatMarkers = { '.', 'e', 'E' };
    }
}
