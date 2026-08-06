"""Find StaticMeshActors buried under the ground in the current editor level.

Usage (Unreal Editor -> Output Log -> Cmd dropdown set to "Python"):

    import find_buried_meshes as fbm
    fbm.run()                       # list buried actors, log a report
    fbm.run(select=True)            # also select them in the viewport
    fbm.run(strict=True)            # sample all 4 bounds corners (stricter)
    fbm.run(move_to_folder="_Buried")   # move flagged actors to an outliner folder

An actor is "buried" when the top of its bounding box sits below the world
surface found by tracing straight down through the actor (the actor itself is
ignored, so the trace reports the ground/landscape above it).
"""

import unreal

# 1-unit slack so a mesh sitting flush with the ground is not flagged.
_EPSILON = 1.0
# Trace far enough above/below the bounds to clear any terrain.
_TRACE_PAD = 100000.0
_TRACE_CHANNEL = unreal.TraceTypeQuery.TRACE_TYPE_QUERY1  # Visibility


def _editor_world():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()


def _static_mesh_actors():
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    return [a for a in subsystem.get_all_level_actors()
            if isinstance(a, unreal.StaticMeshActor)]


def _ground_z_at(world, x, y, top_z, bottom_z, ignore_actor):
    """Trace down through (x, y); return the surface Z above the actor, or None."""
    start = unreal.Vector(x, y, top_z + _TRACE_PAD)
    end = unreal.Vector(x, y, bottom_z - _TRACE_PAD)
    result = unreal.SystemLibrary.line_trace_single(
        world, start, end, _TRACE_CHANNEL,
        trace_complex=False,
        actors_to_ignore=[ignore_actor],
        draw_debug_type=unreal.DrawDebugTrace.NONE,
        ignore_self=True)
    # UE Python returns (bool, HitResult) for functions with a return + out param.
    if isinstance(result, tuple):
        was_hit, hit = result
    else:
        was_hit, hit = bool(result), result
    if not was_hit or hit is None:
        return None
    return _hit_z(hit)


def _hit_z(hit):
    """Read the Z of a HitResult across UE versions (attribute vs get_editor_property)."""
    for prop in ("impact_point", "location"):
        try:
            v = hit.get_editor_property(prop)
            if v is not None:
                return v.z
        except Exception:
            pass
    try:
        return hit.get_editor_property("trace_start").z - hit.get_editor_property("distance")
    except Exception:
        return None


def _sample_points(origin, extent, strict):
    """XY sample points: center only, or center + 4 bounds corners in strict mode."""
    points = [(origin.x, origin.y)]
    if strict:
        for dx in (-extent.x, extent.x):
            for dy in (-extent.y, extent.y):
                points.append((origin.x + dx, origin.y + dy))
    return points


def find_buried(strict=False):
    """Return a list of dicts describing buried StaticMeshActors."""
    world = _editor_world()
    buried = []
    for actor in _static_mesh_actors():
        origin, extent = actor.get_actor_bounds(only_colliding_components=False)
        top_z = origin.z + extent.z
        bottom_z = origin.z - extent.z

        # Buried only if the top is below the ground at EVERY sampled point.
        min_clearance = None
        grounded = True
        for x, y in _sample_points(origin, extent, strict):
            ground_z = _ground_z_at(world, x, y, top_z, bottom_z, actor)
            if ground_z is None:
                grounded = False
                break
            clearance = ground_z - top_z  # >0 means top is below ground
            if clearance <= _EPSILON:
                grounded = False
                break
            min_clearance = clearance if min_clearance is None else min(min_clearance, clearance)

        if grounded and min_clearance is not None:
            buried.append({
                "actor": actor,
                "label": actor.get_actor_label(),
                "location": actor.get_actor_location(),
                "top_z": top_z,
                "depth_below_ground": min_clearance,
            })

    buried.sort(key=lambda e: e["depth_below_ground"], reverse=True)
    return buried


def run(strict=False, select=False, move_to_folder=None):
    """Scan the level, log a report, and optionally select / re-folder results."""
    total = len(_static_mesh_actors())
    buried = find_buried(strict=strict)

    unreal.log("=" * 60)
    unreal.log("Buried StaticMeshActor scan  (strict corners=%s)" % strict)
    unreal.log("  static mesh actors: %d   buried: %d" % (total, len(buried)))
    for e in buried:
        loc = e["location"]
        unreal.log("  [BURIED] %-32s  loc=(%.1f, %.1f, %.1f)  depth=%.1f"
                   % (e["label"], loc.x, loc.y, loc.z, e["depth_below_ground"]))
    if not buried:
        unreal.log("  none found.")
    unreal.log("=" * 60)

    actors = [e["actor"] for e in buried]

    if select and actors:
        unreal.get_editor_subsystem(unreal.EditorActorSubsystem).set_selected_level_actors(actors)

    if move_to_folder and actors:
        for a in actors:
            a.set_folder_path(unreal.Name(move_to_folder))
        unreal.log("  moved %d actor(s) to outliner folder '%s'" % (len(actors), move_to_folder))

    return buried
