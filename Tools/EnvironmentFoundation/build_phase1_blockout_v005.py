import math
import random
import unreal

MAP_PATH = "/Game/Environment/Blockout/L_Phase1_Blockout_v005"

# ---------------------------------------------------------------------------
# LOCKED MACRO LAYOUT — v0.05
# World reference footprint: ~1000m x 1000m.
#
# Platform surface-area ratios:
#   State 2 Ruins : 330 x 330 = 108,900 m2  (~25.14%)
#   State 1 Forest: 330 x 330 = 108,900 m2  (~25.14%)
#   State 3 Field : 470 x 370 = 173,900 m2  (~40.13%)
#   State 4 Arena : diameter 230m = 41,548 m2 (~9.59%)
#
# Main three platforms form a triangle.
# Boss Arena connects ONLY from State 3.
#
# No tilted ground planes. All platforms are flat.
# ---------------------------------------------------------------------------

def load(path):
    obj = unreal.load_asset(path)
    if not obj:
        raise RuntimeError("Missing engine asset: " + path)
    return obj

CUBE = load("/Engine/BasicShapes/Cube.Cube")
CYL = load("/Engine/BasicShapes/Cylinder.Cylinder")

def spawn_static(label, mesh, location, scale, rotation=None):
    if rotation is None:
        rotation = unreal.Rotator(0.0, 0.0, 0.0)

    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.StaticMeshActor,
        unreal.Vector(*location),
        rotation
    )
    if not actor:
        raise RuntimeError("Failed to spawn " + label)

    actor.set_actor_label(label)
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    actor.static_mesh_component.set_collision_enabled(
        unreal.CollisionEnabled.QUERY_AND_PHYSICS
    )
    return actor

def cube(label, x, y, center_z, sx_m, sy_m, sz_m, yaw=0.0):
    return spawn_static(
        label,
        CUBE,
        (x, y, center_z),
        (sx_m, sy_m, sz_m),
        unreal.Rotator(0.0, yaw, 0.0)
    )

def cyl(label, x, y, base_z, radius_m, height_m):
    # Engine basic cylinder is approximately 1m diameter x 1m high.
    return spawn_static(
        label,
        CYL,
        (x, y, base_z + (height_m * 100.0 * 0.5)),
        (radius_m * 2.0, radius_m * 2.0, height_m)
    )

def platform_box(label, cx_m, cy_m, sx_m, sy_m, top_z_m):
    thickness_m = 1.0
    center_z_cm = (top_z_m - thickness_m * 0.5) * 100.0
    return cube(
        label,
        cx_m * 100.0,
        cy_m * 100.0,
        center_z_cm,
        sx_m,
        sy_m,
        thickness_m
    )

def stepped_bridge(label_prefix, start_m, end_m, width_m, start_top_m, end_top_m, steps):
    """
    Flat horizontal slabs only. No tilted planes.
    Height change is distributed in small walkable increments.
    """
    sx, sy = start_m
    ex, ey = end_m

    dx = ex - sx
    dy = ey - sy
    full_len = math.sqrt(dx*dx + dy*dy)
    yaw = math.degrees(math.atan2(dy, dx))

    ux = dx / full_len
    uy = dy / full_len
    seg_len = full_len / steps

    slab_thickness = 0.30

    for i in range(steps):
        t0 = i / steps
        t1 = (i + 1) / steps
        tm = (t0 + t1) * 0.5

        x_m = sx + dx * tm
        y_m = sy + dy * tm

        # Each segment gets a flat top; adjacent segments differ slightly in height.
        top_m = start_top_m + (end_top_m - start_top_m) * t1
        center_z_cm = (top_m - slab_thickness * 0.5) * 100.0

        cube(
            f"{label_prefix}_{i:02d}",
            x_m * 100.0,
            y_m * 100.0,
            center_z_cm,
            seg_len + 0.15,
            width_m,
            slab_thickness,
            yaw=yaw
        )

def build_arch(prefix, cx_cm, cy_cm, top_z_m, width_m, height_m, thickness_m=1.4, yaw=0.0):
    """
    Simple ruin arch placeholder:
      two vertical stone legs + horizontal lintel.
    """
    a = math.radians(yaw)
    px = math.cos(a)
    py = math.sin(a)

    half = width_m * 0.5
    leg_offset_m = max(half - thickness_m * 0.5, 0.5)

    for side, sign in [("L", -1), ("R", 1)]:
        x = cx_cm + px * leg_offset_m * sign * 100.0
        y = cy_cm + py * leg_offset_m * sign * 100.0
        cube(
            f"{prefix}_{side}",
            x, y,
            top_z_m * 100.0 + (height_m * 100.0 * 0.5),
            thickness_m,
            thickness_m,
            height_m,
            yaw=yaw
        )

    lintel_z = top_z_m * 100.0 + height_m * 100.0 + thickness_m * 50.0
    cube(
        f"{prefix}_Lintel",
        cx_cm, cy_cm,
        lintel_z,
        width_m,
        thickness_m,
        thickness_m,
        yaw=yaw
    )

# ---------------------------------------------------------------------------
# STATE 2 — RUINS / ROCKS — SPAWN — ~25%
# ---------------------------------------------------------------------------

def build_state2_ruins():
    rng = random.Random(5202)

    cx_m, cy_m = -285.0, -255.0
    top_m = 0.0

    platform_box(
        "S2_PLATFORM_Ruins",
        cx_m, cy_m,
        330.0, 330.0,
        top_m
    )

    cx = cx_m * 100.0
    cy = cy_m * 100.0
    base_z_cm = top_m * 100.0

    # Standalone ruined pillars.
    # Majority 6–8m, a smaller number 9–10m, a few 11–12m.
    pillar_heights = (
        [6.0, 6.5, 7.0, 7.5, 8.0] * 7 +
        [9.0, 9.5, 10.0] * 3 +
        [11.0, 12.0]
    )

    for i, h in enumerate(pillar_heights):
        # Keep a broad spawn/combat clearing around the center.
        for _ in range(100):
            x_m = rng.uniform(cx_m - 145.0, cx_m + 145.0)
            y_m = rng.uniform(cy_m - 145.0, cy_m + 145.0)

            if (x_m - cx_m)**2 + (y_m - cy_m)**2 < 38.0**2:
                continue

            # Keep three connector mouths readable.
            if abs(x_m - cx_m) < 22 and y_m > cy_m + 110:
                continue
            if x_m > cx_m + 105 and abs(y_m - (cy_m + 105)) < 22:
                continue
            if x_m > cx_m + 105 and abs(y_m - (cy_m - 105)) < 22:
                continue
            break

        cyl(
            f"S2_Pillar_{i:02d}",
            x_m * 100.0,
            y_m * 100.0,
            base_z_cm,
            rng.uniform(0.8, 1.35),
            h
        )

    # Large rocky masses / boulder placeholders.
    for i in range(46):
        for _ in range(100):
            x_m = rng.uniform(cx_m - 150.0, cx_m + 150.0)
            y_m = rng.uniform(cy_m - 150.0, cy_m + 150.0)

            if (x_m - cx_m)**2 + (y_m - cy_m)**2 < 45.0**2:
                continue
            break

        h = rng.uniform(2.0, 6.5)
        cube(
            f"S2_Rock_{i:02d}",
            x_m * 100.0,
            y_m * 100.0,
            base_z_cm + h * 50.0,
            rng.uniform(3.0, 8.0),
            rng.uniform(2.5, 7.0),
            h,
            yaw=rng.uniform(0.0, 180.0)
        )

    # 8 ruined arches distributed mainly around the perimeter / lanes.
    arches = [
        (-365, -360, 12, 7.5,  15),
        (-310, -370, 14, 8.0, -10),
        (-230, -355, 13, 7.0,   8),
        (-405, -275, 11, 6.5,  90),
        (-180, -270, 15, 9.0,  90),
        (-395, -185, 12, 7.0,  78),
        (-295, -165, 14, 8.5,   5),
        (-195, -185, 13, 7.5, -10),
    ]

    for i, (x_m, y_m, width_m, height_m, yaw) in enumerate(arches):
        build_arch(
            f"S2_Arch_{i:02d}",
            x_m * 100.0,
            y_m * 100.0,
            top_m,
            width_m,
            height_m,
            thickness_m=1.4,
            yaw=yaw
        )

    # Spawn is intentionally inside State 2.
    p = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.PlayerStart,
        unreal.Vector(cx + 500.0, cy + 300.0, base_z_cm + 120.0),
        unreal.Rotator(0.0, 35.0, 0.0)
    )
    if p:
        p.set_actor_label("Phase1_PlayerStart_STATE2")

# ---------------------------------------------------------------------------
# STATE 1 — FOREST — ~25%
# ---------------------------------------------------------------------------

def build_state1_forest():
    rng = random.Random(5101)

    cx_m, cy_m = -285.0, 255.0
    top_m = 1.0

    platform_box(
        "S1_PLATFORM_Forest",
        cx_m, cy_m,
        330.0, 330.0,
        top_m
    )

    base_z_cm = top_m * 100.0

    # Dense tree grid with ~3m nominal spacing and small jitter.
    # Clear two exploratory routes, connector mouths, and two small ruin clearings.
    spacing = 3.0
    half = 160.0

    ruin_clearings = [
        (cx_m - 55.0, cy_m + 40.0, 15.0),
        (cx_m + 62.0, cy_m - 48.0, 12.0),
    ]

    tree_index = 0
    x = cx_m - half

    while x <= cx_m + half:
        y = cy_m - half

        while y <= cy_m + half:
            jx = rng.uniform(-0.35, 0.35)
            jy = rng.uniform(-0.35, 0.35)
            tx = x + jx
            ty = y + jy

            # Main curved-ish route from south edge toward northern/central forest.
            route1_y = cy_m - 18.0 + 18.0 * math.sin((tx - cx_m) / 55.0)
            if abs(ty - route1_y) < 3.8:
                y += spacing
                continue

            # Secondary path to east connector / field.
            route2_y = cy_m + 58.0 + 10.0 * math.sin((tx - cx_m) / 42.0 + 0.7)
            if abs(ty - route2_y) < 3.2 and tx > cx_m - 90:
                y += spacing
                continue

            # Leave connector mouths clear.
            if abs(tx - cx_m) < 10 and ty < cy_m - 138:
                y += spacing
                continue

            if tx > cx_m + 140 and abs(ty - (cy_m - 130)) < 10:
                y += spacing
                continue

            if tx > cx_m + 140 and abs(ty - (cy_m + 120)) < 10:
                y += spacing
                continue

            # Two small ruin clearings only.
            in_ruin = False
            for qx, qy, qr in ruin_clearings:
                if (tx - qx)**2 + (ty - qy)**2 < qr**2:
                    in_ruin = True
                    break
            if in_ruin:
                y += spacing
                continue

            h = rng.uniform(10.0, 18.0)
            r = rng.uniform(0.20, 0.34)

            cyl(
                f"S1_Tree_{tree_index:05d}",
                tx * 100.0,
                ty * 100.0,
                base_z_cm,
                r,
                h
            )
            tree_index += 1

            y += spacing
        x += spacing

    # Exactly two compact ruin groups.
    for group_i, (rx_m, ry_m, rr) in enumerate(ruin_clearings):
        cube(
            f"S1_Ruin_{group_i}_WallA",
            rx_m * 100.0,
            ry_m * 100.0,
            base_z_cm + 170.0,
            11.0, 1.2, 3.4,
            yaw=12 if group_i == 0 else -18
        )
        cube(
            f"S1_Ruin_{group_i}_WallB",
            (rx_m - 4.8) * 100.0,
            (ry_m + 4.0) * 100.0,
            base_z_cm + 135.0,
            1.2, 8.0, 2.7,
            yaw=4 if group_i == 0 else -10
        )
        cyl(
            f"S1_Ruin_{group_i}_Column",
            (rx_m + 4.5) * 100.0,
            (ry_m - 3.0) * 100.0,
            base_z_cm,
            0.65,
            3.8
        )

# ---------------------------------------------------------------------------
# STATE 3 — FIELD — ~40%
# ---------------------------------------------------------------------------

def build_state3_field():
    rng = random.Random(5303)

    cx_m, cy_m = 230.0, -30.0
    top_m = 0.5

    platform_box(
        "S3_PLATFORM_Field",
        cx_m, cy_m,
        470.0, 370.0,
        top_m
    )

    base_z_cm = top_m * 100.0

    # Keep the main central 70% almost completely empty.
    # Only sparse small rocks near the outer portions.
    rocks = []
    while len(rocks) < 18:
        x_m = rng.uniform(cx_m - 215.0, cx_m + 215.0)
        y_m = rng.uniform(cy_m - 165.0, cy_m + 165.0)

        # Preserve a large empty center.
        nx = abs(x_m - cx_m) / 215.0
        ny = abs(y_m - cy_m) / 165.0
        if nx < 0.55 and ny < 0.55:
            continue

        # Keep connector mouths and boss approach clear.
        if x_m < cx_m - 190 and abs(y_m - (cy_m + 150)) < 18:
            continue
        if x_m < cx_m - 190 and abs(y_m - (cy_m - 120)) < 18:
            continue
        if abs(x_m - 275.0) < 30 and y_m > cy_m + 130:
            continue

        rocks.append((x_m, y_m))

    for i, (x_m, y_m) in enumerate(rocks):
        h = rng.uniform(0.8, 2.2)
        cube(
            f"S3_SmallRock_{i:02d}",
            x_m * 100.0,
            y_m * 100.0,
            base_z_cm + h * 50.0,
            rng.uniform(1.0, 3.0),
            rng.uniform(0.8, 2.5),
            h,
            yaw=rng.uniform(0.0, 180.0)
        )

# ---------------------------------------------------------------------------
# STATE 4 — BOSS ARENA — ~10%
# ---------------------------------------------------------------------------

def build_state4_arena():
    cx_m, cy_m = 275.0, 300.0
    top_m = 2.5

    # Circular raised platform, diameter 230m.
    cyl(
        "S4_PLATFORM_BossArena",
        cx_m * 100.0,
        cy_m * 100.0,
        top_m * 100.0 - 100.0,
        115.0,
        1.0
    )

    # Main arena floor marker slightly above platform.
    cyl(
        "S4_ArenaFloor",
        cx_m * 100.0,
        cy_m * 100.0,
        top_m * 100.0,
        88.0,
        0.20
    )

    # Tall outer pillar structures.
    count = 36
    for i in range(count):
        a = 2.0 * math.pi * i / count
        radius_m = 102.0

        # Main south/south-west entrance gap toward field connector.
        deg = math.degrees(a) % 360.0
        if 220.0 <= deg <= 270.0:
            continue

        x_m = cx_m + math.cos(a) * radius_m
        y_m = cy_m + math.sin(a) * radius_m

        # 9–13m varied ring, visually taller than normal ruin pillars.
        h = 9.0 + (i % 5)
        cyl(
            f"S4_ArenaPillar_{i:02d}",
            x_m * 100.0,
            y_m * 100.0,
            top_m * 100.0,
            1.15,
            h
        )

    # Four large broken arch structures around the ring, away from entrance.
    arch_angles = [20, 85, 145, 190]
    for i, deg in enumerate(arch_angles):
        a = math.radians(deg)
        radius_m = 96.0
        ax = cx_m + math.cos(a) * radius_m
        ay = cy_m + math.sin(a) * radius_m

        build_arch(
            f"S4_RingArch_{i:02d}",
            ax * 100.0,
            ay * 100.0,
            top_m,
            14.0,
            12.0,
            thickness_m=1.8,
            yaw=deg + 90.0
        )

# ---------------------------------------------------------------------------
# CONNECTIONS — TRIANGLE + BOSS FROM FIELD ONLY
# ---------------------------------------------------------------------------

def build_connections():
    # Ruins <-> Forest
    # Gap is ~180m; elevation difference is only 1m.
    stepped_bridge(
        "CONN_S2_S1",
        (-285.0, -90.0),
        (-285.0,  90.0),
        width_m=24.0,
        start_top_m=0.0,
        end_top_m=1.0,
        steps=10
    )

    # Ruins <-> Field
    # East edge of ruins to west/south-west edge of field.
    stepped_bridge(
        "CONN_S2_S3",
        (-120.0, -150.0),
        (-5.0,   -150.0),
        width_m=28.0,
        start_top_m=0.0,
        end_top_m=0.5,
        steps=5
    )

    # Forest <-> Field
    # East edge of forest to west/north-west edge of field.
    stepped_bridge(
        "CONN_S1_S3",
        (-120.0, 125.0),
        (-5.0,   125.0),
        width_m=28.0,
        start_top_m=1.0,
        end_top_m=0.5,
        steps=5
    )

    # Field <-> Boss Arena ONLY.
    # Boss is raised; use many shallow flat steps, not tilted geometry.
    stepped_bridge(
        "CONN_S3_S4",
        (275.0, 155.0),
        (275.0, 185.0),
        width_m=34.0,
        start_top_m=0.5,
        end_top_m=2.5,
        steps=10
    )

# ---------------------------------------------------------------------------
# LIGHTING
# ---------------------------------------------------------------------------

def add_lighting():
    sun = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.DirectionalLight,
        unreal.Vector(0,0,30000),
        unreal.Rotator(-32.0,-32.0,0.0)
    )
    if sun:
        sun.set_actor_label("Blockout_Sun")

    sky = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SkyLight,
        unreal.Vector(0,0,18000),
        unreal.Rotator(0,0,0)
    )
    if sky:
        sky.set_actor_label("Blockout_SkyLight")

    try:
        atm = unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.SkyAtmosphere,
            unreal.Vector(0,0,0),
            unreal.Rotator(0,0,0)
        )
        if atm:
            atm.set_actor_label("Blockout_SkyAtmosphere")
    except Exception as ex:
        unreal.log_warning("SkyAtmosphere skipped: " + str(ex))

def main():
    unreal.log("=== Environment Foundation v0.05 ===")
    unreal.log("Locked platform-ratio layout: Ruins 25 / Forest 25 / Field 40 / Arena 10.")

    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        raise RuntimeError("Refusing to overwrite existing v0.05 map: " + MAP_PATH)

    if not unreal.EditorLevelLibrary.new_level(MAP_PATH):
        raise RuntimeError("Could not create map: " + MAP_PATH)

    # Platforms / locations.
    build_state2_ruins()
    build_state1_forest()
    build_state3_field()
    build_state4_arena()

    # Navigation layout.
    build_connections()

    # Presentation only.
    add_lighting()

    if not unreal.EditorLevelLibrary.save_current_level():
        raise RuntimeError("Failed to save v0.05 map.")

    unreal.log("=== BLOCKOUT V0.05 GENERATION SUCCEEDED ===")
    unreal.log("Map: " + MAP_PATH)

main()
