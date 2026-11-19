# ONE-OFF generator for Content/Maps/Concourse.umap. The level is the source of truth for the 3D scene: do not rerun this unless you
# want to throw away every change made in the editor. Run it through Tools/make_concourse_level.ps1 (editor commandlet, Python).
# The numbers are the layout the old runtime builder used (centimetres; the kiosk faces +X, the concourse runs along +X to the platform edge).
import unreal

MAP = "/Game/Maps/Concourse"

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

XMin, XMax, PlatformEdgeX = -110.0, 1520.0, 1100.0
HalfWidth, WallHeight = 820.0, 420.0

if not les.new_level(MAP):
    raise RuntimeError("could not create " + MAP)

count = {"blocks": 0, "lights": 0}


def V(x, y, z):
    return unreal.Vector(x, y, z)


def block(label, folder, shape_cylinder, centre, size, token):
    a = eas.spawn_actor_from_class(unreal.KioskBlock, V(*centre))
    a.set_actor_label(label)
    a.set_folder_path(folder)
    a.set_actor_scale3d(V(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0))  # basic shapes are 100 cm
    comp = a.get_component_by_class(unreal.KioskBlockComponent)
    comp.set_editor_property("colour_token", token)
    comp.set_editor_property("shape", unreal.KioskShape.CYLINDER if shape_cylinder else unreal.KioskShape.CUBE)
    count["blocks"] += 1
    return a


Length = XMax - XMin
MidX = (XMax + XMin) * 0.5
FloorLength = PlatformEdgeX - XMin
FloorMidX = (PlatformEdgeX + XMin) * 0.5
FullWidth = HalfWidth * 2.0

# Shell.
block("Floor", "Concourse/Shell", False, (FloorMidX, 0, -10), (FloorLength, FullWidth, 20), "Muted")
block("Ceiling", "Concourse/Shell", False, (MidX, 0, WallHeight + 10), (Length, FullWidth, 20), "Surface")
block("KioskWall", "Concourse/Shell", False, (XMin + 10, 0, WallHeight * 0.5), (20, FullWidth, WallHeight), "Bg")
block("FarWall", "Concourse/Shell", False, (XMax - 10, 0, WallHeight * 0.5), (20, FullWidth, WallHeight), "BorderStrong")
block("WallLeft", "Concourse/Shell", False, (MidX, -HalfWidth - 10, WallHeight * 0.5), (Length, 20, WallHeight), "Bg")
block("WallRight", "Concourse/Shell", False, (MidX, HalfWidth + 10, WallHeight * 0.5), (Length, 20, WallHeight), "Bg")

# Platform edge: tactile yellow strip, a railing, and the track bed below.
TrackLength = XMax - PlatformEdgeX
TrackMidX = (PlatformEdgeX + XMax) * 0.5
block("EdgeStrip", "Concourse/Platform", False, (PlatformEdgeX - 30, 0, 1), (40, FullWidth, 2), "Chip")
block("TrackBed", "Concourse/Platform", False, (TrackMidX, 0, -110), (TrackLength, FullWidth, 20), "Ink")
block("RailA", "Concourse/Platform", False, (TrackMidX, -70, -96), (TrackLength, 6, 4), "Muted")
block("RailB", "Concourse/Platform", False, (TrackMidX, 70, -96), (TrackLength, 6, 4), "Muted")
block("RailingBar", "Concourse/Platform", False, (PlatformEdgeX, 0, 105), (6, FullWidth, 6), "Help")
for i in range(9):
    y = -HalfWidth + FullWidth * i / 8.0
    block("RailingPost%d" % i, "Concourse/Platform", True, (PlatformEdgeX, y, 52), (8, 8, 104), "Help")

# Pillars and benches.
for side in range(2):
    y = -420.0 if side == 0 else 420.0
    block("Pillar%d" % side, "Concourse/Furniture", True, (550, y, WallHeight * 0.5), (60, 60, WallHeight), "Surface")
    block("Bench%d" % side, "Concourse/Furniture", False, (780, y * 0.6, 22), (50, 180, 45), "Brand")

# Ceiling light panels with a point light under each.
idx = 0
for x in (200.0, 600.0, 1000.0):
    for y in (-300.0, 300.0):
        block("LightPanel%d" % idx, "Concourse/Lights", False, (x, y, WallHeight - 2), (120, 60, 4), "White")
        light = eas.spawn_actor_from_class(unreal.PointLight, V(x, y, WallHeight - 40))
        light.set_actor_label("Light%d" % idx)
        light.set_folder_path("Concourse/Lights")
        lc = light.get_component_by_class(unreal.PointLightComponent)
        lc.set_mobility(unreal.ComponentMobility.MOVABLE)
        lc.set_editor_property("intensity_units", unreal.LightUnits.UNITLESS)  # the old runtime light was unitless; placed actors default to candelas
        lc.set_editor_property("intensity", 400.0)
        lc.set_editor_property("attenuation_radius", 900.0)
        lc.set_light_color(unreal.LinearColor(1.0, 0.97, 0.92), True)
        lc.set_editor_property("cast_shadows", False)
        count["lights"] += 1
        idx += 1

# Three weak shadowless fills (two from opposite sides, one up at the ceiling) so no wall, pillar or ceiling face is pure black.
fills = [((-50.0, 210.0), 2.5), ((-35.0, 40.0), 1.5), ((60.0, 120.0), 1.5)]  # (pitch, yaw), lux; the last shines up onto the ceiling
for i, ((pitch, yaw), lux) in enumerate(fills):
    fill = eas.spawn_actor_from_class(unreal.DirectionalLight, V(0, 0, 500), unreal.Rotator(roll=0.0, pitch=pitch, yaw=yaw))
    fill.set_actor_label("Fill%d" % i)
    fill.set_folder_path("Concourse/Lights")
    fc = fill.get_component_by_class(unreal.DirectionalLightComponent)
    fc.set_mobility(unreal.ComponentMobility.MOVABLE)
    fc.set_editor_property("intensity", lux)
    fc.set_editor_property("cast_shadows", False)
    fc.set_editor_property("forward_shading_priority", 3 - i)  # one clear winner, no engine warning
    count["lights"] += 1

# The kiosk (its parts are components of the cabinet actor) and the visitor.
kiosk = eas.spawn_actor_from_class(unreal.KioskCabinet, V(0, 0, 0))
kiosk.set_actor_label("Kiosk")
visitor = eas.spawn_actor_from_class(unreal.KioskVisitorPawn, V(300, 0, 93), unreal.Rotator(roll=0.0, pitch=0.0, yaw=180.0))
visitor.set_actor_label("Visitor")
visitor.set_editor_property("auto_possess_player", unreal.AutoReceiveInput.PLAYER0)

# No baked lighting: every light is dynamic, so a default Stationary light dropped in the editor never asks for a lighting build.
unreal.EditorLevelLibrary.get_editor_world().get_world_settings().set_editor_property("force_no_precomputed_lighting", True)

if not les.save_current_level():
    raise RuntimeError("could not save the level")
unreal.log("make_concourse_level: saved %s with %d blocks, %d lights, 1 kiosk, 1 visitor" % (MAP, count["blocks"], count["lights"]))
