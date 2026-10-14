"""Original portable halo mesh, costume texture and character portrait."""

import json
import math
import struct

import numpy as np
from PIL import Image, ImageDraw


def bind_point(rig, index):
    matrices = []
    for joint in rig:
        rx, ry, rz = joint["rotation"]
        cx, cy, cz = math.cos(rx), math.cos(ry), math.cos(rz)
        sx, sy, sz = math.sin(rx), math.sin(ry), math.sin(rz)
        mx = np.array([[1, 0, 0], [0, cx, -sx], [0, sx, cx]])
        my = np.array([[cy, 0, sy], [0, 1, 0], [-sy, 0, cy]])
        mz = np.array([[cz, -sz, 0], [sz, cz, 0], [0, 0, 1]])
        matrix = np.eye(4)
        matrix[:3, :3] = mz @ my @ mx @ np.diag(joint["scale"])
        matrix[:3, 3] = joint["translation"]
        if joint["parent"] >= 0:
            matrix = matrices[joint["parent"]] @ matrix
        matrices.append(matrix)
    return matrices[index][:3, 3]


def halo(path, center):
    positions, normals, indices = [], [], []
    major, minor = 48, 8
    radius, thickness = 3.8, 0.26
    for i in range(major):
        angle = math.tau * i / major
        for j in range(minor):
            tube = math.tau * j / minor
            direction = np.array(
                [
                    math.cos(angle) * math.cos(tube),
                    math.sin(tube),
                    math.sin(angle) * math.cos(tube),
                ]
            )
            point = (
                center
                + np.array([radius * math.cos(angle), 0, radius * math.sin(angle)])
                + thickness * direction
            )
            positions.extend(point)
            normals.extend(direction)
    for i in range(major):
        for j in range(minor):
            a = i * minor + j
            b = ((i + 1) % major) * minor + j
            c = ((i + 1) % major) * minor + (j + 1) % minor
            d = i * minor + (j + 1) % minor
            indices.extend((a, d, c, a, c, b))
    # Eight thick, closed diamond petals give a readable flower silhouette.
    for i in range(8):
        angle = math.tau * i / 8
        outward = np.array([math.cos(angle), 0, math.sin(angle)])
        tangent = np.array([-math.sin(angle), 0, math.cos(angle)])
        mid = center + outward * 4.65
        points = [
            mid - outward * 0.65,
            mid + tangent * 0.42,
            mid + outward * 0.65,
            mid - tangent * 0.42,
            mid + np.array([0, 0.32, 0]),
            mid - np.array([0, 0.32, 0]),
        ]
        for k in range(4):
            for face in ((k, (k + 1) % 4, 4), ((k + 1) % 4, k, 5)):
                p = [points[n] for n in face]
                normal = np.cross(p[1] - p[0], p[2] - p[0])
                normal /= np.linalg.norm(normal)
                offset = len(positions) // 3
                for point in p:
                    positions.extend(point)
                    normals.extend(normal)
                indices.extend((offset, offset + 1, offset + 2))
    vertex_count = len(positions) // 3
    buffers = [
        struct.pack(f"<{len(positions)}f", *positions),
        struct.pack(f"<{len(normals)}f", *normals),
        struct.pack(f"<{len(indices)}H", *indices),
    ]
    data = bytearray()
    views = []
    for buffer in buffers:
        data.extend(b"\0" * (-len(data) % 4))
        views.append(dict(buffer=0, byteOffset=len(data), byteLength=len(buffer)))
        data.extend(buffer)
    coords = np.array(positions).reshape((-1, 3))
    doc = dict(
        asset=dict(version="2.0", generator="Glim authoring example"),
        scene=0,
        scenes=[dict(nodes=[0])],
        nodes=[dict(mesh=0, name="Petal halo")],
        meshes=[
            dict(
                name="Petal halo",
                primitives=[
                    dict(attributes=dict(POSITION=0, NORMAL=1), indices=2, material=0)
                ],
            )
        ],
        materials=[
            dict(
                name="Brass light",
                doubleSided=True,
                pbrMetallicRoughness=dict(
                    baseColorFactor=[0.95, 0.72, 0.26, 1],
                    metallicFactor=0,
                    roughnessFactor=0.6,
                ),
            )
        ],
        accessors=[
            dict(
                bufferView=0,
                componentType=5126,
                count=vertex_count,
                type="VEC3",
                min=coords.min(axis=0).tolist(),
                max=coords.max(axis=0).tolist(),
            ),
            dict(bufferView=1, componentType=5126, count=vertex_count, type="VEC3"),
            dict(bufferView=2, componentType=5123, count=len(indices), type="SCALAR"),
        ],
        bufferViews=views,
        buffers=[dict(byteLength=len(data))],
    )
    encoded = json.dumps(doc, separators=(",", ":")).encode()
    encoded += b" " * (-len(encoded) % 4)
    data.extend(b"\0" * (-len(data) % 4))
    length = 12 + 8 + len(encoded) + 8 + len(data)
    path.write_bytes(
        struct.pack("<III", 0x46546C67, 2, length)
        + struct.pack("<II", len(encoded), 0x4E4F534A)
        + encoded
        + struct.pack("<II", len(data), 0x004E4942)
        + data
    )


def make_art(output, rig):
    # Rigid import uses the selected bone's local coordinate system.
    center = np.array([0, 5.4, 0])
    halo(output / "halo.glb", center)
    image = Image.new("RGBA", (128, 128))
    draw = ImageDraw.Draw(image)
    for y in range(128):
        t = y / 127
        color = (
            int(33 + 34 * (1 - t)),
            int(118 + 49 * (1 - t)),
            int(108 + 43 * (1 - t)),
            255,
        )
        draw.line((0, y, 127, y), fill=color)
    image.save(output / "glim_body.png")
    portrait = Image.new("RGBA", (256, 256), (15, 34, 40, 255))
    draw = ImageDraw.Draw(portrait)
    draw.ellipse((24, 32, 232, 240), fill=(24, 60, 65), outline=(76, 143, 139), width=2)
    draw.ellipse((70, 213, 193, 235), fill=(11, 25, 30))
    draw.ellipse((49, 45, 208, 101), outline=(249, 195, 95), width=9)
    for i in range(8):
        angle = math.tau * i / 8
        x, y = 128 + 93 * math.cos(angle), 72 + 29 * math.sin(angle)
        draw.polygon(
            [(x, y - 10), (x + 6, y), (x, y + 10), (x - 6, y)], fill=(255, 211, 119)
        )
    draw.polygon([(77, 99), (69, 74), (100, 89)], fill=(78, 175, 154))
    draw.polygon([(161, 89), (189, 74), (179, 103)], fill=(78, 175, 154))
    draw.ellipse(
        (54, 91, 202, 229), fill=(46, 140, 129), outline=(130, 216, 184), width=3
    )
    draw.ellipse(
        (73, 132, 111, 188), fill=(16, 43, 49), outline=(244, 207, 117), width=3
    )
    draw.ellipse(
        (145, 132, 183, 188), fill=(16, 43, 49), outline=(244, 207, 117), width=3
    )
    draw.ellipse((84, 139, 100, 155), fill=(220, 255, 226))
    draw.ellipse((156, 139, 172, 155), fill=(220, 255, 226))
    draw.arc((110, 175, 146, 200), 0, 180, fill=(18, 57, 57), width=4)
    draw.ellipse((58, 199, 108, 229), fill=(222, 165, 67))
    draw.ellipse((148, 199, 198, 229), fill=(222, 165, 67))
    portrait.save(output / "portrait.png")
