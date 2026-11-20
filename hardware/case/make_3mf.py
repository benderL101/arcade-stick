"""Combines the top and its accent into one two-colour 3MF.

Each 3MF holds one object with two parts that share the same position:
the body on filament 1 and the rings / stripes / name on filament 2.
It's marked as a Bambu Studio file, so the part filaments are kept.
The committed 3mf/top_2colour.3mf was then opened in Bambu Studio, the
prime tower turned off (no room for it beside the top), Arrange pressed
and saved, so it opens ready to print. Do the same after re-running this.

Usage: python make_3mf.py   (after export.sh has written stl/)
Needs only the Python standard library.
"""
import os
import struct
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
PAIRS = [("top", "accent")]
PLATE = 256  # Bambu X1 Carbon bed, mm


def read_stl(path):
    """Returns (vertices, triangles) with shared vertices merged."""
    with open(path, "rb") as f:
        data = f.read()
    tris = []
    if data[:5] == b"solid" and b"facet" in data[:400]:
        pts = []
        for line in data.decode("ascii", "replace").splitlines():
            line = line.strip()
            if line.startswith("vertex"):
                pts.append(tuple(float(v) for v in line.split()[1:4]))
        tris = [pts[i:i + 3] for i in range(0, len(pts), 3)]
    else:
        n = struct.unpack_from("<I", data, 80)[0]
        for i in range(n):
            v = struct.unpack_from("<12f", data, 84 + i * 50)
            tris.append([v[3:6], v[6:9], v[9:12]])
    index, verts, out = {}, [], []
    for t in tris:
        ids = []
        for p in t:
            key = tuple(round(c, 4) for c in p)
            if key not in index:
                index[key] = len(verts)
                verts.append(key)
            ids.append(index[key])
        if len(set(ids)) == 3:
            out.append(ids)
    return verts, out


def mesh_xml(oid, name, verts, tris, colour):
    v = "".join(f'<vertex x="{x}" y="{y}" z="{z}"/>' for x, y, z in verts)
    t = "".join(f'<triangle v1="{a}" v2="{b}" v3="{c}"/>' for a, b, c in tris)
    return (f'<object id="{oid}" name="{name}" type="model" pid="1" pindex="{colour}"><mesh>'
            f'<vertices>{v}</vertices><triangles>{t}</triangles></mesh></object>')


def write_3mf(name, body, accent):
    bv, bt = read_stl(os.path.join(HERE, "stl", body + ".stl"))
    av, at = read_stl(os.path.join(HERE, "stl", accent + ".stl"))
    # Move both parts together so the body sits flat in the middle of the
    # 256 x 256 mm plate, where Bambu Studio's own Arrange puts it (clear of
    # the X1's no-print corner). Without this it hangs off the plate.
    lo = [min(v[i] for v in bv) for i in range(3)]
    hi = [max(v[i] for v in bv) for i in range(3)]
    off = (PLATE / 2 - (lo[0] + hi[0]) / 2, PLATE / 2 - (lo[1] + hi[1]) / 2, -lo[2])
    shift = lambda vs: [tuple(round(v[i] + off[i], 4) for i in range(3)) for v in vs]
    bv, av = shift(bv), shift(av)
    model = (
        '<?xml version="1.0" encoding="UTF-8"?>'
        '<model unit="millimeter" xml:lang="en-US" '
        'xmlns="http://schemas.microsoft.com/3dmanufacturing/core/2015/02" '
        'xmlns:BambuStudio="http://schemas.bambulab.com/package/2021">'
        # Marked as a Bambu Studio file, so it reads model_settings.config
        # (the parts and their filaments) instead of geometry and colours only.
        '<metadata name="Application">BambuStudio-02.08.04.57</metadata>'
        '<metadata name="BambuStudio:3mfVersion">1</metadata>'
        f'<metadata name="Title">Arcade Stick case {name}</metadata>'
        '<resources>'
        # Bambu Studio only reads geometry and colours from a 3MF it didn't
        # write, so each part gets its own colour: that's what makes it load
        # the body and the design on two separate filaments.
        '<basematerials id="1"><base name="Body" displaycolor="#202326FF"/>'
        '<base name="Design" displaycolor="#D93A3FFF"/></basematerials>'
        + mesh_xml(2, "Body", bv, bt, 0)
        + mesh_xml(3, "Design", av, at, 1)
        + f'<object id="4" name="{name}" type="model"><components>'
        '<component objectid="2"/><component objectid="3"/></components></object>'
        '</resources><build><item objectid="4"/></build></model>')
    # Bambu Studio reads part names and filament numbers from this file
    settings = (
        '<?xml version="1.0" encoding="UTF-8"?><config>'
        f'<object id="4"><metadata key="name" value="{name}"/>'
        '<metadata key="extruder" value="1"/>'
        '<part id="2" subtype="normal_part"><metadata key="name" value="Body"/>'
        '<metadata key="extruder" value="1"/></part>'
        '<part id="3" subtype="normal_part"><metadata key="name" value="Design"/>'
        '<metadata key="extruder" value="2"/></part>'
        '</object></config>')
    types = (
        '<?xml version="1.0" encoding="UTF-8"?>'
        '<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">'
        '<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>'
        '<Default Extension="model" ContentType="application/vnd.ms-package.3dmanufacturing-3dmodel+xml"/>'
        '<Default Extension="config" ContentType="text/xml"/></Types>')
    rels = (
        '<?xml version="1.0" encoding="UTF-8"?>'
        '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
        '<Relationship Target="/3D/3dmodel.model" Id="rel0" '
        'Type="http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel"/></Relationships>')
    out = os.path.join(HERE, "3mf", name + ".3mf")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr("[Content_Types].xml", types)
        z.writestr("_rels/.rels", rels)
        z.writestr("3D/3dmodel.model", model)
        z.writestr("Metadata/model_settings.config", settings)
    print(f"{out}: body {len(bt)} + design {len(at)} triangles")


for body, accent in PAIRS:
    write_3mf(body + "_2colour", body, accent)
