import time

import unreal


world = unreal.EditorLevelLibrary.get_editor_world()
if world is None:
    raise RuntimeError("No editor world is available for the stylized point-light test")

location = unreal.Vector(0.0, 0.0, 300.0)
point_light = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.PointLight, location)
if point_light is None:
    raise RuntimeError("Failed to spawn the point light")

point_light.set_actor_label("StylizedUniformValidationPointLight")
unreal.log("STYLIZED_POINT_LIGHT_TEST: spawned")

# The original failure happened on a renderer worker immediately after the light
# entered the scene. Keep the editor alive long enough for Lumen to gather it.
time.sleep(8.0)

unreal.log("STYLIZED_POINT_LIGHT_TEST: passed")
unreal.SystemLibrary.execute_console_command(world, "QUIT_EDITOR")
