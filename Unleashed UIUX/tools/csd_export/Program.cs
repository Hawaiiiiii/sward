// csd_export — robust .yncp/.xncp -> clean JSON using SharpNeedle (the library Kunai
// is built on). Replaces the fragile hand-written byte parser. Walks the real CSD
// object model: scene tree -> families -> casts (quad + transform/color/sprite) +
// per-cast animation keyframe tracks + sprite crops + texture names.
//
// Usage: csd_export <input.yncp> <output.json>
using System;
using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Text;

class Program {
    const string KDIR = @"C:\Users\DavidErikGarciaArena\Documents\UI-UX Sonic World Adventure for SGFX - Project Quality Hero\external_tools\kunai\net8.0";

    static int Main(string[] args) {
        AppDomain.CurrentDomain.AssemblyResolve += (s, e) => {
            var n = new AssemblyName(e.Name).Name;
            var p = Path.Combine(KDIR, n + ".dll");
            return File.Exists(p) ? Assembly.LoadFrom(p) : null;
        };
        try { return Run(args); }
        catch (Exception ex) { Console.Error.WriteLine("ERROR: " + ex); return 1; }
    }

    // ---- tiny JSON writer (no external deps) ----
    static StringBuilder sb;
    static void Raw(string s) => sb.Append(s);
    static string Q(string s) {
        if (s == null) return "null";
        var b = new StringBuilder("\"");
        foreach (var c in s) b.Append(c == '"' ? "\\\"" : c == '\\' ? "\\\\" : c == '\n' ? "\\n" : c == '\r' ? "\\r" : c == '\t' ? "\\t" : c.ToString());
        return b.Append('"').ToString();
    }
    static string F(float v) => float.IsFinite(v) ? v.ToString("0.######", CultureInfo.InvariantCulture) : "0";

    [MethodImpl(MethodImplOptions.NoInlining)]
    static int Run(string[] args) {
        if (args.Length < 2) { Console.Error.WriteLine("usage: csd_export <in.yncp> <out.json>"); return 2; }
        string inPath = args[0], outPath = args[1];

        var file = SharpNeedle.IO.HostFile.Open(inPath);
        var proj = new SharpNeedle.Framework.Ninja.Csd.CsdProject();
        proj.Read(file);

        // texture names
        var texNames = new List<string>();
        if (proj.Textures is IEnumerable texList)
            foreach (var t in texList) texNames.Add((string)t.GetType().GetProperty("Name")?.GetValue(t));

        sb = new StringBuilder();
        Raw("{");
        Raw("\"file\":" + Q(Path.GetFileName(inPath)) + ",");
        Raw("\"project\":" + Q(proj.Project.Name) + ",");
        Raw("\"textures\":[" + string.Join(",", texNames.ConvertAll(Q)) + "],");
        Raw("\"scenes\":[");

        int sceneCount = 0, castCount = 0, animCount = 0;
        bool firstScene = true;
        void WalkNode(object node, string path) {
            var scenes = node.GetType().GetProperty("Scenes").GetValue(node);   // CsdDictionary<Scene>
            foreach (DictionaryEntry kv in AsEntries(scenes)) {
                if (!firstScene) Raw(",");
                firstScene = false;
                ExportScene(kv.Value, path + "/" + kv.Key);
                sceneCount++;
            }
            var children = node.GetType().GetProperty("Children").GetValue(node);
            foreach (DictionaryEntry kv in AsEntries(children))
                WalkNode(kv.Value, path + "/" + kv.Key);
        }

        void ExportScene(object scene, string path) {
            var st = scene.GetType();
            float frameRate = (float)st.GetProperty("FrameRate").GetValue(scene);
            float aspect = (float)st.GetProperty("AspectRatio").GetValue(scene);
            var families = (IList)st.GetProperty("Families").GetValue(scene);
            var sprites = (IList)st.GetProperty("Sprites").GetValue(scene);

            Raw("{\"path\":" + Q(path) + ",\"frameRate\":" + F(frameRate) + ",\"aspect\":" + F(aspect) + ",");

            // sprites (atlas crops)
            Raw("\"sprites\":[");
            for (int i = 0; i < sprites.Count; i++) {
                var sp = sprites[i]; var spt = sp.GetType();
                int ti = (int)spt.GetField("TextureIndex").GetValue(sp);
                var tl = (System.Numerics.Vector2)spt.GetField("TopLeft").GetValue(sp);
                var br = (System.Numerics.Vector2)spt.GetField("BottomRight").GetValue(sp);
                Raw((i > 0 ? "," : "") + "{\"tex\":" + ti + ",\"crop\":[" + F(tl.X) + "," + F(tl.Y) + "," + F(br.X) + "," + F(br.Y) + "]}");
            }
            Raw("],");

            // flatten casts across families -> (gi=family, ci=index in family), parent index within family
            var castKey = new Dictionary<object, (int gi, int ci)>(ReferenceEqualityComparer.Instance);
            Raw("\"casts\":[");
            bool firstCast = true;
            for (int gi = 0; gi < families.Count; gi++) {
                var fam = families[gi];
                var casts = (IEnumerable)fam.GetType().GetProperty("Casts").GetValue(fam);
                int ci = 0; var list = new List<object>();
                foreach (var c in casts) { list.Add(c); castKey[c] = (gi, ci); ci++; }
                for (ci = 0; ci < list.Count; ci++) {
                    var c = list[ci]; var ct = c.GetType();
                    object parent = ct.GetProperty("Parent").GetValue(c);
                    int pci = (parent != null && castKey.TryGetValue(parent, out var pk)) ? pk.ci : -1;
                    var tl = (System.Numerics.Vector2)ct.GetProperty("TopLeft").GetValue(c);
                    var br = (System.Numerics.Vector2)ct.GetProperty("BottomRight").GetValue(c);
                    var pos = (System.Numerics.Vector2)ct.GetProperty("Position").GetValue(c);
                    var org = (System.Numerics.Vector2)ct.GetProperty("Origin").GetValue(c);
                    var info = ct.GetProperty("Info").GetValue(c); var it = info.GetType();
                    var tr = (System.Numerics.Vector2)it.GetField("Translation").GetValue(info);
                    var scl = (System.Numerics.Vector2)it.GetField("Scale").GetValue(info);
                    float rot = (float)it.GetField("Rotation").GetValue(info);
                    float subi = (float)it.GetField("SpriteIndex").GetValue(info);
                    uint hide = (uint)it.GetField("HideFlag").GetValue(info);
                    string col = Col(it.GetField("Color").GetValue(info));
                    var spriteIdx = (int[])ct.GetProperty("SpriteIndices").GetValue(c);
                    bool enabled = (bool)ct.GetProperty("Enabled").GetValue(c);
                    string name = (string)ct.GetProperty("Name").GetValue(c);

                    Raw((firstCast ? "" : ",") + "{\"gi\":" + gi + ",\"ci\":" + ci + ",\"name\":" + Q(name) +
                        ",\"parent\":" + pci + ",\"enabled\":" + (enabled ? "true" : "false") +
                        ",\"quad\":[" + F(tl.X) + "," + F(tl.Y) + "," + F(br.X) + "," + F(br.Y) + "]" +
                        ",\"pos\":[" + F(pos.X) + "," + F(pos.Y) + "],\"origin\":[" + F(org.X) + "," + F(org.Y) + "]" +
                        ",\"info\":{\"tx\":" + F(tr.X) + ",\"ty\":" + F(tr.Y) + ",\"sx\":" + F(scl.X) + ",\"sy\":" + F(scl.Y) +
                        ",\"rot\":" + F(rot) + ",\"sub\":" + F(subi) + ",\"hide\":" + hide + ",\"color\":" + Q(col) + "}" +
                        ",\"sprites\":[" + (spriteIdx != null ? string.Join(",", Array.ConvertAll(spriteIdx, x => x.ToString())) : "") + "]}");
                    firstCast = false;
                }
            }
            Raw("],");

            // motions (animations) -> per-cast keyframe tracks
            var motions = st.GetProperty("Motions").GetValue(scene);
            Raw("\"anims\":[");
            bool firstAnim = true;
            foreach (DictionaryEntry mkv in AsEntries(motions)) {
                var motion = mkv.Value; var mt = motion.GetType();
                float startF = (float)mt.GetProperty("StartFrame").GetValue(motion);
                float endF = (float)mt.GetProperty("EndFrame").GetValue(motion);
                Raw((firstAnim ? "" : ",") + "{\"name\":" + Q((string)mkv.Key) + ",\"start\":" + F(startF) + ",\"end\":" + F(endF) + ",\"tracks\":[");
                firstAnim = false; animCount++;
                var famMotions = (IList)mt.GetProperty("FamilyMotions").GetValue(motion);
                bool firstTrack = true;
                foreach (var fm in famMotions) {
                    var castMotions = (IList)fm.GetType().GetProperty("CastMotions").GetValue(fm);
                    foreach (var cm in castMotions) {
                        var cmt = cm.GetType();
                        object cast = cmt.GetProperty("Cast").GetValue(cm);
                        (int gi, int ci) ck = castKey.TryGetValue(cast, out var k) ? k : (-1, -1);
                        int count = (int)cmt.GetProperty("Count").GetValue(cm);
                        for (int li = 0; li < count; li++) {
                            var kfl = cmt.GetProperty("Item").GetValue(cm, new object[] { li });
                            var kt = kfl.GetType();
                            string prop = kt.GetProperty("Property").GetValue(kfl).ToString();
                            var frames = (IList)kt.GetProperty("Frames").GetValue(kfl);
                            Raw((firstTrack ? "" : ",") + "{\"gi\":" + ck.gi + ",\"ci\":" + ck.ci + ",\"prop\":" + Q(prop) + ",\"keys\":[");
                            firstTrack = false;
                            for (int fi = 0; fi < frames.Count; fi++) {
                                var kf = frames[fi]; var kff = kf.GetType();
                                uint frame = (uint)kff.GetProperty("Frame").GetValue(kf);
                                string interp = kff.GetProperty("Interpolation").GetValue(kf).ToString();
                                float intan = (float)kff.GetProperty("InTangent").GetValue(kf);
                                float outan = (float)kff.GetProperty("OutTangent").GetValue(kf);
                                var union = kff.GetProperty("Value").GetValue(kf); var ut = union.GetType();
                                string val;
                                if (prop.StartsWith("Color") || prop.StartsWith("Gradient")) val = Q(Col(ut.GetField("Color").GetValue(union)));
                                else if (prop == "HideFlag") val = ((uint)ut.GetField("Uint").GetValue(union)).ToString();
                                else val = F((float)ut.GetField("Float").GetValue(union));
                                Raw((fi > 0 ? "," : "") + "{\"f\":" + frame + ",\"v\":" + val + ",\"i\":" + Q(interp) + ",\"it\":" + F(intan) + ",\"ot\":" + F(outan) + "}");
                            }
                            Raw("]}");
                        }
                    }
                }
                Raw("]}");
            }
            Raw("]}");
            castCount += castKey.Count;
        }

        WalkNode(proj.Project.Root, proj.Project.Name ?? "root");
        Raw("]}");

        File.WriteAllText(outPath, sb.ToString());
        Console.WriteLine($"OK {Path.GetFileName(inPath)} -> {Path.GetFileName(outPath)} : {sceneCount} scenes, {castCount} casts, {animCount} anims, {texNames.Count} textures, {sb.Length} bytes");
        return 0;
    }

    // CsdDictionary<T> enumerates as KeyValuePair<string,T>; project to DictionaryEntry via reflection.
    static IEnumerable<DictionaryEntry> AsEntries(object dict) {
        var en = ((IEnumerable)dict).GetEnumerator();
        while (en.MoveNext()) {
            var kv = en.Current; var t = kv.GetType();
            yield return new DictionaryEntry(t.GetProperty("Key").GetValue(kv), t.GetProperty("Value").GetValue(kv));
        }
    }

    // Color<Byte> -> 0xAARRGGBB (member names read reflectively for robustness).
    static string Col(object color) {
        if (color == null) return "0xFFFFFFFF";
        var t = color.GetType();
        byte G(string n) {
            var f = t.GetField(n); if (f != null) return Convert.ToByte(f.GetValue(color));
            var p = t.GetProperty(n); return p != null ? Convert.ToByte(p.GetValue(color)) : (byte)255;
        }
        return $"0x{G("A"):X2}{G("R"):X2}{G("G"):X2}{G("B"):X2}";
    }
}
