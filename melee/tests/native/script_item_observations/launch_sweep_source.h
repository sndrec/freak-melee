#define LAUNCH_SWEEP_SOURCE \
    "    func LaunchAngle() -> float:\n        return item.launch_angle\n" \
    "    func LaunchSpeed() -> float:\n        return item.launch_speed\n" \
    "    func LaunchX() -> float:\n        return item.launch_origin.x\n" \
    "    func LaunchY() -> float:\n        return item.launch_origin.y\n" \
    "    func LaunchZ() -> float:\n        return item.launch_origin.z\n" \
    "    func ReflectionFacing() -> float:\n        return item.reflect_facing\n" \
    "    func CancelHitlag():\n        item.cancel_hitlag_request()\n" \
    "    func StageSweep() -> bool:\n        return item.sweep_stage(-7.0, 8.0, 9.0)\n" \
    "    func NormalSweep() -> bool:\n        return item.sweep_stage_normal(-7.0, 8.0, 9.0)\n" \
    "    func CheckGround() -> bool:\n        return item.check_ground()\n" \
    "    func ReadNormalX() -> float:\n        return item.surface_normal.x\n" \
    "    func ReadNormalY() -> float:\n        return item.surface_normal.y\n" \
    "    func ReadNormalZ() -> float:\n        return item.surface_normal.z\n" \
    "    func WriteNormal():\n        item.surface_normal.x = 1.25\n        item.surface_normal.y = -0.0\n        item.surface_normal.z = -3.75\n"

static Vec3 sweep_previous, sweep_current;
static unsigned stage_calls;
bool mpCheckAllRemap(Vec3* position, int* line, u32* flags, Vec3* normal,
                     int skip, int only, float x0, float y0, float x1, float y1)
{
    CHECK(position && !line && !flags && skip == -1 && only == -1);
    if (x0 != sweep_previous.x || y0 != sweep_previous.y)
        fprintf(stderr, "sweep start %.9g,%.9g expected %.9g,%.9g (calls %u)\n",
                x0,y0,sweep_previous.x,sweep_previous.y,stage_calls);
    CHECK(x0 == sweep_previous.x && y0 == sweep_previous.y);
    CHECK(x1 == sweep_current.x && y1 == sweep_current.y);
    ++stage_calls;
    *position = (Vec3){31, 32, 33};
    if(normal)*normal=(Vec3){0.25f,0.75f,-0.0f};
    if (observed) continuation();
    return collision_result != 0;
}
