"""Combines the top and its accent into one two-colour 3MF.

Each 3MF holds one object with two parts that share the same position:
the body on filament 1 and the rings / stripes / name on filament 2.
Bambu Studio opens it ready to slice; just pick the two colours.

Usage: python make_3mf.py   (after export.sh has written stl/)
Needs only the Python standard library.
"""
import os
import struct
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
PAIRS = [("top", "accent")]


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


def mesh_xml(oid, name, verts, tris):
    v = "".join(f'<vertex x="{x}" y="{y}" z="{z}"/>' for x, y, z in verts)
    t = "".join(f'<triangle v1="{a}" v2="{b}" v3="{c}"/>' for a, b, c in tris)
    return (f'<object id="{oid}" name="{name}" type="model"><mesh>'
            f'<vertices>{v}</vertices><triangles>{t}</triangles></mesh></object>')


def write_3mf(name, body, accent):
    bv, bt = read_stl(os.path.join(HERE, "stl", body + ".stl"))
    av, at = read_stl(os.path.join(HERE, "stl", accent + ".stl"))
    model = (
        '<?xml version="1.0" encoding="UTF-8"?>'
        '<model unit="millimeter" xml:lang="en-US" '
        'xmlns="http://schemas.microsoft.com/3dmanufacturing/core/2015/02">'
        f'<metadata name="Title">Arcade Stick case {name}</metadata>'
        '<resources>'
        + mesh_xml(1, "Body", bv, bt)
        + mesh_xml(2, "Design", av, at)
        + f'<object id="3" name="{name}" type="model"><components>'
        '<component objectid="1"/><component objectid="2"/></components></object>'
        '</resources><build><item objectid="3"/></build></model>')
    # Bambu Studio reads part names and filament numbers from this file
    settings = (
        '<?xml version="1.0" encoding="UTF-8"?><config>'
        f'<object id="3"><metadata key="name" value="{name}"/>'
        '<metadata key="extruder" value="1"/>'
        '<part id="1" subtype="normal_part"><metadata key="name" value="Body"/>'
        '<metadata key="extruder" value="1"/></part>'
        '<part id="2" subtype="normal_part"><metadata key="name" value="Design"/>'
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
